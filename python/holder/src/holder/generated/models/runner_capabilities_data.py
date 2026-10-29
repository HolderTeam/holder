from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING, Any, TypeVar, cast

from attrs import define as _attrs_define
from attrs import field as _attrs_field
from typing_extensions import Self

from ..types import UNSET, Unset

if TYPE_CHECKING:
    from ..models.ai_local_model_config_data import AiLocalModelConfigData
    from ..models.ai_runner import AiRunner
    from ..models.caste_info import CasteInfo
    from ..models.caste_recommended_model import CasteRecommendedModel


T = TypeVar("T", bound="RunnerCapabilitiesData")


@_attrs_define
class RunnerCapabilitiesData:
    """RunnerCapabilitiesData.

    Attributes:
        local_model_config (AiLocalModelConfigData): AI LocalModelConfigData.
        runners (list[AiRunner]): The runners.
        recommended_models (list[CasteRecommendedModel]): Catalog models this machine's caste can run, best first.
        recommended_install (list[CasteRecommendedModel]): The recommended models that the selected runner has not
            installed.
        caste (CasteInfo | None | Unset): This machine's hardware caste, or null when it cannot be detected.
    """

    local_model_config: AiLocalModelConfigData
    runners: list[AiRunner]
    recommended_models: list[CasteRecommendedModel]
    recommended_install: list[CasteRecommendedModel]
    caste: CasteInfo | None | Unset = UNSET
    additional_properties: dict[str, Any] = _attrs_field(init=False, factory=dict)

    def to_dict(self) -> dict[str, Any]:
        from ..models.caste_info import CasteInfo

        local_model_config = self.local_model_config.to_dict()

        runners = []
        for runners_item_data in self.runners:
            runners_item = runners_item_data.to_dict()
            runners.append(runners_item)

        recommended_models = []
        for recommended_models_item_data in self.recommended_models:
            recommended_models_item = recommended_models_item_data.to_dict()
            recommended_models.append(recommended_models_item)

        recommended_install = []
        for recommended_install_item_data in self.recommended_install:
            recommended_install_item = recommended_install_item_data.to_dict()
            recommended_install.append(recommended_install_item)

        caste: dict[str, Any] | None | Unset
        if isinstance(self.caste, Unset):
            caste = UNSET
        elif isinstance(self.caste, CasteInfo):
            caste = self.caste.to_dict()
        else:
            caste = self.caste

        field_dict: dict[str, Any] = {}
        field_dict.update(self.additional_properties)
        field_dict.update(
            {
                "local_model_config": local_model_config,
                "runners": runners,
                "recommended_models": recommended_models,
                "recommended_install": recommended_install,
            }
        )
        if caste is not UNSET:
            field_dict["caste"] = caste

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        from ..models.ai_local_model_config_data import (
            AiLocalModelConfigData,
        )
        from ..models.ai_runner import AiRunner
        from ..models.caste_info import CasteInfo
        from ..models.caste_recommended_model import (
            CasteRecommendedModel,
        )

        d = dict(src_dict)
        local_model_config = AiLocalModelConfigData.from_dict(
            d.pop("local_model_config")
        )

        runners = []
        _runners = d.pop("runners")
        for runners_item_data in _runners:
            runners_item = AiRunner.from_dict(runners_item_data)

            runners.append(runners_item)

        recommended_models = []
        _recommended_models = d.pop("recommended_models")
        for recommended_models_item_data in _recommended_models:
            recommended_models_item = CasteRecommendedModel.from_dict(
                recommended_models_item_data
            )

            recommended_models.append(recommended_models_item)

        recommended_install = []
        _recommended_install = d.pop("recommended_install")
        for recommended_install_item_data in _recommended_install:
            recommended_install_item = CasteRecommendedModel.from_dict(
                recommended_install_item_data
            )

            recommended_install.append(recommended_install_item)

        def _parse_caste(data: object) -> CasteInfo | None | Unset:
            if data is None:
                return data
            if isinstance(data, Unset):
                return data
            try:
                if not isinstance(data, dict):
                    raise TypeError()
                caste_type_0 = CasteInfo.from_dict(data)

                return caste_type_0
            except (TypeError, ValueError, AttributeError, KeyError):
                pass
            return cast(CasteInfo | None | Unset, data)

        caste = _parse_caste(d.pop("caste", UNSET))

        runner_capabilities_data = cls(
            local_model_config=local_model_config,
            runners=runners,
            recommended_models=recommended_models,
            recommended_install=recommended_install,
            caste=caste,
        )

        runner_capabilities_data.additional_properties = d
        return runner_capabilities_data

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
