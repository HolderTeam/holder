from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING, Any, TypeVar

from attrs import define as _attrs_define
from typing_extensions import Self

if TYPE_CHECKING:
    from ..models.location_action import LocationAction


T = TypeVar("T", bound="LocationActionResponse")


@_attrs_define
class LocationActionResponse:
    """The result of changing a binding or preference.

    Attributes:
        ok (bool): Whether the request succeeded.
        data (LocationAction): The location affected by a binding or preference change.
    """

    ok: bool
    data: LocationAction

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
        from ..models.location_action import LocationAction

        d = dict(src_dict)
        ok = d.pop("ok")

        data = LocationAction.from_dict(d.pop("data"))

        location_action_response = cls(
            ok=ok,
            data=data,
        )

        return location_action_response
