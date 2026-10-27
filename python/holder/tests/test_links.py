"""Qualified link workflow over the generated HTTP contract."""

import json
from dataclasses import FrozenInstanceError

import pytest
from test_library import card_payload, holder_for, ok, project_payload

from holder import CardLink
from holder.exceptions import ClosedError


def link_row(
    *,
    target_id="target-1",
    target_project="p-0",
    target_type="card",
    label=None,
    source_project="p-0",
    source_type="card",
):
    return {
        "source_project_id": source_project,
        "source_type": source_type,
        "source_id": "c-0000",
        "target_project_id": target_project,
        "target_type": target_type,
        "target_id": target_id,
        "kind": "ref",
        "label": label,
        "created_at": 123,
    }


def test_qualified_link_values_and_backlink_scopes():
    requests = []

    def respond(request):
        requests.append(request)
        if request.url.path == "/projects/p-0/cards/c-0000":
            return ok(card_payload(content=True))
        if request.url.path.endswith("/backlinks"):
            scope = request.url.params["scope"]
            assert request.url.params["include_deleted"] == "false"
            rows = [link_row(source_project="p-1", source_type="ai_message")]
            if scope == "local":
                rows = [link_row()]
            return ok(rows)
        if request.url.path.endswith("/links"):
            assert request.url.path == "/projects/p-0/cards/c-0000/links"
            assert request.url.params["include_deleted"] == "false"
            return ok(
                [
                    link_row(target_project="unavailable-project", label="Outside"),
                    link_row(target_type="ai_message"),
                    link_row(target_type="ai_thread"),
                    link_row(target_type="resource"),
                ]
            )
        return ok(project_payload())

    with holder_for(respond) as h:
        card = h.projects.get("p-0").cards.get("c-0000")
        outgoing = card.links
        links = list(outgoing)
        assert all(isinstance(row, CardLink) for row in links)
        assert [(row.target_type, row.target_project_id) for row in links] == [
            ("card", "unavailable-project"),
            ("ai_message", "p-0"),
            ("ai_thread", "p-0"),
            ("resource", "p-0"),
        ]
        assert links[0].source_project_id == "p-0"
        assert links[0].source_type == "card"
        assert links[0].source_id == "c-0000"
        assert links[0].target_id == "target-1"
        assert links[0].label == "Outside"
        assert links[0].created_at == 123
        with pytest.raises(FrozenInstanceError):
            links[0].label = "Changed"
        assert outgoing[0] is links[0]
        assert card.backlinks.scope == "local"
        assert card.backlinks[0].source_project_id == "p-0"
        external = card.backlinks.filter(scope="external")
        assert external[0].source_project_id == "p-1"
        assert external[0].source_type == "ai_message"
        assert external[0].target_project_id == "p-0"
        both = card.backlinks.filter(scope="all_available")
        assert both.scope == "all_available"
        assert both[0].source_project_id == "p-1"
        # No target card was fetched.
        assert not any("target-1" in request.url.path for request in requests)
    with pytest.raises(ClosedError):
        list(card.links.refresh())


def test_link_mutations_send_exact_qualified_selector_and_refresh():
    rows = []
    writes = []

    def respond(request):
        if request.url.path == "/projects/p-0/cards/c-0000":
            return ok(card_payload(content=True))
        if request.url.path.endswith("/links"):
            assert request.url.path == "/projects/p-0/cards/c-0000/links"
            if request.method == "GET":
                return ok(list(rows))
            if request.method == "POST":
                body = json.loads(request.content)
                writes.append((request.method, body))
                row = link_row(
                    target_id=body["target_id"],
                    target_project=body.get("target_project_id", "p-0"),
                    target_type=body["target_type"],
                    label=body["label"],
                )
                rows[:] = [row]
                return ok(row)
            # Removing names the one link in the query, with no body.
            assert request.content == b""
            writes.append((request.method, dict(request.url.params)))
            rows.clear()
            return ok([])
        return ok(project_payload())

    with holder_for(respond) as h:
        card = h.projects.get("p-0").cards.get("c-0000")
        assert list(card.links) == []
        active = iter(card.links)
        with pytest.raises(StopIteration):
            next(active)
        result = card.links.add("target-1", target_project_id="p-1", label="Related")
        assert result.target_project_id == "p-1"
        assert [row.label for row in card.links] == ["Related"]
        assert writes[0] == (
            "POST",
            {
                "target_type": "card",
                "target_id": "target-1",
                "target_project_id": "p-1",
                "kind": "ref",
                "label": "Related",
            },
        )
        card.links.remove("target-1", target_project_id="p-1")
        assert writes[1] == (
            "DELETE",
            {
                "target_type": "card",
                "target_id": "target-1",
                "kind": "ref",
                "target_project_id": "p-1",
            },
        )
        assert list(card.links) == []
        # Without a target project, the target is in the card's own.
        card.links.remove("target-1")
        assert "target_project_id" not in writes[2][1]
        typed = card.links.add("message-1", to_type="ai_message", kind="mentions")
        assert typed.target_type == "ai_message"
        assert writes[3][1]["kind"] == "mentions"
        with pytest.raises(TypeError):
            card.links.remove()
        for invalid in (None, ""):
            with pytest.raises(ValueError):
                card.links.remove(invalid)
        assert len(writes) == 4


def test_link_queries_refresh_after_external_change_and_no_delete_all_method():
    rows = [link_row()]

    def respond(request):
        if request.url.path == "/projects/p-0/cards/c-0000":
            return ok(card_payload(content=True))
        if request.url.path.endswith("/backlinks"):
            return ok(list(rows))
        if request.url.path.endswith("/links"):
            return ok(list(rows))
        return ok(project_payload())

    with holder_for(respond) as h:
        card = h.projects.get("p-0").cards.get("c-0000")
        assert len(list(card.links)) == 1
        assert len(list(card.backlinks)) == 1
        rows.clear()
        assert len(list(card.links)) == 1
        assert len(list(card.backlinks)) == 1
        assert list(card.links.refresh()) == []
        assert list(card.backlinks.refresh()) == []
        assert not hasattr(card.links, "clear")
        with pytest.raises(ValueError):
            card.backlinks.filter(scope="global")
