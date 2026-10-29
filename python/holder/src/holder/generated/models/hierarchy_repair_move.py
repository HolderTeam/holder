from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar, cast

from attrs import define as _attrs_define
from typing_extensions import Self

T = TypeVar("T", bound="HierarchyRepairMove")


@_attrs_define
class HierarchyRepairMove:
    """A hidden branch's top card and its new parent.

    Attributes:
        card_id (str):
        parent_card_id (None | str): Its new parent, or null for a project root.
    """

    card_id: str
    parent_card_id: None | str

    def to_dict(self) -> dict[str, Any]:
        card_id = self.card_id

        parent_card_id: None | str
        parent_card_id = self.parent_card_id

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "card_id": card_id,
                "parent_card_id": parent_card_id,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        card_id = d.pop("card_id")

        def _parse_parent_card_id(data: object) -> None | str:
            if data is None:
                return data
            return cast(None | str, data)

        parent_card_id = _parse_parent_card_id(d.pop("parent_card_id"))

        hierarchy_repair_move = cls(
            card_id=card_id,
            parent_card_id=parent_card_id,
        )

        return hierarchy_repair_move
