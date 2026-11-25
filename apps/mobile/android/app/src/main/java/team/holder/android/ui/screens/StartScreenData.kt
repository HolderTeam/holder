package team.holder.android.ui.screens

import java.time.DayOfWeek
import java.time.Instant
import java.time.LocalDate
import java.time.ZoneId
import java.time.ZoneOffset
import java.time.format.TextStyle
import java.time.temporal.ChronoUnit
import java.util.Locale
import team.holder.android.HolderMilestone
import team.holder.android.HolderNative
import team.holder.android.HolderProject
import team.holder.android.contract.ListCardsOrder

/** How many recently edited cards the start screen lists, across all projects. */
internal const val START_RECENT_LIMIT = 30

/** How far back an unfinished deadline-like milestone still counts as overdue. */
internal const val START_OVERDUE_DAYS = 7L

/** How far ahead an upcoming milestone counts as imminent (today plus this many days). */
internal const val START_UPCOMING_DAYS = 3L

/** Milestone kinds that are "things to do by a date": only these are shown once their day has
 * passed. Anything else (an appointment, a birthday) simply drops off the strip afterwards. */
private val OVERDUE_KINDS = setOf("deadline", "expiry", "renewal", "mot", "service", "exam")

data class StartRecentCard(
    val projectId: String,
    val projectName: String,
    val cardId: String,
    val title: String,
    val snippet: String,
    val updatedAt: Long,
)

data class StartMilestone(
    val projectId: String,
    val projectName: String,
    val cardId: String,
    val cardTitle: String,
    val milestoneId: String,
    val label: String,
    val dueLabel: String,
    val overdue: Boolean,
    val day: LocalDate,
    val startAt: Long,
)

data class StartScreenData(
    val projects: List<HolderProject>,
    val recentCards: List<StartRecentCard>,
    val milestones: List<StartMilestone>,
)

/** Loads everything the start screen shows. Blocking: call from Dispatchers.IO. */
internal fun loadStartScreenData(
    today: LocalDate = LocalDate.now(),
    zone: ZoneId = ZoneId.systemDefault(),
): StartScreenData {
    val projects = HolderNative.listProjects()

    val recent = projects.flatMap { project ->
        HolderNative.listCardsPage(
            project.projectId,
            order = ListCardsOrder.UPDATED_DESC,
            limit = START_RECENT_LIMIT,
            includeContent = true,
        ).items
            .filter { it.deletedAt == null }
            .map { card ->
                StartRecentCard(
                    projectId = project.projectId,
                    projectName = project.name,
                    cardId = card.cardId,
                    title = card.title,
                    snippet = cardSnippet(card.content.orEmpty(), card.title),
                    updatedAt = card.updatedAt,
                )
            }
    }
    val recentCards = mergeRecentCards(recent, START_RECENT_LIMIT)

    val from = today.minusDays(START_OVERDUE_DAYS).atStartOfDay(zone).toEpochSecond()
    val to = today.plusDays(START_UPCOMING_DAYS).atStartOfDay(zone).toEpochSecond()
    val milestones = projects.flatMap { project ->
        HolderNative.listMilestonesInRange(project.projectId, from, to).map { project to it }
    }
    return StartScreenData(projects, recentCards, attentionMilestones(milestones, today, zone))
}

/** Most recently edited first across every project, at most [limit]. */
internal fun mergeRecentCards(cards: List<StartRecentCard>, limit: Int): List<StartRecentCard> =
    cards.sortedByDescending { it.updatedAt }.take(limit)

private val ATX_HEADING = Regex("^#{1,6}[ \t]+")
private val LIST_MARKER = Regex("^([-*+]|\\d+[.)])[ \t]+(\\[[ xX]][ \t]+)?")
private val RESOURCE_LINE = Regex("^!\\[[^\\]]*]\\([^)]*\\)$")

/** One line of preview text for a card: the first non-blank body line that isn't the title
 * itself, with leading Markdown markers (heading, list, quote) stripped. Empty if none. */
internal fun cardSnippet(content: String, title: String): String {
    var titleSkipped = false
    for (raw in content.lineSequence()) {
        var line = raw.trim()
        if (line.isEmpty() || RESOURCE_LINE.matches(line) || line.startsWith("```")) continue
        line = line.removePrefix(">").trim()
        line = ATX_HEADING.replace(line, "")
        line = LIST_MARKER.replace(line, "")
        if (line.isEmpty()) continue
        if (!titleSkipped && line == title.trim()) {
            titleSkipped = true
            continue
        }
        titleSkipped = true
        return line.take(200)
    }
    return ""
}

/**
 * Milestones worth interrupting the user for: anything from today up to [START_UPCOMING_DAYS]
 * ahead (or still running today), plus deadline-like ones from the last [START_OVERDUE_DAYS]
 * days that have passed. Overdue first (oldest first), then upcoming in date order.
 */
internal fun attentionMilestones(
    entries: List<Pair<HolderProject, HolderMilestone>>,
    today: LocalDate,
    zone: ZoneId,
): List<StartMilestone> {
    val lastUpcoming = today.plusDays(START_UPCOMING_DAYS)
    val firstOverdue = today.minusDays(START_OVERDUE_DAYS)
    return entries.mapNotNull { (project, m) ->
        val start = m.day(m.startAt, zone)
        val end = m.endAt?.let { m.day(it, zone) } ?: start
        val ongoing = !start.isAfter(today) && !end.isBefore(today)
        val upcoming = !start.isBefore(today) && !start.isAfter(lastUpcoming)
        val overdue = end.isBefore(today) && !end.isBefore(firstOverdue) &&
            m.kind?.trim()?.lowercase() in OVERDUE_KINDS
        if (!ongoing && !upcoming && !overdue) return@mapNotNull null
        StartMilestone(
            projectId = project.projectId,
            projectName = project.name,
            cardId = m.cardId,
            cardTitle = m.cardTitle.orEmpty(),
            milestoneId = m.milestoneId,
            label = milestoneLabel(m),
            dueLabel = if (overdue) overdueLabel(end, today) else dueLabel(if (ongoing) today else start, today),
            overdue = overdue,
            day = if (overdue) end else start,
            startAt = m.startAt,
        )
    }.sortedWith(compareByDescending<StartMilestone> { it.overdue }.thenBy { it.day }.thenBy { it.startAt })
}

/** An all-day date is held as UTC midnight, so it's read in UTC; a timed one in the device zone. */
private fun HolderMilestone.day(epochSeconds: Long, zone: ZoneId): LocalDate =
    if (allDay) Instant.ofEpochSecond(epochSeconds).atOffset(ZoneOffset.UTC).toLocalDate()
    else Instant.ofEpochSecond(epochSeconds).atZone(zone).toLocalDate()

private fun milestoneLabel(m: HolderMilestone): String =
    m.description?.takeIf { it.isNotBlank() }
        ?: listOfNotNull(m.kind?.takeIf { it.isNotBlank() }, m.cardTitle?.takeIf { it.isNotBlank() })
            .joinToString(": ")
            .ifEmpty { "Milestone" }

internal fun dueLabel(day: LocalDate, today: LocalDate): String {
    val days = ChronoUnit.DAYS.between(today, day)
    return when {
        days == 0L -> "Today"
        days == 1L -> "Tomorrow"
        days in 2..6 -> day.dayOfWeek.shortName()
        else -> day.toString()
    }
}

internal fun overdueLabel(day: LocalDate, today: LocalDate): String {
    val days = ChronoUnit.DAYS.between(day, today)
    return if (days == 1L) "Overdue 1 day" else "Overdue $days days"
}

private fun DayOfWeek.shortName(): String = getDisplayName(TextStyle.SHORT, Locale.getDefault())
