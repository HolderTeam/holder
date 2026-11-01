"""Resources: the things a project keeps besides cards, such as websites, books and files, and the copies of
their files."""

from typing import Annotated, Literal

from authoring import MISSING, Field, Int64, Record, Timestamp

from ..projects.ids import ProjectId

type ResourceId = Annotated[str, Field(min_length=4)]
"""A resource's ID, unique on this device: ASCII letters, digits, - and _. Holder makes a UUID."""

type ResourceType = Annotated[str, Field(min_length=1)]
"""What kind of thing a resource is, such as website or book. Types are free-form."""

type ResourceMetadata = dict[str, list[str]]
"""What is known about a resource: each name, such as identifier or author, with its values in order. A name is
not empty."""


class AssetPlacement(Record):
    """One stored copy of an asset."""

    placement_id: Annotated[str, Field(min_length=1)]
    """The copy's ID."""
    location_id: Annotated[str, Field(min_length=1)]
    """The storage location that holds the copy."""
    encoding: Literal["plain", "holder_asset_v1"]
    """How the copy is stored: as the file is, or encrypted for the project."""
    stored_byte_size: Int64
    """The size of the stored copy, in bytes."""
    created_at: Timestamp
    """When the copy was stored."""


class ResourceAsset(Record):
    """A file that belongs to a resource."""

    asset_id: Annotated[str, Field(min_length=1)]
    """The asset's ID."""
    original_filename: str
    """The file's name when it was imported."""
    media_type: str
    """The file's media type, such as application/pdf."""
    byte_size: Int64
    """The file's size, in bytes."""
    plaintext_sha256: str
    """The SHA-256 of the file's content, in lower-case hex."""
    placements: list[AssetPlacement]
    """The stored copies of the file."""
    created_at: Timestamp
    """When the asset was imported."""
    updated_at: Timestamp
    """When the asset was last changed."""


class Resource(Record):
    """A resource in a project, with its files."""

    resource_id: ResourceId
    """The resource's ID."""
    project_id: ProjectId
    """The project the resource belongs to."""
    type: ResourceType
    """What kind of thing the resource is."""
    label: Annotated[str, Field(min_length=1)]
    """The resource's name."""
    metadata: ResourceMetadata
    """What is known about the resource, by name."""
    assets: list[ResourceAsset]
    """The resource's files."""
    created_at: Timestamp
    """When the resource was made."""
    updated_at: Timestamp
    """When the resource was last changed."""


class NewResource(Record):
    """A resource to make."""

    resource_id: ResourceId | MISSING = MISSING
    """The resource's ID, for imports; omit for a new one."""
    type: ResourceType
    """What kind of thing the resource is."""
    label: Annotated[str, Field(min_length=1)]
    """The resource's name."""
    metadata: ResourceMetadata | MISSING = MISSING
    """What is known about the resource; omit for nothing. A name without values is left out."""
    created_at: Timestamp | MISSING = MISSING
    """When the resource was made, for imports; omit for now."""
    updated_at: Timestamp | MISSING = MISSING
    """When the resource was last changed, for imports; omit for when it was made."""


class ResourceUpdate(Record):
    """A change to a resource. Each field left out keeps its value."""

    type: ResourceType | MISSING = MISSING
    """What kind of thing the resource is."""
    label: Annotated[str, Field(min_length=1)] | MISSING = MISSING
    """The resource's name."""
    metadata: ResourceMetadata | MISSING = MISSING
    """Names whose values change: each name's values replace its old ones, an empty list removes the name, and
    names left out are kept."""
    updated_at: Timestamp | MISSING = MISSING
    """When the resource was changed, for imports; omit for now."""


class AssetImportRequest(Record):
    """Start importing a local file into a project resource."""

    project_id: ProjectId
    """The project that will own the asset."""
    card_id: str
    """The card to attach the imported resource to."""
    location_id: str
    """The storage location to receive the asset."""
    source_path: str
    """A local readable regular file; its path is never saved in project metadata."""


class AssetImportJob(Record):
    """The current state of an asset import job, held in daemon memory."""

    job_id: str
    """The job's ID."""
    status: Literal["queued", "staging", "storing", "committing", "completed", "failed"]
    """The import stage."""
    resource_id: str | None
    """The resource created or reused, when known."""
    asset_id: str | None
    """The asset created or reused, when known."""
    duplicate_reused: bool
    """Whether an existing asset was reused."""
    link_created: bool
    """Whether an attachment link was made."""
    error: str | None
    """The failure message, when the job failed."""
