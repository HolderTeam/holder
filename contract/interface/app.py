"""Holder's operations, as the daemon's routes, and what core's C API needs besides.

Each operation is a FastAPI route whose body never runs: the daemon serves it, and core implements
it, from code generated from the IR. holder_contract.routes says what a route declares.
"""

from typing import Annotated, Literal

from fastapi import Body, Path, Query

from holder_contract import Contract, Field, Int32

from .cards import Card, CardId, CardListItem, NewCard, Page, ProjectId
from .errors import Conflict, DataCorrupted, KeyMissing, KeyringUnavailable, NotFound

contract = Contract(statuses={
    NotFound: 404,
    Conflict: 409,
    # The project's state rules the request out until its key is restored.
    KeyMissing: 409,
    KeyringUnavailable: 503,
    # Not something the caller can fix; the code names the condition.
    DataCorrupted: 500,
})
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
