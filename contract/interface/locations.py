"""Project storage location declarations and their device binding status."""

from typing import Annotated, Literal

from authoring import MISSING, Field, Record, Timestamp

from .projects.ids import ProjectId

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
