"""Holder's operations, as the daemon's routes, and what core's C API needs besides.

Each operation is a FastAPI route whose body never runs: the daemon serves it, and core implements
it, from code generated from the IR. authoring.routes says what a route declares.

A card is named by its project and its card ID; an operation on a card in another project does not find it.
"""

from typing import Annotated, Literal

from fastapi import Body, Header, Path, Query

from authoring import Bytes, Contract, Field, Int32, JsonValue, Stream, Timestamp
from authoring.routes import ErrorResponse

from . import ai
from .card_references import CardReferenceResolveData, CardReferenceResolveRequest
from .cards import (
    Card,
    CardBreadcrumb,
    CardId,
    CardListItem,
    CardMetadata,
    CardMove,
    CardUpdate,
    NewCard,
    Page,
    ProjectId,
    Tag,
    TagCount,
)
from .daemon_routes import (
    AliveResponse,
    ByeResponse,
    ChangeEventData,
    EventCheckpoint,
    EventCheckpointResponse,
    EventResyncData,
    GoogleDriveAuthorizeResponse,
    HealthResponse,
    LastRevision,
    LocationActionResponse,
    LocationBindingRequest,
    LocationTestResponse,
    PreferredLocationRequest,
    StorageLocationListResponse,
)
from .errors import (
    Conflict,
    DataCorrupted,
    HierarchyInvalid,
    HistoryKeyUnavailable,
    HistoryResponseTooLarge,
    HistoryUnavailable,
    InvalidMove,
    KeyMissing,
    KeyringUnavailable,
    NotFound,
    NotTrashed,
    RebuildFailed,
    RecoveryTokenInvalid,
    RevisionAmbiguous,
)
from .git import DueSyncResult, PushOptions, RemoteTest, RemoteTestResult, SyncResult, SyncState
from .history import (
    CardHistoryComparison,
    CardHistoryPage,
    CardHistoryRestoreResult,
    CardHistorySnapshot,
    HistoryObjectKind,
    ProjectHistoryPage,
    RevisionRef,
)
from .links import Link, LinkKind, LinkTargetId, LinkTargetType, NewLink
from .locations import (
    LocationDeleteResult,
    NewStorageLocation,
    ProjectStorageLocation,
    StorageLocationUpdate,
)
from .maintenance import HierarchyRepair, HierarchyRepairRequest, ProjectEncryptionCheck
from .milestones import Calendar, Day, Milestone, MilestoneId, MilestoneRole, MilestoneUpdate, NewMilestone
from .projects import NewProject, Project, ProjectListItem, ProjectUpdate
from .rebuild import RebuildRequest, RebuildResult, ReindexResult
from .recovery import (
    ProjectRecoveryTokenExportResult,
    ProjectRecoveryTokenImportResult,
    RecoveryTokenExportRequest,
    RecoveryTokenGlobalImportResult,
    RecoveryTokenImportRequest,
)
from .resources import AssetImportJob, AssetImportRequest, NewResource, Resource, ResourceId, ResourceUpdate
from .search import SearchCard, SearchMessage
from .trash import TrashDeleteResult, TrashEmptyResult, TrashFilter, TrashItem, TrashItemType

contract = Contract(statuses={
    NotFound: 404,
    Conflict: 409,
    # The project's state rules the request out until its key is restored.
    KeyMissing: 409,
    KeyringUnavailable: 503,
    # Not something the caller can fix; the code names the condition.
    DataCorrupted: 500,
    InvalidMove: 422,
    HierarchyInvalid: 409,
    HistoryResponseTooLarge: 413,
    HistoryKeyUnavailable: 409,
    RevisionAmbiguous: 409,
    NotTrashed: 409,
    HistoryUnavailable: 503,
    RecoveryTokenInvalid: 400,
    RebuildFailed: 400,
})
# What every write to a card's file may fail with, besides its own errors: an encrypted project's key.
KEYS = [KeyringUnavailable, KeyMissing, DataCorrupted]
app = contract.app


@contract.route("GET", "/locations/{location_id}", errors=[NotFound])
def get_storage_location(location_id: Annotated[str, Path(min_length=1)]) -> ProjectStorageLocation:
    """Get a storage declaration and this device's binding status."""


@contract.route("POST", "/locations", status=201, errors=[NotFound])
def create_storage_location(request: Annotated[NewStorageLocation, Body()]) -> ProjectStorageLocation:
    """Create a project's portable storage declaration."""


@contract.route("PATCH", "/locations/{location_id}", errors=[NotFound])
def update_storage_location(
    location_id: Annotated[str, Path(min_length=1)], request: Annotated[StorageLocationUpdate, Body()],
) -> ProjectStorageLocation:
    """Change a storage declaration; configuration replaces the whole map."""


@contract.route("DELETE", "/locations/{location_id}", errors=[NotFound, Conflict])
def delete_storage_location(location_id: Annotated[str, Path(min_length=1)]) -> LocationDeleteResult:
    """Delete an unused declaration, its binding and its preferred setting on this device."""


@contract.route("GET", "/projects/{project_id}/cards/{card_id}",
                errors=[NotFound, KeyringUnavailable, KeyMissing, DataCorrupted])
def get_card(project_id: Annotated[ProjectId, Path()], card_id: Annotated[CardId, Path()]) -> Card:
    """Fetch a card.

    A card in the trash or in another project is not found.
    """


@contract.route("GET", "/projects/{project_id}/cards/{card_id}/metadata", errors=[NotFound])
def get_card_metadata(project_id: Annotated[ProjectId, Path()], card_id: Annotated[CardId, Path()]) -> CardMetadata:
    """Fetch a live card's metadata without reading its content.

    A card in the trash or in another project is not found.
    """


@contract.route("GET", "/projects/{project_id}/cards/{card_id}/breadcrumbs",
                errors=[NotFound, HierarchyInvalid])
def get_card_breadcrumbs(
    project_id: Annotated[ProjectId, Path()], card_id: Annotated[CardId, Path()],
) -> list[CardBreadcrumb]:
    """Read a live card's path from its project root.

    The project comes first, then live ancestor cards and the requested card. A missing or trashed
    card is not found; a broken or cyclic ancestor chain is invalid.
    """


@contract.route("POST", "/card-references/resolve", errors=[NotFound])
def resolve_card_reference(
    request: Annotated[CardReferenceResolveRequest, Body()],
) -> CardReferenceResolveData:
    """Resolve a card reference within a project and deletion scope."""


@contract.route("GET", "/projects/{project_id}/cards",
                errors=[NotFound, KeyringUnavailable, KeyMissing, DataCorrupted])
def list_cards(
    project_id: Annotated[ProjectId, Path()],
    parent: Annotated[Literal["roots"] | CardId | None, Query(
        description="Omit for every level; roots selects root cards; a card ID selects its children.")] = None,
    tag: Annotated[Tag | None, Query(description="Only cards with this tag.")] = None,
    order: Annotated[Literal["card_id_asc", "updated_desc", "sort_key_asc"], Query(
        description="updated_desc breaks ties by descending card ID; sort_key_asc follows sibling order.")]
    = "card_id_asc",
    limit: Annotated[Int32, Query(ge=1, le=5000, description="At most 1000 when include_content is true.")] = 200,
    cursor: Annotated[Annotated[str, Field(min_length=1, max_length=8192)] | None, Query(
        description="The previous page's next_cursor. Keep the project, filters and order unchanged.")] = None,
    include_content: Annotated[bool, Query(description="Include each card's body.")] = False,
    include_child_count: Annotated[bool, Query(description="Include each card's count of live children.")] = False,
) -> Page[CardListItem]:
    """Page through a project's live cards.

    Filters combine. Each page reads current data; iterating is not a snapshot.
    """


@contract.route("POST", "/projects/{project_id}/cards", status=201,
                errors=[NotFound, Conflict, KeyringUnavailable, KeyMissing, DataCorrupted])
def create_card(project_id: Annotated[ProjectId, Path()], card: Annotated[NewCard, Body()]) -> Card:
    """Create a card.

    Fails with NotFound when the project or the parent does not exist.
    """


@contract.route("PATCH", "/projects/{project_id}/cards/{card_id}", errors=[NotFound, InvalidMove, *KEYS])
def update_card(
    project_id: Annotated[ProjectId, Path()],
    card_id: Annotated[CardId, Path()],
    update: Annotated[CardUpdate, Body()],
) -> Card:
    """Change a card.

    Each field left out keeps its value, and an update that leaves every field out changes nothing. The title and
    content are written first, then the parent and place, each as its own change in the project's history. A card
    in the trash or in another project is not found, and neither is a new parent that does not exist or is in the
    trash; a parent under the card itself is an InvalidMove.
    """


@contract.route("DELETE", "/projects/{project_id}/cards/{card_id}", errors=[NotFound, *KEYS])
def trash_card(
    project_id: Annotated[ProjectId, Path()],
    card_id: Annotated[CardId, Path()],
    children: Annotated[Literal["promote", "trash_subtree"], Query(
        description="promote moves the card's live children into its place among its siblings, keeping their own "
                    "descendants; trash_subtree moves every live descendant to the trash with it.")] = "promote",
) -> Card:
    """Move a card to the trash.

    The card and every card that changes with it are written as one change in the project's history. A card already
    in the trash, or in another project, is not found. Returns the card as it is in the trash.
    """


@contract.route("POST", "/projects/{project_id}/cards/{card_id}/restore", errors=[NotFound, Conflict, *KEYS])
def restore_card(project_id: Annotated[ProjectId, Path()], card_id: Annotated[CardId, Path()]) -> Card:
    """Restore a card from the trash.

    Restores this card only. It returns to its parent when that parent is live and reachable, otherwise to its
    nearest live, reachable ancestor or the top level. Children promoted when it was trashed stay where they are;
    descendants trashed with it stay in the trash, and come back under it when restored. A card in another
    project is not found, and one that is not in the trash is a Conflict.
    """


@contract.route("POST", "/projects/{project_id}/cards/{card_id}/move", errors=[NotFound, InvalidMove, *KEYS])
def move_card(
    project_id: Annotated[ProjectId, Path()],
    card_id: Annotated[CardId, Path()],
    move: Annotated[CardMove, Body()],
) -> Card:
    """Move a card in its project's hierarchy.

    When the card's new neighbours leave no room between their places, nearby siblings are re-spaced in the same
    change, keeping their order; other cards' sort_key values a caller holds may then be out of date. A card in the
    trash or in another project is not found, and neither is a target or parent that does not exist or is in the
    trash. A move that changes nothing writes nothing.
    """


@contract.route("PUT", "/projects/{project_id}/cards/{card_id}/tags/{tag}", errors=[NotFound, *KEYS])
def add_card_tag(
    project_id: Annotated[ProjectId, Path()],
    card_id: Annotated[CardId, Path()],
    tag: Annotated[Tag, Path()],
) -> Card:
    """Tag a card.

    The tag is added to the card's trailing tag line, which is made when the card has none. A card that already
    carries the tag, anywhere in its content, is left as it is. A card in the trash or in another project is not
    found.
    """


@contract.route("DELETE", "/projects/{project_id}/cards/{card_id}/tags/{tag}", errors=[NotFound, Conflict, *KEYS])
def remove_card_tag(
    project_id: Annotated[ProjectId, Path()],
    card_id: Annotated[CardId, Path()],
    tag: Annotated[Tag, Path()],
) -> Card:
    """Take a tag off a card.

    The tag is removed from the card's trailing tag line, and the line with it when it is left empty, so that the
    card no longer carries it. A card that does not carry the tag is left as it is. Holder does not rewrite prose,
    so a card that would still carry the tag in its prose is a Conflict, and is left as it is. A card in the trash
    or in another project is not found.
    """


@contract.route("GET", "/projects/{project_id}/tags", errors=[NotFound])
def list_project_tags(project_id: Annotated[ProjectId, Path()]) -> list[TagCount]:
    """List a project's tags.

    Each tag carried by a live card in the project, with how many carry it, by descending count and then by tag.
    """


@contract.route("GET", "/projects/{project_id}/cards/{card_id}/links", errors=[NotFound])
def list_card_links(
    project_id: Annotated[ProjectId, Path()],
    card_id: Annotated[CardId, Path()],
    include_deleted: Annotated[bool, Query(
        description="Include links to targets in the trash. A link to a card in another project is listed whether "
                    "or not that card can be found here.")] = False,
) -> list[Link]:
    """List a card's links.

    The links the card makes, newest first. A card in the trash or in another project is not found.
    """


@contract.route("GET", "/projects/{project_id}/cards/{card_id}/backlinks", errors=[NotFound])
def list_card_backlinks(
    project_id: Annotated[ProjectId, Path()],
    card_id: Annotated[CardId, Path()],
    scope: Annotated[Literal["local", "external", "all_available"], Query(
        description="local lists links from the card's own project; external, links from the other projects on "
                    "this device; all_available, both.")] = "local",
    include_deleted: Annotated[bool, Query(description="Include links from cards and AI messages in the trash.")]
    = False,
) -> list[Link]:
    """List the links to a card.

    Links from cards and AI messages, newest first. External links come only from projects on this device, so
    they are never all there may be. A card in the trash or in another project is not found.
    """


@contract.route("POST", "/projects/{project_id}/cards/{card_id}/links", errors=[NotFound, *KEYS])
def add_card_link(
    project_id: Annotated[ProjectId, Path()],
    card_id: Annotated[CardId, Path()],
    link: Annotated[NewLink, Body()],
) -> Link:
    """Link a card to something.

    A link is one target, of one type in one project, with one kind. Making a link that already exists sets its
    label and keeps when it was made. A target in the card's project must exist and not be in the trash; a card in
    another project may be one this device does not have. A card in the trash or in another project is not found,
    and neither is a target that does not exist or is in the trash.
    """


@contract.route("DELETE", "/projects/{project_id}/cards/{card_id}/links", errors=[NotFound, *KEYS])
def remove_card_link(
    project_id: Annotated[ProjectId, Path()],
    card_id: Annotated[CardId, Path()],
    target_type: Annotated[LinkTargetType, Query()],
    target_id: Annotated[LinkTargetId, Query()],
    kind: Annotated[LinkKind, Query()] = "ref",
    target_project_id: Annotated[ProjectId | None, Query(description="Omit for the card's own project.")] = None,
) -> list[Link]:
    """Remove a link from a card.

    Removes the one link to that target, of that type in that project, with that kind, and returns the card's
    remaining links as list_card_links lists them by default. A card without that link is left as it is. A card in
    the trash or in another project is not found.
    """


@contract.route("GET", "/projects/{project_id}/cards/{card_id}/milestones", errors=[NotFound])
def list_card_milestones(project_id: Annotated[ProjectId, Path()], card_id: Annotated[CardId, Path()]) -> list[Milestone]:
    """List a card's milestones.

    By when they were made, then by ID. A card in the trash or in another project is not found.
    """


@contract.route("POST", "/projects/{project_id}/cards/{card_id}/milestones", status=201, errors=[NotFound, *KEYS])
def add_card_milestone(
    project_id: Annotated[ProjectId, Path()],
    card_id: Annotated[CardId, Path()],
    milestone: Annotated[NewMilestone, Body()],
) -> Milestone:
    """Put a milestone on a card.

    The milestone gets a new ID. Each date must be a real day or instant. A card in the trash or in another project
    is not found.
    """


@contract.route("PATCH", "/projects/{project_id}/cards/{card_id}/milestones/{milestone_id}",
                errors=[NotFound, *KEYS])
def update_card_milestone(
    project_id: Annotated[ProjectId, Path()],
    card_id: Annotated[CardId, Path()],
    milestone_id: Annotated[MilestoneId, Path()],
    update: Annotated[MilestoneUpdate, Body()],
) -> Milestone:
    """Change a milestone.

    Each field left out keeps its value, and an update that changes nothing writes nothing. Each date must be a real
    day or instant. A card in the trash or in another project is not found, and neither is a milestone that is not
    on the card.
    """


@contract.route("DELETE", "/projects/{project_id}/cards/{card_id}/milestones/{milestone_id}",
                errors=[NotFound, *KEYS])
def remove_card_milestone(
    project_id: Annotated[ProjectId, Path()],
    card_id: Annotated[CardId, Path()],
    milestone_id: Annotated[MilestoneId, Path()],
) -> list[Milestone]:
    """Take a milestone off a card.

    Returns the card's remaining milestones, as list_card_milestones lists them. A card without the milestone is
    left as it is. A card in the trash or in another project is not found.
    """


@contract.route("GET", "/projects/{project_id}/milestones/days", errors=[NotFound])
def list_milestones_by_day(
    project_id: Annotated[ProjectId, Path()],
    first_day: Annotated[Day, Query()],
    last_day: Annotated[Day, Query()],
    role: Annotated[MilestoneRole | None, Query(description="Only dates with this role; omit for any.")] = None,
) -> list[Milestone]:
    """List a project's milestones with a day in a range.

    The range holds first_day, last_day and the days between; it is empty when first_day is after last_day. A
    milestone is listed once, by its first day in the range and then by ID. Milestones on cards in the trash are
    left out.
    """


@contract.route("GET", "/projects/{project_id}/milestones/instants", errors=[NotFound])
def list_milestones_by_instant(
    project_id: Annotated[ProjectId, Path()],
    first_instant: Annotated[Timestamp, Query()],
    last_instant: Annotated[Timestamp, Query()],
    role: Annotated[MilestoneRole | None, Query(description="Only dates with this role; omit for any.")] = None,
) -> list[Milestone]:
    """List a project's milestones with an instant in a range.

    The range holds first_instant, last_instant and the instants between; it is empty when first_instant is after
    last_instant. A milestone is listed once, by its first instant in the range and then by ID. Milestones on cards
    in the trash are left out.
    """


@contract.route("GET", "/projects/{project_id}/calendar", errors=[NotFound])
def get_calendar(
    project_id: Annotated[ProjectId, Path()],
    first_instant: Annotated[Timestamp, Query()],
    last_instant: Annotated[Timestamp, Query()],
    first_day: Annotated[Day, Query(description="The day first_instant falls on where the caller is.")],
    last_day: Annotated[Day, Query(description="The day last_instant falls on where the caller is.")],
) -> Calendar:
    """What happens in a project over a window of time.

    The caller gives the window twice, as instants and as the days they fall on where the caller is, because a day
    has no time zone: a milestone is in the window when one of its days is in the days, or one of its instants in
    the instants. Cards are made and changed at instants. Either range is empty when its start is after its end.
    """


@contract.route("GET", "/projects/{project_id}/resources", errors=[NotFound])
def list_resources(project_id: Annotated[ProjectId, Path()]) -> list[Resource]:
    """List a project's resources.

    By when they last changed, latest first, then by ID.
    """


@contract.route("GET", "/projects/{project_id}/resources/{resource_id}", errors=[NotFound])
def get_resource(project_id: Annotated[ProjectId, Path()], resource_id: Annotated[ResourceId, Path()]) -> Resource:
    """Fetch a resource.

    A resource in another project is not found.
    """


@contract.route("POST", "/projects/{project_id}/resources", status=201, errors=[NotFound, Conflict, *KEYS])
def create_resource(project_id: Annotated[ProjectId, Path()], resource: Annotated[NewResource, Body()]) -> Resource:
    """Make a resource.

    A project that does not exist is not found. Resource IDs are unique on this device, so an ID already in use,
    in this project or another, is a Conflict.
    """


@contract.route("PATCH", "/projects/{project_id}/resources/{resource_id}", errors=[NotFound, *KEYS])
def update_resource(
    project_id: Annotated[ProjectId, Path()],
    resource_id: Annotated[ResourceId, Path()],
    update: Annotated[ResourceUpdate, Body()],
) -> Resource:
    """Change a resource.

    Each field left out keeps its value. A resource in another project is not found.
    """


@contract.route("DELETE", "/projects/{project_id}/resources/{resource_id}", errors=[NotFound, *KEYS])
def delete_resource(project_id: Annotated[ProjectId, Path()], resource_id: Annotated[ResourceId, Path()]) -> Resource:
    """Delete a resource.

    The resource goes, and so do the links to it from cards. Its files' stored copies are left where they are.
    Returns the resource as it was. A resource in another project is not found.
    """


@contract.route("GET", "/projects/{project_id}/resources/{resource_id}/backlinks", errors=[NotFound])
def list_resource_backlinks(
    project_id: Annotated[ProjectId, Path()],
    resource_id: Annotated[ResourceId, Path()],
    include_deleted: Annotated[bool, Query(description="Include links from cards and AI messages in the trash.")]
    = False,
) -> list[Link]:
    """List the links to a resource.

    Links of every kind, attachments among them, from cards and AI messages in the resource's project, newest
    first. A resource in another project is not found.
    """


@contract.route("GET", "/projects/{project_id}/cards/{card_id}/resources", errors=[NotFound])
def list_card_resources(project_id: Annotated[ProjectId, Path()], card_id: Annotated[CardId, Path()]) -> list[Resource]:
    """List the resources attached to a card.

    By when they last changed, latest first, then by ID. A card in the trash or in another project is not found.
    """


@contract.route("PUT", "/projects/{project_id}/cards/{card_id}/resources/{resource_id}", errors=[NotFound, *KEYS])
def attach_card_resource(
    project_id: Annotated[ProjectId, Path()],
    card_id: Annotated[CardId, Path()],
    resource_id: Annotated[ResourceId, Path()],
) -> list[Resource]:
    """Attach a resource to a card.

    An attachment is the card's link of kind attachment to the resource. A card that has it already is left as it
    is. Returns the card's attached resources, as list_card_resources lists them. A card in the trash or in another
    project is not found, and neither is a resource that is not in the card's project.
    """


@contract.route("DELETE", "/projects/{project_id}/cards/{card_id}/resources/{resource_id}",
                errors=[NotFound, *KEYS])
def detach_card_resource(
    project_id: Annotated[ProjectId, Path()],
    card_id: Annotated[CardId, Path()],
    resource_id: Annotated[ResourceId, Path()],
) -> list[Resource]:
    """Detach a resource from a card.

    The resource itself is kept. A card without the attachment is left as it is, and a resource that no longer
    exists can still be detached. Returns the card's attached resources, as list_card_resources lists them. A card
    in the trash or in another project is not found.
    """


@contract.route("POST", "/imports", status=202, core=False, responses={
    404: {"model": ErrorResponse, "description": "Storage location not found in the project."},
    409: {"model": ErrorResponse, "description": "Storage binding required on this device."},
    501: {"model": ErrorResponse, "description": "The daemon cannot import assets."},
    503: {"model": ErrorResponse, "description": "Secret store unavailable."},
})
def start_asset_import(request: Annotated[AssetImportRequest, Body()]) -> AssetImportJob:
    """Start a local-file asset import.

    The location and its credentials are checked before the job is queued. Poll the job for later failures.
    """


@contract.route("GET", "/imports/{job_id}", core=False, responses={
    404: {"model": ErrorResponse, "description": "Job not found; jobs are held in daemon memory."},
})
def get_asset_import_job(job_id: Annotated[str, Path()]) -> AssetImportJob:
    """Poll an asset import job.

    A job queued before the daemon restarted is not found.
    """


@contract.route("GET", "/resources/{resource_id}/assets/{asset_id}/content", core=False, responses={
    200: {"headers": {
        "Content-Type": {"description": "Stored asset media type.", "schema": {"type": "string"}},
        "Content-Disposition": {"description": "Attachment with sanitized filename.",
                                "schema": {"type": "string"}},
    }},
    404: {"model": ErrorResponse, "description": "Resource, asset, or placement not found."},
    409: {"model": ErrorResponse, "description": "Storage binding required or storage conflict."},
    422: {"model": ErrorResponse, "description": "Asset integrity verification failed."},
    502: {"model": ErrorResponse, "description": "Storage authentication or permission failure."},
    503: {"model": ErrorResponse, "description": "Storage unavailable or transient failure."},
    507: {"model": ErrorResponse, "description": "Storage capacity exceeded."},
})
def get_resource_asset_content(
    resource_id: Annotated[str, Path()], asset_id: Annotated[str, Path()],
    placement_id: Annotated[str | None, Query()] = None,
) -> Bytes:
    """Stream verified asset content.

    This authenticated download returns raw bytes. Content-Type preserves the asset media type and
    Content-Disposition contains a sanitized original filename. Without placement_id the daemon selects a
    stored placement.
    """


@contract.route("GET", "/projects", errors=[])
def list_projects(
    name: Annotated[Annotated[str, Field(min_length=1)] | None, Query(
        description="Only projects whose name contains this text.")] = None,
    updated_after: Annotated[Timestamp | None, Query(
        description="Only projects last changed at or after this instant.")] = None,
    updated_before: Annotated[Timestamp | None, Query(
        description="Only projects last changed at or before this instant.")] = None,
    order: Annotated[Literal["updated_at_desc", "updated_at_asc", "created_at_desc", "created_at_asc", "name_desc",
                             "name_asc"] | None, Query(
        description="Omit for Home first, then the latest changed.")] = None,
    limit: Annotated[Int32, Query(ge=1, le=1000)] = 100,
    offset: Annotated[Int32, Query(ge=0, description="How many of the ordered projects to skip.")] = 0,
    include_card_counts: Annotated[bool, Query(description="Include each project's counts of live cards.")] = False,
) -> list[ProjectListItem]:
    """List the projects on this device.

    Filters combine. Projects that tie in the order keep the order they were stored in.
    """


@contract.route("GET", "/projects/{project_id}", errors=[NotFound])
def get_project(project_id: Annotated[ProjectId, Path()]) -> Project:
    """Fetch a project."""


@contract.route("POST", "/projects", status=201, errors=[Conflict, *KEYS])
def create_project(project: Annotated[NewProject, Body()]) -> Project:
    """Make a project.

    Its repository is made, with its settings, and an encrypted project's key. An ID a project on this device has
    is a Conflict. Making a project that was removed from this device makes it known here again.
    """


@contract.route("PATCH", "/projects/{project_id}", errors=[NotFound, *KEYS])
def update_project(project_id: Annotated[ProjectId, Path()], update: Annotated[ProjectUpdate, Body()]) -> Project:
    """Change a project's settings.

    Each field left out keeps its value, and an update that changes nothing writes nothing. The settings are
    committed to the project's repository; a Git remote is set there before it is stored, and one Git does not
    accept is an invalid argument.
    """


@contract.route("DELETE", "/projects/{project_id}", errors=[NotFound])
def delete_project(project_id: Annotated[ProjectId, Path()]) -> Project:
    """Remove a project from this device.

    The project's files and keys are kept, and its removal is remembered, so that looking for projects does not
    bring it back; making it again does. Returns the project as it was.
    """


@contract.route("GET", "/projects/{project_id}/encryption-check", errors=[NotFound])
def check_project_encryption(project_id: Annotated[ProjectId, Path()]) -> ProjectEncryptionCheck:
    """Check whether an encrypted project's card files are safe to push.

    A plain project needs no check and reports a successful result with zero files.
    """


@contract.route("POST", "/projects/{project_id}/repair-hierarchy", errors=[NotFound, *KEYS])
def repair_project_hierarchy(
    project_id: Annotated[ProjectId, Path()], request: Annotated[HierarchyRepairRequest, Body()],
) -> HierarchyRepair:
    """Find hidden live card branches, and optionally move them into the reachable tree.

    An empty body plans without writing. Applying moves each hidden branch to its nearest reachable
    ancestor in one commit; a second apply changes nothing. Parent cycles are reported and left alone.
    """


@contract.route("POST", "/projects/{project_id}/recovery-token/export",
                errors=[NotFound, KeyringUnavailable, KeyMissing, DataCorrupted])
def export_project_recovery_token(
    project_id: Annotated[ProjectId, Path()], request: Annotated[RecoveryTokenExportRequest, Body()],
) -> ProjectRecoveryTokenExportResult:
    """Wrap an encrypted project's key in a portable token protected by a PIN."""


@contract.route("POST", "/projects/{project_id}/recovery-token/import",
                errors=[NotFound, RecoveryTokenInvalid, KeyringUnavailable, DataCorrupted])
def import_project_recovery_token(
    project_id: Annotated[ProjectId, Path()], request: Annotated[RecoveryTokenImportRequest, Body()],
) -> ProjectRecoveryTokenImportResult:
    """Restore an encrypted project's key from a token for that project."""


@contract.route("POST", "/recovery-token/import",
                errors=[RecoveryTokenInvalid, KeyringUnavailable, DataCorrupted],
                created_when="project_created")
def import_recovery_token_global(request: Annotated[RecoveryTokenImportRequest, Body()]) -> RecoveryTokenGlobalImportResult:
    """Restore a token, finding its project here or making one from the token's metadata.

    If the token names a Git remote, configure it and attempt a pull. Remote and pull failures
    are reported in the result. Returns 201 when the project was created, otherwise 200.
    """


@contract.route("POST", "/rebuild", errors=[NotFound, RebuildFailed])
def rebuild_project(request: Annotated[RebuildRequest, Body()]) -> RebuildResult:
    """Rebuild a project's database state from its files.

    A missing project is NotFound. Files that cannot be read back give RebuildFailed.
    """


@contract.route("POST", "/reindex")
def reindex_search() -> ReindexResult:
    """Rebuild all search indexes from database state."""


@contract.route("GET", "/projects/{project_id}/git/sync-status", errors=[NotFound])
def get_project_sync_state(project_id: Annotated[ProjectId, Path()]) -> SyncState:
    """What this device last recorded about a project's sync."""


@contract.route("POST", "/projects/{project_id}/git/pull", errors=[NotFound, *KEYS])
def pull_project(project_id: Annotated[ProjectId, Path()]) -> SyncResult:
    """Pull a project's checked-out branch from its Git remote.

    Changes fast-forward, or merge card by card where both sides changed, and the project's index is updated
    before the result returns. A Git failure is an outcome in the result, recorded in the sync state. Git work on
    one project runs one at a time.
    """


@contract.route("POST", "/projects/{project_id}/git/push", errors=[NotFound, *KEYS])
def push_project(project_id: Annotated[ProjectId, Path()], options: Annotated[PushOptions, Body()]) -> SyncResult:
    """Push a project to its Git remote.

    An encrypted project is checked first, so no plaintext card leaves the device: one whose files are not safe
    to push is DataCorrupted, and nothing is pushed. A Git failure is an outcome in the result, recorded in the
    sync state.
    """


@contract.route("POST", "/projects/{project_id}/git/sync", errors=[NotFound, *KEYS])
def sync_project(project_id: Annotated[ProjectId, Path()], options: Annotated[PushOptions, Body()]) -> SyncResult:
    """Pull a project, then push it.

    The push is tried only when the pull succeeds. Otherwise as pull_project and push_project.
    """


@contract.route("POST", "/projects/{project_id}/git/sync-if-due", errors=[NotFound, *KEYS])
def sync_project_if_due(
    project_id: Annotated[ProjectId, Path()],
    push_interval_seconds: Annotated[Int32, Query(ge=0)] = 0,
    pull_interval_seconds: Annotated[Int32, Query(ge=0)] = 0,
) -> DueSyncResult:
    """Try the project's pull and push phases only when their cadence and retry policy say they are due.

    Zero uses the built-in intervals (20 minutes for push, 5 minutes for pull). A project without a remote and
    a project with nothing due return both phases as not attempted without changing sync state. A due push can
    run after a failed pull; this keeps scheduled pushes independent from the pull retry schedule.
    Git failures are phase outcomes and are recorded in the project's sync state.
    """


@contract.route("POST", "/projects/{project_id}/git/test-remote", errors=[NotFound])
def test_project_remote(project_id: Annotated[ProjectId, Path()],
                        remote: Annotated[RemoteTest, Body()]) -> RemoteTestResult:
    """Test whether a Git remote can be reached, with this device's credentials.

    Lists the remote's refs and nothing more: nothing is fetched, and neither the project nor its repository
    changes.
    """


@contract.route("GET", "/projects/{project_id}/history",
                errors=[NotFound, HistoryKeyUnavailable, HistoryResponseTooLarge, HistoryUnavailable])
def list_project_history(
    project_id: Annotated[ProjectId, Path()],
    limit: Annotated[Int32, Query(ge=1, le=200)] = 50,
    cursor: Annotated[str | None, Query(description="A full commit OID from next_cursor.")] = None,
    kind: Annotated[HistoryObjectKind | None, Query(description="Only activities affecting this object kind.")] = None,
) -> ProjectHistoryPage:
    """Return bounded, cursor-paginated Git activities for a project.

    Each commit groups its changed paths by Holder object kind. Unrecognised paths remain visible as unknown.
    A missing decryption key is HistoryKeyUnavailable; other history read failures are HistoryUnavailable.
    """


@contract.route("GET", "/projects/{project_id}/history/cards/{card_id}",
                errors=[NotFound, HistoryKeyUnavailable, HistoryResponseTooLarge, HistoryUnavailable])
def list_card_history(
    project_id: Annotated[ProjectId, Path()],
    card_id: Annotated[CardId, Path()],
    limit: Annotated[Int32, Query(ge=1, le=200)] = 50,
    cursor: Annotated[str | None, Query(description="A full commit ID from next_cursor.")] = None,
) -> CardHistoryPage:
    """List saved history for a card.

    Returns commits reachable from the captured project HEAD that changed the card. Adjacent autosaves may be
    grouped into one displayed entry.
    """


@contract.route("GET", "/projects/{project_id}/history/cards/{card_id}/compare",
                errors=[NotFound, RevisionAmbiguous, HistoryKeyUnavailable, HistoryResponseTooLarge,
                        HistoryUnavailable])
def compare_card_history(
    project_id: Annotated[ProjectId, Path()],
    card_id: Annotated[CardId, Path()],
    to_oid: Annotated[RevisionRef, Query(description="The ending revision, or the change to inspect.")],
    from_oid: Annotated[RevisionRef | None, Query(description="Required in since mode; ignored in change mode.")]
    = None,
    mode: Annotated[Literal["since", "change"], Query(description="Compare two snapshots or one introduced change.")]
    = "since",
) -> CardHistoryComparison:
    """Compare saved card versions or one revision's change.

    In since mode, compares the selected before and after snapshots. In change mode, compares the first parent
    of to_oid with to_oid. A root commit has a previously nonexistent card as its before state; a merge uses only
    its first parent.
    """


@contract.route("GET", "/projects/{project_id}/history/cards/{card_id}/snapshot",
                errors=[NotFound, RevisionAmbiguous, HistoryKeyUnavailable, HistoryResponseTooLarge,
                        HistoryUnavailable])
def get_card_history_snapshot(
    project_id: Annotated[ProjectId, Path()],
    card_id: Annotated[CardId, Path()],
    oid: Annotated[RevisionRef, Query(description="The revision containing the card snapshot.")],
) -> CardHistorySnapshot:
    """Read a card snapshot at a selected revision, including an explicit absent state."""


@contract.route("POST", "/projects/{project_id}/history/cards/{card_id}/restore",
                errors=[NotFound, RevisionAmbiguous, HistoryKeyUnavailable, HistoryUnavailable])
def restore_card_history_snapshot(
    project_id: Annotated[ProjectId, Path()],
    card_id: Annotated[CardId, Path()],
    oid: Annotated[RevisionRef, Query(description="The revision containing the snapshot to restore.")],
) -> CardHistoryRestoreResult:
    """Restore a card to a saved snapshot.

    Restores the complete snapshot as a new commit, preserving the old history. Lifecycle rules still apply:
    restoring a trashed snapshot promotes live children, and restoring a live snapshot places the card under a
    reachable parent.
    """


@contract.route("GET", "/trash")
def list_trash(
    project_id: Annotated[ProjectId, Query(description="The project whose Trash is listed.")],
    type: Annotated[str | None, Query(description="card, ai_message, or all; an unknown type lists nothing.")] = None,
) -> list[TrashItem]:
    """List a project's trashed cards and AI messages.

    A project that is not registered has an empty Trash.
    """


@contract.route("DELETE", "/trash", errors=[KeyringUnavailable, KeyMissing, DataCorrupted])
def empty_trash(
    project_id: Annotated[ProjectId, Query(description="The project whose Trash is emptied.")],
    type: Annotated[TrashFilter, Query(description="Which item kinds to remove.")] = "all",
) -> TrashEmptyResult:
    """Permanently remove a project's trashed items.

    An empty or unregistered project succeeds without changes. Old Git revisions of cards remain available.
    """


@contract.route("DELETE", "/trash/{type}/{id}",
                errors=[NotFound, NotTrashed, KeyringUnavailable, KeyMissing, DataCorrupted])
def delete_trash_item(
    type: Annotated[TrashItemType, Path()],
    id: Annotated[str, Path()],
) -> TrashDeleteResult:
    """Permanently remove a trashed item by its global ID (legacy route)."""


@contract.route("DELETE", "/projects/{project_id}/trash/{type}/{id}",
                errors=[NotFound, NotTrashed, KeyringUnavailable, KeyMissing, DataCorrupted])
def delete_project_trash_item(
    project_id: Annotated[ProjectId, Path()],
    type: Annotated[TrashItemType, Path()],
    id: Annotated[str, Path()],
) -> TrashDeleteResult:
    """Permanently remove a trashed item from the named project.

    Old Git revisions of cards remain available.
    """


@contract.route("GET", "/search/cards")
def search_cards(
    project_id: Annotated[ProjectId, Query(description="The project to search.")],
    q: Annotated[str, Query(min_length=1, description="An SQLite FTS5 query for card titles and bodies.")],
    limit: Annotated[Int32, Query(ge=1)] = 20,
    offset: Annotated[Int32, Query(ge=0)] = 0,
) -> list[SearchCard]:
    """Search a project's cards, ordered by relevance, best first."""


@contract.route("GET", "/search/ai")
def search_ai_messages(
    project_id: Annotated[ProjectId, Query(description="The project to search.")],
    q: Annotated[str, Query(min_length=1, description="An SQLite FTS5 query for AI message content.")],
    limit: Annotated[Int32, Query(ge=1)] = 20,
    offset: Annotated[Int32, Query(ge=0)] = 0,
) -> list[SearchMessage]:
    """Search a project's AI messages, ordered by relevance, best first."""


# Daemon-only routes use Python for their public contract and the daemon's HTTP host for execution.
@contract.route("GET", "/alive", core=False, public=True, raw_media_type="application/json")
def get_alive() -> AliveResponse:
    """Liveness check returning JSON, unauthenticated."""


@contract.route("GET", "/ping", core=False, public=True, raw_media_type="text/plain")
def get_ping() -> Literal["pong"]:
    """Liveness check returning plain text, unauthenticated."""


@contract.route("GET", "/health", core=False, raw_media_type="application/json")
def get_health() -> HealthResponse:
    """Health check."""


@contract.route("POST", "/bye", core=False, raw_media_type="application/json")
def post_bye() -> ByeResponse:
    """Say that this client is leaving.

    A hint to a daemon started with --idle-exit: it shortens the idle wait only while nothing else happens.
    Running work, a stream, or another request keeps the daemon running. Other daemon modes ignore it.
    """


@contract.route("GET", "/locations", core=False, raw_media_type="application/json")
def get_locations(project_id: Annotated[str, Query(min_length=1)]) -> StorageLocationListResponse:
    """List project storage locations and this device's preferred location."""


@contract.route("PUT", "/locations/{location_id}/binding", core=False, raw_media_type="application/json",
                responses={404: {"model": ErrorResponse, "description": "Location not found."},
                           503: {"model": ErrorResponse, "description": "Secret store unavailable."}})
def put_location_binding(location_id: Annotated[str, Path()],
                         request: Annotated[LocationBindingRequest, Body()]) -> LocationActionResponse:
    """Bind private device credentials or path; secret values are never returned."""


@contract.route("DELETE", "/locations/{location_id}/binding", core=False, raw_media_type="application/json",
                responses={404: {"model": ErrorResponse, "description": "Location not found."},
                           503: {"model": ErrorResponse, "description": "Secret store unavailable."}})
def delete_location_binding(location_id: Annotated[str, Path()]) -> LocationActionResponse:
    """Remove this device's private binding.

    Also clears the preferred location if it was this one.
    """


@contract.route("POST", "/locations/{location_id}/test", core=False, raw_media_type="application/json",
                responses={404: {"model": ErrorResponse, "description": "Location not found."},
                           409: {"model": ErrorResponse, "description": "Binding required or storage conflict."},
                           422: {"model": ErrorResponse, "description": "Probe verification failed."},
                           502: {"model": ErrorResponse, "description": "Storage rejected credentials or permissions."},
                           503: {"model": ErrorResponse, "description": "Storage or secret store unavailable."},
                           507: {"model": ErrorResponse, "description": "Storage is full."}})
def test_location(location_id: Annotated[str, Path()]) -> LocationTestResponse:
    """Upload, verify, and remove a storage probe."""


@contract.route("PUT", "/locations/preferred", core=False, raw_media_type="application/json",
                responses={404: {"model": ErrorResponse, "description": "Location not found in the project."},
                           503: {"model": ErrorResponse, "description": "Secret store unavailable."}})
def put_preferred_location(request: Annotated[PreferredLocationRequest, Body()]) -> LocationActionResponse:
    """Set this device's preferred project storage location."""


@contract.route("POST", "/locations/{location_id}/oauth/google-drive/authorize", core=False,
                raw_media_type="application/json")
def authorize_google_drive_location(location_id: Annotated[str, Path()]) -> GoogleDriveAuthorizeResponse:
    """Start connecting a Google Drive location.

    The returned URL redirects to this daemon's callback on the host and port the request used.
    """


@contract.route("GET", "/locations/{location_id}/oauth/google-drive/callback", core=False, public=True,
                raw_media_type="text/html", responses={400: {"description": "Authorization failed.",
                                                        "content": {"text/html": {},
                                                                    "application/json": {"schema": {
                                                                        "$ref": "#/components/schemas/ErrorResponse"}}}}})
def finish_google_drive_location(
    location_id: Annotated[str, Path()], code: Annotated[str | None, Query()] = None,
    state: Annotated[str | None, Query()] = None,
    error: Annotated[str | None, Query(description="Set by Google on denial or failure.")] = None,
) -> str:
    """Finish connecting a Google Drive location.

    Google redirects the browser here without a bearer token. The authorize state authenticates the callback.
    A failure returns an explanatory HTML page.
    """


@contract.route("GET", "/events/cursor", core=False, raw_media_type="application/json",
                operation_id="get_event_cursor",
                responses={404: {"model": ErrorResponse, "description": "Project not found."},
                           503: {"model": ErrorResponse, "description": "Feed or project unavailable."}})
def get_event_cursor(project_id: Annotated[str | None, Query(min_length=1)] = None) -> EventCheckpointResponse:
    """Get a change-feed checkpoint before fetching a live snapshot.

    Fetch this checkpoint, read current entities, then subscribe using its cursor as Last-Event-ID.
    """


@contract.route("GET", "/events", core=False, operation_id="stream_changes",
                sse_events={"ready": EventCheckpoint,
                            "project.changed": ChangeEventData,
                            "card.changed": ChangeEventData,
                            "resource.changed": ChangeEventData,
                            "location.changed": ChangeEventData,
                            "thread.changed": ChangeEventData,
                            "message.changed": ChangeEventData,
                            "run.changed": ChangeEventData,
                            "resync_required": EventResyncData},
                responses={404: {"model": ErrorResponse, "description": "Project not found."},
                           503: {"model": ErrorResponse, "description": "Observer or project unavailable."}})
def stream_changes(
    project_id: Annotated[str | None, Query(min_length=1)] = None,
    last_revision: Annotated[LastRevision | None, Query()] = None,
    last_event_id: Annotated[str | None, Header(alias="Last-Event-ID")] = None,
) -> Stream[EventCheckpoint | ChangeEventData | EventResyncData]:
    """Subscribe to committed daemon state changes.

    The stream begins with ready or resync_required, then sends project, card, resource, location, thread,
    message, and run invalidations. Its opaque event ID supports bounded replay; resync_required calls for a
    fresh snapshot. Disconnecting does not cancel running work.
    """


@contract.route("GET", "/openapi.yaml", core=False, public=True, raw_media_type="application/yaml",
                raw_schema={})
def get_openapi_yaml() -> str:
    """Download the OpenAPI specification."""


@contract.route("GET", "/openapi.json", core=False, public=True, raw_media_type="application/json",
                raw_schema={})
def get_openapi_json() -> JsonValue:
    """Download the OpenAPI specification as JSON."""


@contract.route("GET", "/ai_catalog.yaml", core=False, public=True, raw_media_type="application/yaml",
                raw_schema={},
                responses={404: {"model": ErrorResponse, "description": "Catalog not found."}})
def get_ai_catalog_yaml() -> str:
    """Download the canonical AI catalog config."""


@contract.route("GET", "/ai_catalog.json", core=False, public=True, raw_media_type="application/json",
                raw_schema={},
                responses={404: {"model": ErrorResponse, "description": "Catalog not found."}})
def get_ai_catalog_json() -> JsonValue:
    """Download the canonical AI catalog config as JSON."""


@contract.route("GET", "/git_providers.yaml", core=False, public=True, raw_media_type="application/yaml",
                raw_schema={},
                responses={404: {"model": ErrorResponse, "description": "Config not found."}})
def get_git_providers_yaml() -> str:
    """Download the Git provider UI and runtime config."""


@contract.route("GET", "/git_providers.json", core=False, public=True, raw_media_type="application/json",
                raw_schema={},
                responses={404: {"model": ErrorResponse, "description": "Config not found."}})
def get_git_providers_json() -> JsonValue:
    """Download the Git provider UI and runtime config as JSON."""


@contract.route("GET", "/docs", core=False, public=True, raw_media_type="text/html",
                responses={404: {"model": ErrorResponse, "description": "Documentation assets not installed."}})
def get_docs() -> str:
    """Browse the API documentation."""


@contract.route("GET", "/docs/{file}", core=False, public=True, raw_media_type="*/*",
                responses={404: {"model": ErrorResponse, "description": "Documentation file not found."}})
def get_docs_file(file: Annotated[str, Path()]) -> Bytes:
    """Download an API documentation file with its media type."""
# AI routes served by the daemon. Their bodies are implemented in daemon/src/api/.

@contract.route('GET', '/ai/runner/pull/{job_id}', core=False, raw_media_type="application/json", responses={404: {"model": ErrorResponse, "description": 'Not found'}})
def get_pull_status(
    job_id: Annotated[str, Path()],
    runner_id: Annotated[str | None, Query()] = None,
) -> ai.RunnerPullStatusResponse:
    """Fetch pull job status"""


@contract.route('POST', '/ai/runner/pull', core=False, raw_media_type="application/json", responses={404: {"model": ErrorResponse, "description": 'Runner not configured'}, 503: {"model": ErrorResponse, "description": 'Runner unavailable (runner_unavailable)'}})
def post_ai_runner_pull(
    request: Annotated[ai.PostAiRunnerPullRequest, Body()],
) -> ai.RunnerPullStartResponse:
    """Start pulling a local model

    Pulls on runner_id, or the auto-local runner when it is omitted or empty. Follow progress with GET /ai/runner/pull/{job_id} or its events stream."""


@contract.route('GET', '/ai/providers/credentials', core=False, raw_media_type="application/json")
def list_provider_credentials(
) -> ai.AiProviderCredentialsListResponse:
    """List configured cloud provider credentials"""


@contract.route('PUT', '/ai/providers/credentials', core=False, raw_media_type="application/json", responses={503: {"model": ErrorResponse, "description": 'Secret store unavailable, or it failed storing the key'}})
def upsert_provider_credential(
    request: Annotated[ai.AiProviderCredentialUpsertRequest, Body()],
) -> ai.AiProviderCredentialUpsertResponse:
    """Create or update a cloud provider API key"""


@contract.route('DELETE', '/ai/providers/credentials/{provider}', core=False, raw_media_type="application/json", responses={503: {"model": ErrorResponse, "description": 'Secret store unavailable, or it failed removing the key'}})
def delete_provider_credential(
    provider: Annotated[str, Path()],
) -> ai.AiProviderCredentialDeleteResponse:
    """Delete a configured cloud provider API key

    Removes the stored key and disables the provider. Deleting a provider with no key succeeds."""


@contract.route('GET', '/ai/local-models/config', core=False, raw_media_type="application/json")
def get_local_model_config(
) -> ai.AiLocalModelConfigResponse:
    """Get local model configuration"""


@contract.route('PUT', '/ai/local-models/config', core=False, raw_media_type="application/json")
def put_local_model_config(
    request: Annotated[ai.AiLocalModelConfigUpdateRequest, Body()],
) -> ai.AiLocalModelConfigResponse:
    """Set or clear local model configuration"""


@contract.route('GET', '/ai/messages', core=False, raw_media_type="application/json")
def get_ai_messages(
    thread_id: Annotated[str, Query(min_length=1)],
    # None means the query parameter is absent; a sent value is always 0 or 1.
    include_deleted: Annotated[Literal[0, 1], Query(description='Set to 1 to include deleted messages.')] = None,  # noqa: RUF013
) -> ai.AiMessageListResponse:
    """List AI messages by thread"""


@contract.route('GET', '/ai/messages/{message_id}', core=False, raw_media_type="application/json", responses={404: {"model": ErrorResponse, "description": 'Not found'}})
def get_ai_message(
    message_id: Annotated[str, Path()],
) -> ai.AiMessageGetResponse:
    """Fetch an AI message

    A message in the trash is not found."""


@contract.route('GET', '/ai/messages/{message_id}/links', core=False, raw_media_type="application/json", responses={404: {"model": ErrorResponse, "description": 'Not found'}})
def get_ai_message_links(
    message_id: Annotated[str, Path()],
    include_deleted: Annotated[Literal[0, 1], Query(description='Include links to deleted local targets. External card links remain listed even when their targets are unavailable or trashed.')] = None,  # noqa: RUF013
) -> ai.CardLinkListResponse:
    """List outgoing links for an AI message"""


@contract.route('GET', '/ai/messages/{message_id}/backlinks', core=False, raw_media_type="application/json", responses={404: {"model": ErrorResponse, "description": 'Not found'}})
def get_ai_message_backlinks(
    message_id: Annotated[str, Path()],
    include_deleted: Annotated[Literal[0, 1], Query(description='Include backlinks from deleted sources.')] = None,  # noqa: RUF013
) -> ai.CardLinkListResponse:
    """List backlinks to an AI message"""


@contract.route('POST', '/ai/messages', core=False, raw_media_type="application/json", status=201, responses={409: {"model": ErrorResponse, "description": 'A message with this ID already exists'}})
def post_ai_messages(
    request: Annotated[ai.AiMessageCreateRequest, Body()],
) -> ai.AiMessageCreateResponse:
    """Create an AI message"""


@contract.route('PATCH', '/ai/messages/{message_id}', core=False, raw_media_type="application/json", responses={404: {"model": ErrorResponse, "description": 'Not found'}})
def patch_ai_messages_message_id(
    message_id: Annotated[str, Path()],
    request: Annotated[ai.AiMessageUpdateRequest, Body()],
) -> ai.AiMessageUpdateResponse:
    """Update an AI message"""


@contract.route('DELETE', '/ai/messages/{message_id}', core=False, raw_media_type="application/json", responses={404: {"model": ErrorResponse, "description": 'Not found'}})
def delete_ai_messages_message_id(
    message_id: Annotated[str, Path()],
) -> ai.AiMessageUpdateResponse:
    """Move an AI message to trash"""


@contract.route('POST', '/ai/messages/{message_id}/restore', core=False, raw_media_type="application/json", responses={404: {"model": ErrorResponse, "description": 'Not found'}})
def post_ai_messages_message_id_restore(
    message_id: Annotated[str, Path()],
) -> ai.AiMessageUpdateResponse:
    """Restore an AI message from trash"""


@contract.route('POST', '/ai/messages/{message_id}/links', core=False, raw_media_type="application/json", status=201, responses={404: {"model": ErrorResponse, "description": 'Not found'}})
def post_ai_messages_message_id_links(
    message_id: Annotated[str, Path()],
    request: Annotated[ai.CardLinkCreateRequest, Body()],
) -> ai.CardLinkCreateResponse:
    """Create or update a link from an AI message"""


@contract.route('DELETE', '/ai/messages/{message_id}/links', core=False, raw_media_type="application/json", responses={404: {"model": ErrorResponse, "description": 'Not found'}})
def delete_ai_messages_message_id_links(
    message_id: Annotated[str, Path()],
    request: Annotated[ai.CardLinkDeleteRequest, Body()] = None,
) -> ai.AiMessageCreateResponse:
    """Delete links from an AI message

    Provide to_card_id (and optional to_type/kind) to delete matching links. A fully specified selector without target_project_id is rejected if more than one target project matches. Provide target_project_id together with to_card_id, to_type and kind to select one qualified target. If to_card_id is omitted, deletes all outgoing links."""


@contract.route('POST', '/ai/messages/capture', core=False, raw_media_type="application/json", status=201, responses={404: {"model": ErrorResponse, "description": 'Project or thread not found'}, 409: {"model": ErrorResponse, "description": 'A generated ID already exists'}})
def post_ai_messages_capture(
    request: Annotated[ai.AiMessageCaptureRequest, Body()],
) -> ai.AiMessageCaptureResponse:
    """Capture prompt/response pair with provenance

    Appends the prompt as a user message and the response as an assistant message carrying provider, model, url and context."""


@contract.route('GET', '/ai/nudges', core=False, raw_media_type="application/json")
def get_ai_nudges(
    project_id: Annotated[str, Query(min_length=1)],
    card_id: Annotated[str | None, Query(description="Only this card's nudges; empty means every card's.")] = None,
) -> ai.AiNudgeListResponse:
    """List a project's current nudges

    Nudges whose evidence no longer holds are removed before listing, and dismissed nudges are left out."""


@contract.route('POST', '/ai/nudges/{nudge_id}/dismiss', core=False, raw_media_type="application/json", responses={404: {"model": ErrorResponse, "description": 'Nudge not found'}})
def post_ai_nudges_nudge_id_dismiss(
    nudge_id: Annotated[str, Path()],
) -> ai.AiNudgeDismissResponse:
    """Dismiss a nudge"""


@contract.route('POST', '/ai/nudges/evaluate', core=False, raw_media_type="application/json")
def post_ai_nudges_evaluate(
    candidate: Annotated[ai.AiNudgeCandidate, Body()],
) -> ai.AiNudgeEvaluateResponse:
    """Evaluate a nudge candidate and record the nudge it warrants

    Answers whether the candidate is accepted and whether it warrants a nudge, with the reason. A candidate the daemon does not know is answered, not rejected."""


@contract.route('GET', '/ai/providers/catalog', core=False, raw_media_type="application/json")
def get_ai_provider_catalog(
) -> ai.AiProviderCatalogResponse:
    """Get provider-agnostic cloud catalog with configured flags"""


@contract.route('GET', '/ai/runners', core=False, raw_media_type="application/json")
def get_ai_runners(
) -> ai.AiRunnerListResponse:
    """List configured local model runners"""


@contract.route('GET', '/ai/runners/{runner_id}', core=False, raw_media_type="application/json", responses={404: {"model": ErrorResponse, "description": 'Not found'}})
def get_ai_runner(
    runner_id: Annotated[str, Path()],
) -> ai.AiRunnerResponse:
    """Get a local model runner"""


@contract.route('POST', '/ai/runners', core=False, raw_media_type="application/json", status=201)
def post_ai_runners(
    request: Annotated[ai.AiRunnerCreateRequest, Body()],
) -> ai.AiRunnerResponse:
    """Configure a local model runner"""


@contract.route('PATCH', '/ai/runners/{runner_id}', core=False, raw_media_type="application/json", responses={404: {"model": ErrorResponse, "description": 'Runner not found'}})
def patch_ai_runners_runner_id(
    runner_id: Annotated[str, Path()],
    request: Annotated[ai.AiRunnerUpdateRequest, Body()],
) -> ai.AiRunnerResponse:
    """Update a local model runner"""


@contract.route('DELETE', '/ai/runners/{runner_id}', core=False, raw_media_type="application/json", responses={404: {"model": ErrorResponse, "description": 'Runner not found'}})
def delete_ai_runners_runner_id(
    runner_id: Annotated[str, Path()],
) -> ai.AiRunnerDeleteResponse:
    """Remove a configured local model runner"""


@contract.route('POST', '/ai/runners/{runner_id}/retry', core=False, raw_media_type="application/json", responses={404: {"model": ErrorResponse, "description": 'Runner not found or not configured'}})
def post_ai_runners_runner_id_retry(
    runner_id: Annotated[str, Path()],
) -> ai.AiRunnerResponse:
    """Retry one local model runner"""


@contract.route('GET', '/ai/runs', core=False, raw_media_type="application/json")
def get_ai_runs(
    project_id: Annotated[str | None, Query()] = None,
    thread_id: Annotated[str | None, Query()] = None,
) -> ai.AiRunListResponse:
    """List AI runs by project or thread

    Needs project_id or thread_id; with both, thread_id is used."""


@contract.route('GET', '/ai/runs/{run_id}', core=False, raw_media_type="application/json", responses={404: {"model": ErrorResponse, "description": 'Not found'}})
def get_ai_run(
    run_id: Annotated[str, Path()],
) -> ai.AiRunGetResponse:
    """Fetch an AI run"""


@contract.route('GET', '/ai/capabilities', core=False, raw_media_type="application/json")
def get_ai_capabilities(
    runner_id: Annotated[str | None, Query(description='The runner whose installed models decide recommended_install; defaults to auto-local.')] = None,
) -> ai.RunnerCapabilitiesResponse:
    """Get local model runner capabilities"""


@contract.route('GET', '/ai/status', core=False, raw_media_type="application/json")
def get_ai_status(
) -> ai.AiStatusResponse:
    """Get live AI runtime status"""


@contract.route('GET', '/ai/threads', core=False, raw_media_type="application/json")
def get_ai_threads(
    project_id: Annotated[str, Query(min_length=1)],
) -> ai.AiThreadListResponse:
    """List AI threads by project"""


@contract.route('GET', '/ai/threads/{thread_id}', core=False, raw_media_type="application/json", responses={404: {"model": ErrorResponse, "description": 'Not found'}})
def get_ai_thread(
    thread_id: Annotated[str, Path()],
) -> ai.AiThreadGetResponse:
    """Fetch an AI thread"""


@contract.route('POST', '/ai/threads', core=False, raw_media_type="application/json", status=201, responses={409: {"model": ErrorResponse, "description": 'A thread with this ID already exists'}})
def post_ai_threads(
    request: Annotated[ai.AiThreadCreateRequest, Body()],
) -> ai.AiThreadCreateResponse:
    """Create an AI thread"""


@contract.route('PATCH', '/ai/threads/{thread_id}', core=False, raw_media_type="application/json", responses={404: {"model": ErrorResponse, "description": 'Not found'}})
def patch_ai_threads_thread_id(
    thread_id: Annotated[str, Path()],
    request: Annotated[ai.AiThreadUpdateRequest, Body()],
) -> ai.AiThreadPatchResponse:
    """Update AI thread metadata"""


@contract.route('DELETE', '/ai/threads/{thread_id}', core=False, raw_media_type="application/json", responses={404: {"model": ErrorResponse, "description": 'Not found'}})
def delete_ai_threads_thread_id(
    thread_id: Annotated[str, Path()],
) -> ai.AiThreadPatchResponse:
    """Delete an AI thread"""


@contract.route('GET', '/ai/providers/settings', core=False, raw_media_type="application/json")
def list_provider_settings(
) -> ai.AiProviderSettingsResponse:
    """List cloud provider enabled settings"""


@contract.route('PUT', '/ai/providers/settings', core=False, raw_media_type="application/json")
def set_provider_setting(
    request: Annotated[ai.AiProviderSettingUpdateRequest, Body()],
) -> ai.AiProviderSettingResponse:
    """Set whether a cloud provider is enabled"""


@contract.route('DELETE', '/ai/providers/settings/{provider}', core=False, raw_media_type="application/json")
def delete_provider_setting(
    provider: Annotated[str, Path()],
) -> ai.AiProviderSettingDeleteResponse:
    """Remove a cloud provider enabled override"""


@contract.route('POST', '/ai/runs', core=False, sse_events={'run_started': ai.AiRunEventData, 'progress': ai.AiRunEventData, 'router': ai.AiRunEventData, 'router_result': ai.AiRunEventData, 'chunk': ai.AiRunEventData, 'fallback': ai.AiRunEventData, 'done': ai.AiRunEventData, 'failed': ai.AiRunEventData}, responses={503: {"model": ErrorResponse, "description": 'No local runner or configured cloud provider available, or stream limit reached'}})
def post_ai_runs(
    request: Annotated[ai.AiRunCreateRequest, Body()],
) -> Stream[ai.AiRunEventData]:
    """Create and execute an AI run with routing and streaming

    Creates one run and streams run_started (with run_id), provider-dependent progress/router/fallback/chunk events, then done or failed. Every frame has an opaque replay ID. Reconnect via GET /ai/runs/{run_id}/events rather than repeating this POST. Disconnecting does not cancel execution; terminal events follow persisted status."""


@contract.route('GET', '/ai/runs/{run_id}/events', core=False, sse_events={'run_started': ai.AiRunEventData, 'progress': ai.AiRunEventData, 'router': ai.AiRunEventData, 'router_result': ai.AiRunEventData, 'chunk': ai.AiRunEventData, 'fallback': ai.AiRunEventData, 'done': ai.AiRunEventData, 'failed': ai.AiRunEventData, 'resync_required': ai.AiStreamResyncData}, responses={204: {"description": 'The run has finished and Last-Event-ID is past its last event: stop reconnecting'}, 404: {"model": ErrorResponse, "description": 'Not found'}, 503: {"model": ErrorResponse, "description": 'Observer unavailable or stream limit reached'}})
def get_ai_runs_run_id_events(
    run_id: Annotated[str, Path()],
    last_event_id: Annotated[str | None, Header(alias='Last-Event-ID', description='Opaque SSE event ID; unavailable history emits resync_required and closes.')] = None,
) -> Stream[ai.AiRunEventData]:
    """Stream run events (SSE)

    Replays bounded retained run events with absolute event IDs; done or failed closes the stream. Missing in-memory history can recover persisted terminal status. An expired or foreign cursor emits resync_required; fetch the run before reconnecting. A reconnect after the stream's last event is answered with 204, the event-stream standard's signal to stop reconnecting."""


@contract.route('GET', '/ai/runner/pull/{job_id}/events', core=False, sse_events={'progress': ai.RunnerPullJob, 'completed': ai.RunnerPullJob, 'failed': ai.RunnerPullJob}, responses={204: {"description": 'The pull has finished: stop reconnecting'}, 404: {"model": ErrorResponse, "description": 'Runner not configured, or pull job not found'}, 503: {"model": ErrorResponse, "description": 'Observer unavailable or stream limit reached'}})
def get_ai_runner_pull_job_id_events(
    job_id: Annotated[str, Path()],
    runner_id: Annotated[str | None, Query()] = None,
    last_event_id: Annotated[str | None, Header(alias='Last-Event-ID', description='Opaque SSE event ID, sent by a reconnecting client.')] = None,
) -> Stream[ai.RunnerPullJob]:
    """Stream pull job progress (SSE)

    Streams the job's progress snapshot whenever it changes; completed or failed closes the stream. A reconnect receives the current snapshot, and once the pull has finished it is answered with 204, the event-stream standard's signal to stop reconnecting. Disconnecting does not cancel the pull."""
