from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar

from attrs import define as _attrs_define
from typing_extensions import Self

from ..types import UNSET, Unset

T = TypeVar("T", bound="HierarchyRepairRequest")


@_attrs_define
class HierarchyRepairRequest:
    """Ask for a repair plan, or apply it.

    Attributes:
        apply (bool | Unset): Apply the repair. Without it, only report. Default: False.
    """

    apply: bool | Unset = False

    def to_dict(self) -> dict[str, Any]:
        apply = self.apply

        field_dict: dict[str, Any] = {}

        field_dict.update({})
        if apply is not UNSET:
            field_dict["apply"] = apply

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        apply = d.pop("apply", UNSET)

        hierarchy_repair_request = cls(
            apply=apply,
        )

        return hierarchy_repair_request
