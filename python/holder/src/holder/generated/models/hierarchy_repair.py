from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING, Any, TypeVar, cast

from attrs import define as _attrs_define
from typing_extensions import Self

if TYPE_CHECKING:
    from ..models.hierarchy_repair_move import HierarchyRepairMove


T = TypeVar("T", bound="HierarchyRepair")


@_attrs_define
class HierarchyRepair:
    """A plan to make hidden live cards reachable, and whether it was applied.

    Attributes:
        applied (bool): Whether changes were written.
        moved (list[HierarchyRepairMove]): The hidden branches whose top cards move.
        changed_card_count (int): Cards moved or re-spaced to make room.
        cycle_card_ids (list[str]): Cards in parent cycles; left for manual repair.
    """

    applied: bool
    moved: list[HierarchyRepairMove]
    changed_card_count: int
    cycle_card_ids: list[str]

    def to_dict(self) -> dict[str, Any]:
        applied = self.applied

        moved = []
        for moved_item_data in self.moved:
            moved_item = moved_item_data.to_dict()
            moved.append(moved_item)

        changed_card_count = self.changed_card_count

        cycle_card_ids = self.cycle_card_ids

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "applied": applied,
                "moved": moved,
                "changed_card_count": changed_card_count,
                "cycle_card_ids": cycle_card_ids,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        from ..models.hierarchy_repair_move import HierarchyRepairMove

        d = dict(src_dict)
        applied = d.pop("applied")

        moved = []
        _moved = d.pop("moved")
        for moved_item_data in _moved:
            moved_item = HierarchyRepairMove.from_dict(moved_item_data)

            moved.append(moved_item)

        changed_card_count = d.pop("changed_card_count")

        cycle_card_ids = cast(list[str], d.pop("cycle_card_ids"))

        hierarchy_repair = cls(
            applied=applied,
            moved=moved,
            changed_card_count=changed_card_count,
            cycle_card_ids=cycle_card_ids,
        )

        return hierarchy_repair
