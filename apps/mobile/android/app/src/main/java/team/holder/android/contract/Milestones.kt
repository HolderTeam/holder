// Generated from contract/interface by compiler. Do not edit; run make -C contract update.
package team.holder.android.contract

/** One of a milestone's dates: a role and exactly one of a day or an instant. */
data class MilestoneDate(
    /** What the date is for. */
    val role: String,
    /** The calendar day; present when the date is a day. */
    val date: String? = null,
    /** The instant; present when the date is an instant. */
    val datetime: String? = null,
)

/** A milestone on a card: its dates, which may repeat a role or be none, and what it is. */
data class Milestone(
    /** The milestone's ID. */
    val milestoneId: String,
    /** The project of the card the milestone is on. */
    val projectId: String,
    /** The card the milestone is on. */
    val cardId: String,
    /** The milestone's dates, in the order they were given. */
    val dates: List<MilestoneDate>,
    /** What kind of milestone it is; null when it has none. */
    val kind: String? = null,
    /** A note on the milestone; null when it has none. */
    val description: String? = null,
    /** When the milestone was made. */
    val createdAt: Long,
    /** When the milestone was last changed. */
    val updatedAt: Long,
)

/** A milestone on a calendar, with the title of its card. */
data class CalendarMilestone(
    /** The milestone's ID. */
    val milestoneId: String,
    /** The project of the card the milestone is on. */
    val projectId: String,
    /** The card the milestone is on. */
    val cardId: String,
    /** The milestone's dates, in the order they were given. */
    val dates: List<MilestoneDate>,
    /** What kind of milestone it is; null when it has none. */
    val kind: String? = null,
    /** A note on the milestone; null when it has none. */
    val description: String? = null,
    /** When the milestone was made. */
    val createdAt: Long,
    /** When the milestone was last changed. */
    val updatedAt: Long,
    /** The title of the card the milestone is on. */
    val cardTitle: String,
)

/** What happens in a project over a window of time: its milestones, and the cards made and changed. */
data class Calendar(
    /** The milestones with a date in the window, as a day in its days or an instant in its instants, each once, by when they were made and then by ID. */
    val milestones: List<CalendarMilestone>,
    /** The live cards made in the window, by when they were made and then by ID. */
    val createdCards: List<CardMetadata>,
    /** The live cards last changed in the window, other than by being made, by when and then by ID. */
    val updatedCards: List<CardMetadata>,
)

/** A milestone to put on a card. */
data class NewMilestone(
    /** The milestone's dates, in order; empty for a milestone without a date. */
    val dates: List<MilestoneDate>,
    /** What kind of milestone it is; null or omitted for none. */
    val kind: String? = null,
    /** A note on the milestone; null or omitted for none. */
    val description: String? = null,
)

/** A change to a milestone. Each field left out keeps its value. */
data class MilestoneUpdate(
    /** The milestone's new dates, replacing all of them; empty for none. */
    val dates: List<MilestoneDate>? = null,
    /** What kind of milestone it is; null for none. */
    val kind: Change<String> = Change.Keep,
    /** A note on the milestone; null for none. */
    val description: Change<String> = Change.Keep,
)

/** The milestones operations of core's C API. Each fails with a [ContractException]. */
object Milestones {
    init {
        System.loadLibrary("holder_jni")
    }

    /**
     * List a card's milestones.
     * By when they were made, then by ID. A card in the trash or in another project is not found.
     */
    external fun listCardMilestones(
        contextHandle: Long,
        projectId: String,
        cardId: String,
    ): List<Milestone>

    /**
     * Put a milestone on a card.
     * The milestone gets a new ID. Each date must be a real day or instant. A card in the trash or in another project
     * is not found.
     */
    external fun addCardMilestone(
        contextHandle: Long,
        projectId: String,
        cardId: String,
        milestone: NewMilestone,
    ): Milestone

    /**
     * Change a milestone.
     * Each field left out keeps its value, and an update that changes nothing writes nothing. Each date must be a real
     * day or instant. A card in the trash or in another project is not found, and neither is a milestone that is not
     * on the card.
     */
    external fun updateCardMilestone(
        contextHandle: Long,
        projectId: String,
        cardId: String,
        milestoneId: String,
        update: MilestoneUpdate,
    ): Milestone

    /**
     * Take a milestone off a card.
     * Returns the card's remaining milestones, as list_card_milestones lists them. A card without the milestone is
     * left as it is. A card in the trash or in another project is not found.
     */
    external fun removeCardMilestone(
        contextHandle: Long,
        projectId: String,
        cardId: String,
        milestoneId: String,
    ): List<Milestone>

    /**
     * List a project's milestones with a day in a range.
     * The range holds first_day, last_day and the days between; it is empty when first_day is after last_day. A
     * milestone is listed once, by its first day in the range and then by ID. Milestones on cards in the trash are
     * left out.
     */
    external fun listMilestonesByDay(
        contextHandle: Long,
        projectId: String,
        firstDay: String,
        lastDay: String,
        role: String? = null,
    ): List<Milestone>

    /**
     * List a project's milestones with an instant in a range.
     * The range holds first_instant, last_instant and the instants between; it is empty when first_instant is after
     * last_instant. A milestone is listed once, by its first instant in the range and then by ID. Milestones on cards
     * in the trash are left out.
     */
    external fun listMilestonesByInstant(
        contextHandle: Long,
        projectId: String,
        firstInstant: Long,
        lastInstant: Long,
        role: String? = null,
    ): List<Milestone>

    /**
     * What happens in a project over a window of time.
     * The caller gives the window twice, as instants and as the days they fall on where the caller is, because a day
     * has no time zone: a milestone is in the window when one of its days is in the days, or one of its instants in
     * the instants. Cards are made and changed at instants. Either range is empty when its start is after its end.
     */
    external fun getCalendar(
        contextHandle: Long,
        projectId: String,
        firstInstant: Long,
        lastInstant: Long,
        firstDay: String,
        lastDay: String,
    ): Calendar
}
