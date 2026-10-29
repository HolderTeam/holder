from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar, cast

from attrs import define as _attrs_define
from attrs import field as _attrs_field
from typing_extensions import Self

from ..types import UNSET, Unset

T = TypeVar("T", bound="RunnerPullStartResponseData")


@_attrs_define
class RunnerPullStartResponseData:
    """RunnerPullStart responseData.

    Attributes:
        job_id (str): The job id.
        model (str): The model.
        status (str): The status.
        runner_id (None | str | Unset): The runner id.
        model_ref (None | str | Unset): The model ref.
    """

    job_id: str
    model: str
    status: str
    runner_id: None | str | Unset = UNSET
    model_ref: None | str | Unset = UNSET
    additional_properties: dict[str, Any] = _attrs_field(init=False, factory=dict)

    def to_dict(self) -> dict[str, Any]:
        job_id = self.job_id

        model = self.model

        status = self.status

        runner_id: None | str | Unset
        if isinstance(self.runner_id, Unset):
            runner_id = UNSET
        else:
            runner_id = self.runner_id

        model_ref: None | str | Unset
        if isinstance(self.model_ref, Unset):
            model_ref = UNSET
        else:
            model_ref = self.model_ref

        field_dict: dict[str, Any] = {}
        field_dict.update(self.additional_properties)
        field_dict.update(
            {
                "job_id": job_id,
                "model": model,
                "status": status,
            }
        )
        if runner_id is not UNSET:
            field_dict["runner_id"] = runner_id
        if model_ref is not UNSET:
            field_dict["model_ref"] = model_ref

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        job_id = d.pop("job_id")

        model = d.pop("model")

        status = d.pop("status")

        def _parse_runner_id(data: object) -> None | str | Unset:
            if data is None:
                return data
            if isinstance(data, Unset):
                return data
            return cast(None | str | Unset, data)

        runner_id = _parse_runner_id(d.pop("runner_id", UNSET))

        def _parse_model_ref(data: object) -> None | str | Unset:
            if data is None:
                return data
            if isinstance(data, Unset):
                return data
            return cast(None | str | Unset, data)

        model_ref = _parse_model_ref(d.pop("model_ref", UNSET))

        runner_pull_start_response_data = cls(
            job_id=job_id,
            model=model,
            status=status,
            runner_id=runner_id,
            model_ref=model_ref,
        )

        runner_pull_start_response_data.additional_properties = d
        return runner_pull_start_response_data

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
