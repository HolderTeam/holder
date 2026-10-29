"""Daemon-only health, storage binding, event, and document responses."""

from typing import Annotated, Literal

from holder_contract import MISSING, Field, Int32, Int64, Record

type StorageLocationProvider = Literal["local_directory", "s3_compatible", "google-drive"]
"""The storage provider kind in a location listing."""


class AliveResponse(Record):
    """An unauthenticated liveness response."""

    ok: bool
    """The daemon is alive."""


class ByeResponse(Record):
    """Acknowledgement that a client is leaving."""

    ok: bool
    """The hint was received."""


class HealthPrivacy(Record):
    """The daemon's project encryption support."""

    backend: str
    """Encryption used for encrypted projects."""
    project_mode_supported: bool
    """Whether the daemon supports per-project encryption."""


class HealthData(Record):
    """Current health and version information."""

    db_ok: bool
    """Whether the database is available."""
    uptime_ms: Int64
    """Milliseconds since the HTTP server started."""
    api_version: str
    """The API version."""
    server_version: str
    """The daemon version."""
    pid: Int32
    """The daemon process ID."""
    privacy: HealthPrivacy
    """Project encryption capabilities."""


class HealthResponse(Record):
    """The daemon's health response."""

    ok: bool
    """Whether the request succeeded."""
    data: HealthData
    """Health and version information."""


class StorageLocation(Record):
    """A portable storage declaration with this device's binding status."""

    location_id: str
    """The location ID."""
    project_id: str
    """The owning project."""
    name: str
    """Display name."""
    provider: StorageLocationProvider
    """Storage provider kind."""
    configuration: dict[str, str]
    """Portable non-secret provider configuration."""
    bound: bool
    """Whether this device has a binding."""
    binding_preview: str | None
    """Safe binding display text; credentials are never returned."""
    created_at: Int64
    """Creation time."""
    updated_at: Int64
    """Last update time."""


class StorageLocationListResponse(Record):
    """Project storage locations and this device's preference."""

    ok: bool
    """Whether the request succeeded."""
    data: list[StorageLocation]
    """The project's locations."""
    preferred_location_id: str | None
    """This device's preferred location, when set."""


class LocationBindingRequest(Record):
    """Private provider values for a location on this device."""

    values: dict[str, str]
    """Provider credentials or local path."""
    preview: str | MISSING = MISSING
    """Safe display text for the binding; defaults to Configured."""


class LocationAction(Record):
    """The location affected by a binding or preference change."""

    location_id: str
    """The affected location ID."""
    bound: bool | MISSING = MISSING
    """Whether this device has a binding, when returned."""
    binding_preview: str | None | MISSING = MISSING
    """Safe binding text, when returned."""


class LocationActionResponse(Record):
    """The result of changing a binding or preference."""

    ok: bool
    """Whether the request succeeded."""
    data: LocationAction
    """The affected location."""


class LocationTest(Record):
    """The result of checking a storage location."""

    available: bool
    """Whether the storage probe succeeded."""


class LocationTestResponse(Record):
    """The result of testing a storage location."""

    ok: bool
    """Whether the request succeeded."""
    data: LocationTest
    """Storage availability."""


class PreferredLocationRequest(Record):
    """Set this device's preferred location for a project."""

    project_id: str
    """The owning project."""
    location_id: str
    """The location to prefer."""


class GoogleDriveAuthorization(Record):
    """Where to send the user to authorize Google Drive."""

    authorization_url: str
    """The Google authorization URL."""


class GoogleDriveAuthorizeResponse(Record):
    """The URL to start Google Drive authorization."""

    ok: bool
    """Whether the request succeeded."""
    data: GoogleDriveAuthorization
    """The authorization URL."""


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
