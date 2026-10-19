from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar

from attrs import define as _attrs_define
from attrs import field as _attrs_field
from typing_extensions import Self

from ..types import UNSET, Unset

T = TypeVar("T", bound="RunnerPullStartResponseData")


@_attrs_define
class RunnerPullStartResponseData:
    """
    Attributes:
        job_id (str):
        model (str):
        status (str):
        runner_id (str | Unset):
        model_ref (str | Unset):
    """

    job_id: str
    model: str
    status: str
    runner_id: str | Unset = UNSET
    model_ref: str | Unset = UNSET
    additional_properties: dict[str, Any] = _attrs_field(init=False, factory=dict)

    def to_dict(self) -> dict[str, Any]:
        job_id = self.job_id

        model = self.model

        status = self.status

        runner_id = self.runner_id

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

        runner_id = d.pop("runner_id", UNSET)

        model_ref = d.pop("model_ref", UNSET)

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
