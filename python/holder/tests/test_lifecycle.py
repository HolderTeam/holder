"""Trash, restore and hierarchy repair follow the daemon's lifecycle rules."""

import json

import pytest
from test_library import card_payload, failure, holder_for, ok, project_payload

from holder import HierarchyRepair, RepairedCard
from holder.exceptions import APIError


def lifecycle_handler(requests, writes):
    def respond(request):
        requests.append(request)
        if request.method in ("DELETE", "POST"):
            writes.append(request)
            if request.url.path.endswith("/repair-hierarchy"):
                apply = json.loads(request.content).get("apply", False)
                return ok(
                    {
                        "applied": apply,
                        "moved": [
                            {"card_id": "c-0001", "parent_card_id": None},
                            {"card_id": "c-0002", "parent_card_id": "c-0003"},
                        ],
                        "changed_card_count": 3,
                        "cycle_card_ids": ["c-0009"],
                    }
                )
            return ok(card_payload(content=True))
        if (
            request.url.path.startswith("/projects/")
            and "/cards" not in request.url.path
        ):
            return ok(project_payload())
        return ok(card_payload(content=True))

    return respond


@pytest.mark.parametrize(
    "kwargs,expected",
    [({}, "promote"), ({"children": "trash_subtree"}, "trash_subtree")],
)
def test_trash_sends_the_children_policy(kwargs, expected):
    requests, writes = [], []
    with holder_for(lifecycle_handler(requests, writes)) as h:
        card = h.projects.get("p-0").cards.get("c-0000")
        assert card.trash(**kwargs) is None
    assert len(writes) == 1
    assert writes[0].method == "DELETE"
    assert writes[0].url.path == "/projects/p-0/cards/c-0000"
    assert writes[0].url.params["children"] == expected
    # A trashed card cannot be read, so trash() does not refresh it.
    assert requests[-1] is writes[0]


def test_trash_rejects_an_unknown_policy_without_writing():
    requests, writes = [], []
    with holder_for(lifecycle_handler(requests, writes)) as h:
        card = h.projects.get("p-0").cards.get("c-0000")
        with pytest.raises(ValueError):
            card.trash(children="everything")
    assert writes == []


def test_restore_posts_and_refreshes():
    requests, writes = [], []
    with holder_for(lifecycle_handler(requests, writes)) as h:
        card = h.projects.get("p-0").cards.get("c-0000")
        assert card.restore() is card
    assert [w.url.path for w in writes] == ["/projects/p-0/cards/c-0000/restore"]
    assert [r.method for r in requests[-2:]] == ["POST", "GET"]


@pytest.mark.parametrize("apply", [False, True])
def test_repair_hierarchy_reports_the_plan(apply):
    requests, writes = [], []
    with holder_for(lifecycle_handler(requests, writes)) as h:
        repair = h.projects.get("p-0").repair_hierarchy(apply=apply)
    assert writes[0].url.path == "/projects/p-0/repair-hierarchy"
    assert json.loads(writes[0].content) == {"apply": apply}
    assert repair == HierarchyRepair(
        applied=apply,
        moved=[RepairedCard("c-0001", None), RepairedCard("c-0002", "c-0003")],
        changed_card_count=3,
        cycle_card_ids=["c-0009"],
    )


def test_repair_hierarchy_validates_apply_and_reports_failures():
    def respond(request):
        if request.method == "POST":
            return failure(400)
        return ok(project_payload())

    with holder_for(respond) as h:
        project = h.projects.get("p-0")
        with pytest.raises(TypeError):
            project.repair_hierarchy(apply="yes")
        with pytest.raises(APIError):
            project.repair_hierarchy(apply=True)
