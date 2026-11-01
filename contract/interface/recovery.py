"""Export and restore an encrypted project's recovery token."""

from typing import Annotated, Literal

from authoring import Field, Record

from .cards import ProjectId


class RecoveryTokenExportRequest(Record):
    """PIN used to wrap the project key."""

    pin: Annotated[str, Field(min_length=1)]
    """User-provided PIN used to wrap or unwrap the key."""


class RecoveryTokenImportRequest(RecoveryTokenExportRequest):
    """PIN and serialized .hrk token to restore."""

    recovery_token: Annotated[str, Field(min_length=1)]
    """Serialized .hrk token JSON."""


class ProjectRecoveryTokenExportResult(Record):
    """A portable encrypted project key."""

    project_id: ProjectId
    """The project whose key was exported."""
    key_id: str
    """The encrypted project's key ID."""
    recovery_token: str
    """Serialized .hrk token JSON."""


class ProjectRecoveryTokenImportResult(Record):
    """The project whose key was restored."""

    project_id: ProjectId
    """The project whose key was restored."""


class RecoveryTokenGlobalImportResult(ProjectRecoveryTokenImportResult):
    """The result of restoring a token to this device."""

    project_created: bool
    """Whether this device made the project from the token."""
    remote_hint_present: bool
    """Whether the token names a Git remote."""
    remote_configured: bool
    """Whether that remote was configured."""
    remote_error: str | None
    """Why remote configuration failed, or null."""
    pull_status: Literal["not_attempted", "succeeded", "failed"]
    """Outcome of the attempted pull."""
    pull_error: str | None
    """Why pulling failed, or null."""
