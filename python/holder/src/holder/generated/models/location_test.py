from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar

from attrs import define as _attrs_define
from typing_extensions import Self

T = TypeVar("T", bound="LocationTest")


@_attrs_define
class LocationTest:
    """The result of checking a storage location.

    Attributes:
        available (bool): Whether the storage probe succeeded.
    """

    available: bool

    def to_dict(self) -> dict[str, Any]:
        available = self.available

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "available": available,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        available = d.pop("available")

        location_test = cls(
            available=available,
        )

        return location_test
