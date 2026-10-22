from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING, Any, TypeVar, cast

from attrs import define as _attrs_define
from attrs import field as _attrs_field
from typing_extensions import Self

if TYPE_CHECKING:
    from ..models.ai_provider_catalog_model import AiProviderCatalogModel
    from ..models.ai_provider_catalog_provider_api import AiProviderCatalogProviderApi
    from ..models.ai_provider_catalog_provider_auth import AiProviderCatalogProviderAuth


T = TypeVar("T", bound="AiProviderCatalogProvider")


@_attrs_define
class AiProviderCatalogProvider:
    """
    Attributes:
        id (str):
        display_name (str):
        enabled (bool): This device's setting, or the catalog default without one.
        configured (bool): Whether this device has an API key for the provider.
        cost_tier (None | str):
        setup_url (None | str):
        docs_url (None | str):
        api_key_label (None | str):
        api_key_hint (None | str):
        api (AiProviderCatalogProviderApi):
        auth (AiProviderCatalogProviderAuth):
        models (list[AiProviderCatalogModel]):
    """

    id: str
    display_name: str
    enabled: bool
    configured: bool
    cost_tier: None | str
    setup_url: None | str
    docs_url: None | str
    api_key_label: None | str
    api_key_hint: None | str
    api: AiProviderCatalogProviderApi
    auth: AiProviderCatalogProviderAuth
    models: list[AiProviderCatalogModel]
    additional_properties: dict[str, Any] = _attrs_field(init=False, factory=dict)

    def to_dict(self) -> dict[str, Any]:
        id = self.id

        display_name = self.display_name

        enabled = self.enabled

        configured = self.configured

        cost_tier: None | str
        cost_tier = self.cost_tier

        setup_url: None | str
        setup_url = self.setup_url

        docs_url: None | str
        docs_url = self.docs_url

        api_key_label: None | str
        api_key_label = self.api_key_label

        api_key_hint: None | str
        api_key_hint = self.api_key_hint

        api = self.api.to_dict()

        auth = self.auth.to_dict()

        models = []
        for models_item_data in self.models:
            models_item = models_item_data.to_dict()
            models.append(models_item)

        field_dict: dict[str, Any] = {}
        field_dict.update(self.additional_properties)
        field_dict.update(
            {
                "id": id,
                "display_name": display_name,
                "enabled": enabled,
                "configured": configured,
                "cost_tier": cost_tier,
                "setup_url": setup_url,
                "docs_url": docs_url,
                "api_key_label": api_key_label,
                "api_key_hint": api_key_hint,
                "api": api,
                "auth": auth,
                "models": models,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        from ..models.ai_provider_catalog_model import (
            AiProviderCatalogModel,
        )
        from ..models.ai_provider_catalog_provider_api import (
            AiProviderCatalogProviderApi,
        )
        from ..models.ai_provider_catalog_provider_auth import (
            AiProviderCatalogProviderAuth,
        )

        d = dict(src_dict)
        id = d.pop("id")

        display_name = d.pop("display_name")

        enabled = d.pop("enabled")

        configured = d.pop("configured")

        def _parse_cost_tier(data: object) -> None | str:
            if data is None:
                return data
            return cast(None | str, data)

        cost_tier = _parse_cost_tier(d.pop("cost_tier"))

        def _parse_setup_url(data: object) -> None | str:
            if data is None:
                return data
            return cast(None | str, data)

        setup_url = _parse_setup_url(d.pop("setup_url"))

        def _parse_docs_url(data: object) -> None | str:
            if data is None:
                return data
            return cast(None | str, data)

        docs_url = _parse_docs_url(d.pop("docs_url"))

        def _parse_api_key_label(data: object) -> None | str:
            if data is None:
                return data
            return cast(None | str, data)

        api_key_label = _parse_api_key_label(d.pop("api_key_label"))

        def _parse_api_key_hint(data: object) -> None | str:
            if data is None:
                return data
            return cast(None | str, data)

        api_key_hint = _parse_api_key_hint(d.pop("api_key_hint"))

        api = AiProviderCatalogProviderApi.from_dict(d.pop("api"))

        auth = AiProviderCatalogProviderAuth.from_dict(d.pop("auth"))

        models = []
        _models = d.pop("models")
        for models_item_data in _models:
            models_item = AiProviderCatalogModel.from_dict(models_item_data)

            models.append(models_item)

        ai_provider_catalog_provider = cls(
            id=id,
            display_name=display_name,
            enabled=enabled,
            configured=configured,
            cost_tier=cost_tier,
            setup_url=setup_url,
            docs_url=docs_url,
            api_key_label=api_key_label,
            api_key_hint=api_key_hint,
            api=api,
            auth=auth,
            models=models,
        )

        ai_provider_catalog_provider.additional_properties = d
        return ai_provider_catalog_provider

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
