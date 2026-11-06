// Generated from contract/interface by compiler. Do not edit; run make -C contract update.
package team.holder.android.contract

/** A card matching a full-text query. */
data class SearchCard(
    /** The matching card's ID. */
    val cardId: String,
    /** The card's current title. */
    val title: String,
    /** When the card was last updated. */
    val updatedAt: Long,
    /** When the card was created. */
    val createdAt: Long,
    /** A highlighted excerpt from the match. */
    val snippet: String,
    /** The FTS relevance score; lower scores rank first. */
    val rank: Double,
)

/** An AI message matching a full-text query. */
data class SearchMessage(
    /** The matching message's ID. */
    val messageId: String,
    /** When the message was created. */
    val createdAt: Long,
    /** A highlighted excerpt from the match. */
    val snippet: String,
    /** The FTS relevance score; lower scores rank first. */
    val rank: Double,
)

/** The search operations of core's C API. Each fails with a [ContractException]. */
object Search {
    init {
        System.loadLibrary("holder_jni")
    }

    /** Search a project's cards, ordered by relevance, best first. */
    external fun searchCards(
        contextHandle: Long,
        projectId: String,
        q: String,
        limit: Int = 20,
        offset: Int = 0,
    ): List<SearchCard>

    /** Search a project's AI messages, ordered by relevance, best first. */
    external fun searchAiMessages(
        contextHandle: Long,
        projectId: String,
        q: String,
        limit: Int = 20,
        offset: Int = 0,
    ): List<SearchMessage>
}
