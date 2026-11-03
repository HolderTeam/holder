from __future__ import annotations

import json
from datetime import date, datetime, timedelta, timezone
from pathlib import Path
import sqlite3
from typing import Any, cast

import pytest

import holderkit
from holderkit import _native


def test_milestone_lifecycle_and_partial_updates(tmp_path: Path) -> None:
    data = tmp_path / "data"
    with holderkit.open(data) as context:
        project = context.create_project("Calendar")
        card = context.create_card(project.project_id, "Event", "Body #keep")
        milestones = context.milestones
        assert milestones.list(project.project_id, card.card_id) == []
        at = datetime(2026, 6, 2, 9, 30, tzinfo=timezone(timedelta(hours=1)))
        milestone = milestones.add(project.project_id, card.card_id, [
            {"role": "start", "datetime": at},
            {"role": "end", "date": date(2026, 6, 3)},
        ], kind="Appointment", description="Café")
        assert tuple(milestone) == holderkit.MILESTONE_RECORD_FIELDS
        assert milestone["project_id"] == project.project_id and milestone["card_id"] == card.card_id
        assert milestone["dates"] == [
            {"role": "start", "datetime": "2026-06-02T09:30:00+01:00"},
            {"role": "end", "date": "2026-06-03"},
        ]
        assert milestone["description"] == "Café"
        undated = milestones.add(project.project_id, card.card_id)
        assert undated["dates"] == [] and undated["kind"] is None and undated["description"] is None
        assert {row["milestone_id"] for row in milestones.list(project.project_id, card.card_id)} == {
            milestone["milestone_id"], undated["milestone_id"]}

        updated = milestones.update(project.project_id, card.card_id, milestone["milestone_id"], {
            "dates": [{"role": "deadline", "date": "2026-07-01"}],
        })
        assert updated["dates"] == [{"role": "deadline", "date": "2026-07-01"}]
        assert updated["description"] == "Café" and updated["kind"] == "Appointment"
        cleared = milestones.update(project.project_id, card.card_id, milestone["milestone_id"], {
            "kind": None, "description": None,
        })
        assert cleared["kind"] is None and cleared["description"] is None
        assert cleared["created_at"] == milestone["created_at"]
        assert milestones.update(project.project_id, card.card_id, milestone["milestone_id"], {}) == cleared
        assert context.get_card_content(project.project_id, card.card_id) == "Body #keep"
        assert context.tags.list(card.card_id) == ["keep"]
        records = milestones.to_records(project.project_id)
        assert all(tuple(row) == holderkit.PROJECT_MILESTONE_RECORD_FIELDS for row in records)
        assert all(row["project_id"] == project.project_id and row["card_title"] == "Event" for row in records)
    assert json.loads(json.dumps(records)) == records
    with holderkit.open(data) as context:
        assert sorted(context.milestones.to_records(), key=lambda row: row["milestone_id"]) == sorted(
            records, key=lambda row: row["milestone_id"])
        remaining = context.milestones.remove(project.project_id, card.card_id, milestone["milestone_id"])
        assert [row["milestone_id"] for row in remaining] == [undated["milestone_id"]]
        # Removing it again, or one that never existed, changes nothing.
        assert context.milestones.remove(project.project_id, card.card_id, milestone["milestone_id"]) == remaining


def test_milestone_calendar_compares_days_with_days_and_instants_in_the_zone(tmp_path: Path) -> None:
    with holderkit.open(tmp_path / "data") as context:
        project = context.create_project("First")
        other = context.create_project("Other")
        first = context.create_card(project.project_id, "First")
        second = context.create_card(project.project_id, "Second")
        outside = context.create_card(other.project_id, "Outside")
        milestones = context.milestones
        day = milestones.add(project.project_id, first.card_id, [{"role": "deadline", "date": "2026-06-02"}])
        # 22:00 UTC on June 1: still June 1 in UTC, already June 2 in Tokyo.
        instant = milestones.add(project.project_id, second.card_id,
                                 [{"role": "occurred", "datetime": "2026-06-01T22:00:00Z"}])
        milestones.add(other.project_id, outside.card_id, [{"role": "deadline", "date": "2026-06-02"}])

        def ids(rows: list[holderkit.ProjectMilestoneRecord]) -> list[str]:
            return sorted(row["milestone_id"] for row in rows)

        assert ids(milestones.calendar(project.project_id, date(2026, 6, 2), date(2026, 6, 2), time_zone="UTC")) == [
            day["milestone_id"]]
        assert ids(milestones.calendar(project.project_id, "2026-06-02", "2026-06-02", time_zone="Asia/Tokyo")) == ids(
            [day, instant])  # type: ignore[list-item]
        assert ids(milestones.calendar(project.project_id, "2026-06-01", "2026-06-01", time_zone="UTC")) == [
            instant["milestone_id"]]
        titled = milestones.calendar(project.project_id, "2026-06-01", "2026-06-30", time_zone="UTC")
        assert {row["card_title"] for row in titled} == {"First", "Second"}
        assert all(tuple(row) == holderkit.PROJECT_MILESTONE_RECORD_FIELDS for row in titled)
        assert milestones.calendar(project.project_id, "2026-06-30", "2026-06-01", time_zone="UTC") == []
        # Without a zone, this device's zone; the day milestone is on its day whatever the zone.
        assert day["milestone_id"] in ids(milestones.calendar(project.project_id, "2026-06-02", "2026-06-02"))
        with pytest.raises(ValueError):
            milestones.calendar(project.project_id, "2026-06-01", "June", time_zone="UTC")
        with pytest.raises(ValueError):
            milestones.calendar(project.project_id, "2026-06-01", "2026-06-02", time_zone="Mars/Olympus_Mons")
        with pytest.raises(holderkit.HolderError, match="project not found"):
            milestones.calendar("missing", "2026-06-01", "2026-06-02", time_zone="UTC")
        assert milestones.to_records("missing") == []
        assert len(milestones.to_records()) == 3
        assert len(milestones.to_records(other.project_id)) == 1


def test_milestone_ownership(tmp_path: Path) -> None:
    with holderkit.open(tmp_path / "data") as context:
        project = context.create_project("First")
        other = context.create_project("Other")
        first = context.create_card(project.project_id, "First")
        second = context.create_card(project.project_id, "Second")
        target = context.milestones.add(project.project_id, first.card_id, [{"role": "due", "date": "2026-01-01"}])
        # Another card's milestone: removing through the wrong card changes nothing.
        context.milestones.remove(project.project_id, second.card_id, target["milestone_id"])
        assert context.milestones.list(project.project_id, first.card_id) == [target]
        with pytest.raises(holderkit.HolderError, match="milestone not found"):
            context.milestones.update(project.project_id, second.card_id, target["milestone_id"], {"kind": "wrong"})
        with pytest.raises(holderkit.HolderError, match="milestone not found"):
            context.milestones.update(project.project_id, first.card_id, "missing", {"kind": "wrong"})
        with pytest.raises(holderkit.HolderError, match="card not found"):
            context.milestones.update(other.project_id, first.card_id, target["milestone_id"], {"kind": "wrong"})
        assert context.milestones.list(project.project_id, first.card_id) == [target]


@pytest.mark.parametrize("changes,exception", [
    ({"kind": 12}, TypeError), ({"description": []}, TypeError), ({"unknown": "x"}, ValueError),
    ({"dates": "2026-01-01"}, TypeError), ({"dates": [{"date": "2026-01-01"}]}, ValueError),
    ({"dates": [{"role": "due"}]}, ValueError),
    ({"dates": [{"role": "due", "date": "2026-01-01", "datetime": "2026-01-01T00:00:00Z"}]}, ValueError),
    ({"dates": [{"role": "due", "datetime": datetime(2026, 1, 1)}]}, ValueError),
    ({"dates": [{"role": "due", "date": datetime(2026, 1, 1, tzinfo=timezone.utc)}]}, TypeError),
    ({"dates": [{"role": "due", "date": "2026-02-30"}]}, ValueError),
    ({"dates": [{"role": "due", "when": "2026-01-01"}]}, ValueError),
])
def test_milestone_update_rejects_invalid_inputs(
    tmp_path: Path, changes: dict[str, Any], exception: type[Exception],
) -> None:
    with holderkit.open(tmp_path / "data") as context:
        project = context.create_project("Validation")
        card = context.create_card(project.project_id, "Card")
        original = context.milestones.add(project.project_id, card.card_id, [{"role": "due", "date": "2026-01-01"}])
        with pytest.raises(exception):
            context.milestones.update(project.project_id, card.card_id, original["milestone_id"],
                                      cast(holderkit.MilestoneUpdate, changes))
        assert context.milestones.list(project.project_id, card.card_id) == [original]


def test_milestone_native_argument_errors_and_closed_context(tmp_path: Path) -> None:
    with holderkit.open(tmp_path / "data") as context:
        project = context.create_project("Errors")
        card = context.create_card(project.project_id, "Card")
        milestones = context.milestones
        with pytest.raises(ValueError, match="embedded null"):
            milestones.add(project.project_id, card.card_id, kind="bad\x00kind")
        with pytest.raises(holderkit.HolderError, match="card not found"):
            milestones.list(project.project_id, "missing")
        with pytest.raises(ValueError):
            milestones.list(project.project_id, "")
        with pytest.raises(holderkit.HolderError, match="card not found"):
            milestones.add(project.project_id, "missing")
        with pytest.raises(holderkit.HolderError, match="card not found"):
            milestones.remove(project.project_id, "missing", "id")
        with pytest.raises(TypeError):
            context._context.add_milestone(project.project_id, card.card_id, [("due", "2026-01-01")])  # type: ignore[list-item]
        with pytest.raises(TypeError):
            context._context.update_milestone(project.project_id, card.card_id, "id", [])  # type: ignore[arg-type]
        # Native keyword entry points use the same ownership-safe operations.
        assert context._context.list_milestones(project_id=project.project_id, card_id=card.card_id) == []
        assert context._context.calendar_milestones(
            project_id=project.project_id, first_day="2026-01-01", last_day="2026-01-01", time_zone="UTC") == []
    for operation in (
        lambda: milestones.list(project.project_id, card.card_id), lambda: milestones.to_records(),
        lambda: milestones.add(project.project_id, card.card_id),
        lambda: milestones.remove(project.project_id, card.card_id, "id"),
        lambda: milestones.update(project.project_id, card.card_id, "id", {}),
        lambda: milestones.calendar(project.project_id, "2026-01-01", "2026-01-01"),
    ):
        with pytest.raises(RuntimeError, match="closed"):
            operation()


def test_milestone_dataframe_contract_and_detachment(tmp_path: Path) -> None:
    pd = pytest.importorskip("pandas")
    data = tmp_path / "data"
    with holderkit.open(data) as context:
        empty = context.milestones.to_dataframe()
        project = context.create_project("Frames")
        other = context.create_project("Other")
        card = context.create_card(project.project_id, "Calendar card")
        outside = context.create_card(other.project_id, "Outside")
        context.milestones.add(project.project_id, card.card_id, [{"role": "start", "date": "2026-01-01"}])
        context.milestones.add(project.project_id, card.card_id, [{"role": "due", "datetime": "2026-01-02T10:00:00Z"}],
                               kind="Custom", description="Body")
        context.milestones.add(other.project_id, outside.card_id)
        tables = context.to_dataframes(project.project_id, include_content=True)
        frame = tables["milestones"]
        pd.testing.assert_frame_equal(frame, context.milestones.to_dataframe(project.project_id))
        assert context.to_dataframes("missing")["milestones"].empty
    timestamp_fields = {"created_at", "updated_at"}
    dtypes = {field: "datetime64[ns, UTC]" if field in timestamp_fields else "string"
              for field in holderkit.PROJECT_MILESTONE_RECORD_FIELDS}
    dtypes["dates"] = "object"
    for table in (empty, frame):
        assert tuple(table.columns) == holderkit.PROJECT_MILESTONE_RECORD_FIELDS
        assert {str(column): str(dtype) for column, dtype in table.dtypes.items()} == dtypes
    assert len(frame) == 2
    kinds = sorted(frame["kind"].dropna().tolist())
    assert kinds == ["Custom"]
    assert sorted(frame["dates"].map(lambda dates: dates[0]["role"]).tolist()) == ["due", "start"]
    joined = frame.merge(tables["cards"], on=["project_id", "card_id"])
    assert joined["title"].tolist() == ["Calendar card", "Calendar card"]
    frame.loc[:, "kind"] = "local"
    with holderkit.open(data) as context:
        assert sorted(str(row["kind"]) for row in context.milestones.list(project.project_id, card.card_id)) == [
            "Custom", "None"]


def test_milestone_queries_exclude_trashed_cards(tmp_path: Path) -> None:
    data = tmp_path / "data"
    with holderkit.open(data) as context:
        project = context.create_project("Trash")
        card = context.create_card(project.project_id, "Deleted")
        milestone = context.milestones.add(project.project_id, card.card_id, [{"role": "due", "date": "2026-01-01"}])
    with sqlite3.connect(data / "server" / "holder.db") as database:
        database.execute("UPDATE cards SET deleted_at = 1 WHERE card_id = ?", (card.card_id,))
    with holderkit.open(data) as context:
        assert context.milestones.to_records() == []
        assert context.milestones.calendar(project.project_id, "2026-01-01", "2026-01-01", time_zone="UTC") == []
        with pytest.raises(holderkit.HolderError, match="card not found"):
            context.milestones.update(project.project_id, card.card_id, milestone["milestone_id"], {})


def test_missing_body_mutation_failure_leaves_milestones_unchanged(tmp_path: Path) -> None:
    with holderkit.open(tmp_path / "data") as context:
        project = context.create_project("Missing body")
        card = context.create_card(project.project_id, "Card")
        original = context.milestones.add(project.project_id, card.card_id, [{"role": "due", "date": "2026-01-01"}])
        (Path(project.root_path) / card.rel_path).unlink()
        with pytest.raises(holderkit.HolderError):
            context.milestones.update(project.project_id, card.card_id, original["milestone_id"], {"kind": "new"})
        assert context.milestones.list(project.project_id, card.card_id) == [original]
        with pytest.raises(holderkit.HolderError):
            context.milestones.add(project.project_id, card.card_id)
        assert context.milestones.list(project.project_id, card.card_id) == [original]
        with pytest.raises(holderkit.HolderError):
            context.milestones.remove(project.project_id, card.card_id, original["milestone_id"])
        assert context.milestones.list(project.project_id, card.card_id) == [original]


def test_milestone_read_failure_propagates(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    with holderkit.open(tmp_path / "data") as context:
        project = context.create_project("Failure")
        context.create_card(project.project_id, "Card")

        def fail(self: _native.Context, project_id: str, card_id: str) -> list[dict[str, Any]]:
            raise holderkit.HolderError("milestone read failed")

        monkeypatch.setattr(_native.Context, "list_milestones", fail)
        with pytest.raises(holderkit.HolderError, match="milestone read failed"):
            context.milestones.to_records()
        pytest.importorskip("pandas")
        with pytest.raises(holderkit.HolderError, match="milestone read failed"):
            context.to_dataframes()
