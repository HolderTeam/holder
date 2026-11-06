from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar, cast

from attrs import define as _attrs_define
from typing_extensions import Self

T = TypeVar("T", bound="CardHistoryDiffLine")


@_attrs_define
class CardHistoryDiffLine:
    """One line in a card comparison.

    Attributes:
        origin (str): Space for unchanged, plus for added, minus for removed.
        text (str): The line's text.
        old_line (int | None): Its old line number; null for an addition.
        new_line (int | None): Its new line number; null for a removal.
    """

    origin: str
    text: str
    old_line: int | None
    new_line: int | None

    def to_dict(self) -> dict[str, Any]:
        origin = self.origin

        text = self.text

        old_line: int | None
        old_line = self.old_line

        new_line: int | None
        new_line = self.new_line

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "origin": origin,
                "text": text,
                "old_line": old_line,
                "new_line": new_line,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        origin = d.pop("origin")

        text = d.pop("text")

        def _parse_old_line(data: object) -> int | None:
            if data is None:
                return data
            return cast(int | None, data)

        old_line = _parse_old_line(d.pop("old_line"))

        def _parse_new_line(data: object) -> int | None:
            if data is None:
                return data
            return cast(int | None, data)

        new_line = _parse_new_line(d.pop("new_line"))

        card_history_diff_line = cls(
            origin=origin,
            text=text,
            old_line=old_line,
            new_line=new_line,
        )

        return card_history_diff_line
