"""Cards: the notes a project holds, arranged in a hierarchy."""

from typing import Annotated, Literal

from holder_contract import MISSING, Field, Float64, HolderError, Int32, Int64, Record, Timestamp, operation

type ProjectId = Annotated[str, Field(min_length=1)]
"""A project's ID."""

type CardId = Annotated[str, Field(min_length=1)]
"""A card's ID."""


class NotFound(HolderError):
    """The thing asked for does not exist, or is in the trash."""


class CardSummary(Record):
    """Just enough to name a card, as in a reference or a backlink."""

    card_id: CardId
    """The card's ID."""
    title: str
    """The card's title."""
    deleted_at: Timestamp | None
    """When the card was moved to the trash; null when it is not in the trash."""


class CardMetadata(Record):
    """Everything about a card except its content."""

    card_id: CardId
    """The card's ID."""
    project_id: ProjectId
    """The project the card belongs to."""
    title: str
    """The card's title."""
    rel_path: str
    """The card's file, relative to the project's root."""
    parent_card_id: CardId | None
    """The card's parent; null for a root card."""
    sort_key: Float64
    """The card's place among its siblings; lower comes first."""
    created_at: Timestamp
    """When the card was created."""
    updated_at: Timestamp
    """When the card was last changed."""
    deleted_at: Timestamp | None
    """When the card was moved to the trash; null when it is not in the trash."""


class CardListItem(CardMetadata):
    """A card in a listing."""

    child_count: Int32 | MISSING = MISSING
    """The card's live children; present when asked for."""
    content: str | MISSING = MISSING
    """The card's body; present when asked for."""


class TagOccurrence(Record):
    """A tag written in a card's content."""

    tag: str
    """The normalized tag."""
    byte_start: Int64
    """Inclusive byte offset of the leading # in the content."""
    byte_end: Int64
    """Exclusive byte offset after the tag's name."""


class Card(CardMetadata):
    """A card, with its content and tags."""

    content: str
    """The card's body."""
    tags: list[str]
    """The card's normalized tags."""
    tag_occurrences: list[TagOccurrence]
    """Each tag written in the content, in order."""


class Page[T](Record):
    """One page of a listing."""

    items: list[T]
    """The page's items."""
    next_cursor: str | None
    """Pass back to get the next page; null on the last page."""


class NewCard(Record):
    """A card to create."""

    project_id: ProjectId
    """The project to create it in."""
    title: Annotated[str, Field(min_length=1)]
    """The card's title."""
    content: str = ""
    """The card's body."""
    parent_card_id: CardId | None = None
    """The card's parent; null or omitted for a root card."""
    sort_key: Float64 | MISSING = MISSING
    """The card's place among its siblings; omit to let Holder choose."""
    created_at: Timestamp | MISSING = MISSING
    """When the card was created, for imports; omit for now."""
    updated_at: Timestamp | MISSING = MISSING
    """When the card was last changed, for imports; omit to use created_at."""


@operation(errors=[NotFound])
def get_card(card_id: CardId) -> Card:
    """Fetch a card.

    A card in the trash is not found.
    """


@operation(errors=[NotFound])
def list_cards(
    project_id: ProjectId,
    parent: Annotated[Literal["roots"] | CardId | None, Field(
        description="Omit for every level; roots selects root cards; a card ID selects its children.")] = None,
    tag: Annotated[Annotated[str, Field(min_length=1)] | None, Field(
        description="Only cards with this tag, matched without case and without the leading #. A tag starts with "
                    "an ASCII letter and holds only ASCII letters, digits, _, / and -; a hex colour such as fff is "
                    "not a tag.")] = None,
    order: Annotated[Literal["card_id_asc", "updated_desc", "sort_key_asc"], Field(
        description="updated_desc breaks ties by descending card ID; sort_key_asc follows sibling order.")]
    = "card_id_asc",
    limit: Annotated[Int32, Field(ge=1, le=5000, description="At most 1000 when include_content is true.")] = 200,
    cursor: Annotated[Annotated[str, Field(min_length=1, max_length=8192)] | None, Field(
        description="The previous page's next_cursor. Keep the project, filters and order unchanged.")] = None,
    include_content: Annotated[bool, Field(description="Include each card's body.")] = False,
    include_child_count: Annotated[bool, Field(description="Include each card's count of live children.")] = False,
) -> Page[CardListItem]:
    """Page through a project's live cards.

    Filters combine. Each page reads current data; iterating is not a snapshot.
    """


@operation(errors=[NotFound])
def create_card(card: NewCard) -> Card:
    """Create a card.

    Fails with NotFound when the project or the parent does not exist.
    """
