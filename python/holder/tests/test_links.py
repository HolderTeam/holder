"""Qualified link workflow over the generated HTTP contract."""

import json
from dataclasses import FrozenInstanceError

import httpx
import pytest
from test_library import card_payload, created, holder_for, ok, project_payload

from holder import CardLink
from holder.exceptions import ClosedError


def link_row(
    *,
    target_id="target-1",
    target_project=None,
    target_type="card",
    label=None,
    source_project=None,
):
    row = {
        "from_card_id": "c-0000",
        "to_card_id": target_id,
        "to_type": target_type,
        "kind": "ref",
        "label": label,
        "created_at": 123,
    }
    if target_project is not None:
        row["target_project_id"] = target_project
    if source_project is not None:
        row["source_project_id"] = source_project
    return row


def test_qualified_link_values_and_backlink_coverage():
    requests = []

    def respond(request):
        requests.append(request)
        if request.url.path == "/projects/p-0/cards/c-0000":
            return ok(card_payload(content=True))
        if request.url.path.endswith("/backlinks"):
            scope = request.url.params["scope"]
            assert request.url.params["include_deleted"] == "0"
            rows = [link_row(source_project="p-1", target_project="p-0")]
            if scope == "local":
                rows = [link_row()]
            return httpx.Response(
                200,
                json={
                    "ok": True,
                    "data": rows,
                    "scope": scope,
                    "coverage": "target_project"
                    if scope == "local"
                    else "available_projects",
                },
            )
        if request.url.path.endswith("/links"):
            assert request.url.params["include_deleted"] == "0"
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
        assert links[0].source_id == "c-0000"
        assert links[0].target_id == "target-1"
        assert links[0].label == "Outside"
        assert links[0].created_at == 123
        with pytest.raises(FrozenInstanceError):
            links[0].label = "Changed"
        assert outgoing[0] is links[0]
        assert card.backlinks.coverage == "target_project"
        assert card.backlinks.scope == "local"
        assert card.backlinks[0].source_project_id == "p-0"
        external = card.backlinks.filter(scope="external")
        assert external.coverage == "available_projects"
        assert external[0].source_project_id == "p-1"
        assert external[0].target_project_id == "p-0"
        both = card.backlinks.filter(scope="all_available")
        assert both.scope == "all_available"
        assert both.coverage == "available_projects"
        assert both[0].source_project_id == "p-1"
        assert len(requests) == 10  # No target card was fetched.
    with pytest.raises(ClosedError):
        list(card.links.refresh())


def test_link_mutations_send_exact_qualified_selector_and_refresh():
    rows = []
    writes = []

    def respond(request):
        if request.url.path == "/projects/p-0/cards/c-0000":
            return ok(card_payload(content=True))
        if request.url.path.endswith("/links"):
            if request.method == "GET":
                return ok(list(rows))
            body = json.loads(request.content)
            writes.append((request.method, body))
            if request.method == "POST":
                row = link_row(
                    target_id=body["to_card_id"],
                    target_project=body.get("target_project_id"),
                    target_type=body["to_type"],
                    label=body["label"],
                )
                rows[:] = [row]
                return created(row)
            rows.clear()
            return ok({"card_id": "c-0000"})
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
                "to_card_id": "target-1",
                "target_project_id": "p-1",
                "to_type": "card",
                "kind": "ref",
                "label": "Related",
            },
        )
        card.links.remove("target-1", target_project_id="p-1")
        assert writes[1] == (
            "DELETE",
            {
                "to_card_id": "target-1",
                "target_project_id": "p-1",
                "to_type": "card",
                "kind": "ref",
            },
        )
        assert list(card.links) == []
        card.links.remove("target-1")
        assert writes[2][1]["target_project_id"] == "p-0"
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
            return httpx.Response(
                200,
                json={
                    "ok": True,
                    "data": list(rows),
                    "scope": "local",
                    "coverage": "target_project",
                },
            )
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
