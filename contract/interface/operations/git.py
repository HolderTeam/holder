"""Public git operations."""

from typing import Annotated

from fastapi import Body, Path, Query

from authoring import Contract, Int32

from ..cards import ProjectId
from ..errors import NotFound
from ..git import DueSyncResult, PushOptions, RemoteTest, RemoteTestResult, SyncResult, SyncState
from .common import KEYS


def register(contract: Contract) -> None:
    @contract.route("GET", "/projects/{project_id}/git/sync-status", errors=[NotFound])
    def get_project_sync_state(project_id: Annotated[ProjectId, Path()]) -> SyncState:
        """What this device last recorded about a project's sync."""


    @contract.route("POST", "/projects/{project_id}/git/pull", errors=[NotFound, *KEYS])
    def pull_project(project_id: Annotated[ProjectId, Path()]) -> SyncResult:
        """Pull a project's checked-out branch from its Git remote.

        Changes fast-forward, or merge card by card where both sides changed, and the project's index is updated
        before the result returns. A Git failure is an outcome in the result, recorded in the sync state. Git work on
        one project runs one at a time.
        """


    @contract.route("POST", "/projects/{project_id}/git/push", errors=[NotFound, *KEYS])
    def push_project(project_id: Annotated[ProjectId, Path()], options: Annotated[PushOptions, Body()]) -> SyncResult:
        """Push a project to its Git remote.

        An encrypted project is checked first, so no plaintext card leaves the device: one whose files are not safe
        to push is DataCorrupted, and nothing is pushed. A Git failure is an outcome in the result, recorded in the
        sync state.
        """


    @contract.route("POST", "/projects/{project_id}/git/sync", errors=[NotFound, *KEYS])
    def sync_project(project_id: Annotated[ProjectId, Path()], options: Annotated[PushOptions, Body()]) -> SyncResult:
        """Pull a project, then push it.

        The push is tried only when the pull succeeds. Otherwise as pull_project and push_project.
        """


    @contract.route("POST", "/projects/{project_id}/git/sync-if-due", errors=[NotFound, *KEYS])
    def sync_project_if_due(
        project_id: Annotated[ProjectId, Path()],
        push_interval_seconds: Annotated[Int32, Query(ge=0)] = 0,
        pull_interval_seconds: Annotated[Int32, Query(ge=0)] = 0,
    ) -> DueSyncResult:
        """Try the project's pull and push phases only when their cadence and retry policy say they are due.

        Zero uses the built-in intervals (20 minutes for push, 5 minutes for pull). A project without a remote and
        a project with nothing due return both phases as not attempted without changing sync state. A due push can
        run after a failed pull; this keeps scheduled pushes independent from the pull retry schedule.
        Git failures are phase outcomes and are recorded in the project's sync state.
        """


    @contract.route("POST", "/projects/{project_id}/git/test-remote", errors=[NotFound])
    def test_project_remote(project_id: Annotated[ProjectId, Path()],
                            remote: Annotated[RemoteTest, Body()]) -> RemoteTestResult:
        """Test whether a Git remote can be reached, with this device's credentials.

        Lists the remote's refs and nothing more: nothing is fetched, and neither the project nor its repository
        changes.
        """
