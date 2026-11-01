"""Recovery snapshots: a project's cards, exported so a device that has lost them can restore them.

A recovery snapshot holds a project's live cards with their content, place in the card tree, links,
milestones, and the portable declarations of the resources and storage locations they link to. It
does not hold asset bytes, which stay in their storage locations; credentials or this device's
bindings to locations; Git history; trashed cards; or AI threads and messages. Restoring makes a new
project from it. Core owns the snapshot's encoding: each card is one line, which a platform stores
and hands back without reading it.
"""

from typing import Annotated, Literal

from authoring import MISSING, Field, Float64, Int64, Record, Timestamp

from ..cards.models import CardId
from ..links.models import LinkKind, LinkTargetId, LinkTargetType
from ..locations.models import LocationConfiguration, LocationId, LocationProvider
from ..milestones.models import MilestoneDate
from ..projects.ids import ProjectId
from ..projects.models import PrivacyMode, ProjectName
from ..resources.models import ResourceId, ResourceMetadata, ResourceType


class RecoverySnapshotLink(Record):
    """A card's link, as a snapshot holds it."""

    target_type: LinkTargetType
    """What the link points at. Restore keeps links to cards and resources and drops the rest."""
    target_id: LinkTargetId
    """What the link points at, by its ID when the snapshot was read."""
    target_project_id: ProjectId | MISSING = MISSING
    """For a card in another project, that project; restore keeps such a link as it is."""
    kind: LinkKind
    """What the link means."""
    label: str | None
    """The link's label; null when it has none."""


class RecoverySnapshotMilestone(Record):
    """A card's milestone, as a snapshot holds it."""

    dates: list[MilestoneDate]
    """The milestone's dates."""
    kind: str | None
    """What the milestone is; null when unset."""
    description: str | None
    """More about the milestone; null when unset."""


class RecoverySnapshotPlacement(Record):
    """Where a stored copy of an asset's bytes is, so a restored resource can still be read."""

    placement_id: Annotated[str, Field(min_length=1)]
    """The placement's ID when the snapshot was read; restore gives it a new one."""
    location_id: LocationId
    """The storage location holding the copy, one of the snapshot card's locations."""
    object_key: Annotated[str, Field(min_length=1)]
    """The copy's key within its storage location."""
    encoding: Literal["plain", "holder_asset_v1"]
    """How the copy's bytes are stored."""
    stored_byte_size: Int64
    """The stored copy's size in bytes."""
    stored_sha256: Annotated[str, Field(min_length=1)]
    """The SHA-256 of the stored copy's bytes, as hex."""
    created_at: Timestamp
    """When the copy was stored."""


class RecoverySnapshotAsset(Record):
    """A file kept for a resource, as a snapshot holds it."""

    asset_id: Annotated[str, Field(min_length=1)]
    """The asset's ID when the snapshot was read; restore gives it a new one."""
    original_filename: str
    """The file's name when it was imported."""
    media_type: str
    """The file's media type."""
    byte_size: Int64
    """The file's size in bytes."""
    plaintext_sha256: str
    """The SHA-256 of the file's bytes, as hex."""
    placements: list[RecoverySnapshotPlacement]
    """Where stored copies of the file are."""
    created_at: Timestamp
    """When the asset was imported."""
    updated_at: Timestamp
    """When the asset last changed."""


class RecoverySnapshotResource(Record):
    """A resource a card links to, as a snapshot holds it."""

    resource_id: ResourceId
    """The resource's ID when the snapshot was read; restore gives it a new one and updates links and
    holder://resource/ references in content to it."""
    type: ResourceType
    """What kind of thing the resource is."""
    label: str
    """The resource's label."""
    metadata: ResourceMetadata
    """The resource's metadata."""
    assets: list[RecoverySnapshotAsset]
    """The files kept for the resource."""
    created_at: Timestamp
    """When the resource was made."""
    updated_at: Timestamp
    """When the resource last changed."""


class RecoverySnapshotLocation(Record):
    """A storage location a card's resources use, as a snapshot holds it: its portable declaration only."""

    location_id: LocationId
    """The location's ID when the snapshot was read; restore gives it a new one."""
    name: Annotated[str, Field(min_length=1)]
    """Display name."""
    provider: LocationProvider
    """Storage provider kind."""
    configuration: LocationConfiguration
    """Portable non-secret provider configuration."""
    created_at: Timestamp
    """When the declaration was made."""
    updated_at: Timestamp
    """When the declaration last changed."""


class RecoverySnapshotCard(Record):
    """A card, as a snapshot holds it."""

    card_id: CardId
    """The card's ID when the snapshot was read. Restore gives every card a new ID and uses this one only to
    keep links and the card tree between the cards it restores."""
    project_id: ProjectId
    """The project the card was read from."""
    project_name: ProjectName
    """That project's name."""
    privacy_mode: PrivacyMode
    """That project's privacy mode."""
    title: str
    """The card's title."""
    content: str
    """The card's body."""
    parent_card_id: CardId | None
    """The card's parent; null for a root card. Restore keeps a parent it also restores, and otherwise makes
    the card a root."""
    sort_key: Float64
    """The card's place among its siblings; lower comes first."""
    created_at: Timestamp
    """When the card was created."""
    updated_at: Timestamp
    """When the card was last changed."""
    links: list[RecoverySnapshotLink]
    """The card's outgoing links."""
    milestones: list[RecoverySnapshotMilestone]
    """The card's milestones."""
    resources: list[RecoverySnapshotResource]
    """The resources the card links to."""
    locations: list[RecoverySnapshotLocation]
    """The storage locations those resources' assets are placed in."""


class RecoverySnapshotPage(Record):
    """One page of a project's recovery snapshot."""

    cards: list[RecoverySnapshotCard]
    """The page's cards, most recently updated first."""
    lines: list[str]
    """Each card in core's encoding, in the same order: what a platform stores and later restores."""
    next_cursor: str | None
    """Pass back to read the next page; null on the last page."""


class RecoverySnapshotRestore(Record):
    """A snapshot's cards to restore into a new project."""

    project_name: ProjectName
    """The new project's name."""
    privacy_mode: PrivacyMode
    """The new project's privacy mode."""
    snapshot: str
    """The cards' lines from read_recovery_snapshot, one per line."""
    commit_message: Annotated[str, Field(min_length=1)]
    """The message of the commit that adds the restored cards."""
