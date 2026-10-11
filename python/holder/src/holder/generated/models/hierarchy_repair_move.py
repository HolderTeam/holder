from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar, cast

from attrs import define as _attrs_define
from attrs import field as _attrs_field
from typing_extensions import Self

T = TypeVar("T", bound="HierarchyRepairMove")


@_attrs_define
class HierarchyRepairMove:
    """
    Attributes:
        card_id (str): A hidden branch's top card. Its descendants stay attached and move with it.
        parent_card_id (None | str): Where it goes, or null for the project's top level.
    """

    card_id: str
    parent_card_id: None | str
    additional_properties: dict[str, Any] = _attrs_field(init=False, factory=dict)

    def to_dict(self) -> dict[str, Any]:
        card_id = self.card_id

        parent_card_id: None | str
        parent_card_id = self.parent_card_id

        field_dict: dict[str, Any] = {}
        field_dict.update(self.additional_properties)
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

        hierarchy_repair_move.additional_properties = d
        return hierarchy_repair_move

    @property
    def additional_keys(self) -> list[str]:
        return list(self.additional_properties.keys())

    def __getitem__(self, key: str) -> Any:
        return self.additional_properties[key]

    def __setitem__(self, key: str, value: Any) -> None:
        self.additional_properties[key] = value

    def __delitem__(self, key: str) -> None:
        del self.additional_properties[key]

    def __contains__(self, key: str) -> bool:
        return key in self.additional_properties
