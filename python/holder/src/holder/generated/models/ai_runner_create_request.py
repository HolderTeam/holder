from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar

from attrs import define as _attrs_define
from attrs import field as _attrs_field
from typing_extensions import Self

from ..models.ai_runner_create_request_kind import AiRunnerCreateRequestKind
from ..types import UNSET, Unset

T = TypeVar("T", bound="AiRunnerCreateRequest")


@_attrs_define
class AiRunnerCreateRequest:
    """
    Attributes:
        name (str):
        kind (AiRunnerCreateRequestKind):
        base_url (str): http://host:port
        enabled (bool | Unset): Optional; defaults to true.
    """

    name: str
    kind: AiRunnerCreateRequestKind
    base_url: str
    enabled: bool | Unset = UNSET
    additional_properties: dict[str, Any] = _attrs_field(init=False, factory=dict)

    def to_dict(self) -> dict[str, Any]:
        name = self.name

        kind = self.kind.value

        base_url = self.base_url

        enabled = self.enabled

        field_dict: dict[str, Any] = {}
        field_dict.update(self.additional_properties)
        field_dict.update(
            {
                "name": name,
                "kind": kind,
                "base_url": base_url,
            }
        )
        if enabled is not UNSET:
            field_dict["enabled"] = enabled

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        name = d.pop("name")

        kind = AiRunnerCreateRequestKind(d.pop("kind"))

        base_url = d.pop("base_url")

        enabled = d.pop("enabled", UNSET)

        ai_runner_create_request = cls(
            name=name,
            kind=kind,
            base_url=base_url,
            enabled=enabled,
        )

        ai_runner_create_request.additional_properties = d
        return ai_runner_create_request

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
