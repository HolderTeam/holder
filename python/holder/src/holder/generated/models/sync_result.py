from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING, Any, TypeVar, cast

from attrs import define as _attrs_define
from typing_extensions import Self

from ..models.sync_result_status import SyncResultStatus

if TYPE_CHECKING:
    from ..models.pull_phase import PullPhase
    from ..models.push_phase import PushPhase


T = TypeVar("T", bound="SyncResult")


@_attrs_define
class SyncResult:
    """What a pull, a push or a sync did, as recorded in the project's sync state. A Git failure is an outcome
    here, not an error.

        Attributes:
            project_id (str):
            status (SyncResultStatus): Whether every phase tried succeeded.
            remote_url (None | str): The Git remote used; null when the project has none.
            pull (PullPhase): What a pull did.
            push (PushPhase): What a push did.
    """

    project_id: str
    status: SyncResultStatus
    remote_url: None | str
    pull: PullPhase
    push: PushPhase

    def to_dict(self) -> dict[str, Any]:
        project_id = self.project_id

        status = self.status.value

        remote_url: None | str
        remote_url = self.remote_url

        pull = self.pull.to_dict()

        push = self.push.to_dict()

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "project_id": project_id,
                "status": status,
                "remote_url": remote_url,
                "pull": pull,
                "push": push,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        from ..models.pull_phase import PullPhase
        from ..models.push_phase import PushPhase

        d = dict(src_dict)
        project_id = d.pop("project_id")

        status = SyncResultStatus(d.pop("status"))

        def _parse_remote_url(data: object) -> None | str:
            if data is None:
                return data
            return cast(None | str, data)

        remote_url = _parse_remote_url(d.pop("remote_url"))

        pull = PullPhase.from_dict(d.pop("pull"))

        push = PushPhase.from_dict(d.pop("push"))

        sync_result = cls(
            project_id=project_id,
            status=status,
            remote_url=remote_url,
            pull=pull,
            push=push,
        )

        return sync_result
