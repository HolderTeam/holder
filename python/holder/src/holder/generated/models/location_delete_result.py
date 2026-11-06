from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar

from attrs import define as _attrs_define
from typing_extensions import Self

T = TypeVar("T", bound="LocationDeleteResult")


@_attrs_define
class LocationDeleteResult:
    """The location affected by an action.

    Attributes:
        location_id (str):
    """

    location_id: str

    def to_dict(self) -> dict[str, Any]:
        location_id = self.location_id

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "location_id": location_id,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        location_id = d.pop("location_id")

        location_delete_result = cls(
            location_id=location_id,
        )

        return location_delete_result
