package team.holder.android.ui

import org.junit.Assert.assertEquals
import org.junit.Test
import team.holder.android.HolderCard

class MoveIntoTargetsTest {
    private fun card(id: String, parent: String? = null, sortKey: Double = 0.0, deletedAt: Long? = null) = HolderCard(
        cardId = id, projectId = "p", title = id.replaceFirstChar { it.uppercase() }, parentCardId = parent,
        createdAt = 0, updatedAt = 0, sortKey = sortKey, deletedAt = deletedAt,
    )

    // alpha
    //   beta
    //     gamma
    //   delta
    // echo (trashed)
    //   foxtrot
    // golf
    private val cards = listOf(
        card("golf", sortKey = 3.0),
        card("alpha", sortKey = 1.0),
        card("delta", parent = "alpha", sortKey = 2.0),
        card("beta", parent = "alpha", sortKey = 1.0),
        card("gamma", parent = "beta"),
        card("echo", sortKey = 2.0, deletedAt = 5),
        card("foxtrot", parent = "echo"),
    )

    private fun ids(targets: List<MoveIntoTarget>) = targets.map { it.card.cardId }

    @Test
    fun listsEveryLiveCardInBoardOrder_withItsPath() {
        val targets = moveIntoTargets(card("golf", sortKey = 3.0), cards)
        assertEquals(listOf("alpha", "beta", "gamma", "delta"), ids(targets))
        assertEquals(listOf("", "Alpha", "Alpha ▸ Beta", "Alpha"), targets.map { it.pathLabel })
    }

    @Test
    fun leavesOutTheCardItself_everythingUnderIt_andTheCardItIsAlreadyIn() {
        // Beta is in Alpha, and Gamma is under Beta.
        assertEquals(listOf("delta", "golf"), ids(moveIntoTargets(card("beta", parent = "alpha", sortKey = 1.0), cards)))
    }

    @Test
    fun filter_matchesTitleOrPath_ignoringCase() {
        val targets = moveIntoTargets(card("golf", sortKey = 3.0), cards)
        assertEquals(listOf("beta", "gamma"), ids(filterMoveIntoTargets(targets, "BETA")))
        assertEquals(ids(targets), ids(filterMoveIntoTargets(targets, "  ")))
    }
}
