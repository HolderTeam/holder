"""Git sync: a project's repository and its Git remote, and what this device last did with them."""

from typing import Annotated, Literal

from authoring import MISSING, Field, Int32, Record, Timestamp

from ..cards.models import Card, CardId, CardSummary
from ..links.models import Link
from ..milestones.models import Milestone
from ..projects.ids import ProjectId
from ..projects.models import GitRemoteUrl

type PushStatus = Literal["not_attempted", "pushed", "up_to_date", "auth_failed", "not_found", "network_error",
                          "non_fast_forward", "remote_unset", "unknown_error"]
"""What a push did: nothing, pushed, found the remote already had everything, or failed for one of these reasons.
non_fast_forward means the remote has changes to pull first."""


type ConflictVersionEditedOn = Literal["this_device", "another_device", "unknown"]
"""Which device made the latest change to one version of a conflict pair. Each device answers from its own Git log of
the commits made in its copy of the project, so the two devices give opposite answers for the same version. unknown
means this device's log cannot tell: the change is older than the log, as after a fresh clone."""


class ConflictCopy(Record):
    """A local card version preserved during a divergent pull."""

    original_card_id: CardId
    """The original card's identity, even if the card no longer exists."""
    original: CardSummary | None
    """The original's current title and Trash status, or null if it no longer exists."""
    copy_card: CardSummary
    """The preserved card's current identity and title."""


class ConflictChangedLine(Record):
    """A highlighted line in a conflict comparison."""

    line: Annotated[Int32, Field(ge=1)]
    """The one-based line number within this card's content."""


class ConflictComparison(Record):
    """The two live cards in a recorded conflict pair, read together for review."""

    original: Card
    """The current state of the original card."""
    copy_card: Card
    """The current state of the preserved copy."""
    original_changed_lines: list[ConflictChangedLine]
    """One-based lines of the original's content that differ from the copy."""
    copy_changed_lines: list[ConflictChangedLine]
    """One-based lines of the copy's content that differ from the original."""
    truncated: bool
    """Whether the bounded diff could only highlight part of the change."""
    original_edited_on: ConflictVersionEditedOn
    """Which device made the latest change to the original."""
    copy_edited_on: ConflictVersionEditedOn
    """Which device made the latest change to the preserved copy."""
    review_token: str
    """An opaque token for both reviewed versions, required when saving a resolution."""


class ConflictResolution(Record):
    """Title and body to keep on the original card."""

    review_token: Annotated[str, Field(min_length=1, max_length=256)]
    """The token returned with the reviewed comparison."""
    title: Annotated[str, Field(min_length=1)]
    """The replacement title."""
    content: str
    """The replacement card content, which can be empty."""


class ConflictCopyCleanupReview(Record):
    """What stays on the preserved copy or is affected if the copy moves to Trash."""

    cleanup_token: str
    """An opaque token for the saved result, preserved copy, and the reviewed cleanup effects."""
    copy_links: list[Link]
    """Outgoing links and attachments without an equivalent on the original."""
    copy_milestones: list[Milestone]
    """Milestones without an equivalent kind, description and dates on the original."""
    backlinks: list[Link]
    """Live links on this device that point to the copy itself."""
    children: list[CardSummary]
    """Live children that would move up a level using the normal Trash behavior."""
    promotion_parent: CardSummary | None
    """Where children of the copy would move, or null for the project root."""
    placement_differs: bool
    """Whether the parent or position of the copy differs from that of the original."""
    preserves_original: bool
    """Whether moving the copy to Trash can leave the original's placement untouched."""
    copy_tags: list[str]
    """Tags present on the copy and absent from the saved original."""


class ConflictResolutionResult(Record):
    """The durably saved original and a review for optional cleanup."""

    card: Card
    """The original after saving."""
    cleanup: ConflictCopyCleanupReview
    """The copy and effects to review before moving it to Trash."""


class TrashConflictCopyRequest(Record):
    """Confirmation of the cleanup effects returned when the resolution was saved."""

    cleanup_token: Annotated[str, Field(min_length=1, max_length=256)]
    """The token returned with the cleanup review."""


class PullChanges(Record):
    """What a pull changed on this device to bring two devices' versions of the same cards together. Each card
    change is its own commit, so card history can undo it."""

    merged_card_ids: list[str]
    """Cards changed on both devices in different parts (title, place, text, links or milestones), now holding
    both changes."""
    kept_out_of_trash_card_ids: list[str]
    """Cards moved to Trash on this device while the other device edited them. They stay out of Trash with
    that edit and can be moved to Trash again."""
    restored_from_trash_card_ids: list[str]
    """Cards edited on this device while the other device moved them to Trash. They are back out of Trash
    with this device's edit."""
    conflict_copy_ids: list[str]
    """New cards holding this device's version of a card whose changes clashed with the other device's. The
    original holds the other device's version."""
    hidden_cards_repaired: Int32
    """Live cards the pull left under a missing or trashed parent, made reachable again."""


class LastPullChanges(PullChanges):
    """The most recent pull that changed anything, kept until a later pull changes something. This device only;
    a rebuilt database starts without it."""

    pulled_at: Timestamp
    """When that pull happened."""


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
    last_pull_changes: LastPullChanges | None
    """What the most recent pull that changed anything did; null when no pull has changed anything here."""
    conflict_copies: list[ConflictCopy] | MISSING = MISSING
    """Live copies with a durable original association, newest first. Omitted when there are none."""


class PullPhase(Record):
    """What a pull did."""

    attempted: bool
    """Whether a pull was tried."""
    status: Literal["not_attempted", "succeeded", "remote_unset", "failed"]
    """How the pull ended."""
    conflicts_resolved: Int32
    """Local card versions actually preserved as new cards by this pull."""
    hidden_cards_repaired: Int32
    """Live cards the pull left under a missing or trashed parent, made reachable again."""
    changes: PullChanges
    """What the pull changed to bring the two devices' versions together; all empty when nothing, or when the
    pull failed."""
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
