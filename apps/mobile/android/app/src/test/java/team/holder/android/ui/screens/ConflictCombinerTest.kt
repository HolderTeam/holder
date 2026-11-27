package team.holder.android.ui.screens

import org.junit.Assert.assertEquals
import org.junit.Test

/** The Desktop's cases (git_sync_rules_test.vala), with lines counted from zero. */
class ConflictCombinerTest {
    private fun plain(a: String, aChanged: Set<Int>, b: String, bChanged: Set<Int>) =
        combineConflictVersions(a, aChanged, b, bChanged)

    @Test
    fun keepsBothVersions_sharedLinesOnce_firstVersionFirst() {
        // A clash in the middle.
        assertEquals("one\ntwo\nTWO\nthree", plain("one\ntwo\nthree", setOf(1), "one\nTWO\nthree", setOf(1)))
        // At the start and at the end, with a trailing newline on both.
        assertEquals("a\nA\nb\nc\nC\n", plain("a\nb\nc\n", setOf(0, 2), "A\nb\nC\n", setOf(0, 2)))
        // A line only one version has.
        assertEquals("a\nb\nc", plain("a\nc", emptySet(), "a\nb\nc", setOf(1)))
        // Several lines on one side against one on the other.
        assertEquals("x\n1\n2\nboth\ny", plain("x\n1\n2\ny", setOf(1, 2), "x\nboth\ny", setOf(1)))
        // CRLF lines keep their "\r".
        assertEquals("a\r\nb\r\nB\r\n", plain("a\r\nb\r\n", setOf(1), "a\r\nB\r\n", setOf(1)))
        // Nothing differs.
        assertEquals("same\n", plain("same\n", emptySet(), "same\n", emptySet()))
        // Empty versions.
        assertEquals("new", plain("", emptySet(), "new", setOf(0)))
    }

    @Test
    fun markers_wrapEachClash_withTheLabelsGiven() {
        assertEquals(
            "one\n<<<<<<< this device\ntwo\n=======\nTWO\n>>>>>>> another device\nthree",
            combineConflictVersions("one\ntwo\nthree", setOf(1), "one\nTWO\nthree", setOf(1), true, "this device", "another device"),
        )
        assertEquals(
            "a\n<<<<<<< A\n=======\nb\n>>>>>>> B\nc",
            combineConflictVersions("a\nc", emptySet(), "a\nb\nc", setOf(1), true, "A", "B"),
        )
    }
}
