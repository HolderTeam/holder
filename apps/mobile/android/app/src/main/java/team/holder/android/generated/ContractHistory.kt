// Generated from contract/interface/history.py and contract/interface/app.py by holder_contract.
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

object ContractHistory {
    external fun nativeListCardHistory(
        contextHandle: Long, projectId: String, cardId: String, limit: Int, cursor: String?,
    ): ContractCardHistoryPage
}
