package team.holder.android.ui

import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test
import team.holder.android.HolderCard

private fun card(
    id: String,
    parentCardId: String? = null,
    createdAt: Long = 0L,
    updatedAt: Long = createdAt,
    sortKey: Double = 0.0,
    deletedAt: Long? = null,
) = HolderCard(
    cardId = id,
    projectId = "project-1",
    title = id,
    parentCardId = parentCardId,
    createdAt = createdAt,
    updatedAt = updatedAt,
    sortKey = sortKey,
    deletedAt = deletedAt,
)

class CardSequenceTest {
    @Test
    fun cardDeck_isTheCardsSiblingsInSortKeyOrderIncludingItself() {
        val first = card("first", parentCardId = "parent", sortKey = 0.0)
        val middle = card("middle", parentCardId = "parent", sortKey = 1.0)
        val last = card("last", parentCardId = "parent", sortKey = 2.0)
        val stranger = card("stranger", parentCardId = "other-parent", sortKey = 1.5)
        val all = listOf(last, stranger, first, middle)

        val deck = cardDeck("middle", all)

        assertEquals(listOf(first, middle, last), deck)
    }

    @Test
    fun cardDeck_pagesRootCardsAmongOtherRootSiblings() {
        val root1 = card("root1", parentCardId = null, sortKey = 0.0)
        val root2 = card("root2", parentCardId = null, sortKey = 1.0)
        val childOfSomethingElse = card("child", parentCardId = "someone")
        val all = listOf(root1, root2, childOfSomethingElse)

        val deck = cardDeck("root1", all)

        assertEquals(listOf(root1, root2), deck)
    }

    @Test
    fun cardDeck_isJustTheCardItselfForAnOnlyChild() {
        val onlyChild = card("only", parentCardId = "parent")
        val all = listOf(onlyChild)

        assertEquals(listOf(onlyChild), cardDeck("only", all))
    }

    @Test
    fun cardDeck_isNullWhenTheCardIsMissingFromAllCards() {
        val all = listOf(card("a"), card("b"))

        assertNull(cardDeck("missing", all))
    }

    @Test
    fun initialDeckPage_isWhereTheCardSitsInItsOwnDeck() {
        val deck = listOf(card("a"), card("b"), card("c"))

        assertEquals(1, initialDeckPage("b", deck))
    }

    @Test
    fun initialDeckPage_fallsBackToTheFirstPageWhenTheCardIsntInTheDeck() {
        val deck = listOf(card("a"), card("b"))

        assertEquals(0, initialDeckPage("missing", deck))
    }

    @Test
    fun nextAndPrevious_walkAllCardsByRecencyRegardlessOfParent() {
        val a = card("a", updatedAt = 1L)
        val b = card("b", parentCardId = "other", updatedAt = 2L)
        val c = card("c", updatedAt = 3L)
        val all = listOf(a, b, c)

        val links = cardSequenceLinks("b", "other", all)

        assertEquals(c, links.next)
        assertEquals(a, links.previous)
    }

    @Test
    fun nextAndPrevious_areNullAtEitherEndOfRecencyOrder() {
        val oldest = card("oldest", updatedAt = 1L)
        val newest = card("newest", updatedAt = 2L)
        val all = listOf(oldest, newest)

        assertNull(cardSequenceLinks("newest", null, all).next)
        assertNull(cardSequenceLinks("oldest", null, all).previous)
    }

    @Test
    fun nextAndPrevious_areNullWhenCardIsMissingFromTheList() {
        val all = listOf(card("a", updatedAt = 1L), card("b", updatedAt = 2L))

        val links = cardSequenceLinks("missing", null, all)

        assertNull(links.next)
        assertNull(links.previous)
    }

    @Test
    fun followsAndPrecedes_walkOnlySiblingsSharingTheSameParentBySortKey() {
        val first = card("first", parentCardId = "parent", sortKey = 0.0)
        val middle = card("middle", parentCardId = "parent", sortKey = 1.0)
        val last = card("last", parentCardId = "parent", sortKey = 2.0)
        val stranger = card("stranger", parentCardId = "other-parent", sortKey = 1.5)
        val all = listOf(first, middle, last, stranger)

        val links = cardSequenceLinks("middle", "parent", all)

        assertEquals(last, links.follows)
        assertEquals(first, links.precedes)
    }

    @Test
    fun followsAndPrecedes_areNullForAnOnlyChild() {
        val onlyChild = card("only", parentCardId = "parent", sortKey = 0.0)
        val all = listOf(onlyChild)

        val links = cardSequenceLinks("only", "parent", all)

        assertNull(links.follows)
        assertNull(links.precedes)
    }

    @Test
    fun followsAndPrecedes_breakSortKeyTiesByMostRecentlyUpdatedFirst() {
        val staleTie = card("stale-tie", parentCardId = "parent", sortKey = 1.0, updatedAt = 1L)
        val freshTie = card("fresh-tie", parentCardId = "parent", sortKey = 1.0, updatedAt = 2L)
        val subject = card("subject", parentCardId = "parent", sortKey = 0.0)
        val all = listOf(subject, staleTie, freshTie)

        val links = cardSequenceLinks("subject", "parent", all)

        assertEquals(freshTie, links.follows)
    }

    @Test
    fun followsIsDroppedWhenItWouldDuplicateNext_andPrecedesWhenItWouldDuplicatePrevious() {
        // Two siblings where sort_key order matches update-recency order, so Follows/Precedes
        // would point at the exact same card as Next/Previous.
        val older = card("older", parentCardId = "parent", sortKey = 0.0, updatedAt = 1L)
        val subject = card("subject", parentCardId = "parent", sortKey = 1.0, updatedAt = 2L)
        val newer = card("newer", parentCardId = "parent", sortKey = 2.0, updatedAt = 3L)
        val all = listOf(older, subject, newer)

        val links = cardSequenceLinks("subject", "parent", all)

        assertEquals(newer, links.next)
        assertEquals(older, links.previous)
        assertNull(links.follows)
        assertNull(links.precedes)
    }

    @Test
    fun followsAndPrecedes_surviveDedupWhenTheyDifferFromNextAndPrevious() {
        // Sibling sort order disagrees with recency order, so Follows/Precedes carry
        // information Next/Previous don't.
        val siblingNewer = card("sibling-newer", parentCardId = "parent", sortKey = 1.0, updatedAt = 1L)
        val subject = card("subject", parentCardId = "parent", sortKey = 0.0, updatedAt = 3L)
        val unrelatedMostRecent = card("unrelated-most-recent", updatedAt = 4L)
        val all = listOf(subject, siblingNewer, unrelatedMostRecent)

        val links = cardSequenceLinks("subject", "parent", all)

        assertEquals(unrelatedMostRecent, links.next)
        assertEquals(siblingNewer, links.follows)
    }

    // deckAfterReload (issue #104): a deck reloaded after a sync keeps its card by identity.

    @Test
    fun deckAfterReload_keepsTheShownCard_whenASiblingIsAddedBeforeIt() {
        val a = card("a", "p", sortKey = 1.0)
        val b = card("b", "p", sortKey = 2.0)
        val inserted = card("new", "p", sortKey = 0.5)
        val next = deckAfterReload("b", listOf(a, b), listOf(card("p"), a, b, inserted))
        assertEquals("b", next?.cardId)
        assertEquals(listOf("new", "a", "b"), next?.deck?.map { it.cardId })
    }

    @Test
    fun deckAfterReload_followsACardMovedUnderAnotherParent() {
        val moved = card("b", "q", sortKey = 1.0)
        val next = deckAfterReload("b", listOf(card("a", "p"), card("b", "p", sortKey = 1.0)),
            listOf(card("p"), card("q"), card("a", "p"), moved, card("c", "q", sortKey = 2.0)))
        assertEquals("b", next?.cardId)
        assertEquals(listOf("b", "c"), next?.deck?.map { it.cardId })
    }

    @Test
    fun deckAfterReload_movesToTheNextSibling_whenTheShownCardIsGone() {
        val old = listOf(card("a", "p", sortKey = 1.0), card("b", "p", sortKey = 2.0), card("c", "p", sortKey = 3.0))
        val next = deckAfterReload("b", old, listOf(card("p"), old[0], old[2]))
        assertEquals("c", next?.cardId)
        assertEquals(listOf("a", "c"), next?.deck?.map { it.cardId })
    }

    @Test
    fun deckAfterReload_movesToThePreviousSibling_whenTheLastCardIsTrashed() {
        val old = listOf(card("a", "p", sortKey = 1.0), card("b", "p", sortKey = 2.0))
        val next = deckAfterReload("b", old, listOf(card("p"), old[0], card("b", "p", sortKey = 2.0, deletedAt = 5)))
        assertEquals("a", next?.cardId)
    }

    @Test
    fun deckAfterReload_isNull_whenTheParentWasDeletedWithEveryCardUnderIt() {
        // A remote deletion of the parent: its children are still live rows, but unreachable.
        val old = listOf(card("a", "p", sortKey = 1.0), card("b", "p", sortKey = 2.0))
        assertNull(deckAfterReload("a", old, listOf(card("p", deletedAt = 9), old[0], old[1])))
    }

    @Test
    fun deckAfterReload_prefersASurvivorStillUnderTheSameParent() {
        val old = listOf(card("a", "p", sortKey = 1.0), card("b", "p", sortKey = 2.0), card("c", "p", sortKey = 3.0))
        val cMoved = card("c", "q")
        val next = deckAfterReload("b", old, listOf(card("p"), card("q"), old[0], cMoved))
        assertEquals("a", next?.cardId)
    }

    // boardPathAfterReload (issue #104): Board's open folder after a reload.

    @Test
    fun boardPath_staysPut_andRebuildsTheTrailFromTheCards() {
        val cards = listOf(card("top"), card("mid", "top"), card("leaf", "mid"))
        val (parent, trail) = boardPathAfterReload("leaf", listOf("top" to "old title", "mid" to "mid", "leaf" to "leaf"), cards)
        assertEquals("leaf", parent)
        assertEquals(listOf("top" to "top", "mid" to "mid", "leaf" to "leaf"), trail)
    }

    @Test
    fun boardPath_showsAFolderMovedElsewhereInItsNewPlace() {
        val cards = listOf(card("top"), card("other"), card("leaf", "other"))
        val (parent, trail) = boardPathAfterReload("leaf", listOf("top" to "top", "leaf" to "leaf"), cards)
        assertEquals("leaf", parent)
        assertEquals(listOf("other" to "other", "leaf" to "leaf"), trail)
    }

    @Test
    fun boardPath_fallsBackToTheDeepestSurvivingFolder_whenTheOpenOneIsDeleted() {
        val cards = listOf(card("top"), card("mid", "top"))
        val (parent, trail) = boardPathAfterReload("leaf", listOf("top" to "top", "mid" to "mid", "leaf" to "leaf"), cards)
        assertEquals("mid", parent)
        assertEquals(listOf("top" to "top", "mid" to "mid"), trail)
    }

    @Test
    fun boardPath_treatsAFolderUnderATrashedParentAsGone() {
        val cards = listOf(card("top", deletedAt = 3), card("mid", "top"))
        assertEquals(null to emptyList<Pair<String, String>>(),
            boardPathAfterReload("mid", listOf("top" to "top", "mid" to "mid"), cards))
    }
}
