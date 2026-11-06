from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING, Any, TypeVar

from attrs import define as _attrs_define
from typing_extensions import Self

from ..types import UNSET, Unset

if TYPE_CHECKING:
    from ..models.location_binding_request_values import LocationBindingRequestValues


T = TypeVar("T", bound="LocationBindingRequest")


@_attrs_define
class LocationBindingRequest:
    """Private provider values for a location on this device.

    Attributes:
        values (LocationBindingRequestValues): Provider credentials or local path.
        preview (str | Unset): Safe display text for the binding; defaults to Configured.
    """

    values: LocationBindingRequestValues
    preview: str | Unset = UNSET

    def to_dict(self) -> dict[str, Any]:
        values = self.values.to_dict()

        preview = self.preview

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "values": values,
            }
        )
        if preview is not UNSET:
            field_dict["preview"] = preview

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        from ..models.location_binding_request_values import (
            LocationBindingRequestValues,
        )

        d = dict(src_dict)
        values = LocationBindingRequestValues.from_dict(d.pop("values"))

        preview = d.pop("preview", UNSET)

        location_binding_request = cls(
            values=values,
            preview=preview,
        )

        return location_binding_request
