"""Relevance-ranked matches from a project's full-text index."""

from holder_contract import Float64, Record, Timestamp


class SearchCard(Record):
    """A card matching a full-text query."""

    card_id: str
    """The matching card's ID."""
    title: str
    """The card's current title."""
    updated_at: Timestamp
    """When the card was last updated."""
    created_at: Timestamp
    """When the card was created."""
    snippet: str
    """A highlighted excerpt from the match."""
    rank: Float64
    """The FTS relevance score; lower scores rank first."""


class SearchMessage(Record):
    """An AI message matching a full-text query."""

    message_id: str
    """The matching message's ID."""
    created_at: Timestamp
    """When the message was created."""
    snippet: str
    """A highlighted excerpt from the match."""
    rank: Float64
    """The FTS relevance score; lower scores rank first."""
