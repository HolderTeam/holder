"""High-level change checkpoints, typed invalidations, and watcher ownership."""

import asyncio
import json

import httpx
import pytest
from test_library import holder_for, ok, project_payload

from holder import ChangeReady, EntityChange, Holder, ResyncRequired
from holder.exceptions import ClosedError, ProtocolError


def checkpoint(cursor="epoch:1"):
    return {
        "cursor": cursor,
        "git_revisions": {"p-0": "a" * 40},
        "history_urls": {"p-0": "/projects/p-0/history"},
    }


def frame(cursor, name, data):
    return f"id: {cursor}\nevent: {name}\ndata: {json.dumps(data)}\n\n".encode()


def test_project_watch_replays_changes_after_snapshot_and_requires_resync():
    requests = []
    payload = (
        frame("epoch:1", "ready", checkpoint("epoch:9"))
        + frame(
            "epoch:2",
            "card.changed",
            {
                "entity": "card",
                "entity_id": "c-1",
                "project_id": "p-0",
                "deleted": False,
                "git_revision": None,
            },
        )
        + frame(
            "epoch:3",
            "resync_required",
            {
                **checkpoint("epoch:9"),
                "reason": "history_unavailable",
                "last_revision": None,
            },
        )
    )

    def respond(request):
        requests.append(request)
        if request.url.path == "/projects/p-0":
            return ok(project_payload())
        if request.url.path == "/events/cursor":
            return ok(checkpoint())
        assert request.url.path == "/events"
        return httpx.Response(
            200, content=payload, headers={"content-type": "text/event-stream"}
        )

    async def run():
        with holder_for(respond) as holder:
            project = holder.projects.get("p-0")
            assert project.changes is project.changes
            before = project.changes.checkpoint()
            assert before.project_id == "p-0"
            assert before.git_revisions["p-0"] == "a" * 40
            assert before.history_urls["p-0"] == "/projects/p-0/history"
            assert project.refresh() is project
            async with project.changes.watch(after=before) as watcher:
                ready = await anext(watcher)
                assert isinstance(ready, ChangeReady)
                assert ready.cursor == "epoch:1"
                assert ready.checkpoint.cursor == "epoch:9"
                assert watcher.cursor == "epoch:1"
                change = await anext(watcher)
                assert isinstance(change, EntityChange)
                assert (change.entity, change.id, change.project_id) == (
                    "card",
                    "c-1",
                    "p-0",
                )
                assert not change.deleted and change.git_revision is None
                assert watcher.cursor == "epoch:1"
                resync = await anext(watcher)
                assert isinstance(resync, ResyncRequired)
                assert resync.reason == "history_unavailable"
                assert resync.checkpoint.cursor == "epoch:9"
                assert watcher.cursor == "epoch:2"
                with pytest.raises(StopAsyncIteration):
                    await anext(watcher)
            assert watcher._stream._client.get_async_httpx_client().is_closed
        with pytest.raises(ClosedError):
            project.changes.checkpoint()

    asyncio.run(run())
    stream = requests[-1]
    assert [request.url.path for request in requests[:3]] == [
        "/projects/p-0",
        "/events/cursor",
        "/projects/p-0",
    ]
    assert stream.url.path == "/events"
    assert stream.url.params["project_id"] == "p-0"
    assert stream.headers["Last-Event-ID"] == "epoch:1"


def test_global_watch_and_checkpoint_scope_validation():
    requests = []

    def respond(request):
        requests.append(request)
        if request.url.path == "/projects/p-0":
            return ok(project_payload())
        if request.url.path == "/events/cursor":
            return ok(checkpoint())
        return httpx.Response(
            200,
            content=(
                frame("epoch:1", "ready", checkpoint())
                + frame(
                    "epoch:2",
                    "resource.changed",
                    {
                        "entity": "resource",
                        "entity_id": "r-1",
                        "project_id": "p-0",
                        "deleted": True,
                        "git_revision": "b" * 40,
                    },
                )
            ),
            headers={"content-type": "text/event-stream"},
        )

    async def run():
        with holder_for(respond) as holder:
            global_checkpoint = holder.changes.checkpoint()
            project = holder.projects.get("p-0")
            with pytest.raises(ValueError):
                project.changes.watch(after=global_checkpoint)
            with pytest.raises(ValueError):
                holder.changes.watch(after=project.changes.checkpoint())
            async with holder.changes.watch(after=global_checkpoint) as watcher:
                assert isinstance(await anext(watcher), ChangeReady)
                changed = await anext(watcher)
                assert isinstance(changed, EntityChange)
                assert changed.entity == "resource" and changed.deleted
                assert changed.git_revision == "b" * 40
            assert watcher._stream._client.get_async_httpx_client().is_closed

    asyncio.run(run())
    stream = requests[-1]
    assert "project_id" not in stream.url.params


def test_invalid_or_foreign_entity_event_is_rejected():
    def respond(request):
        if request.url.path == "/projects/p-0":
            return ok(project_payload())
        return httpx.Response(
            200,
            content=frame(
                "epoch:2",
                "card.changed",
                {
                    "entity": "card",
                    "entity_id": "c-1",
                    "project_id": "p-1",
                    "deleted": False,
                    "git_revision": None,
                },
            ),
            headers={"content-type": "text/event-stream"},
        )

    async def run():
        with holder_for(respond) as holder:
            async with holder.projects.get("p-0").changes.watch() as watcher:
                with pytest.raises(ProtocolError):
                    await anext(watcher)

    asyncio.run(run())


def test_local_restart_returns_resync_after_refreshing_credentials(monkeypatch):
    discoveries = []
    requests = []

    def discover(self):
        discoveries.append(1)
        self._url = "http://holder.test"
        self._token = f"token-{len(discoveries)}"

    monkeypatch.setattr(Holder, "_discover", discover)

    def respond(request):
        requests.append(request)
        if request.headers["Authorization"] == "Bearer token-1":
            return httpx.Response(
                401,
                json={
                    "ok": False,
                    "error": {"code": "unauthorized", "message": "Old token"},
                },
            )
        assert request.url.path == "/events/cursor"
        return ok(checkpoint("epoch:9"))

    async def run():
        with Holder(transport=httpx.MockTransport(respond)) as holder:
            async with holder.changes.watch(after="epoch:1") as watcher:
                notice = await anext(watcher)
                assert isinstance(notice, ResyncRequired)
                assert notice.reason == "daemon_restarted"
                assert notice.checkpoint.cursor == "epoch:9"
                with pytest.raises(StopAsyncIteration):
                    await anext(watcher)
            assert watcher._stream._client.get_async_httpx_client().is_closed
            assert holder._token == "token-2"

    asyncio.run(run())
    assert len(discoveries) == 2
    assert [(r.url.path, r.headers["Authorization"]) for r in requests] == [
        ("/events", "Bearer token-1"),
        ("/events/cursor", "Bearer token-1"),
        ("/events/cursor", "Bearer token-2"),
    ]


def test_closing_a_pending_watch_releases_its_owned_stream():
    waiting = asyncio.Event()

    class WaitingBody(httpx.AsyncByteStream):
        closed = False

        async def __aiter__(self):
            yield frame("epoch:1", "ready", checkpoint())
            waiting.set()
            await asyncio.Event().wait()

        async def aclose(self):
            self.closed = True

    body = WaitingBody()

    def respond(request):
        assert request.url.path == "/events"
        return httpx.Response(
            200, stream=body, headers={"content-type": "text/event-stream"}
        )

    async def run():
        with holder_for(respond) as holder:
            async with holder.changes.watch() as watcher:
                assert isinstance(await anext(watcher), ChangeReady)
                pending = asyncio.create_task(anext(watcher))
                await asyncio.wait_for(waiting.wait(), 2)
                await watcher.aclose()
                with pytest.raises(asyncio.CancelledError):
                    await pending
            assert body.closed
            assert watcher._stream._client.get_async_httpx_client().is_closed

    asyncio.run(run())
