// Generated from contract/interface by holder_contract. Do not edit; run make -C contract update.
package team.holder.android.contract

enum class ListCardBacklinksScope(val value: String) {
    LOCAL("local"),
    EXTERNAL("external"),
    ALL_AVAILABLE("all_available"),
}

enum class RemoveCardLinkTargetType(val value: String) {
    CARD("card"),
    AI_MESSAGE("ai_message"),
    AI_THREAD("ai_thread"),
    RESOURCE("resource"),
}

/** What the link is from. */
enum class LinkSourceType(val value: String) {
    CARD("card"),
    AI_MESSAGE("ai_message"),
}

/** What the link points at. */
enum class LinkTargetType(val value: String) {
    CARD("card"),
    AI_MESSAGE("ai_message"),
    AI_THREAD("ai_thread"),
    RESOURCE("resource"),
}

/** What the link points at. */
enum class NewLinkTargetType(val value: String) {
    CARD("card"),
    AI_MESSAGE("ai_message"),
    AI_THREAD("ai_thread"),
    RESOURCE("resource"),
}

/**
 * One of the link kinds Holder knows, with how it reads from each end of a link. A link's kind is free text;
 * a kind Holder does not know has no labels of its own.
 */
data class LinkKindLabels(
    /** The kind, as a link names it. */
    val kind: String,
    /** How the link reads from the card it is from, such as Depends on. */
    val forwardLabel: String,
    /** How the link reads from what it points at, such as Required by. */
    val reverseLabel: String,
)

/**
 * A link from a card, or from an AI message, to something in a project. A link names both of its ends in full:
 * each end's project, type and ID.
 */
data class Link(
    /** The project of the card or AI message the link is from. */
    val sourceProjectId: String,
    /** What the link is from. */
    val sourceType: LinkSourceType,
    /** The ID of the card or AI message the link is from. */
    val sourceId: String,
    /** The project of what the link points at: the source's own project, or, for a card, another project, which need not be on this device. A link grants no access to its target. */
    val targetProjectId: String,
    /** What the link points at. */
    val targetType: LinkTargetType,
    /** The ID of what the link points at. */
    val targetId: String,
    /** What the link means. */
    val kind: String,
    /** A note on the link; null when it has none. */
    val label: String? = null,
    /** When the link was made. */
    val createdAt: Long,
)

/** A link to make from a card. Its source is the card it is made from. */
data class NewLink(
    /** What the link points at. */
    val targetType: NewLinkTargetType,
    /** The ID of what the link points at. */
    val targetId: String,
    /** The project of a card the link points at; omit for the card's own project. Only a card can be in another project, which need not be on this device. */
    val targetProjectId: String? = null,
    /** What the link means. */
    val kind: String = "ref",
    /** A note on the link; null or omitted for none. */
    val label: String? = null,
    /** When the link was made, for imports; omit for now. A link that already exists keeps its own. */
    val createdAt: Long? = null,
)

/** The links operations of core's C API. Each fails with a [ContractException]. */
object Links {
    init {
        System.loadLibrary("holder_jni")
    }

    /**
     * List the link kinds Holder knows, with their labels.
     * The list is the same for every project and device. A link may have any other kind.
     */
    external fun listLinkKinds(
        contextHandle: Long,
    ): List<LinkKindLabels>

    /**
     * List a card's links.
     * The links the card makes, newest first. A card in the trash or in another project is not found.
     */
    external fun listCardLinks(
        contextHandle: Long,
        projectId: String,
        cardId: String,
        includeDeleted: Boolean = false,
    ): List<Link>

    /**
     * List the links to a card.
     * Links from cards and AI messages, newest first. External links come only from projects on this device, so
     * they are never all there may be. A card in the trash or in another project is not found.
     */
    external fun listCardBacklinks(
        contextHandle: Long,
        projectId: String,
        cardId: String,
        scope: ListCardBacklinksScope = ListCardBacklinksScope.LOCAL,
        includeDeleted: Boolean = false,
    ): List<Link>

    /**
     * Link a card to something.
     * A link is one target, of one type in one project, with one kind. Making a link that already exists sets its
     * label and keeps when it was made. A target in the card's project must exist and not be in the trash; a card in
     * another project may be one this device does not have. A card in the trash or in another project is not found,
     * and neither is a target that does not exist or is in the trash.
     */
    external fun addCardLink(
        contextHandle: Long,
        projectId: String,
        cardId: String,
        link: NewLink,
    ): Link

    /**
     * Remove a link from a card.
     * Removes the one link to that target, of that type in that project, with that kind, and returns the card's
     * remaining links as list_card_links lists them by default. A card without that link is left as it is. A card in
     * the trash or in another project is not found.
     */
    external fun removeCardLink(
        contextHandle: Long,
        projectId: String,
        cardId: String,
        targetType: RemoveCardLinkTargetType,
        targetId: String,
        kind: String = "ref",
        targetProjectId: String? = null,
    ): List<Link>
}
