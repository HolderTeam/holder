"""Project card views use the paginated project route."""

import pytest
from test_library import card_payload, holder_for, ok, project_payload


def test_recent_cards_are_limited_cached_and_refreshable():
    requests = []

    def respond(request):
        if request.url.path == "/projects/p-0":
            return ok(project_payload())
        requests.append(request)
        assert request.url.path == "/projects/p-0/cards"
        assert request.url.params["order"] == "updated_desc"
        assert request.url.params["limit"] == "2"
        return ok({"items": [card_payload(1), card_payload(0)], "next_cursor": "more"})

    with holder_for(respond) as holder:
        cards = holder.projects.get("p-0").cards.recent(limit=2)
        assert requests == []
        assert [card.id for card in cards] == ["c-0001", "c-0000"]
        assert len(requests) == 1
        assert [card.id for card in cards] == ["c-0001", "c-0000"]
        assert len(requests) == 1
        assert [card.id for card in cards.refresh()] == ["c-0001", "c-0000"]
        assert len(requests) == 2
        with pytest.raises(ValueError):
            holder.projects.get("p-0").cards.recent(limit=0)


def test_recent_limit_continues_with_only_the_remaining_count():
    requests = []

    def respond(request):
        if request.url.path == "/projects/p-0":
            return ok(project_payload())
        requests.append(request)
        if len(requests) == 1:
            assert request.url.params["limit"] == "200"
            return ok(
                {"items": [card_payload(i) for i in range(200)], "next_cursor": "more"}
            )
        assert request.url.params["cursor"] == "more"
        assert request.url.params["limit"] == "1"
        return ok({"items": [card_payload(200)], "next_cursor": "another"})

    with holder_for(respond) as holder:
        cards = holder.projects.get("p-0").cards.recent(limit=201)
        assert len(list(cards)) == 201
        assert len(requests) == 2


@pytest.mark.parametrize("scope,parent", [("roots", "roots"), ("children", "c-0000")])
def test_sibling_views_request_sort_key_order(scope, parent):
    requests = []

    def respond(request):
        if request.url.path == "/projects/p-0":
            return ok(project_payload())
        if request.url.path == "/cards/c-0000":
            return ok(card_payload(content=True))
        requests.append(request)
        return ok({"items": [card_payload(1)], "next_cursor": None})

    with holder_for(respond) as holder:
        project = holder.projects.get("p-0")
        cards = (
            project.cards.roots()
            if scope == "roots"
            else project.cards.get("c-0000").children
        )
        assert cards[0].id == "c-0001"
    assert requests[0].url.params["parent"] == parent
    assert requests[0].url.params["order"] == "sort_key_asc"
