package team.holder.android.ui

import team.holder.android.HolderCard

/** The four ordering-based connections shown for a card, alongside its explicit links:
 * Next/Previous walk the whole project by last-modified time; Follows/Precedes walk Flowboard's
 * sort_key-ordered siblings (which defaults to creation order until someone drags cards around
 * on desktop -- Android has no reorder UI of its own). Follows is dropped when it's the same
 * card as Next, and Precedes when it's the same as Previous: the two orderings frequently
 * agree (a freshly appended sibling is often also the most recently touched card), so without
 * deduping, an untouched project would show every card connected to itself under two labels. */
data class CardSequenceLinks(
    val next: HolderCard?,
    val previous: HolderCard?,
    val follows: HolderCard?,
    val precedes: HolderCard?,
)

/** parentCardId's children (or, when null, every root card in the project), in Holder's
 * canonical sibling order: sort_key ascending, then card ID ascending for equal keys. That is
 * the order core lists cards in (`ORDER BY sort_key ASC, card_id ASC`) and the order its move
 * resolver uses (`card_tree_less`), so the sibling shown beside a card is the one Move Up/Down
 * swaps it with, even on older or imported data where sort keys tie (issue #102). Card IDs are
 * ASCII, so String comparison here matches core's byte order. The single source of truth for
 * sibling order on Android: Board, [cardSequenceLinks]'s Follows/Precedes and the swipe deck
 * (see CardViewPagerScreen) all walk this same list. */
fun sortKeyOrderedSiblings(parentCardId: String?, allCards: List<HolderCard>): List<HolderCard> =
    allCards
        .filter { it.parentCardId == parentCardId }
        .sortedWith(compareBy<HolderCard> { it.sortKey }.thenBy { it.cardId })

/** Every card in the project, last modified first: the plain card list's order, as on Desktop.
 * sort_key only orders siblings, so across a whole project it reads as no order at all. Ties
 * (cards saved in the same second) fall back to card ID, so the order is stable. */
fun lastModifiedFirst(allCards: List<HolderCard>): List<HolderCard> =
    allCards.sortedWith(compareByDescending<HolderCard> { it.updatedAt }.thenBy { it.cardId })

/** [cardId]'s deck for swipeable card navigation (see CardViewPagerScreen): its siblings in
 * Holder's existing sort_key order, including itself. Null when [cardId] isn't found in
 * [allCards] -- a race or a failed fetch -- leaving the caller to decide what to fall back to,
 * rather than this function quietly returning a single-card deck that would look like a real
 * answer instead of a miss. */
fun cardDeck(cardId: String, allCards: List<HolderCard>): List<HolderCard>? {
    val current = allCards.find { it.cardId == cardId } ?: return null
    return sortKeyOrderedSiblings(current.parentCardId, allCards).ifEmpty { listOf(current) }
}

/** Where [cardId] sits within its own [deck] -- the page a swipeable card-navigation pager
 * should open on. Falls back to the first page if [cardId] is somehow missing from [deck],
 * which shouldn't happen in practice: a deck built by [cardDeck] always includes the card it
 * was built from. */
fun initialDeckPage(cardId: String, deck: List<HolderCard>): Int =
    deck.indexOfFirst { it.cardId == cardId }.coerceAtLeast(0)

fun cardSequenceLinks(cardId: String, parentCardId: String?, allCards: List<HolderCard>): CardSequenceLinks {
    val byRecency = allCards.sortedByDescending { it.updatedAt }
    val recencyIndex = byRecency.indexOfFirst { it.cardId == cardId }
    val next = recencyIndex.takeIf { it >= 0 }?.let { byRecency.getOrNull(it - 1) }
    val previous = recencyIndex.takeIf { it >= 0 }?.let { byRecency.getOrNull(it + 1) }

    val bySortKey = sortKeyOrderedSiblings(parentCardId, allCards)
    val sortKeyIndex = bySortKey.indexOfFirst { it.cardId == cardId }
    // Higher sort_key comes after (newer/"follows"); lower comes before (older/"precedes").
    val newerSibling = sortKeyIndex.takeIf { it >= 0 }?.let { bySortKey.getOrNull(it + 1) }
    val olderSibling = sortKeyIndex.takeIf { it >= 0 }?.let { bySortKey.getOrNull(it - 1) }

    return CardSequenceLinks(
        next = next,
        previous = previous,
        follows = newerSibling?.takeIf { it.cardId != next?.cardId },
        precedes = olderSibling?.takeIf { it.cardId != previous?.cardId },
    )
}

/** What a swipe deck should show after its cards were reloaded: [deck], opened on [cardId]. */
data class DeckAfterReload(val deck: List<HolderCard>, val cardId: String)

/**
 * Keeps a swipe deck on the same card, by identity, when its project's cards are reloaded --
 * after a sync brought changes from another device, say (issue #104). [shownCardId] is the card
 * on screen; [previousDeck] the deck it was in.
 *
 * - Still there: its deck as it is now, wherever it now lives (moved under another parent, it
 *   is shown among its new siblings).
 * - Gone (deleted, trashed, or under a parent that's gone): the nearest card after it in
 *   [previousDeck] that's still under the same parent, then the nearest before it; failing
 *   that, the nearest one still there at all.
 * - Null when nothing in [previousDeck] is left, and the caller leaves the deck.
 */
fun deckAfterReload(
    shownCardId: String,
    previousDeck: List<HolderCard>?,
    allCards: List<HolderCard>,
): DeckAfterReload? {
    val reachable = reachableCards(allCards)
    cardDeck(shownCardId, reachable)?.let { return DeckAfterReload(it, shownCardId) }
    val old = previousDeck ?: return null
    val index = old.indexOfFirst { it.cardId == shownCardId }
    val oldParent = old.getOrNull(index)?.parentCardId ?: old.firstOrNull()?.parentCardId
    val nearestFirst = if (index < 0) old.indices.toList() else (index + 1 until old.size) + (index - 1 downTo 0)
    val survivors = nearestFirst.mapNotNull { i -> reachable.find { it.cardId == old[i].cardId } }
    val neighbour = survivors.firstOrNull { it.parentCardId == oldParent } ?: survivors.firstOrNull() ?: return null
    return DeckAfterReload(requireNotNull(cardDeck(neighbour.cardId, reachable)), neighbour.cardId)
}

/**
 * Reconciles Board's open folder with reloaded cards (issue #104). Returns the folder to show
 * and its breadcrumb trail of (cardId, title) from the root down, rebuilt from the cards
 * themselves so a folder moved or renamed on another device shows its real place and name.
 * When the folder is gone or no longer reachable from the root, falls back to the deepest
 * folder in [breadcrumbs] that still is; to the root (null, empty) when none is.
 */
fun boardPathAfterReload(
    currentParentId: String?,
    breadcrumbs: List<Pair<String, String>>,
    allCards: List<HolderCard>,
): Pair<String?, List<Pair<String, String>>> {
    if (currentParentId == null) return null to emptyList()
    val byId = allCards.filter { it.deletedAt == null }.associateBy { it.cardId }
    fun trail(cardId: String): List<Pair<String, String>>? {
        val path = ArrayList<Pair<String, String>>()
        var id: String? = cardId
        while (id != null) {
            val card = byId[id] ?: return null
            if (path.size > byId.size) return null // a cycle: treat as unreachable
            path += card.cardId to card.title
            id = card.parentCardId
        }
        return path.asReversed()
    }
    trail(currentParentId)?.let { return currentParentId to it }
    for ((cardId, _) in breadcrumbs.asReversed()) {
        trail(cardId)?.let { return cardId to it }
    }
    return null to emptyList()
}

/** The live cards that can be reached from the root: none trashed, and every ancestor live. */
private fun reachableCards(allCards: List<HolderCard>): List<HolderCard> {
    val byId = allCards.filter { it.deletedAt == null }.associateBy { it.cardId }
    val reachable = HashMap<String, Boolean>()
    fun isReachable(card: HolderCard, depth: Int = 0): Boolean = reachable.getOrPut(card.cardId) {
        val parentId = card.parentCardId ?: return@getOrPut true
        val parent = byId[parentId] ?: return@getOrPut false
        depth < byId.size && isReachable(parent, depth + 1)
    }
    return allCards.filter { it.deletedAt == null && isReachable(it) }
}
