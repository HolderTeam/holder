from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING, Any, TypeVar

from attrs import define as _attrs_define
from attrs import field as _attrs_field
from typing_extensions import Self

if TYPE_CHECKING:
    from ..models.ai_provider_credential import AiProviderCredential
    from ..models.ai_runner import AiRunner


T = TypeVar("T", bound="AiStatusData")


@_attrs_define
class AiStatusData:
    """AI StatusData.

    Attributes:
        checked_at (int): The checked at.
        active_runs (int): Runs still in progress across every project.
        active_pull_jobs (int): Active pulls across every runner.
        runners (list[AiRunner]): The runners.
        cloud (list[AiProviderCredential]): The cloud.
        cloud_configured_providers (int): The cloud configured providers.
    """

    checked_at: int
    active_runs: int
    active_pull_jobs: int
    runners: list[AiRunner]
    cloud: list[AiProviderCredential]
    cloud_configured_providers: int
    additional_properties: dict[str, Any] = _attrs_field(init=False, factory=dict)

    def to_dict(self) -> dict[str, Any]:
        checked_at = self.checked_at

        active_runs = self.active_runs

        active_pull_jobs = self.active_pull_jobs

        runners = []
        for runners_item_data in self.runners:
            runners_item = runners_item_data.to_dict()
            runners.append(runners_item)

        cloud = []
        for cloud_item_data in self.cloud:
            cloud_item = cloud_item_data.to_dict()
            cloud.append(cloud_item)

        cloud_configured_providers = self.cloud_configured_providers

        field_dict: dict[str, Any] = {}
        field_dict.update(self.additional_properties)
        field_dict.update(
            {
                "checked_at": checked_at,
                "active_runs": active_runs,
                "active_pull_jobs": active_pull_jobs,
                "runners": runners,
                "cloud": cloud,
                "cloud_configured_providers": cloud_configured_providers,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        from ..models.ai_provider_credential import (
            AiProviderCredential,
        )
        from ..models.ai_runner import AiRunner

        d = dict(src_dict)
        checked_at = d.pop("checked_at")

        active_runs = d.pop("active_runs")

        active_pull_jobs = d.pop("active_pull_jobs")

        runners = []
        _runners = d.pop("runners")
        for runners_item_data in _runners:
            runners_item = AiRunner.from_dict(runners_item_data)

            runners.append(runners_item)

        cloud = []
        _cloud = d.pop("cloud")
        for cloud_item_data in _cloud:
            cloud_item = AiProviderCredential.from_dict(cloud_item_data)

            cloud.append(cloud_item)

        cloud_configured_providers = d.pop("cloud_configured_providers")

        ai_status_data = cls(
            checked_at=checked_at,
            active_runs=active_runs,
            active_pull_jobs=active_pull_jobs,
            runners=runners,
            cloud=cloud,
            cloud_configured_providers=cloud_configured_providers,
        )

        ai_status_data.additional_properties = d
        return ai_status_data

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
