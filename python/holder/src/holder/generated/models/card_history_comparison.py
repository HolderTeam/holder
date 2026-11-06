from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING, Any, TypeVar

from attrs import define as _attrs_define
from typing_extensions import Self

if TYPE_CHECKING:
    from ..models.card_history_diff_line import CardHistoryDiffLine
    from ..models.card_history_version import CardHistoryVersion


T = TypeVar("T", bound="CardHistoryComparison")


@_attrs_define
class CardHistoryComparison:
    """A bounded line comparison of two saved card states.

    Attributes:
        before (CardHistoryVersion): A card's saved state at one commit, including an explicit absent state.
        after (CardHistoryVersion): A card's saved state at one commit, including an explicit absent state.
        summary (str): A short description of the difference.
        lines (list[CardHistoryDiffLine]): The comparison's lines.
        truncated (bool): Whether the line comparison was cut short.
    """

    before: CardHistoryVersion
    after: CardHistoryVersion
    summary: str
    lines: list[CardHistoryDiffLine]
    truncated: bool

    def to_dict(self) -> dict[str, Any]:
        before = self.before.to_dict()

        after = self.after.to_dict()

        summary = self.summary

        lines = []
        for lines_item_data in self.lines:
            lines_item = lines_item_data.to_dict()
            lines.append(lines_item)

        truncated = self.truncated

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "before": before,
                "after": after,
                "summary": summary,
                "lines": lines,
                "truncated": truncated,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        from ..models.card_history_diff_line import CardHistoryDiffLine
        from ..models.card_history_version import CardHistoryVersion

        d = dict(src_dict)
        before = CardHistoryVersion.from_dict(d.pop("before"))

        after = CardHistoryVersion.from_dict(d.pop("after"))

        summary = d.pop("summary")

        lines = []
        _lines = d.pop("lines")
        for lines_item_data in _lines:
            lines_item = CardHistoryDiffLine.from_dict(lines_item_data)

            lines.append(lines_item)

        truncated = d.pop("truncated")

        card_history_comparison = cls(
            before=before,
            after=after,
            summary=summary,
            lines=lines,
            truncated=truncated,
        )

        return card_history_comparison
