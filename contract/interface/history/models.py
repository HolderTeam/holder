"""A project's durable Git activity history."""

from typing import Annotated, Literal

from authoring import MISSING, Field, Int32, Int64, Record, Timestamp

from ..cards.models import CardId

type HistoryObjectKind = Literal["card", "resource", "location", "ai_data", "project_settings", "unknown"]

type RevisionRef = Annotated[str, Field(min_length=8, max_length=40)]
"""A full commit ID or a unique leading prefix. Core checks that every character is hexadecimal."""


class HistoryAuthor(Record):
    """The author recorded on a Git commit."""

    name: str
    """The author's name."""
    email: str
    """The author's email address."""


class HistoryAffectedPath(Record):
    """A changed path with any display details found in its historical version."""

    path: str
    """The path relative to the project root."""
    title: str | MISSING = MISSING
    """A historical title, when available."""
    detail: str | MISSING = MISSING
    """A historical detail, when available."""


class HistoryAffectedObject(Record):
    """Paths changed together in one commit, grouped by Holder object kind."""

    kind: HistoryObjectKind
    """What kind of object these paths belong to."""
    paths: list[str]
    """The changed paths, retained for existing clients."""
    items: list[HistoryAffectedPath]
    """The changed paths with historical display details."""


class ProjectActivity(Record):
    """One Git commit affecting a project."""

    oid: str
    """The full commit object ID."""
    parent_oids: list[str]
    """The commit's parents."""
    author: HistoryAuthor
    """The commit's author."""
    authored_at: Timestamp
    """When the commit was authored."""
    committed_at: Timestamp
    """When the commit was committed."""
    message: str
    """The commit message."""
    affected_objects: list[HistoryAffectedObject]
    """Changed paths grouped by Holder object kind."""
    is_merge: bool
    """Whether the commit has multiple parents."""


class ProjectHistoryPage(Record):
    """One bounded page of project activity, newest first."""

    head_oid: str | None
    """The project HEAD captured for this page; null for an empty repository."""
    activities: list[ProjectActivity]
    """The activities on this page, newest first."""
    next_cursor: str | None
    """The cursor for another page; null at the end."""
    scan_limited: bool
    """Whether History reached its bounded scan before the end."""


class CardHistorySave(Record):
    """One exact commit saved for a card."""

    oid: str
    """The full commit ID."""
    parent_oids: list[str]
    """The commit's parents."""
    authored_at: Timestamp
    """When the commit was authored."""
    committed_at: Timestamp
    """When the commit was committed."""
    message: str
    """The commit message."""


class CardHistoryEntry(Record):
    """One displayed card change, possibly grouping adjacent autosaves."""

    first_oid: str
    """The first commit in the entry."""
    last_oid: str
    """The last commit in the entry."""
    parent_oids: list[str]
    """Parents of the first commit."""
    visible_parent_oids: list[str]
    """Direct parents represented on this page; omitted connections must not be inferred."""
    author: HistoryAuthor
    """The author's name and email."""
    started_at: Timestamp
    """When the first commit was authored."""
    ended_at: Timestamp
    """When the last commit was committed."""
    kind: str
    """The kind of change shown."""
    summary: str
    """A short description of the change."""
    commit_count: Annotated[Int32, Field(ge=1)]
    """The number of commits grouped in this entry."""
    is_merge: bool
    """Whether this entry includes a merge commit."""
    saves: list[CardHistorySave]
    """Exact saved commits in chronological order."""


class CardHistoryPage(Record):
    """One bounded page of saved card changes, newest first."""

    head_oid: str | None
    """The project HEAD captured for this page; null for an empty repository."""
    entries: list[CardHistoryEntry]
    """The card changes on this page."""
    next_cursor: str | None
    """The cursor for another page; null at the end."""
    scan_limited: bool
    """Whether History reached its bounded scan before the end."""


class CardHistoryVersion(Record):
    """A card's saved state at one commit, including an explicit absent state."""

    exists: bool
    """Whether the card existed in this snapshot."""
    oid: str
    """The full commit ID."""
    title: str
    """The card's saved title, empty when absent."""
    body: str
    """The card's saved body, empty when absent."""


class CardHistoryDiffLine(Record):
    """One line in a card comparison."""

    origin: str
    """Space for unchanged, plus for added, minus for removed."""
    text: str
    """The line's text."""
    old_line: Int64 | None
    """Its old line number; null for an addition."""
    new_line: Int64 | None
    """Its new line number; null for a removal."""


class CardHistoryComparison(Record):
    """A bounded line comparison of two saved card states."""

    before: CardHistoryVersion
    """The earlier state."""
    after: CardHistoryVersion
    """The later state."""
    summary: str
    """A short description of the difference."""
    lines: list[CardHistoryDiffLine]
    """The comparison's lines."""
    truncated: bool
    """Whether the line comparison was cut short."""


class CardHistorySnapshot(Record):
    """A card's state at a selected revision."""

    card_id: CardId
    """The card being inspected."""
    snapshot: CardHistoryVersion
    """The state at the selected commit."""


class CardHistoryRestoreResult(Record):
    """A card restored from a saved state by creating a new commit."""

    card_id: CardId
    """The restored card."""
    restored_from_oid: str
    """The full commit ID of the selected historical snapshot."""
    result_oid: str
    """The full commit ID created by restoring it."""
    title: str
    """The card's title after restoration."""
    updated_at: Timestamp
    """When the restored card was updated."""
    deleted_at: Timestamp | None
    """When the restored card is in Trash; null when live."""
