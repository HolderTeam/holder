// Generated from contract/interface by compiler. Do not edit; run make -C contract update.
package team.holder.android.contract

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
)

/** What a pull did. */
data class PullPhase(
    /** Whether a pull was tried. */
    val attempted: Boolean,
    /** How the pull ended. */
    val status: PullPhaseStatus,
    /** Cards changed both here and on the remote, merged by Holder's card rules. */
    val conflictsResolved: Int,
    /** Live cards the pull left under a missing or trashed parent, made reachable again. */
    val hiddenCardsRepaired: Int,
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
