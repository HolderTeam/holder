"""Milestones: the dates on a card, each a calendar day or an instant with a role, and a project's calendar."""

from typing import Annotated

from authoring import MISSING, Field, Record, Timestamp

from .cards.models import CardId, CardMetadata, ProjectId

type MilestoneId = Annotated[str, Field(min_length=1)]
"""A milestone's ID. Holder makes a UUID, but a card's file may hold any ID."""

type MilestoneRole = Annotated[str, Field(min_length=1)]
"""What a date is for, such as start, end or due. Roles are free-form."""

type Day = Annotated[str, Field(min_length=10, max_length=10)]
"""A calendar day, YYYY-MM-DD, with a year from 0001. A day has no time zone, so it is the same day on every
device."""

type DateTime = Annotated[str, Field(min_length=20, max_length=25)]
"""An instant in RFC 3339, with seconds and no fraction, and an offset that is Z or ±HH:MM, such as
2026-10-27T09:30:00+01:00. The offset is kept as given."""

type MilestoneText = Annotated[str, Field(min_length=1)]
"""A milestone's kind or description."""


class MilestoneDate(Record):
    """One of a milestone's dates: a role and exactly one of a day or an instant."""

    role: MilestoneRole
    """What the date is for."""
    date: Day | MISSING = MISSING
    """The calendar day; present when the date is a day."""
    datetime: DateTime | MISSING = MISSING
    """The instant; present when the date is an instant."""


class Milestone(Record):
    """A milestone on a card: its dates, which may repeat a role or be none, and what it is."""

    milestone_id: MilestoneId
    """The milestone's ID."""
    project_id: ProjectId
    """The project of the card the milestone is on."""
    card_id: CardId
    """The card the milestone is on."""
    dates: list[MilestoneDate]
    """The milestone's dates, in the order they were given."""
    kind: MilestoneText | None
    """What kind of milestone it is; null when it has none."""
    description: MilestoneText | None
    """A note on the milestone; null when it has none."""
    created_at: Timestamp
    """When the milestone was made."""
    updated_at: Timestamp
    """When the milestone was last changed."""


class NewMilestone(Record):
    """A milestone to put on a card."""

    dates: list[MilestoneDate]
    """The milestone's dates, in order; empty for a milestone without a date."""
    kind: MilestoneText | None = None
    """What kind of milestone it is; null or omitted for none."""
    description: MilestoneText | None = None
    """A note on the milestone; null or omitted for none."""


class MilestoneUpdate(Record):
    """A change to a milestone. Each field left out keeps its value."""

    dates: list[MilestoneDate] | MISSING = MISSING
    """The milestone's new dates, replacing all of them; empty for none."""
    kind: MilestoneText | None | MISSING = MISSING
    """What kind of milestone it is; null for none."""
    description: MilestoneText | None | MISSING = MISSING
    """A note on the milestone; null for none."""


class CalendarMilestone(Milestone):
    """A milestone on a calendar, with the title of its card."""

    card_title: str
    """The title of the card the milestone is on."""


class Calendar(Record):
    """What happens in a project over a window of time: its milestones, and the cards made and changed."""

    milestones: list[CalendarMilestone]
    """The milestones with a date in the window, as a day in its days or an instant in its instants, each once, by
    when they were made and then by ID."""
    created_cards: list[CardMetadata]
    """The live cards made in the window, by when they were made and then by ID."""
    updated_cards: list[CardMetadata]
    """The live cards last changed in the window, other than by being made, by when and then by ID."""
