from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar, cast

from attrs import define as _attrs_define
from typing_extensions import Self

from ..models.recovery_token_global_import_result_pull_status import (
    RecoveryTokenGlobalImportResultPullStatus,
)

T = TypeVar("T", bound="RecoveryTokenGlobalImportResult")


@_attrs_define
class RecoveryTokenGlobalImportResult:
    """The result of restoring a token to this device.

    Attributes:
        project_id (str):
        project_created (bool): Whether this device made the project from the token.
        remote_hint_present (bool): Whether the token names a Git remote.
        remote_configured (bool): Whether that remote was configured.
        remote_error (None | str): Why remote configuration failed, or null.
        pull_status (RecoveryTokenGlobalImportResultPullStatus): Outcome of the attempted pull.
        pull_error (None | str): Why pulling failed, or null.
    """

    project_id: str
    project_created: bool
    remote_hint_present: bool
    remote_configured: bool
    remote_error: None | str
    pull_status: RecoveryTokenGlobalImportResultPullStatus
    pull_error: None | str

    def to_dict(self) -> dict[str, Any]:
        project_id = self.project_id

        project_created = self.project_created

        remote_hint_present = self.remote_hint_present

        remote_configured = self.remote_configured

        remote_error: None | str
        remote_error = self.remote_error

        pull_status = self.pull_status.value

        pull_error: None | str
        pull_error = self.pull_error

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "project_id": project_id,
                "project_created": project_created,
                "remote_hint_present": remote_hint_present,
                "remote_configured": remote_configured,
                "remote_error": remote_error,
                "pull_status": pull_status,
                "pull_error": pull_error,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        project_id = d.pop("project_id")

        project_created = d.pop("project_created")

        remote_hint_present = d.pop("remote_hint_present")

        remote_configured = d.pop("remote_configured")

        def _parse_remote_error(data: object) -> None | str:
            if data is None:
                return data
            return cast(None | str, data)

        remote_error = _parse_remote_error(d.pop("remote_error"))

        pull_status = RecoveryTokenGlobalImportResultPullStatus(d.pop("pull_status"))

        def _parse_pull_error(data: object) -> None | str:
            if data is None:
                return data
            return cast(None | str, data)

        pull_error = _parse_pull_error(d.pop("pull_error"))

        recovery_token_global_import_result = cls(
            project_id=project_id,
            project_created=project_created,
            remote_hint_present=remote_hint_present,
            remote_configured=remote_configured,
            remote_error=remote_error,
            pull_status=pull_status,
            pull_error=pull_error,
        )

        return recovery_token_global_import_result
