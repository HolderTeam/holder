// Generated from contract/interface by holder_contract. Do not edit.
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

data class ContractPage<T>(
    val items: List<T>,
    val nextCursor: String?,
)

object ContractOperations {
    external fun nativeListCards(
        contextHandle: Long,
        projectId: String,
        parent: String? = null,
        tag: String? = null,
        order: String = "card_id_asc",
        limit: Int = 200,
        cursor: String? = null,
        includeContent: Boolean = false,
        includeChildCount: Boolean = false,
    ): ContractPage<ContractCardListItem>
}
