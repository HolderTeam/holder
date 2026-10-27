"""Holder's operations, as the daemon's routes, and what core's C API needs besides.

Each operation is a FastAPI route whose body never runs: the daemon serves it, and core implements
it, from code generated from the IR. holder_contract.routes says what a route declares.
"""

from typing import Annotated, Literal

from fastapi import Body, Path, Query

from holder_contract import Contract, Field, Int32

from .cards import Card, CardId, CardListItem, CardMove, CardUpdate, NewCard, Page, ProjectId
from .errors import Conflict, DataCorrupted, InvalidMove, KeyMissing, KeyringUnavailable, NotFound

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


@contract.route("GET", "/cards/{card_id}", errors=[NotFound, KeyringUnavailable, KeyMissing, DataCorrupted])
def get_card(card_id: Annotated[CardId, Path()]) -> Card:
    """Fetch a card.

    A card in the trash is not found.
    """


@contract.route("GET", "/projects/{project_id}/cards",
                errors=[NotFound, KeyringUnavailable, KeyMissing, DataCorrupted])
def list_cards(
    project_id: Annotated[ProjectId, Path()],
    parent: Annotated[Literal["roots"] | CardId | None, Query(
        description="Omit for every level; roots selects root cards; a card ID selects its children.")] = None,
    tag: Annotated[Annotated[str, Field(min_length=1)] | None, Query(
        description="Only cards with this tag, matched without case and without the leading #. A tag starts with "
                    "an ASCII letter and holds only ASCII letters, digits, _, / and -; a hex colour such as fff is "
                    "not a tag.")] = None,
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


@contract.route("POST", "/cards", status=201,
                errors=[NotFound, Conflict, KeyringUnavailable, KeyMissing, DataCorrupted])
def create_card(card: Annotated[NewCard, Body()]) -> Card:
    """Create a card.

    Fails with NotFound when the project or the parent does not exist.
    """


@contract.route("PATCH", "/cards/{card_id}", errors=[NotFound, InvalidMove, *KEYS])
def update_card(card_id: Annotated[CardId, Path()], update: Annotated[CardUpdate, Body()]) -> Card:
    """Change a card.

    Each field left out keeps its value, and an update that leaves every field out changes nothing. The title and
    content are written first, then the parent and place, each as its own change in the project's history. A card
    in the trash is not found, and neither is a new parent that does not exist or is in the trash; a parent under
    the card itself is an InvalidMove.
    """


@contract.route("DELETE", "/cards/{card_id}", errors=[NotFound, *KEYS])
def trash_card(
    card_id: Annotated[CardId, Path()],
    children: Annotated[Literal["promote", "trash_subtree"], Query(
        description="promote moves the card's live children into its place among its siblings, keeping their own "
                    "descendants; trash_subtree moves every live descendant to the trash with it.")] = "promote",
) -> Card:
    """Move a card to the trash.

    The card and every card that changes with it are written as one change in the project's history. A card already
    in the trash is not found. Returns the card as it is in the trash.
    """


@contract.route("POST", "/cards/{card_id}/restore", errors=[NotFound, Conflict, *KEYS])
def restore_card(card_id: Annotated[CardId, Path()]) -> Card:
    """Restore a card from the trash.

    Restores this card only. It returns to its parent when that parent is live and reachable, otherwise to its
    nearest live, reachable ancestor or the top level. Children promoted when it was trashed stay where they are;
    descendants trashed with it stay in the trash, and come back under it when restored. A card that is not in the
    trash is a Conflict.
    """


@contract.route("POST", "/cards/{card_id}/move", errors=[NotFound, InvalidMove, *KEYS])
def move_card(card_id: Annotated[CardId, Path()], move: Annotated[CardMove, Body()]) -> Card:
    """Move a card in its project's hierarchy.

    When the card's new neighbours leave no room between their places, nearby siblings are re-spaced in the same
    change, keeping their order; other cards' sort_key values a caller holds may then be out of date. A card in the
    trash is not found, and neither is a target or parent that does not exist, is in the trash or is in another
    project. A move that changes nothing writes nothing.
    """
