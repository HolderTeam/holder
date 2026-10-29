from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING, Any, TypeVar, cast

from attrs import define as _attrs_define
from attrs import field as _attrs_field
from typing_extensions import Self

if TYPE_CHECKING:
    from ..models.ai_provider_catalog_model_limits import AiProviderCatalogModelLimits


T = TypeVar("T", bound="AiProviderCatalogModel")


@_attrs_define
class AiProviderCatalogModel:
    """AI ProviderCatalogModel.

    Attributes:
        id (str): The id.
        endpoint (str): The endpoint.
        role (None | str): The role.
        cost_tier (None | str): The cost tier.
        default_for_low_budget (bool): The default for low budget.
        limits (AiProviderCatalogModelLimits): AI ProviderCatalogModelLimits.
    """

    id: str
    endpoint: str
    role: None | str
    cost_tier: None | str
    default_for_low_budget: bool
    limits: AiProviderCatalogModelLimits
    additional_properties: dict[str, Any] = _attrs_field(init=False, factory=dict)

    def to_dict(self) -> dict[str, Any]:
        id = self.id

        endpoint = self.endpoint

        role: None | str
        role = self.role

        cost_tier: None | str
        cost_tier = self.cost_tier

        default_for_low_budget = self.default_for_low_budget

        limits = self.limits.to_dict()

        field_dict: dict[str, Any] = {}
        field_dict.update(self.additional_properties)
        field_dict.update(
            {
                "id": id,
                "endpoint": endpoint,
                "role": role,
                "cost_tier": cost_tier,
                "default_for_low_budget": default_for_low_budget,
                "limits": limits,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        from ..models.ai_provider_catalog_model_limits import (
            AiProviderCatalogModelLimits,
        )

        d = dict(src_dict)
        id = d.pop("id")

        endpoint = d.pop("endpoint")

        def _parse_role(data: object) -> None | str:
            if data is None:
                return data
            return cast(None | str, data)

        role = _parse_role(d.pop("role"))

        def _parse_cost_tier(data: object) -> None | str:
            if data is None:
                return data
            return cast(None | str, data)

        cost_tier = _parse_cost_tier(d.pop("cost_tier"))

        default_for_low_budget = d.pop("default_for_low_budget")

        limits = AiProviderCatalogModelLimits.from_dict(d.pop("limits"))

        ai_provider_catalog_model = cls(
            id=id,
            endpoint=endpoint,
            role=role,
            cost_tier=cost_tier,
            default_for_low_budget=default_for_low_budget,
            limits=limits,
        )

        ai_provider_catalog_model.additional_properties = d
        return ai_provider_catalog_model

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
