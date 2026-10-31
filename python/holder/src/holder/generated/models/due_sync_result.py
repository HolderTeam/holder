from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING, Any, TypeVar, cast

from attrs import define as _attrs_define
from typing_extensions import Self

if TYPE_CHECKING:
    from ..models.pull_phase import PullPhase
    from ..models.push_phase import PushPhase


T = TypeVar("T", bound="DueSyncResult")


@_attrs_define
class DueSyncResult:
    """A scheduled sync's pull and push outcomes. Both phases are not attempted when neither is due or the
    project has no remote. A due push may still run after a failed pull so local changes get another chance.

        Attributes:
            project_id (str):
            remote_url (None | str): The configured remote; null when there is none.
            pull (PullPhase): What a pull did.
            push (PushPhase): What a push did.
    """

    project_id: str
    remote_url: None | str
    pull: PullPhase
    push: PushPhase

    def to_dict(self) -> dict[str, Any]:
        project_id = self.project_id

        remote_url: None | str
        remote_url = self.remote_url

        pull = self.pull.to_dict()

        push = self.push.to_dict()

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "project_id": project_id,
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

        def _parse_remote_url(data: object) -> None | str:
            if data is None:
                return data
            return cast(None | str, data)

        remote_url = _parse_remote_url(d.pop("remote_url"))

        pull = PullPhase.from_dict(d.pop("pull"))

        push = PushPhase.from_dict(d.pop("push"))

        due_sync_result = cls(
            project_id=project_id,
            remote_url=remote_url,
            pull=pull,
            push=push,
        )

        return due_sync_result
