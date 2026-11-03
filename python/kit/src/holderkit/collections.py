"""Live collection facades and detached record exports."""

from __future__ import annotations

import builtins
from datetime import date, datetime
from typing import TYPE_CHECKING, Any, Iterable, Literal, Mapping, overload

from . import _native
from .data.card import (
    CARD_METADATA_RECORD_FIELDS,
    COMPLETE_CARD_RECORD_FIELDS,
    Card,
    CardMetadataRecord,
    CompleteCardRecord,
    _complete_record_from_native,
    _metadata_record_from_native,
)
from .data.project import PROJECT_RECORD_FIELDS, Project, ProjectRecord, _project_record_from_native
from .data.connection import CONNECTION_RECORD_FIELDS, ConnectionRecord
from .data.tag import (
    TAG_RECORD_FIELDS, ProjectTagRecord, TaggedCardRecord, TagRecord,
    TagAddResult, TagRemoveResult,
)
from .dataframe import records_to_dataframe
from .data.milestone import (
    PROJECT_MILESTONE_RECORD_FIELDS, MilestoneRecord, ProjectMilestoneRecord,
    MilestoneUpdate, _dates_to_native, _milestone_record_from_native,
    _project_milestone_record_from_native, _update_to_native,
)

if TYPE_CHECKING:
    import pandas as pd


_COMPLETE_CARD_PAGE_SIZE = 256


def _day(value: date | str, name: str) -> str:
    if isinstance(value, date) and not isinstance(value, datetime):
        return value.isoformat()
    if isinstance(value, str):
        return value
    raise TypeError(f"{name} must be a date or a YYYY-MM-DD string")


class MilestoneCollection:
    """Core-owned milestone operations and detached calendar records."""

    __slots__ = ("_context",)

    def __init__(self, context: _native.Context) -> None:
        self._context = context

    def list(self, project_id: str, card_id: str) -> builtins.list[MilestoneRecord]:
        return [_milestone_record_from_native(item)
                for item in self._context.list_milestones(project_id, card_id)]

    def add(
        self, project_id: str, card_id: str, dates: Iterable[Mapping[str, Any]] = (), *,
        kind: str | None = None, description: str | None = None,
    ) -> MilestoneRecord:
        """Add a milestone with labelled dates and return it."""

        return _milestone_record_from_native(self._context.add_milestone(
            project_id, card_id, _dates_to_native(dates), kind, description,
        ))

    def update(
        self, project_id: str, card_id: str, milestone_id: str, changes: MilestoneUpdate,
    ) -> MilestoneRecord:
        return _milestone_record_from_native(self._context.update_milestone(
            project_id, card_id, milestone_id, _update_to_native(changes),
        ))

    def remove(self, project_id: str, card_id: str, milestone_id: str) -> builtins.list[MilestoneRecord]:
        """Remove a milestone and return the card's remaining milestones."""

        return [_milestone_record_from_native(item)
                for item in self._context.remove_milestone(project_id, card_id, milestone_id)]

    def calendar(
        self, project_id: str, first_day: date | str, last_day: date | str, *, time_zone: str | None = None,
    ) -> builtins.list[ProjectMilestoneRecord]:
        """Milestones with a date in the days from first_day to last_day in time_zone (this device's
        zone when omitted): a day date by its day, an instant by whether it falls in those days there."""

        return [_project_milestone_record_from_native(item) for item in self._context.calendar_milestones(
            project_id, _day(first_day, "first_day"), _day(last_day, "last_day"), time_zone,
        )]

    def to_records(self, project_id: str | None = None) -> builtins.list[ProjectMilestoneRecord]:
        return self._records_from_cards(CardCollection(self._context).to_records(project_id))

    def _records_from_cards(
        self, cards: Iterable[CardMetadataRecord],
    ) -> builtins.list[ProjectMilestoneRecord]:
        return [{**milestone, "card_title": card["title"]}
                for card in cards for milestone in self.list(card["project_id"], card["card_id"])]

    def to_dataframe(self, project_id: str | None = None) -> pd.DataFrame:
        return records_to_dataframe(self.to_records(project_id), PROJECT_MILESTONE_RECORD_FIELDS)


class TagCollection:
    """Core-extracted tags and explicit semantic mutations, not body parsing."""

    __slots__ = ("_context",)

    def __init__(self, context: _native.Context) -> None:
        self._context = context

    def _project_id(self, card_id: str) -> str:
        if not card_id:
            raise ValueError("card_id must not be empty")
        for project in self._context.list_projects():
            project_id = str(project["project_id"])
            try:
                self._context.get_card(project_id, card_id)
            except _native.HolderError as error:
                if not str(error).startswith("card not found:"):
                    raise
                continue
            return project_id
        raise _native.HolderError(f"card not found: {card_id}")

    def list(self, card_id: str) -> builtins.list[str]:
        return self._context.list_tags(self._project_id(card_id), card_id)

    def list_editable(self, card_id: str) -> builtins.list[str]:
        return self._context.list_editable_tags(self._project_id(card_id), card_id)

    def add(self, card_id: str, tag: str) -> TagAddResult:
        if not isinstance(tag, str):
            raise TypeError("tag must be str")
        project_id = self._project_id(card_id)
        present = tag.lower() in self._context.list_tags(project_id, card_id)
        self._context.add_tag(project_id, card_id, tag)
        return TagAddResult.ALREADY_PRESENT if present else TagAddResult.ADDED

    def remove(self, card_id: str, tag: str) -> TagRemoveResult:
        if not isinstance(tag, str):
            raise TypeError("tag must be str")
        project_id = self._project_id(card_id)
        present = tag.lower() in self._context.list_tags(project_id, card_id)
        self._context.remove_tag(project_id, card_id, tag)
        return TagRemoveResult.REMOVED if present else TagRemoveResult.NOT_PRESENT

    def project_counts(self, project_id: str) -> builtins.list[ProjectTagRecord]:
        """Return core's live-card counts, most-used first, then tag name."""

        return [
            {"project_id": project_id, "tag": str(item["tag"]), "card_count": int(item["card_count"])}
            for item in self._context.list_project_tags(project_id)
        ]

    def cards_with_tag(self, project_id: str, tag: str) -> builtins.list[TaggedCardRecord]:
        """Return detached live-card IDs/titles using core's case-insensitive search."""

        return [
            {"card_id": str(item["card_id"]), "title": str(item["title"])}
            for item in self._context.cards_with_tag(project_id, tag)
        ]

    def to_records(self, project_id: str | None = None) -> builtins.list[TagRecord]:
        return self._records_from_cards(CardCollection(self._context).to_records(project_id))

    def _records_from_cards(
        self, cards: Iterable[CardMetadataRecord],
    ) -> builtins.list[TagRecord]:
        records: builtins.list[TagRecord] = []
        for card in cards:
            tags = self._context.list_tags(card["project_id"], card["card_id"])
            if not tags:
                continue
            editable = set(self._context.list_editable_tags(card["project_id"], card["card_id"]))
            records.extend({
                "project_id": card["project_id"], "card_id": card["card_id"],
                "tag": tag, "editable": tag in editable,
            } for tag in tags)
        return records

    def to_dataframe(self, project_id: str | None = None) -> pd.DataFrame:
        return records_to_dataframe(self.to_records(project_id), TAG_RECORD_FIELDS)


class ConnectionCollection:
    """Explicit outgoing connections of live cards, read through core."""

    __slots__ = ("_context",)

    def __init__(self, context: _native.Context) -> None:
        self._context = context

    def add(
        self, from_card_id: str, to_card_id: str, kind: str,
        label: str | None = None,
    ) -> None:
        """Add or update a connection using core's existing upsert semantics."""

        source_project_id = TagCollection(self._context)._project_id(from_card_id)
        target_project_id = TagCollection(self._context)._project_id(to_card_id)
        self._context.add_link(source_project_id, from_card_id,
            target_project_id if target_project_id != source_project_id else None,
            to_card_id, kind, label)

    def remove(self, from_card_id: str, to_card_id: str, kind: str) -> None:
        """Remove a connection; an absent matching connection is a no-op."""

        source_project_id = TagCollection(self._context)._project_id(from_card_id)
        target_project_id = TagCollection(self._context)._project_id(to_card_id)
        self._context.remove_link(source_project_id, from_card_id,
            target_project_id if target_project_id != source_project_id else None,
            to_card_id, kind)

    def to_records(self, project_id: str | None = None) -> builtins.list[ConnectionRecord]:
        return self._records_from_cards(CardCollection(self._context).to_records(project_id))

    def _records_from_cards(
        self, cards: Iterable[CardMetadataRecord],
    ) -> builtins.list[ConnectionRecord]:
        records: builtins.list[ConnectionRecord] = []
        titles: dict[tuple[str, str], str | None] = {}
        for card in cards:
            titles[(card["project_id"], card["card_id"])] = card["title"]
        for card in cards:
            for link in self._context.list_links(card["project_id"], card["card_id"]):
                target_project = str(link["target_project_id"])
                target_id = str(link["target_id"])
                records.append({
                    "project_id": card["project_id"],
                    "from_card_id": card["card_id"],
                    "to_card_id": target_id,
                    # Null for a target in the card's own project, as before.
                    "target_project_id": None if target_project == card["project_id"] else target_project,
                    "to_type": str(link["target_type"]),
                    "kind": str(link["kind"]),
                    "label": None if link["label"] is None else str(link["label"]),
                    "created_at": int(link["created_at"]),
                    "to_title": self._title(titles, target_project, target_id, str(link["target_type"])),
                })
        return records

    def _title(
        self, titles: dict[tuple[str, str], str | None], project_id: str, card_id: str, target_type: str,
    ) -> str | None:
        """A live card target's title; None for a target that is not a live card on this device."""
        if target_type != "card":
            return None
        key = (project_id, card_id)
        if key not in titles:
            try:
                record = self._context.get_card(project_id, card_id)
                titles[key] = None if record["deleted_at"] is not None else str(record["title"])
            except _native.HolderError:
                titles[key] = None
        return titles[key]

    def to_dataframe(self, project_id: str | None = None) -> pd.DataFrame:
        """Return a detached connection table with the shared record schema."""

        return records_to_dataframe(self.to_records(project_id), CONNECTION_RECORD_FIELDS)


class ProjectCollection:
    """Read live projects or export detached project records."""

    __slots__ = ("_context",)

    def __init__(self, context: _native.Context) -> None:
        self._context = context

    def list(self) -> builtins.list[Project]:
        return [Project._from_native(item, self._context) for item in self._context.list_projects()]

    def to_records(self) -> builtins.list[ProjectRecord]:
        return [_project_record_from_native(item) for item in self._context.list_projects()]

    def to_dataframe(self) -> pd.DataFrame:
        """Return a detached, predictably typed project DataFrame."""

        return records_to_dataframe(self.to_records(), PROJECT_RECORD_FIELDS)


class CardCollection:
    """Read live Card handles from a context or export detached records."""

    __slots__ = ("_context",)

    def __init__(self, context: _native.Context) -> None:
        self._context = context

    def _project_ids(self, project_id: str | None) -> builtins.list[str]:
        return (
            [project_id]
            if project_id is not None
            else [item["project_id"] for item in self._context.list_projects()]
        )

    def _complete_records(
        self, project_id: str | None
    ) -> builtins.list[CompleteCardRecord]:
        records: builtins.list[CompleteCardRecord] = []
        for current_project_id in self._project_ids(project_id):
            cursor: str | None = None
            while True:
                page: Mapping[str, Any] = self._context.list_cards_page(
                    current_project_id, cursor=cursor, limit=_COMPLETE_CARD_PAGE_SIZE, include_content=True
                )
                records.extend(
                    _complete_record_from_native(item) for item in page["cards"]
                )
                next_cursor = page["next_cursor"]
                if next_cursor is None:
                    break
                cursor = str(next_cursor)
        return records

    def list(self, project_id: str | None = None) -> builtins.list[Card]:
        cards: builtins.list[Card] = []
        for current_id in self._project_ids(project_id):
            project = Project(self._context, current_id)
            cards.extend(Card._from_native(record, project)
                         for record in self._context.list_cards(current_id))
        return cards

    @overload
    def to_records(
        self,
        project_id: str | None = None,
        *,
        include_content: Literal[False] = False,
    ) -> builtins.list[CardMetadataRecord]: ...

    @overload
    def to_records(
        self,
        project_id: str | None = None,
        *,
        include_content: Literal[True],
    ) -> builtins.list[CompleteCardRecord]: ...

    def to_records(
        self, project_id: str | None = None, *, include_content: bool = False
    ) -> builtins.list[CardMetadataRecord] | builtins.list[CompleteCardRecord]:
        if include_content:
            return self._complete_records(project_id)

        records: builtins.list[CardMetadataRecord] = []
        for current_project_id in self._project_ids(project_id):
            records.extend(
                _metadata_record_from_native(item)
                for item in self._context.list_cards(current_project_id)
            )
        return records

    def to_dataframe(
        self, project_id: str | None = None, *, include_content: bool = False
    ) -> pd.DataFrame:
        """Return a detached card DataFrame, optionally including bodies."""

        if include_content:
            return records_to_dataframe(
                self.to_records(project_id, include_content=True),
                COMPLETE_CARD_RECORD_FIELDS,
            )
        return records_to_dataframe(
            self.to_records(project_id), CARD_METADATA_RECORD_FIELDS
        )
