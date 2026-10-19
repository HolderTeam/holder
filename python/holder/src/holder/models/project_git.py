"""Project Git operations and their structured outcomes."""

from __future__ import annotations

import time
from dataclasses import dataclass
from typing import TYPE_CHECKING, Any

from ..exceptions import ProtocolError
from ..generated.api.default import (
    get_projects_project_id_git_sync_status,
    patch_projects_project_id,
    post_projects_project_id_git_pull,
    post_projects_project_id_git_push,
    post_projects_project_id_git_sync,
    post_projects_project_id_git_test_remote,
)
from ..generated.models import (
    ProjectGitPushRequest,
    ProjectGitSyncRequest,
    ProjectGitTestRemoteRequest,
    ProjectSync,
    ProjectUpdateRequest,
)
from ..generated.types import UNSET, Unset

if TYPE_CHECKING:
    from .project import Project


def _optional(value: Any) -> Any:
    return None if isinstance(value, Unset) else value


def _check_project(project: Project, data: Any) -> None:
    if data.project_id != project.id:
        raise ProtocolError("Project operation returned a different project")


@dataclass(frozen=True)
class RemoteCheck:
    status: str
    remote_url: str | None
    branch: str
    remote_has_head: bool
    error_code: str | None
    error_message: str | None

    @property
    def reachable(self) -> bool:
        return self.status == "reachable"


@dataclass(frozen=True)
class PushResult:
    status: str
    branch: str
    remote_url: str | None
    ahead_count: int
    behind_count: int
    local_head_commit: str | None
    error_code: str | None
    error_message: str | None
    next_action: str | None

    @property
    def succeeded(self) -> bool:
        return self.status in ("pushed", "up_to_date")


@dataclass(frozen=True)
class PullPhase:
    attempted: bool
    status: str
    conflicts_resolved: int
    error_message: str | None


@dataclass(frozen=True)
class PushPhase:
    attempted: bool
    status: str | None
    ahead_count: int
    behind_count: int
    local_head_commit: str | None
    error_message: str | None


@dataclass(frozen=True)
class SyncResult:
    status: str
    branch: str
    remote_url: str | None
    error_code: str | None
    error_message: str | None
    pull: PullPhase
    push: PushPhase

    @property
    def succeeded(self) -> bool:
        return self.status == "succeeded"


class ProjectGit:
    """Live Git operations for one project. Project.sync is a fetched snapshot."""

    def __init__(self, project: Project) -> None:
        self._project = project

    def status(self) -> ProjectSync:
        """Fetch the current persisted sync state."""
        data = self._project._holder._call(
            get_projects_project_id_git_sync_status, self._project.id
        )
        _check_project(self._project, data)
        return data.sync

    def test_remote(
        self, *, url: str | None | Unset = UNSET, branch: str | Unset = UNSET
    ) -> RemoteCheck:
        """Probe a remote without changing the project or fetching from it."""
        if not isinstance(url, Unset) and url is not None and not isinstance(url, str):
            raise TypeError("url must be a string or None")
        if not isinstance(branch, Unset) and not isinstance(branch, str):
            raise TypeError("branch must be a string")
        data = self._project._holder._call(
            post_projects_project_id_git_test_remote,
            self._project.id,
            body=ProjectGitTestRemoteRequest(remote_url=url, branch=branch),
        )
        _check_project(self._project, data)
        return RemoteCheck(
            str(data.status),
            _optional(data.remote_url),
            data.branch,
            data.remote_has_head,
            _optional(data.error_code),
            _optional(data.error_message),
        )

    def set_remote(self, url: str | None) -> bool:
        """Configure a remote, or disconnect it with None. Return whether it changed."""
        if url is not None and not isinstance(url, str):
            raise TypeError("url must be a string or None")
        data = self._project._holder._call(
            patch_projects_project_id,
            self._project.id,
            body=ProjectUpdateRequest(updated_at=int(time.time()), git_remote_url=url),
            write=True,
        )
        _check_project(self._project, data)
        changed = _optional(data.git_remote_changed)
        if not isinstance(changed, bool):
            raise ProtocolError("Remote update omitted change status")
        self._project.refresh()
        return changed

    def pull(self) -> SyncResult:
        """Pull and reconcile the checked-out branch."""
        data = self._project._holder._call(
            post_projects_project_id_git_pull, self._project.id, write=True
        )
        return self._sync_result(data)

    def push(
        self, *, branch: str | Unset = UNSET, set_upstream: bool = True
    ) -> PushResult:
        """Push the project; inspect the result for Git failures."""
        self._check_push_options(branch, set_upstream)
        data = self._project._holder._call(
            post_projects_project_id_git_push,
            self._project.id,
            body=ProjectGitPushRequest(branch=branch, set_upstream=set_upstream),
            write=True,
        )
        _check_project(self._project, data)
        return PushResult(
            str(data.status),
            data.branch,
            _optional(data.remote_url),
            data.ahead_count,
            data.behind_count,
            _optional(data.local_head_commit),
            _optional(data.error_code),
            _optional(data.error_message),
            _optional(data.next_action),
        )

    def sync(
        self, *, branch: str | Unset = UNSET, set_upstream: bool = True
    ) -> SyncResult:
        """Pull and reconcile, then push only if pull succeeds."""
        self._check_push_options(branch, set_upstream)
        data = self._project._holder._call(
            post_projects_project_id_git_sync,
            self._project.id,
            body=ProjectGitSyncRequest(branch=branch, set_upstream=set_upstream),
            write=True,
        )
        return self._sync_result(data)

    @staticmethod
    def _check_push_options(branch: str | Unset, set_upstream: bool) -> None:
        if not isinstance(branch, Unset) and not isinstance(branch, str):
            raise TypeError("branch must be a string")
        if not isinstance(set_upstream, bool):
            raise TypeError("set_upstream must be a bool")

    def _sync_result(self, data: Any) -> SyncResult:
        _check_project(self._project, data)
        return SyncResult(
            str(data.status),
            data.branch,
            _optional(data.remote_url),
            data.error_code,
            data.error_message,
            PullPhase(
                data.pull.attempted,
                str(data.pull.status),
                data.pull.conflicts_resolved,
                data.pull.error_message,
            ),
            PushPhase(
                data.push.attempted,
                _optional(data.push.status),
                data.push.ahead_count,
                data.push.behind_count,
                data.push.local_head_commit,
                data.push.error_message,
            ),
        )
