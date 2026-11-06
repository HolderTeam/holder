from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar

from attrs import define as _attrs_define
from typing_extensions import Self

T = TypeVar("T", bound="ProjectRecoveryTokenExportResult")


@_attrs_define
class ProjectRecoveryTokenExportResult:
    """A portable encrypted project key.

    Attributes:
        project_id (str):
        key_id (str): The encrypted project's key ID.
        recovery_token (str): Serialized .hrk token JSON.
    """

    project_id: str
    key_id: str
    recovery_token: str

    def to_dict(self) -> dict[str, Any]:
        project_id = self.project_id

        key_id = self.key_id

        recovery_token = self.recovery_token

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "project_id": project_id,
                "key_id": key_id,
                "recovery_token": recovery_token,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        project_id = d.pop("project_id")

        key_id = d.pop("key_id")

        recovery_token = d.pop("recovery_token")

        project_recovery_token_export_result = cls(
            project_id=project_id,
            key_id=key_id,
            recovery_token=recovery_token,
        )

        return project_recovery_token_export_result
