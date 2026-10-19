from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar, cast

from attrs import define as _attrs_define
from attrs import field as _attrs_field
from typing_extensions import Self

from ..types import UNSET, Unset

T = TypeVar("T", bound="LocationActionResponseData")


@_attrs_define
class LocationActionResponseData:
    """
    Attributes:
        location_id (str):
        bound (bool | Unset):
        binding_preview (None | str | Unset):
    """

    location_id: str
    bound: bool | Unset = UNSET
    binding_preview: None | str | Unset = UNSET
    additional_properties: dict[str, Any] = _attrs_field(init=False, factory=dict)

    def to_dict(self) -> dict[str, Any]:
        location_id = self.location_id

        bound = self.bound

        binding_preview: None | str | Unset
        if isinstance(self.binding_preview, Unset):
            binding_preview = UNSET
        else:
            binding_preview = self.binding_preview

        field_dict: dict[str, Any] = {}
        field_dict.update(self.additional_properties)
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

        location_action_response_data = cls(
            location_id=location_id,
            bound=bound,
            binding_preview=binding_preview,
        )

        location_action_response_data.additional_properties = d
        return location_action_response_data

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
