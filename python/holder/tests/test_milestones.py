"""Full labelled milestone workflow through the generated transport."""

import json
from datetime import UTC, date, datetime, timedelta, timezone

import pytest
from test_library import card_payload, created, holder_for, ok, project_payload

from holder import Milestone, MilestoneDate
from holder.exceptions import ClosedError, NotFoundError

FIRST_ID = "11111111-1111-4111-8111-111111111111"
SECOND_ID = "22222222-2222-4222-8222-222222222222"


def row(
    identity, dates=(), *, kind=None, description=None, created_at=10, updated_at=10
):
    return {
        "milestone_id": identity,
        "project_id": "p-0",
        "card_id": "c-0000",
        "dates": list(dates),
        "kind": kind,
        "description": description,
        "created_at": created_at,
        "updated_at": updated_at,
    }


def test_multiple_milestones_keep_undated_repeated_roles_and_offsets():
    records = [
        row(FIRST_ID, kind="task"),
        row(
            SECOND_ID,
            [
                {"role": "deadline", "date": "2026-10-18"},
                {"role": "deadline", "date": "2026-10-20"},
                {"role": "occurred", "datetime": "2026-10-19T00:30:00+02:30"},
            ],
            description="A labelled event",
        ),
    ]
    paths = []

    def respond(request):
        paths.append(request.url.path)
        if request.url.path == "/projects/p-0/cards/c-0000":
            return ok(card_payload(content=True))
        if request.url.path == "/projects/p-0/cards/c-0000/milestones":
            return ok(records)
        return ok(project_payload())

    with holder_for(respond) as h:
        card = h.projects.get("p-0").cards.get("c-0000")
        milestones = card.milestones
        first, second = list(milestones)
        assert isinstance(first, Milestone)
        assert first.id == FIRST_ID
        assert first.project_id == "p-0"
        assert first.kind == "task"
        assert first.dates == ()
        assert second.dates == (
            MilestoneDate("deadline", date(2026, 10, 18)),
            MilestoneDate("deadline", date(2026, 10, 20)),
            MilestoneDate(
                "occurred",
                datetime(
                    2026, 10, 19, 0, 30, tzinfo=timezone(timedelta(hours=2, minutes=30))
                ),
            ),
        )
        assert second.description == "A labelled event"
        assert milestones[1] is second
        # Listing the milestones does not fetch the card.
        assert paths.count("/projects/p-0/cards/c-0000") == 1
        assert milestones.get(SECOND_ID).id == second.id
    with pytest.raises(ClosedError):
        list(milestones.refresh())


def test_create_partial_update_remove_and_refresh_cache():
    records = []
    writes = []

    def respond(request):
        if request.url.path == "/projects/p-0/cards/c-0000":
            return ok(card_payload(content=True))
        if request.url.path == "/projects/p-0/cards/c-0000/milestones":
            if request.method == "GET":
                return ok(list(records))
            body = json.loads(request.content)
            writes.append((request.method, body))
            identity = FIRST_ID if not records else SECOND_ID
            saved = row(
                identity,
                body["dates"],
                kind=body.get("kind"),
                description=body.get("description"),
            )
            records.append(saved)
            return created(saved)
        if request.url.path.startswith("/projects/p-0/cards/c-0000/milestones/"):
            identity = request.url.path.rsplit("/", 1)[1]
            if request.method == "PATCH":
                body = json.loads(request.content)
                writes.append((request.method, body))
                saved = next(
                    item for item in records if item["milestone_id"] == identity
                )
                saved.update(body)
                saved["updated_at"] += 1
                return ok(saved)
            writes.append((request.method, None))
            records[:] = [item for item in records if item["milestone_id"] != identity]
            return ok(list(records))
        return ok(project_payload())

    offset = timezone(timedelta(hours=-5, minutes=-30))
    with holder_for(respond) as h:
        card = h.projects.get("p-0").cards.get("c-0000")
        milestones = card.milestones
        assert list(milestones) == []
        undated = milestones.create(kind="task")
        assert undated.dates == ()
        assert writes[0] == ("POST", {"dates": [], "kind": "task", "description": None})
        assert [item.id for item in milestones] == [undated.id]
        dated = milestones.create(
            dates=[
                MilestoneDate("deadline", date(2026, 1, 1)),
                MilestoneDate("deadline", date(2026, 1, 2)),
                MilestoneDate(
                    "occurred", datetime(2025, 12, 31, 23, 30, tzinfo=offset)
                ),
            ],
            description="Original",
        )
        assert writes[1][1]["dates"] == [
            {"role": "deadline", "date": "2026-01-01"},
            {"role": "deadline", "date": "2026-01-02"},
            {"role": "occurred", "datetime": "2025-12-31T23:30:00-05:30"},
        ]
        assert dated.dates[2].value.utcoffset() == timedelta(hours=-5, minutes=-30)
        snapshot = list(milestones)
        assert len(snapshot) == 2
        assert dated.update(description=None) is dated
        assert writes[2] == ("PATCH", {"description": None})
        assert dated.description is None
        assert len(dated.dates) == 3
        assert len(list(milestones)) == 2
        assert dated.update(dates=[]) is dated
        assert writes[3] == ("PATCH", {"dates": []})
        assert dated.dates == ()
        assert undated.remove() is None
        assert [item.id for item in milestones] == [dated.id]
        # Removing it again changes nothing.
        undated.remove()
        assert writes[-1] == ("DELETE", None)
        with pytest.raises(NotFoundError):
            milestones.get(FIRST_ID)
        assert dated.refresh() is dated


def test_invalid_dates_and_empty_updates_do_not_write():
    writes = []

    def respond(request):
        if request.url.path == "/projects/p-0/cards/c-0000":
            return ok(card_payload(content=True))
        if request.method != "GET":
            writes.append(request)
        if request.url.path.endswith("/milestones"):
            return ok([row(FIRST_ID)])
        return ok(project_payload())

    with holder_for(respond) as h:
        milestone = h.projects.get("p-0").cards.get("c-0000").milestones[0]
        with pytest.raises(TypeError):
            milestone.update()
        with pytest.raises(TypeError):
            milestone.update(dates=None)
        with pytest.raises(ValueError):
            milestone.update(
                dates=[
                    MilestoneDate(
                        "due", datetime(2026, 1, 1, tzinfo=UTC).replace(tzinfo=None)
                    )
                ]
            )
        with pytest.raises(ValueError):
            MilestoneDate("due", datetime(2026, 1, 1, tzinfo=UTC, microsecond=1))
        with pytest.raises(ValueError):
            MilestoneDate("", date(2026, 1, 1))
        assert writes == []
