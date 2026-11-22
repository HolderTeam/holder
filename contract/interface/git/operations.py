"""Git operations."""

from typing import Annotated

from fastapi import Body, Path, Query

from authoring import Contract, Int32

from ..cards.models import Card, CardId, CardMetadata
from ..errors import Conflict, DataCorrupted, KeyMissing, KeyringUnavailable, NotFound
from ..projects.ids import ProjectId
from .models import (
    ConflictComparison,
    ConflictResolution,
    ConflictResolutionResult,
    DueSyncResult,
    PushOptions,
    RemoteTest,
    RemoteTestResult,
    SeparateConflictCopyRequest,
    SyncResult,
    SyncState,
    TrashConflictCopyRequest,
)


def register(contract: Contract) -> None:
    @contract.route("GET", "/projects/{project_id}/git/sync-status", errors=[NotFound])
    def get_project_sync_state(project_id: Annotated[ProjectId, Path()]) -> SyncState:
        """What this device last recorded about a project's sync."""

    @contract.route("GET", "/projects/{project_id}/git/conflict-copies/{copy_id}/comparison",
                    errors=[NotFound, Conflict, KeyringUnavailable, KeyMissing, DataCorrupted])
    def compare_conflict_copy(
        project_id: Annotated[ProjectId, Path()],
        copy_id: Annotated[CardId, Path()],
    ) -> ConflictComparison:
        """Compare a preserved copy with its recorded original.

        Reads both saved cards together. A missing association, an original or copy in Trash, or a card
        that no longer exists is NotFound. A pair changed during the read is Conflict. The operation does not
        change either card.
        """

    @contract.route("POST", "/projects/{project_id}/git/conflict-copies/{copy_id}/resolution",
                    errors=[NotFound, Conflict, KeyringUnavailable, KeyMissing, DataCorrupted])
    def resolve_conflict_copy(
        project_id: Annotated[ProjectId, Path()],
        copy_id: Annotated[CardId, Path()],
        resolution: Annotated[ConflictResolution, Body()],
    ) -> ConflictResolutionResult:
        """Save reviewed text to the original card without changing the preserved copy.

        Holder verifies the token against both live cards before changing anything. A stale token is
        Conflict. A missing association or a card that is missing or in Trash is NotFound. Changes
        to the title and content make a new Git commit. The original's identity, hierarchy, links and
        milestones are retained. Saving the existing title and content makes no commit. The result
        includes a review for optional cleanup of the preserved copy.
        """

    @contract.route("POST", "/projects/{project_id}/git/conflict-copies/{copy_id}/separation",
                    errors=[NotFound, Conflict, KeyringUnavailable, KeyMissing, DataCorrupted])
    def separate_conflict_copy(
        project_id: Annotated[ProjectId, Path()],
        copy_id: Annotated[CardId, Path()],
        request: Annotated[SeparateConflictCopyRequest, Body()],
    ) -> Card:
        """Keep a preserved copy as an ordinary card instead of resolving it into the original.

        Holder verifies the token against both live cards before changing anything. A stale token is
        Conflict. A missing association or a card that is missing or in Trash is NotFound. The copy takes
        the new title and loses its recorded association with the original, in one Git commit, so it is
        no longer listed as a conflict copy. Both cards keep their content, place, links and milestones.
        """

    @contract.route("POST", "/projects/{project_id}/git/conflict-copies/{copy_id}/trash",
                    errors=[NotFound, Conflict, KeyringUnavailable, KeyMissing, DataCorrupted])
    def trash_conflict_copy(
        project_id: Annotated[ProjectId, Path()],
        copy_id: Annotated[CardId, Path()],
        confirmation: Annotated[TrashConflictCopyRequest, Body()],
    ) -> CardMetadata:
        """Move a reviewed conflict copy to Trash after choosing a result.

        Holder verifies the cleanup token against both live cards and the reviewed effects.
        A stale token or cleanup that would move the original is Conflict. A missing association
        or a card missing or in Trash is NotFound.
        The original is untouched. The copy can be restored and its live children move up a level.
        """

    @contract.route("POST", "/projects/{project_id}/git/pull",
                    errors=[NotFound, KeyringUnavailable, KeyMissing, DataCorrupted])
    def pull_project(project_id: Annotated[ProjectId, Path()]) -> SyncResult:
        """Pull a project's checked-out branch from its Git remote.

        Changes fast-forward, or merge card by card where both sides changed, and the project's index is updated
        before the result returns. A Git failure is an outcome in the result, recorded in the sync state. Git work on
        one project runs one at a time.
        """


    @contract.route("POST", "/projects/{project_id}/git/push",
                    errors=[NotFound, KeyringUnavailable, KeyMissing, DataCorrupted])
    def push_project(project_id: Annotated[ProjectId, Path()], options: Annotated[PushOptions, Body()]) -> SyncResult:
        """Push a project to its Git remote.

        An encrypted project is checked first, so no plaintext card leaves the device: one whose files are not safe
        to push is DataCorrupted, and nothing is pushed. A Git failure is an outcome in the result, recorded in the
        sync state.
        """


    @contract.route("POST", "/projects/{project_id}/git/sync",
                    errors=[NotFound, KeyringUnavailable, KeyMissing, DataCorrupted])
    def sync_project(project_id: Annotated[ProjectId, Path()], options: Annotated[PushOptions, Body()]) -> SyncResult:
        """Pull a project, then push it.

        The push is tried only when the pull succeeds. Otherwise as pull_project and push_project.
        """


    @contract.route("POST", "/projects/{project_id}/git/sync-if-due",
                    errors=[NotFound, KeyringUnavailable, KeyMissing, DataCorrupted])
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
