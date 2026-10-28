"""Holder's operations, as the daemon's routes, and what core's C API needs besides.

Each operation is a FastAPI route whose body never runs: the daemon serves it, and core implements
it, from code generated from the IR. holder_contract.routes says what a route declares.

A card is named by its project and its card ID; an operation on a card in another project does not find it.
"""

from typing import Annotated, Literal

from fastapi import Body, Path, Query

from holder_contract import Contract, Field, Int32, Timestamp

from .cards import Card, CardId, CardListItem, CardMove, CardUpdate, NewCard, Page, ProjectId, Tag, TagCount
from .errors import (
    Conflict,
    DataCorrupted,
    HistoryKeyUnavailable,
    HistoryResponseTooLarge,
    HistoryUnavailable,
    InvalidMove,
    KeyMissing,
    KeyringUnavailable,
    NotFound,
    NotTrashed,
    RevisionAmbiguous,
)
from .git import PushOptions, RemoteTest, RemoteTestResult, SyncResult, SyncState
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
from .milestones import Calendar, Day, Milestone, MilestoneId, MilestoneRole, MilestoneUpdate, NewMilestone
from .projects import NewProject, Project, ProjectListItem, ProjectUpdate
from .resources import NewResource, Resource, ResourceId, ResourceUpdate
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
    HistoryResponseTooLarge: 413,
    HistoryKeyUnavailable: 409,
    RevisionAmbiguous: 409,
    NotTrashed: 409,
    HistoryUnavailable: 503,
})
# What every write to a card's file may fail with, besides its own errors: an encrypted project's key.
KEYS = [KeyringUnavailable, KeyMissing, DataCorrupted]
app = contract.app


@contract.route("GET", "/projects/{project_id}/cards/{card_id}",
                errors=[NotFound, KeyringUnavailable, KeyMissing, DataCorrupted])
def get_card(project_id: Annotated[ProjectId, Path()], card_id: Annotated[CardId, Path()]) -> Card:
    """Fetch a card.

    A card in the trash or in another project is not found.
    """


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
