from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar, cast

from attrs import define as _attrs_define
from attrs import field as _attrs_field
from typing_extensions import Self

from ..models.caste_recommended_model_required_caste import (
    CasteRecommendedModelRequiredCaste,
)

T = TypeVar("T", bound="CasteRecommendedModel")


@_attrs_define
class CasteRecommendedModel:
    """CasteRecommendedModel.

    Attributes:
        tag (str): The tag.
        provider (None | str): The provider.
        engine (None | str): The engine.
        category (None | str): The category.
        required_caste (CasteRecommendedModelRequiredCaste): The required caste.
        installed (bool): The installed.
    """

    tag: str
    provider: None | str
    engine: None | str
    category: None | str
    required_caste: CasteRecommendedModelRequiredCaste
    installed: bool
    additional_properties: dict[str, Any] = _attrs_field(init=False, factory=dict)

    def to_dict(self) -> dict[str, Any]:
        tag = self.tag

        provider: None | str
        provider = self.provider

        engine: None | str
        engine = self.engine

        category: None | str
        category = self.category

        required_caste = self.required_caste.value

        installed = self.installed

        field_dict: dict[str, Any] = {}
        field_dict.update(self.additional_properties)
        field_dict.update(
            {
                "tag": tag,
                "provider": provider,
                "engine": engine,
                "category": category,
                "required_caste": required_caste,
                "installed": installed,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        tag = d.pop("tag")

        def _parse_provider(data: object) -> None | str:
            if data is None:
                return data
            return cast(None | str, data)

        provider = _parse_provider(d.pop("provider"))

        def _parse_engine(data: object) -> None | str:
            if data is None:
                return data
            return cast(None | str, data)

        engine = _parse_engine(d.pop("engine"))

        def _parse_category(data: object) -> None | str:
            if data is None:
                return data
            return cast(None | str, data)

        category = _parse_category(d.pop("category"))

        required_caste = CasteRecommendedModelRequiredCaste(d.pop("required_caste"))

        installed = d.pop("installed")

        caste_recommended_model = cls(
            tag=tag,
            provider=provider,
            engine=engine,
            category=category,
            required_caste=required_caste,
            installed=installed,
        )

        caste_recommended_model.additional_properties = d
        return caste_recommended_model

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
