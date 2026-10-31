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

data class ContractTagCount(
    val tag: String,
    val cardCount: Int,
)

data class ContractSearchCard(
    val cardId: String,
    val title: String,
    val updatedAt: Long,
    val createdAt: Long,
    val snippet: String,
    val rank: Double,
)

data class ContractTrashItem(
    val type: String,
    val projectId: String,
    val cardId: String?,
    val messageId: String?,
    val threadId: String?,
    val title: String?,
    val role: String?,
    val deletedAt: Long,
    val createdAt: Long?,
    val relPath: String?,
)


data class ContractCardPage(
    val items: List<ContractCardListItem>,
    val nextCursor: String?,
)

object ContractCards {
    external fun nativeListCardsPage(contextHandle: Long, projectId: String, cursor: String?): ContractCardPage
    external fun nativeGetCardMetadata(contextHandle: Long, projectId: String, cardId: String): ContractCardMetadata
    external fun nativeGetCardContent(contextHandle: Long, projectId: String, cardId: String): String
    external fun nativeListCardTags(contextHandle: Long, projectId: String, cardId: String): List<String>
    external fun nativeCreateCard(contextHandle: Long, projectId: String, title: String, content: String, parentCardId: String?): ContractCardMetadata
    external fun nativeUpdateCard(contextHandle: Long, projectId: String, cardId: String, title: String, content: String): ContractCardMetadata
    external fun nativeTrashCard(contextHandle: Long, projectId: String, cardId: String)
    external fun nativeRestoreCard(contextHandle: Long, projectId: String, cardId: String): ContractCardMetadata
    external fun nativeListProjectTags(contextHandle: Long, projectId: String): List<ContractTagCount>
    external fun nativeSearchCards(contextHandle: Long, projectId: String, query: String, limit: Int): List<ContractSearchCard>
    external fun nativePurgeCard(contextHandle: Long, projectId: String, cardId: String)
    external fun nativeEmptyCardTrash(contextHandle: Long, projectId: String)
    external fun nativeListTrashedCards(contextHandle: Long, projectId: String): List<ContractTrashItem>
}
