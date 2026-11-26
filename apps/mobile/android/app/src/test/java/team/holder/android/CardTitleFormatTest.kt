package team.holder.android

import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test

class CardTitleFormatTest {
    @Test
    fun titleFromFirstLine_usesFirstNonBlankLine() {
        assertEquals("A title", titleFromFirstLine("\n  A title  \nbody"))
    }

    @Test
    fun titleFromFirstLine_stripsMarkdownHeadingMarkers() {
        assertEquals("A title", titleFromFirstLine("### A title\nbody"))
    }

    @Test
    fun titleFromFirstLine_acceptsHeadingLevelsOneThroughSix() {
        for (level in 1..6) {
            assertEquals("Title $level", titleFromFirstLine("${"#".repeat(level)} Title $level"))
        }
    }

    @Test
    fun titleFromFirstLine_doesNotTreatHashWithoutFollowingWhitespaceAsHeading() {
        assertEquals("#Not a heading", titleFromFirstLine("#Not a heading"))
    }

    @Test
    fun titleFromFirstLine_handlesEmptyAndUnicodeContent() {
        assertEquals("", titleFromFirstLine(" \n\t\n"))
        assertEquals("Привет 🌍", titleFromFirstLine("Привет 🌍\r\nbody"))
    }

    @Test
    fun titleFromFirstLine_skipsALeadingResourceReferenceLine() {
        // Attaching a photo as the very first thing typed into a fresh card (see
        // ensureCardCreated in CardEditScreen.kt) would otherwise hand back the raw
        // `![label](holder://resource/<id>)` syntax itself as the "title".
        assertEquals(
            "Real title",
            titleFromFirstLine("![photo](holder://resource/abc-123)\n\nReal title\nbody"),
        )
        assertEquals("", titleFromFirstLine("![photo](holder://resource/abc-123)"))
    }

    @Test
    fun splitLeadingHeading_returnsBodyWithoutHeadingAndOneBlankSeparator() {
        assertEquals("Body\nnext", splitLeadingHeading("# Title\n\nBody\nnext"))
    }

    @Test
    fun splitLeadingHeading_returnsEmptyBodyForHeadingOnlyContent() {
        assertEquals("", splitLeadingHeading("# Title"))
    }

    @Test
    fun splitLeadingHeading_returnsNullWhenContentDoesNotStartWithHeading() {
        assertNull(splitLeadingHeading("Body\n# A later heading"))
        assertNull(splitLeadingHeading("##Title"))
    }

    @Test
    fun combineTitleAndBody_writesHolderHeadingConvention() {
        assertEquals("# Title\n\nBody", combineTitleAndBody("Title", "Body"))
        assertEquals("# Title\n\n", combineTitleAndBody("Title", ""))
    }

    @Test
    fun splitAndCombine_preserveRepresentativeSeparateTitleContent() {
        val stored = combineTitleAndBody("My card", "The body")

        assertEquals("The body", splitLeadingHeading(stored))
        assertEquals("My card", titleFromFirstLine(stored))
    }

    @Test
    fun splitTitleLine_takesOffAHeading() {
        assertEquals(TitleSplit("Body", asHeading = true), splitTitleLine("# Trip plan\n\nBody", "Trip plan"))
    }

    @Test
    fun splitTitleLine_takesOffAPlainFirstLineThatIsTheTitle() {
        // Cards from Desktop or holderctl: the title is the plain first line.
        assertEquals(TitleSplit("Body\nmore", asHeading = false), splitTitleLine("Trip plan\n\nBody\nmore", "Trip plan"))
        assertEquals(TitleSplit("", asHeading = false), splitTitleLine("Trip plan", "Trip plan"))
    }

    @Test
    fun splitTitleLine_leavesABodyWhoseFirstLineIsNotTheTitle() {
        assertNull(splitTitleLine("Something else\n\nBody", "Trip plan"))
        assertNull(splitTitleLine("Body", ""))
    }

    @Test
    fun combineTitleAndBody_writesAPlainTitleBackPlain() {
        assertEquals("Trip plan\n\nBody", combineTitleAndBody("Trip plan", "Body", asHeading = false))
        val stored = "Ideas\n\nA loose note."
        val split = requireNotNull(splitTitleLine(stored, "Ideas"))
        // Round trip: nothing added, no "# Ideas" above "Ideas".
        assertEquals(stored, combineTitleAndBody("Ideas", split.body, split.asHeading))
    }
}
