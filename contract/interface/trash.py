"""Items in a project's Trash and the results of permanent removal."""

from typing import Literal

from holder_contract import Record, Timestamp

type TrashItemType = Literal["card", "ai_message"]
type TrashFilter = Literal["card", "ai_message", "all"]


class TrashItem(Record):
    """A trashed card or AI message."""

    type: TrashItemType
    """The kind of item."""
    project_id: str
    """The project that owns the item."""
    card_id: str | None
    """The card ID, present for a card."""
    message_id: str | None
    """The message ID, present for an AI message."""
    thread_id: str | None
    """The AI thread containing a message."""
    title: str | None
    """The card title, if this is a card."""
    role: str | None
    """The AI message role, if this is a message."""
    deleted_at: Timestamp
    """When the item entered Trash."""
    created_at: Timestamp | None
    """When the AI message was created."""
    rel_path: str | None
    """The card's relative file path."""


class TrashEmptyResult(Record):
    """The project whose Trash was emptied."""

    project_id: str
    """The requested project, including when it had no trashed items."""


class TrashDeleteResult(Record):
    """The ID of a permanently removed item."""

    card_id: str | None
    """The removed card's ID, or null for a message."""
    message_id: str | None
    """The removed AI message's ID, or null for a card."""
