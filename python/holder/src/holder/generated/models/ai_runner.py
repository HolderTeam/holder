from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING, Any, TypeVar, cast

from attrs import define as _attrs_define
from attrs import field as _attrs_field
from typing_extensions import Self

if TYPE_CHECKING:
    from ..models.ai_runner_runtime import AiRunnerRuntime


T = TypeVar("T", bound="AiRunner")


@_attrs_define
class AiRunner:
    """
    Attributes:
        runner_id (str):
        name (str):
        kind (str):
        base_url (None | str):
        source (str):
        enabled (bool):
        created_at (int):
        updated_at (int):
        runtime (AiRunnerRuntime):
    """

    runner_id: str
    name: str
    kind: str
    base_url: None | str
    source: str
    enabled: bool
    created_at: int
    updated_at: int
    runtime: AiRunnerRuntime
    additional_properties: dict[str, Any] = _attrs_field(init=False, factory=dict)

    def to_dict(self) -> dict[str, Any]:
        runner_id = self.runner_id

        name = self.name

        kind = self.kind

        base_url: None | str
        base_url = self.base_url

        source = self.source

        enabled = self.enabled

        created_at = self.created_at

        updated_at = self.updated_at

        runtime = self.runtime.to_dict()

        field_dict: dict[str, Any] = {}
        field_dict.update(self.additional_properties)
        field_dict.update(
            {
                "runner_id": runner_id,
                "name": name,
                "kind": kind,
                "base_url": base_url,
                "source": source,
                "enabled": enabled,
                "created_at": created_at,
                "updated_at": updated_at,
                "runtime": runtime,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        from ..models.ai_runner_runtime import AiRunnerRuntime

        d = dict(src_dict)
        runner_id = d.pop("runner_id")

        name = d.pop("name")

        kind = d.pop("kind")

        def _parse_base_url(data: object) -> None | str:
            if data is None:
                return data
            return cast(None | str, data)

        base_url = _parse_base_url(d.pop("base_url"))

        source = d.pop("source")

        enabled = d.pop("enabled")

        created_at = d.pop("created_at")

        updated_at = d.pop("updated_at")

        runtime = AiRunnerRuntime.from_dict(d.pop("runtime"))

        ai_runner = cls(
            runner_id=runner_id,
            name=name,
            kind=kind,
            base_url=base_url,
            source=source,
            enabled=enabled,
            created_at=created_at,
            updated_at=updated_at,
            runtime=runtime,
        )

        ai_runner.additional_properties = d
        return ai_runner

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
