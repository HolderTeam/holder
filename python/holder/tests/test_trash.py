"""High-level project Trash uses scoped daemon routes."""

import httpx
import pytest
from test_library import card_payload, failure, holder_for, ok, project_payload

from holder import Card, TrashItem
from holder.exceptions import APIError, ClosedError, ProtocolError


def trashed_items():
    return [
        {
            "type": "card",
            "card_id": "c-0000",
            "project_id": "p-0",
            "title": "Old card",
            "deleted_at": 100,
            "rel_path": "c/0.md",
        },
        {
            "type": "ai_message",
            "message_id": "m-1",
            "thread_id": "t-1",
            "project_id": "p-0",
            "role": "assistant",
            "deleted_at": 200,
            "created_at": 50,
        },
    ]


def test_trash_is_lazy_cached_project_scoped_and_filterable():
    requests = []

    def respond(request):
        if request.url.path == "/projects/p-0":
            return ok(project_payload())
        requests.append(request)
        assert request.url.path == "/trash"
        assert request.url.params["project_id"] == "p-0"
        selected = request.url.params.get("type")
        return ok(
            [
                row
                for row in trashed_items()
                if selected is None or row["type"] == selected
            ]
        )

    with holder_for(respond) as holder:
        trash = holder.projects.get("p-0").trash
        assert requests == []
        card, message = list(trash)
        assert isinstance(card, TrashItem)
        assert (card.type, card.id, card.title, card.when.year) == (
            "card",
            "c-0000",
            "Old card",
            1970,
        )
        assert (message.type, message.id, message.thread_id, message.role) == (
            "ai_message",
            "m-1",
            "t-1",
            "assistant",
        )
        assert list(trash) == [card, message]
        assert len(requests) == 1
        assert trash.cards()[0].id == "c-0000"
        assert trash.messages()[0].id == "m-1"
        assert len(requests) == 3
        assert [item.id for item in trash.refresh()] == ["c-0000", "m-1"]
        assert len(requests) == 4
        with pytest.raises(ValueError):
            trash.filter(type="other")
    with pytest.raises(ClosedError):
        list(trash.refresh())


def test_trash_restore_and_permanent_delete_use_the_correct_routes():
    writes = []

    def respond(request):
        if request.url.path == "/projects/p-0":
            return ok(project_payload())
        if request.url.path == "/trash":
            return ok(trashed_items())
        if request.url.path == "/cards/c-0000":
            return ok(card_payload(content=True))
        writes.append(request)
        if request.url.path == "/cards/c-0000/restore":
            return ok({"card_id": "c-0000"})
        if request.url.path == "/ai/messages/m-1/restore":
            return ok({"message_id": "m-1"})
        return ok({})

    with holder_for(respond) as holder:
        card_item, message_item = list(holder.projects.get("p-0").trash)
        assert isinstance(card_item.restore(), Card)
        assert message_item.restore() is None
        card_item.delete_permanently()
        message_item.delete_permanently()
    assert [(r.method, r.url.path) for r in writes] == [
        ("POST", "/cards/c-0000/restore"),
        ("POST", "/ai/messages/m-1/restore"),
        ("DELETE", "/projects/p-0/trash/card/c-0000"),
        ("DELETE", "/projects/p-0/trash/ai_message/m-1"),
    ]


@pytest.mark.parametrize(
    "filter_method,expected_type",
    [(None, None), ("cards", "card"), ("messages", "ai_message")],
)
def test_empty_trash_scopes_the_project_and_optional_type(filter_method, expected_type):
    writes = []

    def respond(request):
        if request.method == "GET":
            return ok(project_payload())
        writes.append(request)
        return ok({"project_id": "p-0"})

    with holder_for(respond) as holder:
        trash = holder.projects.get("p-0").trash
        if filter_method:
            trash = getattr(trash, filter_method)()
        assert trash.empty() is None
    assert len(writes) == 1
    assert writes[0].method == "DELETE"
    assert writes[0].url.path == "/trash"
    assert writes[0].url.params["project_id"] == "p-0"
    assert writes[0].url.params.get("type") == expected_type


@pytest.mark.parametrize(
    "row",
    [
        {**trashed_items()[0], "project_id": "p-1"},
        {**trashed_items()[0], "card_id": None},
        {**trashed_items()[0], "type": "unknown"},
    ],
)
def test_trash_rejects_invalid_or_cross_project_items(row):
    def respond(request):
        if request.url.path == "/projects/p-0":
            return ok(project_payload())
        return ok([row])

    with holder_for(respond) as holder, pytest.raises(ProtocolError):
        list(holder.projects.get("p-0").trash)


def test_permanent_delete_failure_surfaces_without_retry():
    writes = []

    def respond(request):
        if request.url.path == "/projects/p-0":
            return ok(project_payload())
        if request.url.path == "/trash":
            return ok(trashed_items())
        writes.append(request)
        return failure(409)

    with holder_for(respond) as holder:
        item = holder.projects.get("p-0").trash[0]
        with pytest.raises(APIError) as error:
            item.delete_permanently()
    assert error.value.status == 409
    assert len(writes) == 1


def test_bulk_empty_transport_failure_is_not_retried():
    writes = []

    def respond(request):
        if request.url.path == "/projects/p-0":
            return ok(project_payload())
        writes.append(request)
        raise httpx.ReadError("Interrupted", request=request)

    with holder_for(respond) as holder:
        from holder.exceptions import ConnectionError

        with pytest.raises(ConnectionError):
            holder.projects.get("p-0").trash.empty()
    assert len(writes) == 1
