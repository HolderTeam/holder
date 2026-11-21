from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar

from attrs import define as _attrs_define
from typing_extensions import Self

T = TypeVar("T", bound="ConflictChangedLine")


@_attrs_define
class ConflictChangedLine:
    """A highlighted line in a conflict comparison.

    Attributes:
        line (int): The one-based line number within this card's content.
    """

    line: int

    def to_dict(self) -> dict[str, Any]:
        line = self.line

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "line": line,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        line = d.pop("line")

        conflict_changed_line = cls(
            line=line,
        )

        return conflict_changed_line
