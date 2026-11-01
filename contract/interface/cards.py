"""Cards: the notes a project holds, arranged in a hierarchy."""

from typing import Annotated, Literal

from authoring import MISSING, Field, Float64, Int32, Int64, Record, Timestamp

type ProjectId = Annotated[str, Field(min_length=1)]
"""A project's ID."""

type CardId = Annotated[str, Field(min_length=1)]
"""A card's ID, unique within its project. Copies of a project, such as a fork, keep their card IDs."""

type Tag = Annotated[str, Field(min_length=1)]
"""A tag, without the leading #, matched without case. A tag starts with an ASCII letter and holds only ASCII
letters, digits, _, / and -; a hex colour such as fff is not a tag."""


class CardSummary(Record):
    """Just enough to name a card, as in a reference or a backlink."""

    card_id: CardId
    """The card's ID."""
    title: str
    """The card's title."""
    deleted_at: Timestamp | None
    """When the card was moved to the trash; null when it is not in the trash."""


class CardBreadcrumb(Record):
    """One project or live card in a card's path from its project root."""

    kind: Literal["project", "card"]
    """Whether this step names the project or a card."""
    id: str
    """The project or card ID."""
    title: str
    """The project or card title."""


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


class CardUpdate(Record):
    """Changes to a card. Each field left out keeps its value."""

    title: Annotated[str, Field(min_length=1)] | MISSING = MISSING
    """The card's new title."""
    content: str | MISSING = MISSING
    """The card's new body."""
    parent_card_id: CardId | None | MISSING = MISSING
    """The card's new parent; null makes it a root card."""
    sort_key: Float64 | MISSING = MISSING
    """The card's new place among its siblings."""
    updated_at: Timestamp | MISSING = MISSING
    """When the card was changed, for imports; omit for now."""


class CardMove(Record):
    """Where to move a card, relative to another card or among its siblings."""

    intent: Literal["into", "before", "after", "left", "right", "to_start", "to_end", "up_level"]
    """into makes the card target_card_id's last child; before and after place it beside target_card_id, as its
    sibling; left and right swap it with its previous or next sibling; to_start and to_end make it the first or
    last of its siblings; up_level makes it its grandparent's last child."""
    target_card_id: CardId | MISSING = MISSING
    """The card to move into, before or after; required for those intents, and ignored by the others."""
    parent_card_id: CardId | None | MISSING = MISSING
    """For left, right, to_start and to_end: the parent among whose children to place it, null for the root
    cards; omit for its current parent. Ignored by the other intents."""


class TagCount(Record):
    """A tag and how many of a project's live cards carry it."""

    tag: Tag
    """The tag, normalized: in lower case."""
    card_count: Annotated[Int32, Field(ge=1)]
    """The live cards in the project that carry the tag: at least one, or the tag is not listed."""
