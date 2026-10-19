"""High-level card reference lookup and resolution."""

import json

import pytest
from test_library import card_payload, holder_for, ok, project_payload

from holder import CardReferenceCandidate, CardReferenceResult
from holder.exceptions import AmbiguousCardReferenceError, ProtocolError


def test_resolve_opens_one_live_card_and_ignores_collection_filters():
    requests = []

    def respond(request):
        requests.append(request)
        if request.url.path == "/projects/p-0":
            return ok(project_payload())
        if request.url.path == "/card-references/resolve":
            assert request.method == "POST"
            assert json.loads(request.content) == {
                "project_id": "p-0",
                "reference": "Roadmap",
                "scope": "live",
            }
            return ok(
                {
                    "status": "resolved",
                    "match_kind": "exact_title",
                    "card": {
                        "card_id": "c-0001",
                        "title": "Roadmap",
                        "deleted_at": None,
                    },
                }
            )
        assert request.url.path == "/cards/c-0001"
        return ok(card_payload(1, content=True))

    with holder_for(respond) as holder:
        card = (
            holder.projects.get("p-0")
            .cards.roots()
            .filter(tag="other")
            .resolve("Roadmap")
        )
        assert card is not None and card.id == "c-0001"
    assert [request.url.path for request in requests] == [
        "/projects/p-0",
        "/card-references/resolve",
        "/cards/c-0001",
    ]


def test_not_found_and_ambiguous_outcomes():
    def respond(request):
        if request.url.path == "/projects/p-0":
            return ok(project_payload())
        reference = json.loads(request.content)["reference"]
        if reference == "missing":
            return ok({"status": "not_found"})
        return ok(
            {
                "status": "ambiguous",
                "match_kind": "exact_title",
                "candidates": [
                    {"card_id": "c-0001", "title": "Same", "deleted_at": None},
                    {"card_id": "c-0002", "title": "Same", "deleted_at": None},
                ],
            }
        )

    with holder_for(respond) as holder:
        cards = holder.projects.get("p-0").cards
        assert cards.resolve("missing") is None
        assert cards.lookup("missing") == CardReferenceResult(
            "not_found", None, None, ()
        )
        with pytest.raises(AmbiguousCardReferenceError) as error:
            cards.resolve("Same")
        assert error.value.reference == "Same"
        assert error.value.candidates == (
            CardReferenceCandidate("c-0001", "Same", None),
            CardReferenceCandidate("c-0002", "Same", None),
        )


def test_lookup_can_inspect_trashed_match_without_loading_a_live_card():
    requests = []

    def respond(request):
        requests.append(request)
        if request.url.path == "/projects/p-0":
            return ok(project_payload())
        assert json.loads(request.content) == {
            "project_id": "p-0",
            "reference": "c-0001",
            "scope": "either",
        }
        return ok(
            {
                "status": "resolved",
                "match_kind": "full_id",
                "card": {"card_id": "c-0001", "title": "Old", "deleted_at": 42},
            }
        )

    with holder_for(respond) as holder:
        result = holder.projects.get("p-0").cards.lookup("c-0001", scope="either")
        assert result.card == CardReferenceCandidate("c-0001", "Old", 42)
        assert result.match_kind == "full_id"
    assert len(requests) == 2


def test_lookup_validates_inputs_and_rejects_invalid_outcomes():
    def respond(request):
        if request.url.path == "/projects/p-0":
            return ok(project_payload())
        return ok({"status": "resolved", "match_kind": "exact_title"})

    with holder_for(respond) as holder:
        cards = holder.projects.get("p-0").cards
        with pytest.raises(ValueError, match="reference"):
            cards.lookup("")
        with pytest.raises(ValueError, match="scope"):
            cards.lookup("title", scope="unknown")
        with pytest.raises(ProtocolError):
            cards.lookup("title")
