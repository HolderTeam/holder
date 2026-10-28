"""A project's durable Git activity history."""

from typing import Literal

from holder_contract import MISSING, Record, Timestamp

type HistoryObjectKind = Literal["card", "resource", "location", "ai_data", "project_settings", "unknown"]


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
