// Generated from contract/interface by compiler. Do not edit; run make -C contract update.
package team.holder.android.contract

enum class EmptyTrashType(val value: String) {
    CARD("card"),
    AI_MESSAGE("ai_message"),
    ALL("all"),
}

enum class DeleteTrashItemType(val value: String) {
    CARD("card"),
    AI_MESSAGE("ai_message"),
}

enum class DeleteProjectTrashItemType(val value: String) {
    CARD("card"),
    AI_MESSAGE("ai_message"),
}

/** The kind of item. */
enum class TrashItemType(val value: String) {
    CARD("card"),
    AI_MESSAGE("ai_message"),
}

/** A trashed card or AI message. */
data class TrashItem(
    /** The kind of item. */
    val type: TrashItemType,
    /** The project that owns the item. */
    val projectId: String,
    /** The card ID, present for a card. */
    val cardId: String? = null,
    /** The message ID, present for an AI message. */
    val messageId: String? = null,
    /** The AI thread containing a message. */
    val threadId: String? = null,
    /** The card title, if this is a card. */
    val title: String? = null,
    /** The AI message role, if this is a message. */
    val role: String? = null,
    /** When the item entered Trash. */
    val deletedAt: Long,
    /** When the AI message was created. */
    val createdAt: Long? = null,
    /** The card's relative file path. */
    val relPath: String? = null,
)

/** The project whose Trash was emptied. */
data class TrashEmptyResult(
    /** The requested project, including when it had no trashed items. */
    val projectId: String,
)

/** The ID of a permanently removed item. */
data class TrashDeleteResult(
    /** The removed card's ID, or null for a message. */
    val cardId: String? = null,
    /** The removed AI message's ID, or null for a card. */
    val messageId: String? = null,
)

/** The item still exists but is not in Trash, so permanent removal is refused. */
class NotTrashedException(message: String) : ContractException(4095, message)

/** The trash operations of core's C API. Each fails with a [ContractException]. */
object Trash {
    init {
        System.loadLibrary("holder_jni")
    }

    /**
     * List a project's trashed cards and AI messages.
     * A project that is not registered has an empty Trash.
     */
    external fun listTrash(
        contextHandle: Long,
        projectId: String,
        type: String? = null,
    ): List<TrashItem>

    /**
     * Permanently remove a project's trashed items.
     * An empty or unregistered project succeeds without changes. Old Git revisions of cards remain available.
     */
    external fun emptyTrash(
        contextHandle: Long,
        projectId: String,
        type: EmptyTrashType = EmptyTrashType.ALL,
    ): TrashEmptyResult

    /** Permanently remove a trashed item by its global ID (legacy route). */
    external fun deleteTrashItem(
        contextHandle: Long,
        type: DeleteTrashItemType,
        id: String,
    ): TrashDeleteResult

    /**
     * Permanently remove a trashed item from the named project.
     * Old Git revisions of cards remain available.
     */
    external fun deleteProjectTrashItem(
        contextHandle: Long,
        projectId: String,
        type: DeleteProjectTrashItemType,
        id: String,
    ): TrashDeleteResult
}
