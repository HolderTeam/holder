from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar, cast

from attrs import define as _attrs_define
from typing_extensions import Self

T = TypeVar("T", bound="SyncState")


@_attrs_define
class SyncState:
    """What this device last recorded about a project's sync: its last pull and push, what is waiting to go, and
    when it will retry.

        Attributes:
            project_id (str):
            last_commit_at (int | None): When the project's repository last had a commit; null when unknown.
            last_push_at (int | None): When the project was last pushed; null when never.
            last_pull_at (int | None): When the project was last pulled; null when never.
            uncommitted_changes_count (int): Files changed and not yet committed.
            unpushed_commits_count (int): Commits not yet on the remote.
            last_push_status (None | str): How the last push ended, as a PushStatus; null when never.
            last_pull_status (None | str): How the last pull ended; null when never.
            last_sync_error (None | str): The last failure's message; null when the last sync succeeded.
            last_sync_error_at (int | None): When the last failure happened; null when none is recorded.
            retry_count (int): Failed pushes in a row.
            next_retry_at (int | None): When a push will next be tried; null when none is waiting.
            pull_retry_count (int): Failed pulls in a row.
            next_pull_retry_at (int | None): When a pull will next be tried; null when none is waiting.
            updated_at (int | None): When this state was last recorded; null when nothing has been.
    """

    project_id: str
    last_commit_at: int | None
    last_push_at: int | None
    last_pull_at: int | None
    uncommitted_changes_count: int
    unpushed_commits_count: int
    last_push_status: None | str
    last_pull_status: None | str
    last_sync_error: None | str
    last_sync_error_at: int | None
    retry_count: int
    next_retry_at: int | None
    pull_retry_count: int
    next_pull_retry_at: int | None
    updated_at: int | None

    def to_dict(self) -> dict[str, Any]:
        project_id = self.project_id

        last_commit_at: int | None
        last_commit_at = self.last_commit_at

        last_push_at: int | None
        last_push_at = self.last_push_at

        last_pull_at: int | None
        last_pull_at = self.last_pull_at

        uncommitted_changes_count = self.uncommitted_changes_count

        unpushed_commits_count = self.unpushed_commits_count

        last_push_status: None | str
        last_push_status = self.last_push_status

        last_pull_status: None | str
        last_pull_status = self.last_pull_status

        last_sync_error: None | str
        last_sync_error = self.last_sync_error

        last_sync_error_at: int | None
        last_sync_error_at = self.last_sync_error_at

        retry_count = self.retry_count

        next_retry_at: int | None
        next_retry_at = self.next_retry_at

        pull_retry_count = self.pull_retry_count

        next_pull_retry_at: int | None
        next_pull_retry_at = self.next_pull_retry_at

        updated_at: int | None
        updated_at = self.updated_at

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "project_id": project_id,
                "last_commit_at": last_commit_at,
                "last_push_at": last_push_at,
                "last_pull_at": last_pull_at,
                "uncommitted_changes_count": uncommitted_changes_count,
                "unpushed_commits_count": unpushed_commits_count,
                "last_push_status": last_push_status,
                "last_pull_status": last_pull_status,
                "last_sync_error": last_sync_error,
                "last_sync_error_at": last_sync_error_at,
                "retry_count": retry_count,
                "next_retry_at": next_retry_at,
                "pull_retry_count": pull_retry_count,
                "next_pull_retry_at": next_pull_retry_at,
                "updated_at": updated_at,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        project_id = d.pop("project_id")

        def _parse_last_commit_at(data: object) -> int | None:
            if data is None:
                return data
            return cast(int | None, data)

        last_commit_at = _parse_last_commit_at(d.pop("last_commit_at"))

        def _parse_last_push_at(data: object) -> int | None:
            if data is None:
                return data
            return cast(int | None, data)

        last_push_at = _parse_last_push_at(d.pop("last_push_at"))

        def _parse_last_pull_at(data: object) -> int | None:
            if data is None:
                return data
            return cast(int | None, data)

        last_pull_at = _parse_last_pull_at(d.pop("last_pull_at"))

        uncommitted_changes_count = d.pop("uncommitted_changes_count")

        unpushed_commits_count = d.pop("unpushed_commits_count")

        def _parse_last_push_status(data: object) -> None | str:
            if data is None:
                return data
            return cast(None | str, data)

        last_push_status = _parse_last_push_status(d.pop("last_push_status"))

        def _parse_last_pull_status(data: object) -> None | str:
            if data is None:
                return data
            return cast(None | str, data)

        last_pull_status = _parse_last_pull_status(d.pop("last_pull_status"))

        def _parse_last_sync_error(data: object) -> None | str:
            if data is None:
                return data
            return cast(None | str, data)

        last_sync_error = _parse_last_sync_error(d.pop("last_sync_error"))

        def _parse_last_sync_error_at(data: object) -> int | None:
            if data is None:
                return data
            return cast(int | None, data)

        last_sync_error_at = _parse_last_sync_error_at(d.pop("last_sync_error_at"))

        retry_count = d.pop("retry_count")

        def _parse_next_retry_at(data: object) -> int | None:
            if data is None:
                return data
            return cast(int | None, data)

        next_retry_at = _parse_next_retry_at(d.pop("next_retry_at"))

        pull_retry_count = d.pop("pull_retry_count")

        def _parse_next_pull_retry_at(data: object) -> int | None:
            if data is None:
                return data
            return cast(int | None, data)

        next_pull_retry_at = _parse_next_pull_retry_at(d.pop("next_pull_retry_at"))

        def _parse_updated_at(data: object) -> int | None:
            if data is None:
                return data
            return cast(int | None, data)

        updated_at = _parse_updated_at(d.pop("updated_at"))

        sync_state = cls(
            project_id=project_id,
            last_commit_at=last_commit_at,
            last_push_at=last_push_at,
            last_pull_at=last_pull_at,
            uncommitted_changes_count=uncommitted_changes_count,
            unpushed_commits_count=unpushed_commits_count,
            last_push_status=last_push_status,
            last_pull_status=last_pull_status,
            last_sync_error=last_sync_error,
            last_sync_error_at=last_sync_error_at,
            retry_count=retry_count,
            next_retry_at=next_retry_at,
            pull_retry_count=pull_retry_count,
            next_pull_retry_at=next_pull_retry_at,
            updated_at=updated_at,
        )

        return sync_state
