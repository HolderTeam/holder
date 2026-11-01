"""Resolve a card's ID prefix or title within a project and deletion scope."""

from typing import Annotated, Literal

from authoring import MISSING, Field, Record, Timestamp

from .cards import CardId, ProjectId


class CardReferenceResolveRequest(Record):
    """A reference to resolve among one project's cards."""

    project_id: ProjectId
    """The project to search."""
    reference: Annotated[str, Field(min_length=1)]
    """A full canonical UUID, eligible UUID prefix, or exact card title."""
    scope: Literal["live", "trashed", "either"]
    """Which cards may match."""


class CardReferenceCandidate(Record):
    """A card that matches a reference."""

    card_id: CardId
    """The card's canonical full UUID."""
    title: str
    """The card's title."""
    deleted_at: Timestamp | None
    """When the card entered Trash, or null while live."""


class CardReferenceResolveData(Record):
    """A resolved, ambiguous, or absent card reference."""

    status: Literal["resolved", "ambiguous", "not_found"]
    """The resolution outcome."""
    match_kind: str | MISSING = MISSING
    """How a reference matched: full_id, id_prefix or exact_title. Absent when no card matched."""
    card: CardReferenceCandidate | MISSING = MISSING
    """The matching card for a resolved reference."""
    candidates: list[CardReferenceCandidate] | MISSING = MISSING
    """Two candidate cards for an ambiguous reference."""
