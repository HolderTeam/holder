// Generated from contract/interface by holder_contract. Do not edit; run make -C contract update.
package team.holder.android.contract

/** How the project's files are stored. */
enum class ProjectEncryptionCheckPrivacyMode(val value: String) {
    ENCRYPTED_GIT("encrypted_git"),
    PLAIN("plain"),
}

/** Whether an encrypted project's card files are safe to push. */
data class EncryptionSafetyCheck(
    /** Whether every checked file passed. */
    val ok: Boolean,
    /** Card files examined. */
    val checkedFiles: Int,
    /** Card files that failed the safety check. */
    val unsafeFiles: Int,
    /** Project-relative paths of unsafe files. */
    val unsafePaths: List<String>,
    /** A short explanation of the result. */
    val message: String,
)

/** A project's privacy mode and the result of checking its files. */
data class ProjectEncryptionCheck(
    /** The project checked. */
    val projectId: String,
    /** How the project's files are stored. */
    val privacyMode: ProjectEncryptionCheckPrivacyMode,
    /** The safety check's result. */
    val check: EncryptionSafetyCheck,
)

/** A hidden branch's top card and its new parent. */
data class HierarchyRepairMove(
    /** The hidden branch's top card. */
    val cardId: String,
    /** Its new parent, or null for a project root. */
    val parentCardId: String? = null,
)

/** A plan to make hidden live cards reachable, and whether it was applied. */
data class HierarchyRepair(
    /** Whether changes were written. */
    val applied: Boolean,
    /** The hidden branches whose top cards move. */
    val moved: List<HierarchyRepairMove>,
    /** Cards moved or re-spaced to make room. */
    val changedCardCount: Int,
    /** Cards in parent cycles; left for manual repair. */
    val cycleCardIds: List<String>,
)

/** The numbers of entities restored from a project's files. */
data class RebuildResult(
    /** The project rebuilt. */
    val projectId: String,
    /** Cards restored. */
    val cards: Int,
    /** AI messages restored. */
    val aiMessages: Int,
    /** AI threads restored. */
    val aiThreads: Int,
    /** Links restored. */
    val links: Int,
)

/** Completion of a full database search-index rebuild. */
data class ReindexResult(
    /** A short completion message. */
    val message: String,
)

/** Ask for a repair plan, or apply it. */
data class HierarchyRepairRequest(
    /** Apply the repair. Without it, only report. */
    val apply: Boolean = false,
)

/** The project whose files should be read into the database. */
data class RebuildRequest(
    /** The project to rebuild. */
    val projectId: String,
)

/** A project's files cannot be read back into the database. */
class RebuildFailedException(message: String) : ContractException(4002, message)

/** The maintenance operations of core's C API. Each fails with a [ContractException]. */
object Maintenance {
    init {
        System.loadLibrary("holder_jni")
    }

    /**
     * Check whether an encrypted project's card files are safe to push.
     * A plain project needs no check and reports a successful result with zero files.
     */
    external fun checkProjectEncryption(
        contextHandle: Long,
        projectId: String,
    ): ProjectEncryptionCheck

    /**
     * Find hidden live card branches, and optionally move them into the reachable tree.
     * An empty body plans without writing. Applying moves each hidden branch to its nearest reachable
     * ancestor in one commit; a second apply changes nothing. Parent cycles are reported and left alone.
     */
    external fun repairProjectHierarchy(
        contextHandle: Long,
        projectId: String,
        request: HierarchyRepairRequest,
    ): HierarchyRepair

    /**
     * Rebuild a project's database state from its files.
     * A missing project is NotFound. Files that cannot be read back give RebuildFailed.
     */
    external fun rebuildProject(
        contextHandle: Long,
        request: RebuildRequest,
    ): RebuildResult

    /** Rebuild all search indexes from database state. */
    external fun reindexSearch(
        contextHandle: Long,
    ): ReindexResult
}
