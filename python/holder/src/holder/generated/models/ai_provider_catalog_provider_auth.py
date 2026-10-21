from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar

from attrs import define as _attrs_define
from attrs import field as _attrs_field
from typing_extensions import Self

from ..types import UNSET, Unset

T = TypeVar("T", bound="AiProviderCatalogProviderAuth")


@_attrs_define
class AiProviderCatalogProviderAuth:
    """
    Attributes:
        type_ (str | Unset):
        key_param (str | Unset):
        header_name (str | Unset):
        bearer_prefix (str | Unset):
    """

    type_: str | Unset = UNSET
    key_param: str | Unset = UNSET
    header_name: str | Unset = UNSET
    bearer_prefix: str | Unset = UNSET
    additional_properties: dict[str, Any] = _attrs_field(init=False, factory=dict)

    def to_dict(self) -> dict[str, Any]:
        type_ = self.type_

        key_param = self.key_param

        header_name = self.header_name

        bearer_prefix = self.bearer_prefix

        field_dict: dict[str, Any] = {}
        field_dict.update(self.additional_properties)
        field_dict.update({})
        if type_ is not UNSET:
            field_dict["type"] = type_
        if key_param is not UNSET:
            field_dict["key_param"] = key_param
        if header_name is not UNSET:
            field_dict["header_name"] = header_name
        if bearer_prefix is not UNSET:
            field_dict["bearer_prefix"] = bearer_prefix

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        type_ = d.pop("type", UNSET)

        key_param = d.pop("key_param", UNSET)

        header_name = d.pop("header_name", UNSET)

        bearer_prefix = d.pop("bearer_prefix", UNSET)

        ai_provider_catalog_provider_auth = cls(
            type_=type_,
            key_param=key_param,
            header_name=header_name,
            bearer_prefix=bearer_prefix,
        )

        ai_provider_catalog_provider_auth.additional_properties = d
        return ai_provider_catalog_provider_auth

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
