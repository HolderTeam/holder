from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar

from attrs import define as _attrs_define
from attrs import field as _attrs_field
from typing_extensions import Self

T = TypeVar("T", bound="AiProviderCatalogModelLimits")


@_attrs_define
class AiProviderCatalogModelLimits:
    """
    Attributes:
        rpm (int): Requests per minute; 0 when unlimited or unknown.
        tpm (int): Tokens per minute; 0 when unlimited or unknown.
        rpd (int): Requests per day; 0 when unlimited or unknown.
    """

    rpm: int
    tpm: int
    rpd: int
    additional_properties: dict[str, Any] = _attrs_field(init=False, factory=dict)

    def to_dict(self) -> dict[str, Any]:
        rpm = self.rpm

        tpm = self.tpm

        rpd = self.rpd

        field_dict: dict[str, Any] = {}
        field_dict.update(self.additional_properties)
        field_dict.update(
            {
                "rpm": rpm,
                "tpm": tpm,
                "rpd": rpd,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        rpm = d.pop("rpm")

        tpm = d.pop("tpm")

        rpd = d.pop("rpd")

        ai_provider_catalog_model_limits = cls(
            rpm=rpm,
            tpm=tpm,
            rpd=rpd,
        )

        ai_provider_catalog_model_limits.additional_properties = d
        return ai_provider_catalog_model_limits

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
