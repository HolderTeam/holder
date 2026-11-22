// Generated from contract/interface by compiler. Do not edit; run make -C contract update.
package team.holder.android.contract

/** Which device made the latest change to the original. */
enum class ConflictComparisonOriginalEditedOn(val value: String) {
    THIS_DEVICE("this_device"),
    ANOTHER_DEVICE("another_device"),
    UNKNOWN("unknown"),
}

/** Which device made the latest change to the preserved copy. */
enum class ConflictComparisonCopyEditedOn(val value: String) {
    THIS_DEVICE("this_device"),
    ANOTHER_DEVICE("another_device"),
    UNKNOWN("unknown"),
}

/** How the pull ended. */
enum class PullPhaseStatus(val value: String) {
    NOT_ATTEMPTED("not_attempted"),
    SUCCEEDED("succeeded"),
    REMOTE_UNSET("remote_unset"),
    FAILED("failed"),
}

/** How the push ended. */
enum class PushPhaseStatus(val value: String) {
    NOT_ATTEMPTED("not_attempted"),
    PUSHED("pushed"),
    UP_TO_DATE("up_to_date"),
    AUTH_FAILED("auth_failed"),
    NOT_FOUND("not_found"),
    NETWORK_ERROR("network_error"),
    NON_FAST_FORWARD("non_fast_forward"),
    REMOTE_UNSET("remote_unset"),
    UNKNOWN_ERROR("unknown_error"),
}

/** Whether every phase tried succeeded. */
enum class SyncResultStatus(val value: String) {
    SUCCEEDED("succeeded"),
    FAILED("failed"),
}

/** What testing the remote found; remote_unset when there was none to test. */
enum class RemoteTestResultStatus(val value: String) {
    REACHABLE("reachable"),
    AUTH_FAILED("auth_failed"),
    NOT_FOUND("not_found"),
    NETWORK_ERROR("network_error"),
    INVALID_REMOTE_URL("invalid_remote_url"),
    REMOTE_UNSET("remote_unset"),
    UNKNOWN_ERROR("unknown_error"),
}

/** What the link points at. */
enum class ConflictCopyLinkTargetTargetType(val value: String) {
    CARD("card"),
    AI_MESSAGE("ai_message"),
    AI_THREAD("ai_thread"),
    RESOURCE("resource"),
}

/**
 * The most recent pull that changed anything, kept until a later pull changes something. This device only;
 * a rebuilt database starts without it.
 */
data class LastPullChanges(
    /** Cards changed on both devices in different parts (title, place, text, links or milestones), now holding both changes. */
    val mergedCardIds: List<String>,
    /** Cards moved to Trash on this device while the other device edited them. They stay out of Trash with that edit and can be moved to Trash again. */
    val keptOutOfTrashCardIds: List<String>,
    /** Cards edited on this device while the other device moved them to Trash. They are back out of Trash with this device's edit. */
    val restoredFromTrashCardIds: List<String>,
    /** New cards holding this device's version of a card whose changes clashed with the other device's. The original holds the other device's version. */
    val conflictCopyIds: List<String>,
    /** Live cards the pull left under a missing or trashed parent, made reachable again. */
    val hiddenCardsRepaired: Int,
    /** When that pull happened. */
    val pulledAt: Long,
)

/** A local card version preserved during a divergent pull. */
data class ConflictCopy(
    /** The original card's identity, even if the card no longer exists. */
    val originalCardId: String,
    /** The original's current title and Trash status, or null if it no longer exists. */
    val original: CardSummary? = null,
    /** The preserved card's current identity and title. */
    val copyCard: CardSummary,
)

/**
 * What this device last recorded about a project's sync: its last pull and push, what is waiting to go, and
 * when it will retry.
 */
data class SyncState(
    /** The project. */
    val projectId: String,
    /** When the project's repository last had a commit; null when unknown. */
    val lastCommitAt: Long? = null,
    /** When the project was last pushed; null when never. */
    val lastPushAt: Long? = null,
    /** When the project was last pulled; null when never. */
    val lastPullAt: Long? = null,
    /** Files changed and not yet committed. */
    val uncommittedChangesCount: Int,
    /** Commits not yet on the remote. */
    val unpushedCommitsCount: Int,
    /** How the last push ended, as a PushStatus; null when never. */
    val lastPushStatus: String? = null,
    /** How the last pull ended; null when never. */
    val lastPullStatus: String? = null,
    /** The last failure's message; null when the last sync succeeded. */
    val lastSyncError: String? = null,
    /** When the last failure happened; null when none is recorded. */
    val lastSyncErrorAt: Long? = null,
    /** Failed pushes in a row. */
    val retryCount: Int,
    /** When a push will next be tried; null when none is waiting. */
    val nextRetryAt: Long? = null,
    /** Failed pulls in a row. */
    val pullRetryCount: Int,
    /** When a pull will next be tried; null when none is waiting. */
    val nextPullRetryAt: Long? = null,
    /** When this state was last recorded; null when nothing has been. */
    val updatedAt: Long? = null,
    /** What the most recent pull that changed anything did; null when no pull has changed anything here. */
    val lastPullChanges: LastPullChanges? = null,
    /** Live copies with a durable original association, newest first. Omitted when there are none. */
    val conflictCopies: List<ConflictCopy>? = null,
)

/** A highlighted line in a conflict comparison. */
data class ConflictChangedLine(
    /** The one-based line number within this card's content. */
    val line: Int,
)

/** The two live cards in a recorded conflict pair, read together for review. */
data class ConflictComparison(
    /** The current state of the original card. */
    val original: Card,
    /** The current state of the preserved copy. */
    val copyCard: Card,
    /** One-based lines of the original's content that differ from the copy. */
    val originalChangedLines: List<ConflictChangedLine>,
    /** One-based lines of the copy's content that differ from the original. */
    val copyChangedLines: List<ConflictChangedLine>,
    /** Whether the bounded diff could only highlight part of the change. */
    val truncated: Boolean,
    /** Which device made the latest change to the original. */
    val originalEditedOn: ConflictComparisonOriginalEditedOn,
    /** Which device made the latest change to the preserved copy. */
    val copyEditedOn: ConflictComparisonCopyEditedOn,
    /** An opaque token for both reviewed versions, required when saving a resolution. */
    val reviewToken: String,
)

/** What stays on the preserved copy or is affected if the copy moves to Trash. */
data class ConflictCopyCleanupReview(
    /** An opaque token for the saved result, preserved copy, and the reviewed cleanup effects. */
    val cleanupToken: String,
    /** Outgoing links and attachments without an equivalent on the original. */
    val copyLinks: List<Link>,
    /** Milestones without an equivalent kind, description and dates on the original. */
    val copyMilestones: List<Milestone>,
    /** Live links on this device that point to the copy itself. */
    val backlinks: List<Link>,
    /** Live children that would move up a level using the normal Trash behavior. */
    val children: List<CardSummary>,
    /** Where children of the copy would move, or null for the project root. */
    val promotionParent: CardSummary? = null,
    /** Whether the parent or position of the copy differs from that of the original. */
    val placementDiffers: Boolean,
    /** Whether moving the copy to Trash can leave the original's placement untouched. */
    val preservesOriginal: Boolean,
    /** Tags present on the copy and absent from the saved original. */
    val copyTags: List<String>,
)

/** The durably saved original and a review for optional cleanup. */
data class ConflictResolutionResult(
    /** The original after saving. */
    val card: Card,
    /** The copy and effects to review before moving it to Trash. */
    val cleanup: ConflictCopyCleanupReview,
)

/**
 * What a pull changed on this device to bring two devices' versions of the same cards together. Each card
 * change is its own commit, so card history can undo it.
 */
data class PullChanges(
    /** Cards changed on both devices in different parts (title, place, text, links or milestones), now holding both changes. */
    val mergedCardIds: List<String>,
    /** Cards moved to Trash on this device while the other device edited them. They stay out of Trash with that edit and can be moved to Trash again. */
    val keptOutOfTrashCardIds: List<String>,
    /** Cards edited on this device while the other device moved them to Trash. They are back out of Trash with this device's edit. */
    val restoredFromTrashCardIds: List<String>,
    /** New cards holding this device's version of a card whose changes clashed with the other device's. The original holds the other device's version. */
    val conflictCopyIds: List<String>,
    /** Live cards the pull left under a missing or trashed parent, made reachable again. */
    val hiddenCardsRepaired: Int,
)

/** What a pull did. */
data class PullPhase(
    /** Whether a pull was tried. */
    val attempted: Boolean,
    /** How the pull ended. */
    val status: PullPhaseStatus,
    /** Local card versions actually preserved as new cards by this pull. */
    val conflictsResolved: Int,
    /** Live cards the pull left under a missing or trashed parent, made reachable again. */
    val hiddenCardsRepaired: Int,
    /** What the pull changed to bring the two devices' versions together; all empty when nothing, or when the pull failed. */
    val changes: PullChanges,
    /** Why the pull failed; null when it did not. */
    val errorMessage: String? = null,
)

/** What a push did. */
data class PushPhase(
    /** Whether a push was tried. */
    val attempted: Boolean,
    /** How the push ended. */
    val status: PushPhaseStatus,
    /** Commits here that the remote does not have. */
    val aheadCount: Int,
    /** Commits on the remote that are not here. */
    val behindCount: Int,
    /** The commit this device's branch is at; null when unknown. */
    val localHeadCommit: String? = null,
    /** Why the push failed; null when it did not. */
    val errorMessage: String? = null,
)

/**
 * What a pull, a push or a sync did, as recorded in the project's sync state. A Git failure is an outcome
 * here, not an error.
 */
data class SyncResult(
    /** The project. */
    val projectId: String,
    /** Whether every phase tried succeeded. */
    val status: SyncResultStatus,
    /** The Git remote used; null when the project has none. */
    val remoteUrl: String? = null,
    /** The pull; not attempted for a push. */
    val pull: PullPhase,
    /** The push; not attempted for a pull, or after a pull that failed. */
    val push: PushPhase,
)

/**
 * A scheduled sync's pull and push outcomes. Both phases are not attempted when neither is due or the
 * project has no remote. A due push may still run after a failed pull so local changes get another chance.
 */
data class DueSyncResult(
    /** The project. */
    val projectId: String,
    /** The configured remote; null when there is none. */
    val remoteUrl: String? = null,
    /** The pull, or not attempted when it is not due. */
    val pull: PullPhase,
    /** The push, or not attempted when it is not due. */
    val push: PushPhase,
)

/** Whether a Git remote can be reached with this device's credentials. */
data class RemoteTestResult(
    /** The project. */
    val projectId: String,
    /** What testing the remote found; remote_unset when there was none to test. */
    val status: RemoteTestResultStatus,
    /** The remote tested; null when there was none. */
    val remoteUrl: String? = null,
    /** Whether the remote has any commits. */
    val remoteHasHead: Boolean,
    /** Why the remote could not be reached; null when it could. */
    val errorMessage: String? = null,
)

/** Title and body to keep on the original card. */
data class ConflictResolution(
    /** The token returned with the reviewed comparison. */
    val reviewToken: String,
    /** The replacement title. */
    val title: String,
    /** The replacement card content, which can be empty. */
    val content: String,
)

/** The reviewed pair and the title the preserved copy keeps as a card of its own. */
data class SeparateConflictCopyRequest(
    /** The token returned with the reviewed comparison. */
    val reviewToken: String,
    /** The copy's new title, usually without the "(conflicted copy)" suffix. */
    val title: String,
)

/** One of the copy's own links, as the cleanup review listed it in copy_links. */
data class ConflictCopyLinkTarget(
    /** The project of what the link points at. */
    val targetProjectId: String,
    /** What the link points at. */
    val targetType: ConflictCopyLinkTargetTargetType,
    /** The ID of what the link points at. */
    val targetId: String,
    /** What the link means. */
    val kind: String,
)

/** One milestone from the cleanup review's copy_milestones. */
data class ConflictCopyMilestoneChoice(
    /** The milestone's ID on the copy. */
    val milestoneId: String,
)

/** One tag from the cleanup review's copy_tags. */
data class ConflictCopyTagChoice(
    /** The tag, without its #. */
    val tag: String,
)

/** One card from the cleanup review's children. */
data class ConflictCopyChildChoice(
    /** The child's ID. */
    val cardId: String,
)

/** A card's link to the copy, as the cleanup review listed it in backlinks. */
data class ConflictCopyBacklinkSource(
    /** The project of the card the link is from. */
    val sourceProjectId: String,
    /** The card the link is from. */
    val sourceId: String,
    /** What the link means. */
    val kind: String,
)

/**
 * Confirmation of the cleanup effects returned when the resolution was saved, and which of the copy's own
 * details to carry over to the original first. Everything named must be in the reviewed cleanup; anything not
 * carried over stays with the copy in Trash.
 */
data class TrashConflictCopyRequest(
    /** The token returned with the cleanup review. */
    val cleanupToken: String,
    /** Links from copy_links to make on the original, with their labels. */
    val carryLinks: List<ConflictCopyLinkTarget>? = null,
    /** Milestones from copy_milestones to put on the original, as new milestones with the same dates. */
    val carryMilestones: List<ConflictCopyMilestoneChoice>? = null,
    /** Tags from copy_tags to add to the original. */
    val carryTags: List<ConflictCopyTagChoice>? = null,
    /** Children of the copy to move under the original instead of up a level. */
    val adoptChildren: List<ConflictCopyChildChoice>? = null,
    /** Card links from backlinks to point at the original instead of the copy. */
    val retargetBacklinks: List<ConflictCopyBacklinkSource>? = null,
)

/** How to push. */
data class PushOptions(
    /** The branch to push; omit for the repository's default. */
    val branch: String? = null,
    /** Whether the remote branch becomes the local branch's upstream. */
    val setUpstream: Boolean = true,
)

/** A remote to test. */
data class RemoteTest(
    /** The Git remote to test; omit for the project's own. */
    val remoteUrl: String? = null,
)

/** The git operations of core's C API. Each fails with a [ContractException]. */
object Git {
    init {
        System.loadLibrary("holder_jni")
    }

    /** What this device last recorded about a project's sync. */
    external fun getProjectSyncState(
        contextHandle: Long,
        projectId: String,
    ): SyncState

    /**
     * Compare a preserved copy with its recorded original.
     * Reads both saved cards together. A missing association, an original or copy in Trash, or a card
     * that no longer exists is NotFound. A pair changed during the read is Conflict. The operation does not
     * change either card.
     */
    external fun compareConflictCopy(
        contextHandle: Long,
        projectId: String,
        copyId: String,
    ): ConflictComparison

    /**
     * Save reviewed text to the original card without changing the preserved copy.
     * Holder verifies the token against both live cards before changing anything. A stale token is
     * Conflict. A missing association or a card that is missing or in Trash is NotFound. Changes
     * to the title and content make a new Git commit. The original's identity, hierarchy, links and
     * milestones are retained. Saving the existing title and content makes no commit. The result
     * includes a review for optional cleanup of the preserved copy.
     */
    external fun resolveConflictCopy(
        contextHandle: Long,
        projectId: String,
        copyId: String,
        resolution: ConflictResolution,
    ): ConflictResolutionResult

    /**
     * Keep a preserved copy as an ordinary card instead of resolving it into the original.
     * Holder verifies the token against both live cards before changing anything. A stale token is
     * Conflict. A missing association or a card that is missing or in Trash is NotFound. The copy takes
     * the new title and loses its recorded association with the original, in one Git commit, so it is
     * no longer listed as a conflict copy. Both cards keep their content, place, links and milestones.
     */
    external fun separateConflictCopy(
        contextHandle: Long,
        projectId: String,
        copyId: String,
        request: SeparateConflictCopyRequest,
    ): Card

    /**
     * Move a reviewed conflict copy to Trash after choosing a result.
     * Holder verifies the cleanup token against both live cards and the reviewed effects.
     * A stale token, cleanup that would move the original, or a detail to carry over that the review
     * did not list is Conflict. A missing association or a card missing or in Trash is NotFound.
     * Holder first carries the chosen details over to the original: links, milestones and tags are
     * added to it, chosen children move under it, and chosen card links to the copy point at it
     * instead. Each is its own change. The original is otherwise untouched. The copy can be restored,
     * and its other live children move up a level.
     */
    external fun trashConflictCopy(
        contextHandle: Long,
        projectId: String,
        copyId: String,
        confirmation: TrashConflictCopyRequest,
    ): CardMetadata

    /**
     * Pull a project's checked-out branch from its Git remote.
     * Changes fast-forward, or merge card by card where both sides changed, and the project's index is updated
     * before the result returns. A Git failure is an outcome in the result, recorded in the sync state. Git work on
     * one project runs one at a time.
     */
    external fun pullProject(
        contextHandle: Long,
        projectId: String,
    ): SyncResult

    /**
     * Push a project to its Git remote.
     * An encrypted project is checked first, so no plaintext card leaves the device: one whose files are not safe
     * to push is DataCorrupted, and nothing is pushed. A Git failure is an outcome in the result, recorded in the
     * sync state.
     */
    external fun pushProject(
        contextHandle: Long,
        projectId: String,
        options: PushOptions,
    ): SyncResult

    /**
     * Pull a project, then push it.
     * The push is tried only when the pull succeeds. Otherwise as pull_project and push_project.
     */
    external fun syncProject(
        contextHandle: Long,
        projectId: String,
        options: PushOptions,
    ): SyncResult

    /**
     * Try the project's pull and push phases only when their cadence and retry policy say they are due.
     * Zero uses the built-in intervals (20 minutes for push, 5 minutes for pull). A project without a remote and
     * a project with nothing due return both phases as not attempted without changing sync state. A due push can
     * run after a failed pull; this keeps scheduled pushes independent from the pull retry schedule.
     * Git failures are phase outcomes and are recorded in the project's sync state.
     */
    external fun syncProjectIfDue(
        contextHandle: Long,
        projectId: String,
        pushIntervalSeconds: Int = 0,
        pullIntervalSeconds: Int = 0,
    ): DueSyncResult

    /**
     * Test whether a Git remote can be reached, with this device's credentials.
     * Lists the remote's refs and nothing more: nothing is fetched, and neither the project nor its repository
     * changes.
     */
    external fun testProjectRemote(
        contextHandle: Long,
        projectId: String,
        remote: RemoteTest,
    ): RemoteTestResult
}
