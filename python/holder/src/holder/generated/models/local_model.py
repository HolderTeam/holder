from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar

from attrs import define as _attrs_define
from attrs import field as _attrs_field
from typing_extensions import Self

from ..types import UNSET, Unset

T = TypeVar("T", bound="LocalModel")


@_attrs_define
class LocalModel:
    """
    Attributes:
        name (str):
        runner_id (str | Unset): The runner that has this model.
        model_ref (str | Unset): The runner-qualified model reference to request it by.
        digest (str | Unset):
        size (int | Unset):
        modified_at (str | Unset):
    """

    name: str
    runner_id: str | Unset = UNSET
    model_ref: str | Unset = UNSET
    digest: str | Unset = UNSET
    size: int | Unset = UNSET
    modified_at: str | Unset = UNSET
    additional_properties: dict[str, Any] = _attrs_field(init=False, factory=dict)

    def to_dict(self) -> dict[str, Any]:
        name = self.name

        runner_id = self.runner_id

        model_ref = self.model_ref

        digest = self.digest

        size = self.size

        modified_at = self.modified_at

        field_dict: dict[str, Any] = {}
        field_dict.update(self.additional_properties)
        field_dict.update(
            {
                "name": name,
            }
        )
        if runner_id is not UNSET:
            field_dict["runner_id"] = runner_id
        if model_ref is not UNSET:
            field_dict["model_ref"] = model_ref
        if digest is not UNSET:
            field_dict["digest"] = digest
        if size is not UNSET:
            field_dict["size"] = size
        if modified_at is not UNSET:
            field_dict["modified_at"] = modified_at

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        name = d.pop("name")

        runner_id = d.pop("runner_id", UNSET)

        model_ref = d.pop("model_ref", UNSET)

        digest = d.pop("digest", UNSET)

        size = d.pop("size", UNSET)

        modified_at = d.pop("modified_at", UNSET)

        local_model = cls(
            name=name,
            runner_id=runner_id,
            model_ref=model_ref,
            digest=digest,
            size=size,
            modified_at=modified_at,
        )

        local_model.additional_properties = d
        return local_model

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
