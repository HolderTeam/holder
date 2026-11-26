package team.holder.android.ui.screens

import java.time.LocalDate
import java.time.ZoneId
import java.time.ZoneOffset
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test
import team.holder.android.HolderMilestone
import team.holder.android.HolderProject

class StartScreenDataTest {
    private val zone = ZoneId.of("Europe/London")
    private val today = LocalDate.of(2026, 11, 25) // a Wednesday
    private val home = HolderProject("p-home", "Home", null, "plain")
    private val work = HolderProject("p-work", "Work", null, "plain")

    private fun allDay(id: String, day: LocalDate, kind: String? = null, description: String? = null, end: LocalDate? = null) =
        HolderMilestone(
            milestoneId = id,
            cardId = "card-$id",
            startAt = day.atStartOfDay().toEpochSecond(ZoneOffset.UTC),
            endAt = end?.atStartOfDay()?.toEpochSecond(ZoneOffset.UTC),
            allDay = true,
            kind = kind,
            description = description,
            createdAt = 0,
            updatedAt = 0,
            cardTitle = "Card $id",
        )

    @Test
    fun cardSnippet_skipsTitleHeadingAndBlankLines() {
        assertEquals("Oat milk, coffee", cardSnippet("# Shopping\n\nOat milk, coffee\nmore", "Shopping"))
    }

    @Test
    fun cardSnippet_usesFirstLineWhenItIsNotTheTitle() {
        assertEquals("Body first", cardSnippet("Body first\nsecond", "Separate title"))
    }

    @Test
    fun cardSnippet_stripsListQuoteAndTaskMarkers() {
        assertEquals("Ring the plumber", cardSnippet("# T\n- [ ] Ring the plumber", "T"))
        assertEquals("A quote", cardSnippet("# T\n> A quote", "T"))
        assertEquals("Numbered", cardSnippet("# T\n1. Numbered", "T"))
    }

    @Test
    fun cardSnippet_skipsResourceLinesAndCodeFences() {
        assertEquals("After", cardSnippet("# T\n![photo](holder://resource/1)\n```\nAfter", "T"))
    }

    @Test
    fun cardSnippet_emptyWhenOnlyTitle() {
        assertEquals("", cardSnippet("# Only a title\n\n", "Only a title"))
    }

    @Test
    fun mergeRecentCards_ordersAcrossProjectsAndLimits() {
        val cards = listOf(
            StartRecentCard("p1", "Home", "a", "A", "", 100),
            StartRecentCard("p2", "Work", "b", "B", "", 300),
            StartRecentCard("p1", "Home", "c", "C", "", 200),
        )
        assertEquals(listOf("b", "c"), mergeRecentCards(cards, 2).map { it.cardId })
    }

    @Test
    fun attention_includesTodayAndUpcomingWithinWindow() {
        val result = attentionMilestones(
            listOf(
                home to allDay("today", today),
                home to allDay("in3", today.plusDays(3)),
                home to allDay("in4", today.plusDays(4)),
            ),
            today,
            zone,
        )
        assertEquals(listOf("today", "in3"), result.map { it.milestoneId })
        assertEquals("Today", result[0].dueLabel)
    }

    @Test
    fun attention_pastDueDateWithNoKindIsOverdueAndLabelledDue() {
        val due = allDay("due", today.minusDays(1)).copy(role = "due", hasSpan = false, cardTitle = "Tax return")
        val result = attentionMilestones(listOf(home to due), today, zone)
        assertTrue(result.single().overdue)
        assertEquals("Due: Tax return", result.single().label)
    }

    @Test
    fun attention_pastDeadlineIsOverdueButPastAppointmentIsDropped() {
        val result = attentionMilestones(
            listOf(
                work to allDay("deadline", today.minusDays(2), kind = "Deadline"),
                home to allDay("appointment", today.minusDays(1), kind = "Appointment"),
                work to allDay("tooOld", today.minusDays(8), kind = "Deadline"),
            ),
            today,
            zone,
        )
        assertEquals(listOf("deadline"), result.map { it.milestoneId })
        assertTrue(result.single().overdue)
        assertEquals("Overdue 2 days", result.single().dueLabel)
    }

    @Test
    fun attention_ongoingSpanCountsAsToday() {
        val result = attentionMilestones(
            listOf(home to allDay("trip", today.minusDays(2), kind = "Event", end = today.plusDays(1))),
            today,
            zone,
        )
        assertEquals("Today", result.single().dueLabel)
    }

    @Test
    fun attention_overdueFirstThenByDate() {
        val result = attentionMilestones(
            listOf(
                home to allDay("tomorrow", today.plusDays(1)),
                home to allDay("today", today),
                work to allDay("late", today.minusDays(1), kind = "deadline"),
            ),
            today,
            zone,
        )
        assertEquals(listOf("late", "today", "tomorrow"), result.map { it.milestoneId })
    }

    @Test
    fun attention_labelPrefersDescriptionThenKindAndCardTitle() {
        val result = attentionMilestones(
            listOf(
                home to allDay("a", today, description = "Submit draft"),
                home to allDay("b", today.plusDays(1), kind = "MOT"),
            ),
            today,
            zone,
        )
        assertEquals(listOf("Submit draft", "MOT: Card b"), result.map { it.label })
    }

    @Test
    fun dueLabel_namesNearDays() {
        assertEquals("Today", dueLabel(today, today))
        assertEquals("Tomorrow", dueLabel(today.plusDays(1), today))
        assertEquals(today.plusDays(2).dayOfWeek.getDisplayName(java.time.format.TextStyle.SHORT, java.util.Locale.getDefault()),
            dueLabel(today.plusDays(2), today))
    }

    @Test
    fun overdueLabel_singularAndPlural() {
        assertEquals("Overdue 1 day", overdueLabel(today.minusDays(1), today))
        assertEquals("Overdue 3 days", overdueLabel(today.minusDays(3), today))
    }
}
