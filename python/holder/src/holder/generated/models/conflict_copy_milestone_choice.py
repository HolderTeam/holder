from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar

from attrs import define as _attrs_define
from typing_extensions import Self

T = TypeVar("T", bound="ConflictCopyMilestoneChoice")


@_attrs_define
class ConflictCopyMilestoneChoice:
    """One milestone from the cleanup review's copy_milestones.

    Attributes:
        milestone_id (str):
    """

    milestone_id: str

    def to_dict(self) -> dict[str, Any]:
        milestone_id = self.milestone_id

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "milestone_id": milestone_id,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        milestone_id = d.pop("milestone_id")

        conflict_copy_milestone_choice = cls(
            milestone_id=milestone_id,
        )

        return conflict_copy_milestone_choice
