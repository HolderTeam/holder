"""Optional content, ordering, and counts on existing collections."""

import pytest
from test_library import card_payload, holder_for, ok, project_payload

from holder.exceptions import ProtocolError


def test_card_content_is_loaded_with_a_filtered_page_and_is_cached():
    requests = []

    def respond(request):
        requests.append(request)
        if request.url.path == "/projects/p-0":
            return ok(project_payload())
        assert request.url.path == "/projects/p-0/cards"
        assert request.url.params["parent"] == "roots"
        assert request.url.params["tag"] == "research"
        assert request.url.params["include_content"] == "true"
        assert request.url.params["include_child_count"] == "true"
        return ok(
            {
                "items": [{**card_payload(content=True), "child_count": 2}],
                "next_cursor": None,
            }
        )

    with holder_for(respond) as holder:
        project = holder.projects.get("p-0")
        roots = project.cards.roots().filter(tag="research")
        loaded = roots.with_content().with_child_counts()
        assert len(requests) == 1
        card = loaded[0]
        assert card.content == "current body"
        assert card.child_count == 2
        assert len(requests) == 2
        assert card.content == "current body"
        assert len(requests) == 2
        assert "include_content" not in roots._query


def test_project_order_and_counts_compose_with_filters_and_pagination():
    requests = []

    def respond(request):
        requests.append(request)
        assert request.url.path == "/projects"
        assert request.url.params["name"] == "Research"
        assert request.url.params["order"] == "name_desc"
        assert request.url.params["count"] == "true"
        assert request.url.params["limit"] == "100"
        assert request.url.params["offset"] == str((len(requests) - 1) * 100)
        first = (len(requests) - 1) * 100
        size = 100 if len(requests) == 1 else 1
        rows = []
        for index in range(first, first + size):
            row = project_payload(index)
            row.update(card_count=4, root_card_count=2)
            rows.append(row)
        return ok(rows)

    with holder_for(respond) as holder:
        original = holder.projects.filter(name="Research")
        projects = original.with_card_counts().order_by("name", descending=True)
        assert projects[100].id == "p-100"
        assert projects[0].card_count == 4
        assert projects[0].root_card_count == 2
        assert len(requests) == 2
        assert "count" not in original._query
        assert "order" not in original._query


def test_project_order_conveniences_and_input_validation():
    with holder_for(lambda request: ok([])) as holder:
        assert holder.projects.alphabetical()._query["order"] == "name_asc"
        assert holder.projects.recent()._query["order"] == "updated_at_desc"
        assert holder.projects.order_by("created_at")._query["order"] == (
            "created_at_asc"
        )
        with pytest.raises(ValueError, match="field"):
            holder.projects.order_by("title")
        with pytest.raises(TypeError, match="descending"):
            holder.projects.order_by("name", descending=1)


def test_requested_content_and_counts_must_be_returned():
    def respond(request):
        if request.url.path == "/projects/p-0":
            return ok(project_payload())
        if request.url.path == "/projects/p-0/cards":
            return ok({"items": [card_payload()], "next_cursor": None})
        return ok([project_payload()])

    with holder_for(respond) as holder:
        project = holder.projects.get("p-0")
        with pytest.raises(ProtocolError, match="content"):
            _ = project.cards.with_content()[0]
        with pytest.raises(ProtocolError, match="counts"):
            _ = holder.projects.with_card_counts()[0]
