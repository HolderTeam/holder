from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING, Any, TypeVar

from attrs import define as _attrs_define
from typing_extensions import Self

if TYPE_CHECKING:
    from ..models.location_test import LocationTest


T = TypeVar("T", bound="LocationTestResponse")


@_attrs_define
class LocationTestResponse:
    """The result of testing a storage location.

    Attributes:
        ok (bool): Whether the request succeeded.
        data (LocationTest): The result of checking a storage location.
    """

    ok: bool
    data: LocationTest

    def to_dict(self) -> dict[str, Any]:
        ok = self.ok

        data = self.data.to_dict()

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "ok": ok,
                "data": data,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        from ..models.location_test import LocationTest

        d = dict(src_dict)
        ok = d.pop("ok")

        data = LocationTest.from_dict(d.pop("data"))

        location_test_response = cls(
            ok=ok,
            data=data,
        )

        return location_test_response
