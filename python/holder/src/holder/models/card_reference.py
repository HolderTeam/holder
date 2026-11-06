"""Results of looking up a card by ID, ID prefix, or title."""

from __future__ import annotations

from dataclasses import dataclass
from typing import Any, Literal, Self

from ..exceptions import ProtocolError
from ..generated.types import Unset


@dataclass(frozen=True)
class CardReferenceCandidate:
    """A lightweight match; it can refer to a card in Trash."""

    id: str
    title: str
    deleted_at: int | None

    @classmethod
    def _from(cls, row: Any) -> Self:
        if (
            not isinstance(row.card_id, str)
            or not row.card_id
            or not isinstance(row.title, str)
            or (row.deleted_at is not None and not isinstance(row.deleted_at, int))
        ):
            raise ProtocolError("Invalid card reference candidate")
        return cls(row.card_id, row.title, row.deleted_at)


@dataclass(frozen=True)
class CardReferenceResult:
    """A lookup outcome, including limited examples when ambiguous."""

    status: Literal["resolved", "ambiguous", "not_found"]
    match_kind: Literal["full_id", "id_prefix", "exact_title"] | None
    card: CardReferenceCandidate | None
    candidates: tuple[CardReferenceCandidate, ...]

    @classmethod
    def _from(cls, row: Any) -> Self:
        status = str(row.status)
        match_kind = None if isinstance(row.match_kind, Unset) else str(row.match_kind)
        card = (
            None
            if isinstance(row.card, Unset)
            else CardReferenceCandidate._from(row.card)
        )
        candidates = (
            ()
            if isinstance(row.candidates, Unset)
            else tuple(CardReferenceCandidate._from(item) for item in row.candidates)
        )
        if (
            (
                status == "resolved"
                and card is not None
                and match_kind in ("full_id", "id_prefix", "exact_title")
                and not candidates
            )
            or (
                status == "ambiguous"
                and card is None
                and match_kind in ("id_prefix", "exact_title")
                and len(candidates) >= 2
            )
            or (
                status == "not_found"
                and card is None
                and match_kind is None
                and not candidates
            )
        ):
            return cls(status, match_kind, card, candidates)
        raise ProtocolError("Invalid card reference result")
