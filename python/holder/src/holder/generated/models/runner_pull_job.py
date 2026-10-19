from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING, Any, TypeVar, cast

from attrs import define as _attrs_define
from attrs import field as _attrs_field
from typing_extensions import Self

from ..types import UNSET, Unset

if TYPE_CHECKING:
    from ..models.runner_pull_progress import RunnerPullProgress


T = TypeVar("T", bound="RunnerPullJob")


@_attrs_define
class RunnerPullJob:
    """
    Attributes:
        job_id (str):
        model (str):
        status (str):
        updated_at (int):
        progress (RunnerPullProgress):
        runner_id (str | Unset):
        model_ref (str | Unset):
        error (None | str | Unset):
    """

    job_id: str
    model: str
    status: str
    updated_at: int
    progress: RunnerPullProgress
    runner_id: str | Unset = UNSET
    model_ref: str | Unset = UNSET
    error: None | str | Unset = UNSET
    additional_properties: dict[str, Any] = _attrs_field(init=False, factory=dict)

    def to_dict(self) -> dict[str, Any]:
        job_id = self.job_id

        model = self.model

        status = self.status

        updated_at = self.updated_at

        progress = self.progress.to_dict()

        runner_id = self.runner_id

        model_ref = self.model_ref

        error: None | str | Unset
        if isinstance(self.error, Unset):
            error = UNSET
        else:
            error = self.error

        field_dict: dict[str, Any] = {}
        field_dict.update(self.additional_properties)
        field_dict.update(
            {
                "job_id": job_id,
                "model": model,
                "status": status,
                "updated_at": updated_at,
                "progress": progress,
            }
        )
        if runner_id is not UNSET:
            field_dict["runner_id"] = runner_id
        if model_ref is not UNSET:
            field_dict["model_ref"] = model_ref
        if error is not UNSET:
            field_dict["error"] = error

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        from ..models.runner_pull_progress import RunnerPullProgress

        d = dict(src_dict)
        job_id = d.pop("job_id")

        model = d.pop("model")

        status = d.pop("status")

        updated_at = d.pop("updated_at")

        progress = RunnerPullProgress.from_dict(d.pop("progress"))

        runner_id = d.pop("runner_id", UNSET)

        model_ref = d.pop("model_ref", UNSET)

        def _parse_error(data: object) -> None | str | Unset:
            if data is None:
                return data
            if isinstance(data, Unset):
                return data
            return cast(None | str | Unset, data)

        error = _parse_error(d.pop("error", UNSET))

        runner_pull_job = cls(
            job_id=job_id,
            model=model,
            status=status,
            updated_at=updated_at,
            progress=progress,
            runner_id=runner_id,
            model_ref=model_ref,
            error=error,
        )

        runner_pull_job.additional_properties = d
        return runner_pull_job

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
