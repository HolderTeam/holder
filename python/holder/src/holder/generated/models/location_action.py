from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar, cast

from attrs import define as _attrs_define
from typing_extensions import Self

from ..types import UNSET, Unset

T = TypeVar("T", bound="LocationAction")


@_attrs_define
class LocationAction:
    """The location affected by a binding or preference change.

    Attributes:
        location_id (str): The affected location ID.
        bound (bool | Unset): Whether this device has a binding, when returned.
        binding_preview (None | str | Unset): Safe binding text, when returned.
    """

    location_id: str
    bound: bool | Unset = UNSET
    binding_preview: None | str | Unset = UNSET

    def to_dict(self) -> dict[str, Any]:
        location_id = self.location_id

        bound = self.bound

        binding_preview: None | str | Unset
        if isinstance(self.binding_preview, Unset):
            binding_preview = UNSET
        else:
            binding_preview = self.binding_preview

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "location_id": location_id,
            }
        )
        if bound is not UNSET:
            field_dict["bound"] = bound
        if binding_preview is not UNSET:
            field_dict["binding_preview"] = binding_preview

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        location_id = d.pop("location_id")

        bound = d.pop("bound", UNSET)

        def _parse_binding_preview(data: object) -> None | str | Unset:
            if data is None:
                return data
            if isinstance(data, Unset):
                return data
            return cast(None | str | Unset, data)

        binding_preview = _parse_binding_preview(d.pop("binding_preview", UNSET))

        location_action = cls(
            location_id=location_id,
            bound=bound,
            binding_preview=binding_preview,
        )

        return location_action
