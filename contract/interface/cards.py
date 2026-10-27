"""Cards: the notes a project holds, arranged in a hierarchy."""

from typing import Annotated

from holder_contract import MISSING, Field, Float64, Int32, Int64, Record, Timestamp

type ProjectId = Annotated[str, Field(min_length=1)]
"""A project's ID."""

type CardId = Annotated[str, Field(min_length=1)]
"""A card's ID."""


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
