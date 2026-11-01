"""Milestones operations."""

from typing import Annotated

from fastapi import Body, Path, Query

from authoring import Contract, Timestamp

from ..cards.models import CardId
from ..errors import NotFound
from ..operations.common import KEYS
from ..projects.ids import ProjectId
from .models import Calendar, Day, Milestone, MilestoneId, MilestoneRole, MilestoneUpdate, NewMilestone


def register(contract: Contract) -> None:
    @contract.route("GET", "/projects/{project_id}/cards/{card_id}/milestones", errors=[NotFound])
    def list_card_milestones(project_id: Annotated[ProjectId, Path()], card_id: Annotated[CardId, Path()]) -> list[Milestone]:
        """List a card's milestones.

        By when they were made, then by ID. A card in the trash or in another project is not found.
        """


    @contract.route("POST", "/projects/{project_id}/cards/{card_id}/milestones", status=201, errors=[NotFound, *KEYS])
    def add_card_milestone(
        project_id: Annotated[ProjectId, Path()],
        card_id: Annotated[CardId, Path()],
        milestone: Annotated[NewMilestone, Body()],
    ) -> Milestone:
        """Put a milestone on a card.

        The milestone gets a new ID. Each date must be a real day or instant. A card in the trash or in another project
        is not found.
        """


    @contract.route("PATCH", "/projects/{project_id}/cards/{card_id}/milestones/{milestone_id}",
                    errors=[NotFound, *KEYS])
    def update_card_milestone(
        project_id: Annotated[ProjectId, Path()],
        card_id: Annotated[CardId, Path()],
        milestone_id: Annotated[MilestoneId, Path()],
        update: Annotated[MilestoneUpdate, Body()],
    ) -> Milestone:
        """Change a milestone.

        Each field left out keeps its value, and an update that changes nothing writes nothing. Each date must be a real
        day or instant. A card in the trash or in another project is not found, and neither is a milestone that is not
        on the card.
        """


    @contract.route("DELETE", "/projects/{project_id}/cards/{card_id}/milestones/{milestone_id}",
                    errors=[NotFound, *KEYS])
    def remove_card_milestone(
        project_id: Annotated[ProjectId, Path()],
        card_id: Annotated[CardId, Path()],
        milestone_id: Annotated[MilestoneId, Path()],
    ) -> list[Milestone]:
        """Take a milestone off a card.

        Returns the card's remaining milestones, as list_card_milestones lists them. A card without the milestone is
        left as it is. A card in the trash or in another project is not found.
        """


    @contract.route("GET", "/projects/{project_id}/milestones/days", errors=[NotFound])
    def list_milestones_by_day(
        project_id: Annotated[ProjectId, Path()],
        first_day: Annotated[Day, Query()],
        last_day: Annotated[Day, Query()],
        role: Annotated[MilestoneRole | None, Query(description="Only dates with this role; omit for any.")] = None,
    ) -> list[Milestone]:
        """List a project's milestones with a day in a range.

        The range holds first_day, last_day and the days between; it is empty when first_day is after last_day. A
        milestone is listed once, by its first day in the range and then by ID. Milestones on cards in the trash are
        left out.
        """


    @contract.route("GET", "/projects/{project_id}/milestones/instants", errors=[NotFound])
    def list_milestones_by_instant(
        project_id: Annotated[ProjectId, Path()],
        first_instant: Annotated[Timestamp, Query()],
        last_instant: Annotated[Timestamp, Query()],
        role: Annotated[MilestoneRole | None, Query(description="Only dates with this role; omit for any.")] = None,
    ) -> list[Milestone]:
        """List a project's milestones with an instant in a range.

        The range holds first_instant, last_instant and the instants between; it is empty when first_instant is after
        last_instant. A milestone is listed once, by its first instant in the range and then by ID. Milestones on cards
        in the trash are left out.
        """


    @contract.route("GET", "/projects/{project_id}/calendar", errors=[NotFound])
    def get_calendar(
        project_id: Annotated[ProjectId, Path()],
        first_instant: Annotated[Timestamp, Query()],
        last_instant: Annotated[Timestamp, Query()],
        first_day: Annotated[Day, Query(description="The day first_instant falls on where the caller is.")],
        last_day: Annotated[Day, Query(description="The day last_instant falls on where the caller is.")],
    ) -> Calendar:
        """What happens in a project over a window of time.

        The caller gives the window twice, as instants and as the days they fall on where the caller is, because a day
        has no time zone: a milestone is in the window when one of its days is in the days, or one of its instants in
        the instants. Cards are made and changed at instants. Either range is empty when its start is after its end.
        """
