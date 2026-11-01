"""The daemon's liveness, health and departure responses."""

from authoring import Int32, Int64, Record


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
