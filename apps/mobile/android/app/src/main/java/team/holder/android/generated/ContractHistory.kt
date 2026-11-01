// Generated from contract/interface/history/models.py and contract/interface/history/operations.py by holder_contract.
// Do not edit; run make -C contract update.
package team.holder.android.generated

data class ContractCardHistorySave(
    val oid: String,
    val parentOids: List<String>,
    val authoredAt: Long,
    val committedAt: Long,
    val message: String,
)

data class ContractHistoryAuthor(val name: String, val email: String)

data class ContractCardHistoryEntry(
    val firstOid: String,
    val lastOid: String,
    val parentOids: List<String>,
    val visibleParentOids: List<String>,
    val author: ContractHistoryAuthor,
    val startedAt: Long,
    val endedAt: Long,
    val kind: String,
    val summary: String,
    val commitCount: Int,
    val isMerge: Boolean,
    val saves: List<ContractCardHistorySave>,
)

data class ContractCardHistoryPage(
    val headOid: String?,
    val entries: List<ContractCardHistoryEntry>,
    val nextCursor: String?,
    val scanLimited: Boolean,
)

data class ContractCardHistoryVersion(
    val exists: Boolean,
    val oid: String,
    val title: String,
    val body: String,
)

data class ContractCardHistoryDiffLine(
    val origin: String,
    val text: String,
    val oldLine: Long?,
    val newLine: Long?,
)

data class ContractCardHistoryComparison(
    val before: ContractCardHistoryVersion,
    val after: ContractCardHistoryVersion,
    val summary: String,
    val lines: List<ContractCardHistoryDiffLine>,
    val truncated: Boolean,
)

data class ContractCardHistoryRestoreResult(
    val cardId: String,
    val restoredFromOid: String,
    val resultOid: String,
    val title: String,
    val updatedAt: Long,
    val deletedAt: Long?,
)

object ContractHistory {
    external fun nativeListCardHistory(
        contextHandle: Long, projectId: String, cardId: String, limit: Int, cursor: String?,
    ): ContractCardHistoryPage
    external fun nativeCompareCardHistory(
        contextHandle: Long, projectId: String, cardId: String, fromOid: String?, toOid: String,
    ): ContractCardHistoryComparison
    external fun nativeRestoreCardHistory(
        contextHandle: Long, projectId: String, cardId: String, oid: String,
    ): ContractCardHistoryRestoreResult
}
