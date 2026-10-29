from __future__ import annotations

from collections.abc import Mapping
from typing import (
    Any,
    Literal,
    TypeVar,
    cast,
)

from attrs import define as _attrs_define
from attrs import field as _attrs_field
from typing_extensions import Self

from ..types import UNSET, Unset

T = TypeVar("T", bound="AiRunnerCreateRequest")


@_attrs_define
class AiRunnerCreateRequest:
    """AI RunnerCreateRequest.

    Attributes:
        name (str): The name.
        kind (Literal['ollama']): The kind.
        base_url (str): http://host:port
        enabled (bool | None | Unset): Optional; defaults to true.
    """

    name: str
    kind: Literal["ollama"]
    base_url: str
    enabled: bool | None | Unset = UNSET
    additional_properties: dict[str, Any] = _attrs_field(init=False, factory=dict)

    def to_dict(self) -> dict[str, Any]:
        name = self.name

        kind = self.kind

        base_url = self.base_url

        enabled: bool | None | Unset
        if isinstance(self.enabled, Unset):
            enabled = UNSET
        else:
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

        kind = cast(Literal["ollama"], d.pop("kind"))
        if kind != "ollama":
            raise ValueError(f"kind must match const 'ollama', got '{kind}'")

        base_url = d.pop("base_url")

        def _parse_enabled(data: object) -> bool | None | Unset:
            if data is None:
                return data
            if isinstance(data, Unset):
                return data
            return cast(bool | None | Unset, data)

        enabled = _parse_enabled(d.pop("enabled", UNSET))

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
