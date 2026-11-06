"""Detached milestone records in the contract's shape, and partial updates.

A milestone has labelled dates. Each date has a role (what it is for, such as "start" or
"deadline") and exactly one of a calendar day (``YYYY-MM-DD``) or an instant (RFC 3339 with an
offset, whole seconds). A day has no time zone; an instant is a moment.
"""

from datetime import date, datetime
from typing import Any, Iterable, Mapping, TypedDict


class _MilestoneDateRole(TypedDict):
    role: str


class MilestoneDateRecord(_MilestoneDateRole, total=False):
    """A role and exactly one of ``date`` (a day) or ``datetime`` (an instant)."""

    date: str
    datetime: str


class MilestoneRecord(TypedDict):
    milestone_id: str
    project_id: str
    card_id: str
    dates: list[MilestoneDateRecord]
    kind: str | None
    description: str | None
    created_at: int
    updated_at: int


class ProjectMilestoneRecord(MilestoneRecord):
    card_title: str | None


class MilestoneUpdate(TypedDict, total=False):
    """Absent keeps a field; ``kind`` and ``description`` accept None to clear. ``dates`` replaces
    all of the milestone's dates."""

    dates: Iterable[Mapping[str, Any]]
    kind: str | None
    description: str | None


MILESTONE_RECORD_FIELDS: tuple[str, ...] = (
    "milestone_id", "project_id", "card_id", "dates", "kind", "description", "created_at", "updated_at",
)
PROJECT_MILESTONE_RECORD_FIELDS: tuple[str, ...] = (*MILESTONE_RECORD_FIELDS, "card_title")


def _date_to_native(value: Mapping[str, Any]) -> dict[str, str]:
    """A date as core takes it: a role and one of a day or an instant, from strings or from
    ``date`` and aware ``datetime`` objects."""
    if not isinstance(value, Mapping):
        raise TypeError("each date must be a mapping with a role and a date or datetime")
    unknown = set(value) - {"role", "date", "datetime"}
    if unknown:
        raise ValueError(f"Unknown milestone date field: {sorted(unknown)[0]}")
    role = value.get("role")
    if not isinstance(role, str) or not role:
        raise ValueError("role must be a nonempty string")
    out = {"role": role}
    day, moment = value.get("date"), value.get("datetime")
    if (day is None) == (moment is None):
        raise ValueError("a date needs exactly one of date or datetime")
    if day is not None:
        if isinstance(day, datetime) or not isinstance(day, (date, str)):
            raise TypeError("date must be a date or a YYYY-MM-DD string")
        out["date"] = day.isoformat() if isinstance(day, date) else day
    else:
        if isinstance(moment, datetime):
            if moment.utcoffset() is None:
                raise ValueError("datetime must have an explicit UTC offset")
            out["datetime"] = moment.isoformat(timespec="seconds")
        elif isinstance(moment, str):
            out["datetime"] = moment
        else:
            raise TypeError("datetime must be an aware datetime or an RFC 3339 string")
    return out


def _dates_to_native(dates: Iterable[Mapping[str, Any]]) -> list[dict[str, str]]:
    if isinstance(dates, (str, bytes, Mapping)):
        raise TypeError("dates must be a sequence of dates")
    return [_date_to_native(value) for value in dates]


def _update_to_native(changes: MilestoneUpdate) -> dict[str, Any]:
    out: dict[str, Any] = {}
    fields: Mapping[str, Any] = changes
    for field, value in fields.items():
        if field == "dates":
            out["dates"] = _dates_to_native(value)
        elif field in {"kind", "description"}:
            if value is not None and not isinstance(value, str):
                raise TypeError(f"{field} must be str or None")
            out[field] = value
        else:
            raise ValueError(f"Unknown milestone update field: {field}")
    return out


def _milestone_record_from_native(item: Mapping[str, Any]) -> MilestoneRecord:
    dates: list[MilestoneDateRecord] = []
    for value in item["dates"]:
        record: MilestoneDateRecord = {"role": str(value["role"])}
        if "date" in value:
            record["date"] = str(value["date"])
        if "datetime" in value:
            record["datetime"] = str(value["datetime"])
        dates.append(record)
    return {
        "milestone_id": str(item["milestone_id"]), "project_id": str(item["project_id"]),
        "card_id": str(item["card_id"]), "dates": dates,
        "kind": None if item["kind"] is None else str(item["kind"]),
        "description": None if item["description"] is None else str(item["description"]),
        "created_at": int(item["created_at"]), "updated_at": int(item["updated_at"]),
    }


def _project_milestone_record_from_native(item: Mapping[str, Any]) -> ProjectMilestoneRecord:
    return {
        **_milestone_record_from_native(item),
        "card_title": None if item.get("card_title") is None else str(item["card_title"]),
    }
