"""Project intelligence entities and streamed runs."""

import json

import httpx
import pytest
from test_library import holder_for, ok, project_payload

from holder.exceptions import ConnectionError, NotFoundError, ProtocolError
from holder.intelligence_events import RunStream


def thread_row(thread_id="t-1", project_id="p-0"):
    return {
        "thread_id": thread_id,
        "project_id": project_id,
        "title": "Planning",
        "created_at": 1,
        "updated_at": 2,
        "card_id": None,
    }


def message_row(message_id="m-1", thread_id="t-1"):
    return {
        "message_id": message_id,
        "thread_id": thread_id,
        "role": "user",
        "source": "manual",
        "content": "Hello",
        "created_at": 2,
        "provider": None,
        "model": None,
        "deleted_at": None,
        "prompt_hash": None,
        "meta_json": None,
    }


def run_row(run_id="r-1", project_id="p-0", thread_id="t-1"):
    return {
        "run_id": run_id,
        "project_id": project_id,
        "thread_id": thread_id,
        "message_id": None,
        "mode": "auto",
        "prompt": "Hello",
        "status": "completed",
        "created_at": 3,
        "updated_at": 4,
        "chosen_model": "model-1",
        "error": None,
    }


def test_threads_messages_search_capture_and_run_history():
    requests = []

    def respond(request):
        requests.append(request)
        path = request.url.path
        if path == "/projects/p-0":
            return ok(project_payload())
        if path == "/ai/threads" and request.method == "POST":
            assert json.loads(request.content)["project_id"] == "p-0"
            return httpx.Response(201, json={"ok": True, "data": {"thread_id": "t-1"}})
        if path == "/ai/threads/t-1":
            return ok(thread_row())
        if path == "/ai/messages" and request.method == "POST":
            assert json.loads(request.content)["thread_id"] == "t-1"
            return httpx.Response(201, json={"ok": True, "data": {"message_id": "m-1"}})
        if path == "/ai/messages/m-1":
            return ok(message_row())
        if path == "/ai/messages" and request.method == "GET":
            assert request.url.params["thread_id"] == "t-1"
            return ok([message_row()])
        if path == "/search/ai":
            assert request.url.params["project_id"] == "p-0"
            return ok(
                [{"message_id": "m-1", "created_at": 2, "snippet": "Hi", "rank": 1.5}]
            )
        if path == "/ai/messages/capture":
            assert json.loads(request.content)["thread_id"] == "t-1"
            return httpx.Response(
                201,
                json={
                    "ok": True,
                    "data": {
                        "thread_id": "t-1",
                        "user_message_id": "m-1",
                        "assistant_message_id": "m-2",
                    },
                },
            )
        if path == "/ai/runs":
            assert request.url.params["project_id"] == "p-0"
            return ok([run_row()])
        if path == "/ai/runs/r-1":
            return ok(run_row())
        raise AssertionError(path)

    with holder_for(respond) as holder:
        ai = holder.projects.get("p-0").intelligence
        thread = ai.threads.create(title="Planning")
        assert thread.id == "t-1"
        assert thread.messages.add("Hello").id == "m-1"
        assert thread.messages[0].content == "Hello"
        result = ai.search("hello")[0]
        assert result.snippet == "Hi"
        assert result.message.id == "m-1"
        capture = ai.capture("Question", "Answer", thread=thread)
        assert capture.thread_id == "t-1"
        assert thread.runs[0].id == "r-1"
        assert ai.runs.get("r-1").chosen_model == "model-1"
    assert len(requests) >= 10


def test_thread_lookup_rejects_another_project():
    def respond(request):
        if request.url.path == "/projects/p-0":
            return ok(project_payload())
        return ok(thread_row(project_id="foreign"))

    with holder_for(respond) as holder, pytest.raises(NotFoundError):
        holder.projects.get("p-0").intelligence.threads.get("t-1")


def test_message_links_and_backlinks_stay_project_scoped():
    link = {
        "from_card_id": "m-1",
        "source_project_id": "p-0",
        "to_card_id": "c-1",
        "target_project_id": "p-0",
        "to_type": "card",
        "kind": "ref",
        "label": None,
        "created_at": 3,
    }

    def respond(request):
        path = request.url.path
        if path == "/projects/p-0":
            return ok(project_payload())
        if path == "/ai/messages/m-1":
            return ok(message_row())
        if path == "/ai/threads/t-1":
            return ok(thread_row())
        if path == "/ai/messages/m-1/links" and request.method == "POST":
            assert json.loads(request.content)["to_card_id"] == "c-1"
            return httpx.Response(201, json={"ok": True, "data": link})
        if path == "/ai/messages/m-1/links":
            return ok([link])
        if path == "/ai/messages/m-1/backlinks":
            return ok([link])
        raise AssertionError(path)

    with holder_for(respond) as holder:
        message = holder.projects.get("p-0").intelligence.message("m-1")
        assert message.links.add("c-1").target_id == "c-1"
        assert message.links[0].source_id == "m-1"
        assert message.backlinks[0].target_id == "c-1"


def test_run_creation_streams_and_replay_uses_cursor():
    requests = []

    def respond(request):
        requests.append(request)
        if request.url.path == "/projects/p-0":
            return ok(project_payload())
        if request.url.path == "/ai/threads/t-1":
            return ok(thread_row())
        if request.url.path == "/ai/runs" and request.method == "POST":
            assert json.loads(request.content) == {
                "prompt": "Explain this",
                "project_id": "p-0",
                "thread_id": "t-1",
                "mode": "auto",
            }
            stream = (
                'id: e1\nevent: run_started\ndata: {"run_id":"r-1","status":"started"}\n\n'
                'id: e2\nevent: chunk\ndata: {"run_id":"r-1","delta":"Hello"}\n\n'
                'id: e3\nevent: done\ndata: {"run_id":"r-1","status":"completed"}\n\n'
            )
            return httpx.Response(
                200, text=stream, headers={"content-type": "text/event-stream"}
            )
        if request.url.path == "/ai/runs/r-1":
            return ok(run_row())
        assert request.url.path == "/ai/runs/r-1/events"
        assert request.headers["Last-Event-ID"] == "e2"
        return httpx.Response(
            200,
            text='id: e3\nevent: done\ndata: {"run_id":"r-1","status":"completed"}\n\n',
            headers={"content-type": "text/event-stream"},
        )

    with holder_for(respond) as holder:
        project = holder.projects.get("p-0")
        thread = project.intelligence.threads.get("t-1")
        with thread.ask("Explain this") as stream:
            assert stream.run_id == "r-1"
            events = list(stream)
            assert [event.kind for event in events] == ["run_started", "chunk", "done"]
            assert events[1].text == "Hello"
            assert stream.cursor == "e3"
        with project.intelligence.runs.get("r-1").events(after="e2") as replay:
            assert [event.kind for event in replay] == ["done"]


def test_run_stream_requires_started_id_and_preserves_resync():
    def respond(request):
        if request.url.path == "/projects/p-0":
            return ok(project_payload())
        return httpx.Response(
            200,
            text='event: resync_required\ndata: {"reason":"expired"}\n\n',
            headers={"content-type": "text/event-stream"},
        )

    with holder_for(respond) as holder:
        project = holder.projects.get("p-0")
        with (
            pytest.raises(ProtocolError, match="run ID"),
            project.intelligence.run("Hello"),
        ):
            pass


def test_following_a_run_reconnects_after_a_dropped_connection():
    requests = []

    def respond(request):
        if request.url.path == "/projects/p-0":
            return ok(project_payload())
        requests.append(request)
        if len(requests) == 1:
            return httpx.Response(
                200,
                text='retry: 0\nid: e1\nevent: chunk\ndata: {"run_id":"r-1","delta":"Hi"}\n\n',
                headers={"content-type": "text/event-stream"},
            )
        assert request.headers["Last-Event-ID"] == "e1"
        return httpx.Response(
            200,
            text='id: e2\nevent: done\ndata: {"run_id":"r-1"}\n\n',
            headers={"content-type": "text/event-stream"},
        )

    with holder_for(respond) as holder:
        holder.projects.get("p-0")
        with RunStream(holder, run_id="r-1") as stream:
            assert [event.kind for event in stream] == ["chunk", "done"]
            assert stream.cursor == "e2"
    assert len(requests) == 2


def test_new_run_closing_early_and_refused_streams_raise():
    def respond(request):
        if request.url.path == "/projects/p-0":
            return ok(project_payload())
        if request.url.path == "/ai/runs":
            return httpx.Response(
                200,
                text='id: e1\nevent: run_started\ndata: {"run_id":"r-1"}\n\n',
                headers={"content-type": "text/event-stream"},
            )
        return httpx.Response(
            404,
            json={"ok": False, "error": {"code": "not_found", "message": "Run not found."}},
        )

    with holder_for(respond) as holder:
        project = holder.projects.get("p-0")
        with project.intelligence.run("Hello") as stream:
            assert next(stream).kind == "run_started"
            with pytest.raises(ConnectionError, match="before a terminal event"):
                next(stream)
        with (
            pytest.raises(NotFoundError, match="Run not found"),
            RunStream(holder, run_id="missing"),
        ):
            pass


def test_provider_key_settings_and_partial_model_configuration():
    requests = []

    def respond(request):
        requests.append(request)
        path = request.url.path
        if path == "/ai/providers/catalog":
            return ok(
                {
                    "providers": [
                        {
                            "id": "example",
                            "display_name": "Example",
                            "enabled": True,
                            "configured": False,
                            "cost_tier": None,
                            "setup_url": None,
                            "docs_url": None,
                            "api_key_label": None,
                            "api_key_hint": None,
                            "api": {"base_url": "https://example.test"},
                            "auth": {"type": "bearer"},
                            "models": [
                                {
                                    "id": "small",
                                    "endpoint": "/v1/chat",
                                    "role": None,
                                    "cost_tier": "free",
                                    "default_for_low_budget": True,
                                    "limits": {"rpm": 10, "tpm": 0, "rpd": 0},
                                }
                            ],
                        }
                    ],
                    "default_provider": None,
                }
            )
        if path == "/ai/providers/credentials" and request.method == "PUT":
            assert json.loads(request.content) == {
                "provider": "example",
                "api_key": "secret",
            }
            return ok(
                {
                    "provider": "example",
                    "configured": True,
                    "api_key_preview": "***",
                    "created_at": 1,
                    "updated_at": 1,
                }
            )
        if path == "/ai/providers/settings" and request.method == "PUT":
            assert json.loads(request.content) == {
                "provider": "example",
                "enabled": False,
            }
            return ok({"provider": "example", "enabled": False, "updated_at": 2})
        if path == "/ai/local-models/config" and request.method == "GET":
            return ok(
                {
                    "fast_model": "old",
                    "strong_model": "strong",
                    "deep_model": None,
                    "updated_at": 1,
                }
            )
        if path == "/ai/local-models/config" and request.method == "PUT":
            assert json.loads(request.content) == {
                "fast_model": "new",
                "strong_model": "strong",
                "deep_model": None,
            }
            return ok(
                {
                    "fast_model": "new",
                    "strong_model": "strong",
                    "deep_model": None,
                    "updated_at": 2,
                }
            )
        raise AssertionError(path)

    with holder_for(respond) as holder:
        provider = holder.intelligence.providers.get("example")
        assert provider.models[0]["limits"] == {"rpm": 10, "tpm": 0, "rpd": 0}
        provider.set_key("secret")
        provider.set_enabled(False)
        assert holder.intelligence.models.configure(fast="new").strong == "strong"
    assert len(requests) == 7


def runner_row():
    return {
        "runner_id": "manual-1",
        "name": "Desk",
        "kind": "ollama",
        "base_url": "http://localhost:11434",
        "source": "manual",
        "enabled": True,
        "created_at": 1,
        "updated_at": 2,
        "runtime": {
            "configured": True,
            "available": True,
            "spawn_attempted": False,
            "last_checked": 2,
            "version": None,
            "error": None,
            "models": [],
            "pulls": [],
        },
    }


def test_runners_and_model_pull_use_runner_scope():
    finished_pull = {
        "job_id": "j-1",
        "runner_id": "manual-1",
        "model": "tiny",
        "status": "completed",
        "active": False,
        "updated_at": 3,
        "progress": {
            "completed": 1,
            "total": 1,
            "percent": 100.0,
            "stage": "done",
        },
        "error": None,
    }

    def respond(request):
        path = request.url.path
        if path == "/ai/runners" and request.method == "GET":
            return ok({"runners": [runner_row()]})
        if path == "/ai/runners" and request.method == "POST":
            assert json.loads(request.content) == {
                "name": "Desk",
                "kind": "ollama",
                "base_url": "http://localhost:11434",
                "enabled": True,
            }
            return httpx.Response(201, json={"ok": True, "data": runner_row()})
        if path == "/ai/runner/pull" and request.method == "POST":
            assert json.loads(request.content) == {
                "model": "tiny",
                "runner_id": "manual-1",
            }
            return ok(
                {
                    "job_id": "j-1",
                    "runner_id": "manual-1",
                    "model": "tiny",
                    "status": "running",
                }
            )
        if path == "/ai/runner/pull/j-1" and request.method == "GET":
            assert request.url.params["runner_id"] == "manual-1"
            return ok(finished_pull)
        if path == "/ai/runner/pull/j-1/events":
            assert request.url.params["runner_id"] == "manual-1"
            snapshot = json.dumps(finished_pull)
            return httpx.Response(
                200,
                text=f"event: progress\ndata: {snapshot}\n\nevent: completed\ndata: {snapshot}\n\n",
                headers={"content-type": "text/event-stream"},
            )
        raise AssertionError(path)

    with holder_for(respond) as holder:
        ai = holder.intelligence
        assert ai.runners[0].name == "Desk"
        assert (
            ai.runners.create(name="Desk", base_url="http://localhost:11434").id
            == "manual-1"
        )
        job = ai.models.pull("tiny", runner_id="manual-1")
        assert job.refresh().status == "completed"
        with job.events() as stream:
            assert [event.kind for event in stream] == ["progress", "completed"]


def runner_with_models(runner_id, *names):
    row = runner_row()
    row["runner_id"] = runner_id
    row["runtime"]["models"] = [
        {
            "name": name,
            "runner_id": runner_id,
            "model_ref": f"{runner_id}::{name}",
            "digest": "sha256:x",
            "size": 7,
            "modified_at": "now",
        }
        for name in names
    ]
    return row


def recommendation(tag, installed):
    return {
        "tag": tag,
        "provider": None,
        "engine": None,
        "category": "general",
        "required_caste": "mini",
        "installed": installed,
    }


def test_status_installed_and_recommendations_cover_every_runner():
    capability_queries = []

    def respond(request):
        path = request.url.path
        if path == "/ai/status":
            # The daemon's real shape: device-wide counts plus every runner.
            return ok(
                {
                    "checked_at": 9,
                    "active_runs": 1,
                    "active_pull_jobs": 0,
                    "runners": [
                        runner_with_models("auto-local", "tiny"),
                        runner_with_models("manual-1"),
                    ],
                    "cloud": [],
                    "cloud_configured_providers": 0,
                }
            )
        if path == "/ai/runners":
            return ok(
                {
                    "runners": [
                        runner_with_models("auto-local", "tiny"),
                        runner_with_models("manual-1", "big"),
                    ]
                }
            )
        if path == "/ai/capabilities":
            capability_queries.append(dict(request.url.params))
            return ok(
                {
                    "local_model_config": {
                        "fast_model": None,
                        "strong_model": None,
                        "deep_model": None,
                        "updated_at": None,
                    },
                    "caste": {"name": "user", "reason": None},
                    "runners": [runner_with_models("auto-local", "tiny")],
                    "recommended_models": [
                        recommendation("tiny", True),
                        recommendation("small", False),
                    ],
                    "recommended_install": [recommendation("small", False)],
                }
            )
        raise AssertionError(path)

    with holder_for(respond) as holder:
        ai = holder.intelligence
        status = ai.status()
        assert (status.checked_at, status.active_runs, status.active_pull_jobs) == (9, 1, 0)
        assert [runner.id for runner in status.runners] == ["auto-local", "manual-1"]
        assert status.runners[0].available is True

        installed = ai.models.installed()
        assert [(model.runner_id, model.name) for model in installed] == [
            ("auto-local", "tiny"),
            ("manual-1", "big"),
        ]
        assert installed[1].model_ref == "manual-1::big"

        assert [row["tag"] for row in ai.models.recommended()] == ["tiny", "small"]
        assert [row["tag"] for row in ai.models.to_install(runner_id="manual-1")] == [
            "small"
        ]
        assert capability_queries == [{}, {"runner_id": "manual-1"}]
        with pytest.raises(ValueError):
            ai.models.recommended(runner_id="")
