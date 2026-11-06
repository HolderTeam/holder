"""Change-feed checkpoints and events."""

from typing import Annotated, Literal

from authoring import Field, Record


class EventCheckpoint(Record):
    """A replay cursor and the observed Git state."""

    cursor: str
    """Opaque process-local SSE cursor; preserve it for Last-Event-ID."""
    git_revisions: dict[str, str | None]
    """Observed project Git HEADs, null for missing or unborn repositories."""
    history_urls: dict[str, str]
    """Project history endpoints for durable recovery."""


class EventCheckpointResponse(Record):
    """A change-feed checkpoint response."""

    ok: bool
    """Whether the request succeeded."""
    data: EventCheckpoint
    """The checkpoint."""


class ChangeEventData(Record):
    """One entity invalidation from the live change feed."""

    entity: Literal["project", "card", "resource", "location", "thread", "message", "run"]
    """The changed entity type."""
    entity_id: str
    """The changed entity ID."""
    project_id: str | None
    """The owning project, when known."""
    deleted: bool
    """Whether the entity disappeared from the observed database snapshot."""
    git_revision: str | None
    """The observed project Git HEAD, when available."""


class EventResyncData(EventCheckpoint):
    """A feed checkpoint that requires a fresh snapshot."""

    reason: Literal["history_unavailable", "observer_unavailable"]
    """Why replay is unavailable."""
    last_revision: str | None
    """The last revision supplied by the caller, when any."""


type LastRevision = Annotated[str, Field(pattern="^[0-9a-f]{40}$")]
"""A project Git revision used while recovering a stream."""
