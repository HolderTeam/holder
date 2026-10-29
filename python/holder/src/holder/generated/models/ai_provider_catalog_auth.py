from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar, cast

from attrs import define as _attrs_define
from attrs import field as _attrs_field
from typing_extensions import Self

from ..types import UNSET, Unset

T = TypeVar("T", bound="AiProviderCatalogAuth")


@_attrs_define
class AiProviderCatalogAuth:
    """AI ProviderCatalogAuth.

    Attributes:
        type_ (None | str | Unset): The type.
        key_param (None | str | Unset): The key param.
        header_name (None | str | Unset): The header name.
        bearer_prefix (None | str | Unset): The bearer prefix.
    """

    type_: None | str | Unset = UNSET
    key_param: None | str | Unset = UNSET
    header_name: None | str | Unset = UNSET
    bearer_prefix: None | str | Unset = UNSET
    additional_properties: dict[str, Any] = _attrs_field(init=False, factory=dict)

    def to_dict(self) -> dict[str, Any]:
        type_: None | str | Unset
        if isinstance(self.type_, Unset):
            type_ = UNSET
        else:
            type_ = self.type_

        key_param: None | str | Unset
        if isinstance(self.key_param, Unset):
            key_param = UNSET
        else:
            key_param = self.key_param

        header_name: None | str | Unset
        if isinstance(self.header_name, Unset):
            header_name = UNSET
        else:
            header_name = self.header_name

        bearer_prefix: None | str | Unset
        if isinstance(self.bearer_prefix, Unset):
            bearer_prefix = UNSET
        else:
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

        def _parse_type_(data: object) -> None | str | Unset:
            if data is None:
                return data
            if isinstance(data, Unset):
                return data
            return cast(None | str | Unset, data)

        type_ = _parse_type_(d.pop("type", UNSET))

        def _parse_key_param(data: object) -> None | str | Unset:
            if data is None:
                return data
            if isinstance(data, Unset):
                return data
            return cast(None | str | Unset, data)

        key_param = _parse_key_param(d.pop("key_param", UNSET))

        def _parse_header_name(data: object) -> None | str | Unset:
            if data is None:
                return data
            if isinstance(data, Unset):
                return data
            return cast(None | str | Unset, data)

        header_name = _parse_header_name(d.pop("header_name", UNSET))

        def _parse_bearer_prefix(data: object) -> None | str | Unset:
            if data is None:
                return data
            if isinstance(data, Unset):
                return data
            return cast(None | str | Unset, data)

        bearer_prefix = _parse_bearer_prefix(d.pop("bearer_prefix", UNSET))

        ai_provider_catalog_auth = cls(
            type_=type_,
            key_param=key_param,
            header_name=header_name,
            bearer_prefix=bearer_prefix,
        )

        ai_provider_catalog_auth.additional_properties = d
        return ai_provider_catalog_auth

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
