from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar

from attrs import define as _attrs_define
from typing_extensions import Self

T = TypeVar("T", bound="ConflictCopyChildChoice")


@_attrs_define
class ConflictCopyChildChoice:
    """One card from the cleanup review's children.

    Attributes:
        card_id (str):
    """

    card_id: str

    def to_dict(self) -> dict[str, Any]:
        card_id = self.card_id

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "card_id": card_id,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        card_id = d.pop("card_id")

        conflict_copy_child_choice = cls(
            card_id=card_id,
        )

        return conflict_copy_child_choice
