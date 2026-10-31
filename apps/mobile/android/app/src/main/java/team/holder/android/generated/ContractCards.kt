// Generated from contract/interface/cards.py and contract/interface/app.py by holder_contract.
// Do not edit; run make -C contract update.
package team.holder.android.generated

data class ContractCardListItem(
    val cardId: String,
    val projectId: String,
    val title: String,
    val relPath: String,
    val parentCardId: String?,
    val sortKey: Double,
    val createdAt: Long,
    val updatedAt: Long,
    val deletedAt: Long?,
    val childCount: Int?,
    val content: String?,
)

data class ContractCardMetadata(
    val cardId: String,
    val projectId: String,
    val title: String,
    val relPath: String,
    val parentCardId: String?,
    val sortKey: Double,
    val createdAt: Long,
    val updatedAt: Long,
    val deletedAt: Long?,
)

data class ContractCardPage(
    val items: List<ContractCardListItem>,
    val nextCursor: String?,
)

object ContractCards {
    external fun nativeListCardsPage(contextHandle: Long, projectId: String, cursor: String?): ContractCardPage
    external fun nativeGetCardMetadata(contextHandle: Long, projectId: String, cardId: String): ContractCardMetadata
    external fun nativeCreateCard(contextHandle: Long, projectId: String, title: String, content: String, parentCardId: String?): ContractCardMetadata
    external fun nativeUpdateCard(contextHandle: Long, projectId: String, cardId: String, title: String, content: String): ContractCardMetadata
    external fun nativeTrashCard(contextHandle: Long, projectId: String, cardId: String)
    external fun nativeRestoreCard(contextHandle: Long, projectId: String, cardId: String): ContractCardMetadata
}
