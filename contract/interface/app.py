"""Holder's operations, as the daemon's routes, and what core's C API needs besides.

Each operation is a FastAPI route whose body never runs: the daemon serves it, and core implements
it, from code generated from the IR. holder_contract.routes says what a route declares.

A card is named by its project and its card ID; an operation on a card in another project does not find it.
"""

from typing import Annotated, Literal

from fastapi import Body, Path, Query

from holder_contract import Contract, Field, Int32

from .cards import Card, CardId, CardListItem, CardMove, CardUpdate, NewCard, Page, ProjectId, Tag, TagCount
from .errors import Conflict, DataCorrupted, InvalidMove, KeyMissing, KeyringUnavailable, NotFound
from .links import Link, LinkKind, LinkTargetId, LinkTargetType, NewLink

contract = Contract(statuses={
    NotFound: 404,
    Conflict: 409,
    # The project's state rules the request out until its key is restored.
    KeyMissing: 409,
    KeyringUnavailable: 503,
    # Not something the caller can fix; the code names the condition.
    DataCorrupted: 500,
    InvalidMove: 422,
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
