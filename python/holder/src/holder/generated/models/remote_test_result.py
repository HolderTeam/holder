from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar, cast

from attrs import define as _attrs_define
from typing_extensions import Self

from ..models.remote_test_result_status import RemoteTestResultStatus

T = TypeVar("T", bound="RemoteTestResult")


@_attrs_define
class RemoteTestResult:
    """Whether a Git remote can be reached with this device's credentials.

    Attributes:
        project_id (str):
        status (RemoteTestResultStatus): What testing the remote found; remote_unset when there was none to test.
        remote_url (None | str): The remote tested; null when there was none.
        remote_has_head (bool): Whether the remote has any commits.
        error_message (None | str): Why the remote could not be reached; null when it could.
    """

    project_id: str
    status: RemoteTestResultStatus
    remote_url: None | str
    remote_has_head: bool
    error_message: None | str

    def to_dict(self) -> dict[str, Any]:
        project_id = self.project_id

        status = self.status.value

        remote_url: None | str
        remote_url = self.remote_url

        remote_has_head = self.remote_has_head

        error_message: None | str
        error_message = self.error_message

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "project_id": project_id,
                "status": status,
                "remote_url": remote_url,
                "remote_has_head": remote_has_head,
                "error_message": error_message,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        project_id = d.pop("project_id")

        status = RemoteTestResultStatus(d.pop("status"))

        def _parse_remote_url(data: object) -> None | str:
            if data is None:
                return data
            return cast(None | str, data)

        remote_url = _parse_remote_url(d.pop("remote_url"))

        remote_has_head = d.pop("remote_has_head")

        def _parse_error_message(data: object) -> None | str:
            if data is None:
                return data
            return cast(None | str, data)

        error_message = _parse_error_message(d.pop("error_message"))

        remote_test_result = cls(
            project_id=project_id,
            status=status,
            remote_url=remote_url,
            remote_has_head=remote_has_head,
            error_message=error_message,
        )

        return remote_test_result
