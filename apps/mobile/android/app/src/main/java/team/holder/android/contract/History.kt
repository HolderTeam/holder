// Generated from contract/interface by holder_contract. Do not edit; run make -C contract update.
package team.holder.android.contract

enum class ListProjectHistoryKind(val value: String) {
    CARD("card"),
    RESOURCE("resource"),
    LOCATION("location"),
    AI_DATA("ai_data"),
    PROJECT_SETTINGS("project_settings"),
    UNKNOWN("unknown"),
}

enum class CompareCardHistoryMode(val value: String) {
    SINCE("since"),
    CHANGE("change"),
}

/** What kind of object these paths belong to. */
enum class HistoryAffectedObjectKind(val value: String) {
    CARD("card"),
    RESOURCE("resource"),
    LOCATION("location"),
    AI_DATA("ai_data"),
    PROJECT_SETTINGS("project_settings"),
    UNKNOWN("unknown"),
}

/** The author recorded on a Git commit. */
data class HistoryAuthor(
    /** The author's name. */
    val name: String,
    /** The author's email address. */
    val email: String,
)

/** A changed path with any display details found in its historical version. */
data class HistoryAffectedPath(
    /** The path relative to the project root. */
    val path: String,
    /** A historical title, when available. */
    val title: String? = null,
    /** A historical detail, when available. */
    val detail: String? = null,
)

/** Paths changed together in one commit, grouped by Holder object kind. */
data class HistoryAffectedObject(
    /** What kind of object these paths belong to. */
    val kind: HistoryAffectedObjectKind,
    /** The changed paths, retained for existing clients. */
    val paths: List<String>,
    /** The changed paths with historical display details. */
    val items: List<HistoryAffectedPath>,
)

/** One Git commit affecting a project. */
data class ProjectActivity(
    /** The full commit object ID. */
    val oid: String,
    /** The commit's parents. */
    val parentOids: List<String>,
    /** The commit's author. */
    val author: HistoryAuthor,
    /** When the commit was authored. */
    val authoredAt: Long,
    /** When the commit was committed. */
    val committedAt: Long,
    /** The commit message. */
    val message: String,
    /** Changed paths grouped by Holder object kind. */
    val affectedObjects: List<HistoryAffectedObject>,
    /** Whether the commit has multiple parents. */
    val isMerge: Boolean,
)

/** One bounded page of project activity, newest first. */
data class ProjectHistoryPage(
    /** The project HEAD captured for this page; null for an empty repository. */
    val headOid: String? = null,
    /** The activities on this page, newest first. */
    val activities: List<ProjectActivity>,
    /** The cursor for another page; null at the end. */
    val nextCursor: String? = null,
    /** Whether History reached its bounded scan before the end. */
    val scanLimited: Boolean,
)

/** One exact commit saved for a card. */
data class CardHistorySave(
    /** The full commit ID. */
    val oid: String,
    /** The commit's parents. */
    val parentOids: List<String>,
    /** When the commit was authored. */
    val authoredAt: Long,
    /** When the commit was committed. */
    val committedAt: Long,
    /** The commit message. */
    val message: String,
)

/** One displayed card change, possibly grouping adjacent autosaves. */
data class CardHistoryEntry(
    /** The first commit in the entry. */
    val firstOid: String,
    /** The last commit in the entry. */
    val lastOid: String,
    /** Parents of the first commit. */
    val parentOids: List<String>,
    /** Direct parents represented on this page; omitted connections must not be inferred. */
    val visibleParentOids: List<String>,
    /** The author's name and email. */
    val author: HistoryAuthor,
    /** When the first commit was authored. */
    val startedAt: Long,
    /** When the last commit was committed. */
    val endedAt: Long,
    /** The kind of change shown. */
    val kind: String,
    /** A short description of the change. */
    val summary: String,
    /** The number of commits grouped in this entry. */
    val commitCount: Int,
    /** Whether this entry includes a merge commit. */
    val isMerge: Boolean,
    /** Exact saved commits in chronological order. */
    val saves: List<CardHistorySave>,
)

/** One bounded page of saved card changes, newest first. */
data class CardHistoryPage(
    /** The project HEAD captured for this page; null for an empty repository. */
    val headOid: String? = null,
    /** The card changes on this page. */
    val entries: List<CardHistoryEntry>,
    /** The cursor for another page; null at the end. */
    val nextCursor: String? = null,
    /** Whether History reached its bounded scan before the end. */
    val scanLimited: Boolean,
)

/** A card's saved state at one commit, including an explicit absent state. */
data class CardHistoryVersion(
    /** Whether the card existed in this snapshot. */
    val exists: Boolean,
    /** The full commit ID. */
    val oid: String,
    /** The card's saved title, empty when absent. */
    val title: String,
    /** The card's saved body, empty when absent. */
    val body: String,
)

/** One line in a card comparison. */
data class CardHistoryDiffLine(
    /** Space for unchanged, plus for added, minus for removed. */
    val origin: String,
    /** The line's text. */
    val text: String,
    /** Its old line number; null for an addition. */
    val oldLine: Long? = null,
    /** Its new line number; null for a removal. */
    val newLine: Long? = null,
)

/** A bounded line comparison of two saved card states. */
data class CardHistoryComparison(
    /** The earlier state. */
    val before: CardHistoryVersion,
    /** The later state. */
    val after: CardHistoryVersion,
    /** A short description of the difference. */
    val summary: String,
    /** The comparison's lines. */
    val lines: List<CardHistoryDiffLine>,
    /** Whether the line comparison was cut short. */
    val truncated: Boolean,
)

/** A card's state at a selected revision. */
data class CardHistorySnapshot(
    /** The card being inspected. */
    val cardId: String,
    /** The state at the selected commit. */
    val snapshot: CardHistoryVersion,
)

/** A card restored from a saved state by creating a new commit. */
data class CardHistoryRestoreResult(
    /** The restored card. */
    val cardId: String,
    /** The full commit ID of the selected historical snapshot. */
    val restoredFromOid: String,
    /** The full commit ID created by restoring it. */
    val resultOid: String,
    /** The card's title after restoration. */
    val title: String,
    /** When the restored card was updated. */
    val updatedAt: Long,
    /** When the restored card is in Trash; null when live. */
    val deletedAt: Long? = null,
)

/** The project's Git history cannot be read right now. */
class HistoryUnavailableException(message: String) : ContractException(5031, message)

/** This device cannot use the encrypted project's key to read its history. */
class HistoryKeyUnavailableException(message: String) : ContractException(4093, message)

/** A commit ID prefix matches more than one revision; use a longer prefix or the full ID. */
class RevisionAmbiguousException(message: String) : ContractException(4094, message)

/** The bounded History response would exceed 2 MiB. */
class HistoryResponseTooLargeException(message: String) : ContractException(4130, message)

/** The history operations of core's C API. Each fails with a [ContractException]. */
object History {
    init {
        System.loadLibrary("holder_jni")
    }

    /**
     * Return bounded, cursor-paginated Git activities for a project.
     * Each commit groups its changed paths by Holder object kind. Unrecognised paths remain visible as unknown.
     * A missing decryption key is HistoryKeyUnavailable; other history read failures are HistoryUnavailable.
     */
    external fun listProjectHistory(
        contextHandle: Long,
        projectId: String,
        limit: Int = 50,
        cursor: String? = null,
        kind: ListProjectHistoryKind? = null,
    ): ProjectHistoryPage

    /**
     * List saved history for a card.
     * Returns commits reachable from the captured project HEAD that changed the card. Adjacent autosaves may be
     * grouped into one displayed entry.
     */
    external fun listCardHistory(
        contextHandle: Long,
        projectId: String,
        cardId: String,
        limit: Int = 50,
        cursor: String? = null,
    ): CardHistoryPage

    /**
     * Compare saved card versions or one revision's change.
     * In since mode, compares the selected before and after snapshots. In change mode, compares the first parent
     * of to_oid with to_oid. A root commit has a previously nonexistent card as its before state; a merge uses only
     * its first parent.
     */
    external fun compareCardHistory(
        contextHandle: Long,
        projectId: String,
        cardId: String,
        toOid: String,
        fromOid: String? = null,
        mode: CompareCardHistoryMode = CompareCardHistoryMode.SINCE,
    ): CardHistoryComparison

    /** Read a card snapshot at a selected revision, including an explicit absent state. */
    external fun getCardHistorySnapshot(
        contextHandle: Long,
        projectId: String,
        cardId: String,
        oid: String,
    ): CardHistorySnapshot

    /**
     * Restore a card to a saved snapshot.
     * Restores the complete snapshot as a new commit, preserving the old history. Lifecycle rules still apply:
     * restoring a trashed snapshot promotes live children, and restoring a live snapshot places the card under a
     * reachable parent.
     */
    external fun restoreCardHistorySnapshot(
        contextHandle: Long,
        projectId: String,
        cardId: String,
        oid: String,
    ): CardHistoryRestoreResult
}
