from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING, Any, TypeVar, cast

from attrs import define as _attrs_define
from attrs import field as _attrs_field
from typing_extensions import Self

if TYPE_CHECKING:
    from ..models.local_model import LocalModel
    from ..models.runner_pull_job import RunnerPullJob


T = TypeVar("T", bound="AiRunnerRuntime")


@_attrs_define
class AiRunnerRuntime:
    """AI RunnerRuntime.

    Attributes:
        configured (bool): The configured.
        available (bool): The available.
        spawn_attempted (bool): The spawn attempted.
        last_checked (int): The last checked.
        version (None | str): The version.
        error (None | str): The error.
        models (list[LocalModel]): The models.
        pulls (list[RunnerPullJob]): The pulls.
    """

    configured: bool
    available: bool
    spawn_attempted: bool
    last_checked: int
    version: None | str
    error: None | str
    models: list[LocalModel]
    pulls: list[RunnerPullJob]
    additional_properties: dict[str, Any] = _attrs_field(init=False, factory=dict)

    def to_dict(self) -> dict[str, Any]:
        configured = self.configured

        available = self.available

        spawn_attempted = self.spawn_attempted

        last_checked = self.last_checked

        version: None | str
        version = self.version

        error: None | str
        error = self.error

        models = []
        for models_item_data in self.models:
            models_item = models_item_data.to_dict()
            models.append(models_item)

        pulls = []
        for pulls_item_data in self.pulls:
            pulls_item = pulls_item_data.to_dict()
            pulls.append(pulls_item)

        field_dict: dict[str, Any] = {}
        field_dict.update(self.additional_properties)
        field_dict.update(
            {
                "configured": configured,
                "available": available,
                "spawn_attempted": spawn_attempted,
                "last_checked": last_checked,
                "version": version,
                "error": error,
                "models": models,
                "pulls": pulls,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        from ..models.local_model import LocalModel
        from ..models.runner_pull_job import RunnerPullJob

        d = dict(src_dict)
        configured = d.pop("configured")

        available = d.pop("available")

        spawn_attempted = d.pop("spawn_attempted")

        last_checked = d.pop("last_checked")

        def _parse_version(data: object) -> None | str:
            if data is None:
                return data
            return cast(None | str, data)

        version = _parse_version(d.pop("version"))

        def _parse_error(data: object) -> None | str:
            if data is None:
                return data
            return cast(None | str, data)

        error = _parse_error(d.pop("error"))

        models = []
        _models = d.pop("models")
        for models_item_data in _models:
            models_item = LocalModel.from_dict(models_item_data)

            models.append(models_item)

        pulls = []
        _pulls = d.pop("pulls")
        for pulls_item_data in _pulls:
            pulls_item = RunnerPullJob.from_dict(pulls_item_data)

            pulls.append(pulls_item)

        ai_runner_runtime = cls(
            configured=configured,
            available=available,
            spawn_attempted=spawn_attempted,
            last_checked=last_checked,
            version=version,
            error=error,
            models=models,
            pulls=pulls,
        )

        ai_runner_runtime.additional_properties = d
        return ai_runner_runtime

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
