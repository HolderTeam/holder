from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar, cast

from attrs import define as _attrs_define
from typing_extensions import Self

from ..models.push_status import PushStatus

T = TypeVar("T", bound="PushPhase")


@_attrs_define
class PushPhase:
    """What a push did.

    Attributes:
        attempted (bool): Whether a push was tried.
        status (PushStatus):
        ahead_count (int): Commits here that the remote does not have.
        behind_count (int): Commits on the remote that are not here.
        local_head_commit (None | str): The commit this device's branch is at; null when unknown.
        error_message (None | str): Why the push failed; null when it did not.
    """

    attempted: bool
    status: PushStatus
    ahead_count: int
    behind_count: int
    local_head_commit: None | str
    error_message: None | str

    def to_dict(self) -> dict[str, Any]:
        attempted = self.attempted

        status = self.status.value

        ahead_count = self.ahead_count

        behind_count = self.behind_count

        local_head_commit: None | str
        local_head_commit = self.local_head_commit

        error_message: None | str
        error_message = self.error_message

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "attempted": attempted,
                "status": status,
                "ahead_count": ahead_count,
                "behind_count": behind_count,
                "local_head_commit": local_head_commit,
                "error_message": error_message,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        attempted = d.pop("attempted")

        status = PushStatus(d.pop("status"))

        ahead_count = d.pop("ahead_count")

        behind_count = d.pop("behind_count")

        def _parse_local_head_commit(data: object) -> None | str:
            if data is None:
                return data
            return cast(None | str, data)

        local_head_commit = _parse_local_head_commit(d.pop("local_head_commit"))

        def _parse_error_message(data: object) -> None | str:
            if data is None:
                return data
            return cast(None | str, data)

        error_message = _parse_error_message(d.pop("error_message"))

        push_phase = cls(
            attempted=attempted,
            status=status,
            ahead_count=ahead_count,
            behind_count=behind_count,
            local_head_commit=local_head_commit,
            error_message=error_message,
        )

        return push_phase
