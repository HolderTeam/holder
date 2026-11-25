package team.holder.android.ui.screens

import org.junit.Assert.assertEquals
import org.junit.Test
import team.holder.android.conflictCopyBaseTitle
import team.holder.android.contract.Card
import team.holder.android.contract.ConflictChangedLine
import team.holder.android.contract.ConflictComparison
import team.holder.android.contract.ConflictComparisonCopyEditedOn
import team.holder.android.contract.ConflictComparisonOriginalEditedOn

class ConflictScreensTest {
    private fun card(id: String, title: String, content: String) = Card(
        cardId = id, projectId = "p", title = title, relPath = "cards/$id.md", sortKey = 1.0,
        createdAt = 1, updatedAt = 2, content = content, tags = emptyList(), tagOccurrences = emptyList(),
        removableTags = emptyList(),
    )

    private fun comparison(
        original: ConflictComparisonOriginalEditedOn,
        copy: ConflictComparisonCopyEditedOn,
    ) = ConflictComparison(
        original = card("o", "Hamlet", "Enter Hamlet.\nTo be, or not to be, that is the question:"),
        copyCard = card("c", "Hamlet (conflicted copy)", "Enter Hamlet.\nTo be, or not to be, ay, there's the point,"),
        originalChangedLines = listOf(ConflictChangedLine(2)),
        copyChangedLines = listOf(ConflictChangedLine(2)),
        truncated = false,
        originalEditedOn = original,
        copyEditedOn = copy,
        reviewToken = "token",
    )

    @Test
    fun sides_areLabelledByTheDeviceThatMadeThem_cardFirst() {
        val (card, copy) = conflictSides(
            comparison(ConflictComparisonOriginalEditedOn.ANOTHER_DEVICE, ConflictComparisonCopyEditedOn.THIS_DEVICE),
        )
        assertEquals("Other device", card.label)
        assertEquals("This device", copy.label)
        // Zero-based, for indexing the content's lines.
        assertEquals(setOf(1), card.changedLines)
    }

    @Test
    fun copySide_usesTheTitleWithoutTheConflictedCopySuffix() {
        val (card, copy) = conflictSides(
            comparison(ConflictComparisonOriginalEditedOn.ANOTHER_DEVICE, ConflictComparisonCopyEditedOn.THIS_DEVICE),
        )
        // Keeping this device's version must not rename the card "Hamlet (conflicted copy)".
        assertEquals("Hamlet", copy.title)
        assertEquals(card.title, copy.title)
    }

    @Test
    fun sides_fallBackToCardAndCopy_whenTheDeviceIsUnknown() {
        val (card, copy) = conflictSides(
            comparison(ConflictComparisonOriginalEditedOn.UNKNOWN, ConflictComparisonCopyEditedOn.UNKNOWN),
        )
        assertEquals("The card", card.label)
        assertEquals("The copy", copy.label)
    }

    @Test
    fun sides_sayWhichIsWhich_whenBothCameFromTheSameDevice() {
        val (card, copy) = conflictSides(
            comparison(ConflictComparisonOriginalEditedOn.THIS_DEVICE, ConflictComparisonCopyEditedOn.THIS_DEVICE),
        )
        assertEquals("This device (the card)", card.label)
        assertEquals("This device (the copy)", copy.label)
    }

    @Test
    fun baseTitle_dropsOnlyATrailingSuffix() {
        assertEquals("Trip plan", conflictCopyBaseTitle("Trip plan (conflicted copy)"))
        assertEquals("Notes (conflicted copy) draft", conflictCopyBaseTitle("Notes (conflicted copy) draft"))
        assertEquals("(conflicted copy)", conflictCopyBaseTitle("(conflicted copy)"))
    }

    @Test
    fun keepQuestion_namesTheDevice() {
        assertEquals("Keep this device's version?", keepQuestion("This device"))
        assertEquals("Keep the other device's version?", keepQuestion("Other device"))
        assertEquals("Keep this version?", keepQuestion("The copy"))
    }
}
