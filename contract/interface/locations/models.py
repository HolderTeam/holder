"""Project storage location declarations and their device binding status."""

from typing import Annotated, Literal

from authoring import MISSING, Field, Record, Timestamp

from ..projects.ids import ProjectId

type LocationId = Annotated[str, Field(min_length=1)]
type LocationProvider = Literal["local_directory", "s3_compatible", "google-drive"]
type LocationConfiguration = dict[str, str]
"""Portable non-secret provider options."""


class ProjectStorageLocation(Record):
    """A portable declaration with this device's binding status."""

    location_id: LocationId
    """The location's ID."""
    project_id: ProjectId
    """The owning project."""
    name: Annotated[str, Field(min_length=1)]
    """Display name."""
    provider: LocationProvider
    """Storage provider kind."""
    configuration: LocationConfiguration
    """Portable non-secret provider configuration."""
    bound: bool
    """Whether this device has a private binding."""
    binding_preview: str | None
    """Safe binding display text, or null; credentials are never returned."""
    created_at: Timestamp
    """When the declaration was created."""
    updated_at: Timestamp
    """When the declaration was last updated."""


class NewStorageLocation(Record):
    """A portable location declaration to create."""

    location_id: str | MISSING = MISSING
    """Optional imported ID; Holder makes one when omitted or empty."""
    project_id: ProjectId
    """The owning project."""
    name: Annotated[str, Field(min_length=1)]
    """Display name."""
    provider: LocationProvider
    """Storage provider kind."""
    configuration: LocationConfiguration | MISSING = MISSING
    """Portable non-secret provider configuration; empty when omitted."""
    created_at: Timestamp | MISSING = MISSING
    """Imported creation time; current time when omitted."""
    updated_at: Timestamp | MISSING = MISSING
    """Imported update time; creation time when omitted."""


class StorageLocationUpdate(Record):
    """Changes to a portable location declaration."""

    name: Annotated[str, Field(min_length=1)] | MISSING = MISSING
    """Replacement display name."""
    configuration: LocationConfiguration | MISSING = MISSING
    """Replacement configuration map."""
    updated_at: Timestamp | MISSING = MISSING
    """Update time; current time when omitted."""


class LocationDeleteResult(Record):
    """The location affected by an action."""

    location_id: LocationId
    """The affected location ID."""


class StorageLocationListResponse(Record):
    """Project storage locations and this device's preference."""

    ok: bool
    """Whether the request succeeded."""
    data: list[ProjectStorageLocation]
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


class StorageLocationList(Record):
    """A project's storage locations, with this device's binding status, and this device's preferred one."""

    data: list[ProjectStorageLocation]
    """The project's locations, most recently updated first; none for a project with none, or one this device does
    not have."""
    preferred_location_id: LocationId | None
    """This device's preferred location for the project; null when it has none."""
