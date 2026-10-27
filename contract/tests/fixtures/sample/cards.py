"""Cards, AI messages and runs: the hard cases from Holder's HTTP API."""

from enum import StrEnum
from typing import Annotated, Literal, Protocol

from holder_contract import (
    MISSING,
    Field,
    HolderError,
    Int32,
    JsonValue,
    Record,
    SecretStr,
    Timestamp,
    host_interface,
)

type CardId = Annotated[str, Field(min_length=1)]
"""A card's ID."""


class NotFound(HolderError):
    """The thing asked for does not exist, or is in the trash."""


class Conflict(HolderError):
    """The thing changed since the caller last read it."""

    current_updated_at: Timestamp
    """When it was last changed."""


class TagOccurrence(Record):
    """A tag written in a card's content."""

    tag: str
    """The normalized tag."""
    byte_start: Int32
    """Inclusive byte offset of the leading # in content."""
    byte_end: Int32
    """Exclusive byte offset after the tag name."""


class Card(Record):
    """A card in a project."""

    card_id: CardId
    """The card's ID."""
    title: Annotated[str, Field(min_length=1)]
    """The card's title."""
    parent_card_id: CardId | None
    """Null for a root card."""
    deleted_at: Timestamp | None
    """When the card was moved to the trash; null when it is not in the trash."""
    tags: list[str]
    """The card's normalized tags."""
    tag_occurrences: list[TagOccurrence]
    """Each tag written in content, in order."""


class CardListItem(Card):
    """A card in a listing."""

    child_count: Int32 | MISSING = MISSING
    """Live children, present when requested."""


class Page[T](Record):
    """One page of a listing."""

    items: list[T]
    """The page's items."""
    next_cursor: str | None
    """Pass back to get the next page; null on the last page."""


class Role(StrEnum):
    """Who wrote a message."""

    USER = "user"
    ASSISTANT = "assistant"


class MessagePatch(Record):
    """Changes to an AI message. Omitted fields are left unchanged."""

    role: Role | MISSING = MISSING
    """Omit to leave it unchanged."""
    provider: str | None | MISSING = MISSING
    """Omit to leave it unchanged; null clears it."""
    meta: JsonValue | MISSING = MISSING
    """Free-form details; replaces the stored value."""


class LocationBinding(Record):
    """Private credentials binding a storage location to this device."""

    values: dict[str, SecretStr]
    """Sent by clients, never returned."""


class RunStarted(Record):
    """The run has started."""

    type: Literal["run_started"]
    """The event's type."""
    run_id: str
    """The run's ID."""


class RunChunk(Record):
    """Part of the answer."""

    type: Literal["chunk"]
    """The event's type."""
    text: str
    """The text of this part."""


type RunEvent = Annotated[RunStarted | RunChunk, Field(discriminator="type")]
"""One event of an AI run."""


@host_interface
class KeyringProvider(Protocol):
    """Stores secrets where the platform has no keyring of its own."""

    def lookup(self, service: str, account: str) -> SecretStr | None:
        """Return the secret, or null if none is stored."""

    def store(self, service: str, account: str, secret: SecretStr) -> None:
        """Store a secret, replacing any stored before."""
