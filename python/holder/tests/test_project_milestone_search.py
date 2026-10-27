"""Project-wide searches use the complete labelled milestone model."""

import json
from datetime import UTC, date, datetime, timedelta, timezone
from uuid import UUID

import pytest
from test_library import card_payload, failure, holder_for, ok, project_payload

from holder import Milestone, MilestoneDate
from holder.exceptions import NotFoundError

FIRST_ID = "11111111-1111-4111-8111-111111111111"
SECOND_ID = "22222222-2222-4222-8222-222222222222"


def row(identity=FIRST_ID, *, dates=None, description="Release"):
    return {
        "milestone_id": identity,
        "card_id": "c-0000",
        "dates": dates
        or [
            {"role": "deadline", "date": "2026-06-01"},
            {"role": "deadline", "date": "2026-06-03"},
            {"role": "occurred", "datetime": "2026-06-02T12:30:00+02:30"},
        ],
        "kind": "release",
        "description": description,
        "created_at": 10,
        "updated_at": 11,
    }


def test_date_search_is_lazy_cached_and_returns_full_milestones_without_card_fetches():
    requests = []

    def respond(request):
        requests.append(request)
        if request.url.path == "/projects/p-0":
            return ok(project_payload())
        if request.url.path == "/projects/p-0/cards/c-0000":
            return ok(card_payload(content=True))
        assert request.url.path == "/projects/p-0/milestones-with-dates"
        assert dict(request.url.params) == {
            "role": "deadline",
            "value_type": "date",
            "from": "2026-06-01",
            "to": "2026-06-30",
        }
        return ok([row()])

    with holder_for(respond) as holder:
        project = holder.projects.get("p-0")
        assert project.milestones is project.milestones
        results = project.milestones.between(
            date(2026, 6, 1), date(2026, 6, 30), role="deadline"
        )
        assert len(requests) == 1
        milestone = results[0]
        assert isinstance(milestone, Milestone)
        assert milestone.id == UUID(FIRST_ID) and milestone.card_id == "c-0000"
        assert milestone.dates == (
            MilestoneDate("deadline", date(2026, 6, 1)),
            MilestoneDate("deadline", date(2026, 6, 3)),
            MilestoneDate(
                "occurred",
                datetime(
                    2026, 6, 2, 12, 30, tzinfo=timezone(timedelta(hours=2, minutes=30))
                ),
            ),
        )
        assert len(requests) == 2
        assert list(results) == [milestone]
        assert len(requests) == 2
        assert milestone.card.title == "Card 0"
        assert milestone.card is milestone.card
        assert len(requests) == 3
        assert results.refresh()[0].id == milestone.id
        assert len(requests) == 4


def test_instant_search_converts_aware_bounds_to_epoch_seconds():
    requests = []

    def respond(request):
        if request.url.path == "/projects/p-0":
            return ok(project_payload())
        requests.append(request)
        return ok([row()])

    moment = datetime(
        2026, 6, 2, 12, 30, tzinfo=timezone(timedelta(hours=2, minutes=30))
    )
    with holder_for(respond) as holder:
        project = holder.projects.get("p-0")
        found = list(project.milestones.between(moment, moment, role="occurred"))
        assert found[0].dates[2].value == moment
    assert requests[0].url.path == "/projects/p-0/milestones-with-dates"
    assert dict(requests[0].url.params) == {
        "role": "occurred",
        "value_type": "datetime",
        "from": str(int(moment.timestamp())),
        "to": str(int(moment.timestamp())),
    }


def test_project_result_can_refresh_update_and_remove_without_initial_card_fetch():
    requests = []
    saved = row()

    def respond(request):
        requests.append(request)
        path = request.url.path
        if path == "/projects/p-0":
            return ok(project_payload())
        if path == "/projects/p-0/milestones-with-dates":
            return ok([saved] if saved["dates"] else [])
        if path == "/projects/p-0/cards/c-0000":
            return ok(card_payload(content=True))
        if path == "/cards/c-0000/milestones-with-dates":
            return ok([saved] if saved["dates"] else [])
        if path == f"/cards/c-0000/milestones-with-dates/{FIRST_ID}":
            if request.method == "PATCH":
                body = json.loads(request.content)
                saved.update(body)
                return ok(saved)
            assert request.method == "DELETE"
            return ok({"card_id": "c-0000", "milestone_id": FIRST_ID, "removed": True})
        raise AssertionError(path)

    with holder_for(respond) as holder:
        project = holder.projects.get("p-0")
        results = project.milestones.between(
            date(2026, 6, 1), date(2026, 6, 30), role="deadline"
        )
        milestone = results[0]
        assert [r.url.path for r in requests] == [
            "/projects/p-0",
            "/projects/p-0/milestones-with-dates",
        ]
        assert milestone.refresh() is milestone
        assert milestone.update(description="Changed") is milestone
        assert milestone.description == "Changed"
        assert results[0].description == "Changed"
        assert milestone.update(dates=[]) is milestone
        assert list(results) == []
        assert milestone.remove() is True


def test_search_validates_role_range_and_bound_types_before_query():
    calls = []

    def respond(request):
        calls.append(request)
        return ok(project_payload())

    with holder_for(respond) as holder:
        search = holder.projects.get("p-0").milestones
        with pytest.raises(ValueError):
            search.between(date(2026, 1, 2), date(2026, 1, 1), role="deadline")
        with pytest.raises(ValueError):
            search.between(date(2026, 1, 1), date(2026, 1, 2), role="")
        with pytest.raises(TypeError):
            search.between(
                date(2026, 1, 1), datetime(2026, 1, 2, tzinfo=UTC), role="deadline"
            )
        with pytest.raises(ValueError):
            search.between(
                datetime.fromisoformat("2026-01-01T00:00:00"),
                datetime.fromisoformat("2026-01-02T00:00:00"),
                role="deadline",
            )
        with pytest.raises(ValueError):
            search.between(
                datetime(2026, 1, 1, tzinfo=UTC, microsecond=1),
                datetime(2026, 1, 2, tzinfo=UTC),
                role="deadline",
            )
        with pytest.raises(TypeError):
            search.between("2026-01-01", "2026-01-02", role="deadline")
    assert len(calls) == 1


def test_project_result_lazy_card_lookup_checks_scope():
    def respond(request):
        if request.url.path == "/projects/p-0":
            return ok(project_payload())
        if request.url.path == "/projects/p-0/cards/c-0000":
            # The card is not in this project.
            return failure(404)
        return ok([row(SECOND_ID)])

    with holder_for(respond) as holder:
        result = holder.projects.get("p-0").milestones.between(
            date(2026, 6, 1), date(2026, 6, 30), role="deadline"
        )[0]
        with pytest.raises(NotFoundError):
            _ = result.card
        with pytest.raises(NotFoundError):
            result.update(description="Nope")
