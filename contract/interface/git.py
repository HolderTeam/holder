"""Git sync: a project's repository and its Git remote, and what this device last did with them."""

from typing import Annotated, Literal

from authoring import MISSING, Field, Int32, Record, Timestamp

from .cards import ProjectId
from .projects import GitRemoteUrl

type PushStatus = Literal["not_attempted", "pushed", "up_to_date", "auth_failed", "not_found", "network_error",
                          "non_fast_forward", "remote_unset", "unknown_error"]
"""What a push did: nothing, pushed, found the remote already had everything, or failed for one of these reasons.
non_fast_forward means the remote has changes to pull first."""


class SyncState(Record):
    """What this device last recorded about a project's sync: its last pull and push, what is waiting to go, and
    when it will retry."""

    project_id: ProjectId
    """The project."""
    last_commit_at: Timestamp | None
    """When the project's repository last had a commit; null when unknown."""
    last_push_at: Timestamp | None
    """When the project was last pushed; null when never."""
    last_pull_at: Timestamp | None
    """When the project was last pulled; null when never."""
    uncommitted_changes_count: Int32
    """Files changed and not yet committed."""
    unpushed_commits_count: Int32
    """Commits not yet on the remote."""
    last_push_status: str | None
    """How the last push ended, as a PushStatus; null when never."""
    last_pull_status: str | None
    """How the last pull ended; null when never."""
    last_sync_error: str | None
    """The last failure's message; null when the last sync succeeded."""
    last_sync_error_at: Timestamp | None
    """When the last failure happened; null when none is recorded."""
    retry_count: Int32
    """Failed pushes in a row."""
    next_retry_at: Timestamp | None
    """When a push will next be tried; null when none is waiting."""
    pull_retry_count: Int32
    """Failed pulls in a row."""
    next_pull_retry_at: Timestamp | None
    """When a pull will next be tried; null when none is waiting."""
    updated_at: Timestamp | None
    """When this state was last recorded; null when nothing has been."""


class PullPhase(Record):
    """What a pull did."""

    attempted: bool
    """Whether a pull was tried."""
    status: Literal["not_attempted", "succeeded", "remote_unset", "failed"]
    """How the pull ended."""
    conflicts_resolved: Int32
    """Cards changed both here and on the remote, merged by Holder's card rules."""
    hidden_cards_repaired: Int32
    """Live cards the pull left under a missing or trashed parent, made reachable again."""
    error_message: str | None
    """Why the pull failed; null when it did not."""


class PushPhase(Record):
    """What a push did."""

    attempted: bool
    """Whether a push was tried."""
    status: PushStatus
    """How the push ended."""
    ahead_count: Int32
    """Commits here that the remote does not have."""
    behind_count: Int32
    """Commits on the remote that are not here."""
    local_head_commit: str | None
    """The commit this device's branch is at; null when unknown."""
    error_message: str | None
    """Why the push failed; null when it did not."""


class SyncResult(Record):
    """What a pull, a push or a sync did, as recorded in the project's sync state. A Git failure is an outcome
    here, not an error."""

    project_id: ProjectId
    """The project."""
    status: Literal["succeeded", "failed"]
    """Whether every phase tried succeeded."""
    remote_url: GitRemoteUrl | None
    """The Git remote used; null when the project has none."""
    pull: PullPhase
    """The pull; not attempted for a push."""
    push: PushPhase
    """The push; not attempted for a pull, or after a pull that failed."""


class DueSyncResult(Record):
    """A scheduled sync's pull and push outcomes. Both phases are not attempted when neither is due or the
    project has no remote. A due push may still run after a failed pull so local changes get another chance."""

    project_id: ProjectId
    """The project."""
    remote_url: GitRemoteUrl | None
    """The configured remote; null when there is none."""
    pull: PullPhase
    """The pull, or not attempted when it is not due."""
    push: PushPhase
    """The push, or not attempted when it is not due."""


class PushOptions(Record):
    """How to push."""

    branch: Annotated[str, Field(min_length=1)] | MISSING = MISSING
    """The branch to push; omit for the repository's default."""
    set_upstream: bool = True
    """Whether the remote branch becomes the local branch's upstream."""


class RemoteTest(Record):
    """A remote to test."""

    remote_url: GitRemoteUrl | MISSING = MISSING
    """The Git remote to test; omit for the project's own."""


class RemoteTestResult(Record):
    """Whether a Git remote can be reached with this device's credentials."""

    project_id: ProjectId
    """The project."""
    status: Literal["reachable", "auth_failed", "not_found", "network_error", "invalid_remote_url", "remote_unset",
                    "unknown_error"]
    """What testing the remote found; remote_unset when there was none to test."""
    remote_url: GitRemoteUrl | None
    """The remote tested; null when there was none."""
    remote_has_head: bool
    """Whether the remote has any commits."""
    error_message: str | None
    """Why the remote could not be reached; null when it could."""
