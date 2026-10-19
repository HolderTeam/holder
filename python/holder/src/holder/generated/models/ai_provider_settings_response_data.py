from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING, Any, TypeVar

from attrs import define as _attrs_define
from attrs import field as _attrs_field
from typing_extensions import Self

if TYPE_CHECKING:
    from ..models.ai_provider_setting import AiProviderSetting


T = TypeVar("T", bound="AiProviderSettingsResponseData")


@_attrs_define
class AiProviderSettingsResponseData:
    """
    Attributes:
        providers (list[AiProviderSetting]):
    """

    providers: list[AiProviderSetting]
    additional_properties: dict[str, Any] = _attrs_field(init=False, factory=dict)

    def to_dict(self) -> dict[str, Any]:
        providers = []
        for providers_item_data in self.providers:
            providers_item = providers_item_data.to_dict()
            providers.append(providers_item)

        field_dict: dict[str, Any] = {}
        field_dict.update(self.additional_properties)
        field_dict.update(
            {
                "providers": providers,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        from ..models.ai_provider_setting import AiProviderSetting

        d = dict(src_dict)
        providers = []
        _providers = d.pop("providers")
        for providers_item_data in _providers:
            providers_item = AiProviderSetting.from_dict(providers_item_data)

            providers.append(providers_item)

        ai_provider_settings_response_data = cls(
            providers=providers,
        )

        ai_provider_settings_response_data.additional_properties = d
        return ai_provider_settings_response_data

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
