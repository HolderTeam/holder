package team.holder.android.ui

import team.holder.android.HolderCard

/** A card that Board's "Move into…" can put another card under, with the titles of the cards
 * above it (root first), so two cards with one title can be told apart. */
data class MoveIntoTarget(val card: HolderCard, val path: List<String>) {
    /** "Alpha ▸ Beta", or empty at the root. */
    val pathLabel: String get() = path.joinToString(" ▸ ")
}

/**
 * The cards [moving] can move into, in Board's own order: depth first, each folder's children
 * in [sortKeyOrderedSiblings] order. Leaves out [moving] itself and everything under it (core
 * refuses a card under its own descendant), the card it's already in (nothing would change),
 * and trashed cards and cards under them.
 */
fun moveIntoTargets(moving: HolderCard, allCards: List<HolderCard>): List<MoveIntoTarget> {
    val live = allCards.filter { it.deletedAt == null }
    val targets = ArrayList<MoveIntoTarget>()
    val visited = HashSet<String>()
    fun walk(parentId: String?, path: List<String>) {
        for (card in sortKeyOrderedSiblings(parentId, live)) {
            // Skipping the moving card skips everything under it too.
            if (card.cardId == moving.cardId || !visited.add(card.cardId)) continue
            if (card.cardId != moving.parentCardId) targets += MoveIntoTarget(card, path)
            walk(card.cardId, path + card.title)
        }
    }
    walk(null, emptyList())
    return targets
}

/** [targets] whose title or path contains [query], ignoring case; all of them for a blank one. */
fun filterMoveIntoTargets(targets: List<MoveIntoTarget>, query: String): List<MoveIntoTarget> {
    val text = query.trim()
    if (text.isEmpty()) return targets
    return targets.filter { it.card.title.contains(text, ignoreCase = true) || it.pathLabel.contains(text, ignoreCase = true) }
}
