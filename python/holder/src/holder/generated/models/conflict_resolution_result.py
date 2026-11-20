from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING, Any, TypeVar

from attrs import define as _attrs_define
from typing_extensions import Self

if TYPE_CHECKING:
    from ..models.card import Card
    from ..models.conflict_copy_cleanup_review import ConflictCopyCleanupReview


T = TypeVar("T", bound="ConflictResolutionResult")


@_attrs_define
class ConflictResolutionResult:
    """The durably saved original and a review for optional cleanup.

    Attributes:
        card (Card): A card, with its content and tags.
        cleanup (ConflictCopyCleanupReview): What stays on the preserved copy or is affected if the copy moves to Trash.
    """

    card: Card
    cleanup: ConflictCopyCleanupReview

    def to_dict(self) -> dict[str, Any]:
        card = self.card.to_dict()

        cleanup = self.cleanup.to_dict()

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "card": card,
                "cleanup": cleanup,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        from ..models.card import Card
        from ..models.conflict_copy_cleanup_review import (
            ConflictCopyCleanupReview,
        )

        d = dict(src_dict)
        card = Card.from_dict(d.pop("card"))

        cleanup = ConflictCopyCleanupReview.from_dict(d.pop("cleanup"))

        conflict_resolution_result = cls(
            card=card,
            cleanup=cleanup,
        )

        return conflict_resolution_result
