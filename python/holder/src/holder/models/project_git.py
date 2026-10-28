"""Project Git operations and their structured outcomes."""

from __future__ import annotations

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
    ProjectUpdate,
    PushOptions,
    RemoteTest,
    SyncState,
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
    remote_has_head: bool
    error_message: str | None

    @property
    def reachable(self) -> bool:
        return self.status == "reachable"


@dataclass(frozen=True)
class PullPhase:
    attempted: bool
    status: str
    conflicts_resolved: int
    hidden_cards_repaired: int
    error_message: str | None


@dataclass(frozen=True)
class PushPhase:
    attempted: bool
    status: str
    ahead_count: int
    behind_count: int
    local_head_commit: str | None
    error_message: str | None


@dataclass(frozen=True)
class SyncResult:
    """The outcome of a pull, a push or both; a phase not run is ``not_attempted``."""

    status: str
    remote_url: str | None
    pull: PullPhase
    push: PushPhase

    @property
    def succeeded(self) -> bool:
        return self.status == "succeeded"


class ProjectGit:
    """Live Git operations for one project, and its recorded sync state."""

    def __init__(self, project: Project) -> None:
        self._project = project

    def status(self) -> SyncState:
        """Fetch the current persisted sync state."""
        data = self._project._holder._call(
            get_projects_project_id_git_sync_status, self._project.id
        )
        _check_project(self._project, data)
        return data

    def test_remote(self, *, url: str | Unset = UNSET) -> RemoteCheck:
        """Probe a remote, the project's own unless url is given, without changing
        the project or fetching from it."""
        if not isinstance(url, Unset) and not isinstance(url, str):
            raise TypeError("url must be a string")
        data = self._project._holder._call(
            post_projects_project_id_git_test_remote,
            self._project.id,
            body=RemoteTest(remote_url=url),
        )
        _check_project(self._project, data)
        return RemoteCheck(
            str(data.status),
            _optional(data.remote_url),
            data.remote_has_head,
            _optional(data.error_message),
        )

    def set_remote(self, url: str | None) -> bool:
        """Configure a remote, or disconnect it with None. Return whether it changed."""
        if url is not None and not isinstance(url, str):
            raise TypeError("url must be a string or None")
        before = self._project.refresh().git_remote_url
        data = self._project._holder._call(
            patch_projects_project_id,
            self._project.id,
            body=ProjectUpdate(git_remote_url=url),
            write=True,
        )
        _check_project(self._project, data)
        self._project.refresh()
        return before != url

    def pull(self) -> SyncResult:
        """Pull and reconcile the checked-out branch."""
        data = self._project._holder._call(
            post_projects_project_id_git_pull, self._project.id, write=True
        )
        return self._sync_result(data)

    def push(
        self, *, branch: str | Unset = UNSET, set_upstream: bool = True
    ) -> SyncResult:
        """Push the project; inspect the result's push phase for Git failures."""
        self._check_push_options(branch, set_upstream)
        data = self._project._holder._call(
            post_projects_project_id_git_push,
            self._project.id,
            body=PushOptions(branch=branch, set_upstream=set_upstream),
            write=True,
        )
        return self._sync_result(data)

    def sync(
        self, *, branch: str | Unset = UNSET, set_upstream: bool = True
    ) -> SyncResult:
        """Pull and reconcile, then push only if pull succeeds."""
        self._check_push_options(branch, set_upstream)
        data = self._project._holder._call(
            post_projects_project_id_git_sync,
            self._project.id,
            body=PushOptions(branch=branch, set_upstream=set_upstream),
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
            _optional(data.remote_url),
            PullPhase(
                data.pull.attempted,
                str(data.pull.status),
                data.pull.conflicts_resolved,
                data.pull.hidden_cards_repaired,
                _optional(data.pull.error_message),
            ),
            PushPhase(
                data.push.attempted,
                str(data.push.status),
                data.push.ahead_count,
                data.push.behind_count,
                _optional(data.push.local_head_commit),
                _optional(data.push.error_message),
            ),
        )
