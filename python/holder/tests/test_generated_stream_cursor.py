"""Regression coverage for generated SSE cursor handling (issue #276)."""

import asyncio

import httpx
import pytest

from holder.generated import Client
from holder.generated.api.default import stream_changes
from holder.generated.streams import _EventParser


@pytest.mark.parametrize("id_line, expected", [(None, "cursor-1"), ("id:", None)])
def test_parser_retains_cursor_unless_explicitly_cleared(id_line, expected):
    parser = _EventParser()
    assert parser.feed("id: cursor-1") is None
    assert parser.feed("data: first") is None
    first = parser.feed("")
    assert first.id == "cursor-1"

    if id_line is not None:
        assert parser.feed(id_line) is None
    assert parser.feed("data: second") is None
    second = parser.feed("")
    assert second.id == expected
    assert parser.last_event_id == expected
    assert parser.reconnect_headers({"headers": {"Accept": "text/event-stream"}}) == {
        "headers": {
            "Accept": "text/event-stream",
            **({"Last-Event-ID": expected} if expected is not None else {}),
        }
    }


@pytest.mark.parametrize("clear_cursor", [False, True])
@pytest.mark.parametrize("asynchronous", [False, True])
def test_reconnect_uses_cursor_after_event_without_id(clear_cursor, asynchronous):
    requests = []

    def respond(request):
        requests.append(request)
        if len(requests) == 1:
            assert "Last-Event-ID" not in request.headers
            return httpx.Response(
                200,
                headers={"Content-Type": "text/event-stream"},
                text=(
                    "id: cursor-1\nretry: 0\ndata: first\n\n"
                    + ("id:\n" if clear_cursor else "")
                    + "data: second\n\n"
                ),
            )
        assert len(requests) == 2
        assert request.headers.get("Last-Event-ID") == (
            None if clear_cursor else "cursor-1"
        )
        return httpx.Response(204)

    client = Client(
        base_url="http://holder.test",
        httpx_args={"transport": httpx.MockTransport(respond)},
    )

    async def receive_async():
        async with client, stream_changes.astream(client=client) as events:
            return [event async for event in events]

    if asynchronous:
        received = asyncio.run(receive_async())
    else:
        with client, stream_changes.stream(client=client) as events:
            received = list(events)

    assert [event.data for event in received] == ["first", "second"]
    assert [event.id for event in received] == [
        "cursor-1",
        None if clear_cursor else "cursor-1",
    ]
    assert len(requests) == 2
