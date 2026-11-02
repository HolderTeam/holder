// Generated from contract/interface by compiler. Do not edit; run make -C contract update.
package team.holder.android.contract

enum class ListCardsOrder(val value: String) {
    CARD_ID_ASC("card_id_asc"),
    UPDATED_DESC("updated_desc"),
    SORT_KEY_ASC("sort_key_asc"),
}

enum class TrashCardChildren(val value: String) {
    PROMOTE("promote"),
    TRASH_SUBTREE("trash_subtree"),
}

/** Whether this step names the project or a card. */
enum class CardBreadcrumbKind(val value: String) {
    PROJECT("project"),
    CARD("card"),
}

/** The resolution outcome. */
enum class CardReferenceResolveDataStatus(val value: String) {
    RESOLVED("resolved"),
    AMBIGUOUS("ambiguous"),
    NOT_FOUND("not_found"),
}

/** Which cards may match. */
enum class CardReferenceResolveRequestScope(val value: String) {
    LIVE("live"),
    TRASHED("trashed"),
    EITHER("either"),
}

/** into makes the card target_card_id's last child; before and after place it beside target_card_id, as its sibling; left and right swap it with its previous or next sibling; to_start and to_end make it the first or last of its siblings; up_level makes it its grandparent's last child. */
enum class CardMoveIntent(val value: String) {
    INTO("into"),
    BEFORE("before"),
    AFTER("after"),
    LEFT("left"),
    RIGHT("right"),
    TO_START("to_start"),
    TO_END("to_end"),
    UP_LEVEL("up_level"),
}

/** Omit for every level; roots selects root cards; a card ID selects its children. */
sealed interface ListCardsParent {
    /** "roots" */
    object Roots : ListCardsParent

    data class CardId(val value: String) : ListCardsParent
}

/** A tag written in a card's content. */
data class TagOccurrence(
    /** The normalized tag. */
    val tag: String,
    /** Inclusive byte offset of the leading # in the content. */
    val byteStart: Long,
    /** Exclusive byte offset after the tag's name. */
    val byteEnd: Long,
)

/** A card, with its content and tags. */
data class Card(
    /** The card's ID. */
    val cardId: String,
    /** The project the card belongs to. */
    val projectId: String,
    /** The card's title. */
    val title: String,
    /** The card's file, relative to the project's root. */
    val relPath: String,
    /** The card's parent; null for a root card. */
    val parentCardId: String? = null,
    /** The card's place among its siblings; lower comes first. */
    val sortKey: Double,
    /** When the card was created. */
    val createdAt: Long,
    /** When the card was last changed. */
    val updatedAt: Long,
    /** When the card was moved to the trash; null when it is not in the trash. */
    val deletedAt: Long? = null,
    /** The card's body. */
    val content: String,
    /** The card's normalized tags. */
    val tags: List<String>,
    /** Each tag written in the content, in order. */
    val tagOccurrences: List<TagOccurrence>,
    /** The tags remove_card_tag would take off the card: each is on its trailing tag line and nowhere else in its content. A tag also written in prose stays until the content is edited. */
    val removableTags: List<String>,
)

/** Everything about a card except its content. */
data class CardMetadata(
    /** The card's ID. */
    val cardId: String,
    /** The project the card belongs to. */
    val projectId: String,
    /** The card's title. */
    val title: String,
    /** The card's file, relative to the project's root. */
    val relPath: String,
    /** The card's parent; null for a root card. */
    val parentCardId: String? = null,
    /** The card's place among its siblings; lower comes first. */
    val sortKey: Double,
    /** When the card was created. */
    val createdAt: Long,
    /** When the card was last changed. */
    val updatedAt: Long,
    /** When the card was moved to the trash; null when it is not in the trash. */
    val deletedAt: Long? = null,
)

/** One project or live card in a card's path from its project root. */
data class CardBreadcrumb(
    /** Whether this step names the project or a card. */
    val kind: CardBreadcrumbKind,
    /** The project or card ID. */
    val id: String,
    /** The project or card title. */
    val title: String,
)

/** A card that matches a reference. */
data class CardReferenceCandidate(
    /** The card's canonical full UUID. */
    val cardId: String,
    /** The card's title. */
    val title: String,
    /** When the card entered Trash, or null while live. */
    val deletedAt: Long? = null,
)

/** A resolved, ambiguous, or absent card reference. */
data class CardReferenceResolveData(
    /** The resolution outcome. */
    val status: CardReferenceResolveDataStatus,
    /** How a reference matched: full_id, id_prefix or exact_title. Absent when no card matched. */
    val matchKind: String? = null,
    /** The matching card for a resolved reference. */
    val card: CardReferenceCandidate? = null,
    /** Two candidate cards for an ambiguous reference. */
    val candidates: List<CardReferenceCandidate>? = null,
)

/** A card in a listing. */
data class CardListItem(
    /** The card's ID. */
    val cardId: String,
    /** The project the card belongs to. */
    val projectId: String,
    /** The card's title. */
    val title: String,
    /** The card's file, relative to the project's root. */
    val relPath: String,
    /** The card's parent; null for a root card. */
    val parentCardId: String? = null,
    /** The card's place among its siblings; lower comes first. */
    val sortKey: Double,
    /** When the card was created. */
    val createdAt: Long,
    /** When the card was last changed. */
    val updatedAt: Long,
    /** When the card was moved to the trash; null when it is not in the trash. */
    val deletedAt: Long? = null,
    /** The card's live children; present when asked for. */
    val childCount: Int? = null,
    /** The card's body; present when asked for. */
    val content: String? = null,
)

/** A tag and how many of a project's live cards carry it. */
data class TagCount(
    /** The tag, normalized: in lower case. */
    val tag: String,
    /** The live cards in the project that carry the tag: at least one, or the tag is not listed. */
    val cardCount: Int,
)

/** A reference to resolve among one project's cards. */
data class CardReferenceResolveRequest(
    /** The project to search. */
    val projectId: String,
    /** A full canonical UUID, eligible UUID prefix, or exact card title. */
    val reference: String,
    /** Which cards may match. */
    val scope: CardReferenceResolveRequestScope,
)

/** A card to create. */
data class NewCard(
    /** The card's title. */
    val title: String,
    /** The card's body. */
    val content: String = "",
    /** The card's parent; null or omitted for a root card. */
    val parentCardId: String? = null,
    /** The card's place among its siblings; omit to let Holder choose. */
    val sortKey: Double? = null,
    /** When the card was created, for imports; omit for now. */
    val createdAt: Long? = null,
    /** When the card was last changed, for imports; omit to use created_at. */
    val updatedAt: Long? = null,
)

/** Changes to a card. Each field left out keeps its value. */
data class CardUpdate(
    /** The card's new title. */
    val title: String? = null,
    /** The card's new body. */
    val content: String? = null,
    /** The card's new parent; null makes it a root card. */
    val parentCardId: Change<String> = Change.Keep,
    /** The card's new place among its siblings. */
    val sortKey: Double? = null,
    /** When the card was changed, for imports; omit for now. */
    val updatedAt: Long? = null,
)

/** Where to move a card, relative to another card or among its siblings. */
data class CardMove(
    /** into makes the card target_card_id's last child; before and after place it beside target_card_id, as its sibling; left and right swap it with its previous or next sibling; to_start and to_end make it the first or last of its siblings; up_level makes it its grandparent's last child. */
    val intent: CardMoveIntent,
    /** The card to move into, before or after; required for those intents, and ignored by the others. */
    val targetCardId: String? = null,
    /** For left, right, to_start and to_end: the parent among whose children to place it, null for the root cards; omit for its current parent. Ignored by the other intents. */
    val parentCardId: Change<String> = Change.Keep,
)

/** A card cannot go where it was asked to: under itself or one of its descendants, beside a card that cannot be its sibling, or up a level from the top. */
class InvalidMoveException(message: String) : ContractException(4220, message)

/** A card's ancestor chain is broken, cyclic or too deep to follow. */
class HierarchyInvalidException(message: String) : ContractException(4092, message)

/** The cards operations of core's C API. Each fails with a [ContractException]. */
object Cards {
    init {
        System.loadLibrary("holder_jni")
    }

    /**
     * Fetch a card.
     * A card in the trash or in another project is not found.
     */
    external fun getCard(
        contextHandle: Long,
        projectId: String,
        cardId: String,
    ): Card

    /**
     * Fetch a live card's metadata without reading its content.
     * A card in the trash or in another project is not found.
     */
    external fun getCardMetadata(
        contextHandle: Long,
        projectId: String,
        cardId: String,
    ): CardMetadata

    /**
     * Read a live card's path from its project root.
     * The project comes first, then live ancestor cards and the requested card. A missing or trashed
     * card is not found; a broken or cyclic ancestor chain is invalid.
     */
    external fun getCardBreadcrumbs(
        contextHandle: Long,
        projectId: String,
        cardId: String,
    ): List<CardBreadcrumb>

    /** Resolve a card reference within a project and deletion scope. */
    external fun resolveCardReference(
        contextHandle: Long,
        request: CardReferenceResolveRequest,
    ): CardReferenceResolveData

    /**
     * Page through a project's live cards.
     * Filters combine. Each page reads current data; iterating is not a snapshot.
     */
    external fun listCards(
        contextHandle: Long,
        projectId: String,
        parent: ListCardsParent? = null,
        tag: String? = null,
        order: ListCardsOrder = ListCardsOrder.CARD_ID_ASC,
        limit: Int = 200,
        cursor: String? = null,
        includeContent: Boolean = false,
        includeChildCount: Boolean = false,
    ): Page<CardListItem>

    /**
     * Create a card.
     * Fails with NotFound when the project or the parent does not exist.
     */
    external fun createCard(
        contextHandle: Long,
        projectId: String,
        card: NewCard,
    ): Card

    /**
     * Change a card.
     * Each field left out keeps its value, and an update that leaves every field out changes nothing. The title and
     * content are written first, then the parent and place, each as its own change in the project's history. A card
     * in the trash or in another project is not found, and neither is a new parent that does not exist or is in the
     * trash; a parent under the card itself is an InvalidMove.
     */
    external fun updateCard(
        contextHandle: Long,
        projectId: String,
        cardId: String,
        update: CardUpdate,
    ): Card

    /**
     * Move a card to the trash.
     * The card and every card that changes with it are written as one change in the project's history. A card already
     * in the trash, or in another project, is not found. Returns the card as it is in the trash.
     */
    external fun trashCard(
        contextHandle: Long,
        projectId: String,
        cardId: String,
        children: TrashCardChildren = TrashCardChildren.PROMOTE,
    ): Card

    /**
     * Restore a card from the trash.
     * Restores this card only. It returns to its parent when that parent is live and reachable, otherwise to its
     * nearest live, reachable ancestor or the top level. Children promoted when it was trashed stay where they are;
     * descendants trashed with it stay in the trash, and come back under it when restored. A card in another
     * project is not found, and one that is not in the trash is a Conflict.
     */
    external fun restoreCard(
        contextHandle: Long,
        projectId: String,
        cardId: String,
    ): Card

    /**
     * Move a card in its project's hierarchy.
     * When the card's new neighbours leave no room between their places, nearby siblings are re-spaced in the same
     * change, keeping their order; other cards' sort_key values a caller holds may then be out of date. A card in the
     * trash or in another project is not found, and neither is a target or parent that does not exist or is in the
     * trash. A move that changes nothing writes nothing.
     */
    external fun moveCard(
        contextHandle: Long,
        projectId: String,
        cardId: String,
        move: CardMove,
    ): Card

    /**
     * Tag a card.
     * The tag is added to the card's trailing tag line, which is made when the card has none. A card that already
     * carries the tag, anywhere in its content, is left as it is. A card in the trash or in another project is not
     * found.
     */
    external fun addCardTag(
        contextHandle: Long,
        projectId: String,
        cardId: String,
        tag: String,
    ): Card

    /**
     * Take a tag off a card.
     * The tag is removed from the card's trailing tag line, and the line with it when it is left empty, so that the
     * card no longer carries it. A card that does not carry the tag is left as it is. Holder does not rewrite prose,
     * so a card that would still carry the tag in its prose is a Conflict, and is left as it is. A card in the trash
     * or in another project is not found.
     */
    external fun removeCardTag(
        contextHandle: Long,
        projectId: String,
        cardId: String,
        tag: String,
    ): Card

    /**
     * List a project's tags.
     * Each tag carried by a live card in the project, with how many carry it, by descending count and then by tag.
     */
    external fun listProjectTags(
        contextHandle: Long,
        projectId: String,
    ): List<TagCount>
}
