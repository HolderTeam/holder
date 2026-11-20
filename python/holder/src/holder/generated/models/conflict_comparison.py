from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING, Any, TypeVar

from attrs import define as _attrs_define
from typing_extensions import Self

if TYPE_CHECKING:
    from ..models.card import Card
    from ..models.conflict_changed_line import ConflictChangedLine


T = TypeVar("T", bound="ConflictComparison")


@_attrs_define
class ConflictComparison:
    """The two live cards in a recorded conflict pair, read together for review.

    Attributes:
        original (Card): A card, with its content and tags.
        copy_card (Card): A card, with its content and tags.
        original_changed_lines (list[ConflictChangedLine]): One-based lines of the original's content that differ from
            the copy.
        copy_changed_lines (list[ConflictChangedLine]): One-based lines of the copy's content that differ from the
            original.
        truncated (bool): Whether the bounded diff could only highlight part of the change.
        review_token (str): An opaque token for both reviewed versions, required when saving a resolution.
    """

    original: Card
    copy_card: Card
    original_changed_lines: list[ConflictChangedLine]
    copy_changed_lines: list[ConflictChangedLine]
    truncated: bool
    review_token: str

    def to_dict(self) -> dict[str, Any]:
        original = self.original.to_dict()

        copy_card = self.copy_card.to_dict()

        original_changed_lines = []
        for original_changed_lines_item_data in self.original_changed_lines:
            original_changed_lines_item = original_changed_lines_item_data.to_dict()
            original_changed_lines.append(original_changed_lines_item)

        copy_changed_lines = []
        for copy_changed_lines_item_data in self.copy_changed_lines:
            copy_changed_lines_item = copy_changed_lines_item_data.to_dict()
            copy_changed_lines.append(copy_changed_lines_item)

        truncated = self.truncated

        review_token = self.review_token

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "original": original,
                "copy_card": copy_card,
                "original_changed_lines": original_changed_lines,
                "copy_changed_lines": copy_changed_lines,
                "truncated": truncated,
                "review_token": review_token,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        from ..models.card import Card
        from ..models.conflict_changed_line import ConflictChangedLine

        d = dict(src_dict)
        original = Card.from_dict(d.pop("original"))

        copy_card = Card.from_dict(d.pop("copy_card"))

        original_changed_lines = []
        _original_changed_lines = d.pop("original_changed_lines")
        for original_changed_lines_item_data in _original_changed_lines:
            original_changed_lines_item = ConflictChangedLine.from_dict(
                original_changed_lines_item_data
            )

            original_changed_lines.append(original_changed_lines_item)

        copy_changed_lines = []
        _copy_changed_lines = d.pop("copy_changed_lines")
        for copy_changed_lines_item_data in _copy_changed_lines:
            copy_changed_lines_item = ConflictChangedLine.from_dict(
                copy_changed_lines_item_data
            )

            copy_changed_lines.append(copy_changed_lines_item)

        truncated = d.pop("truncated")

        review_token = d.pop("review_token")

        conflict_comparison = cls(
            original=original,
            copy_card=copy_card,
            original_changed_lines=original_changed_lines,
            copy_changed_lines=copy_changed_lines,
            truncated=truncated,
            review_token=review_token,
        )

        return conflict_comparison
