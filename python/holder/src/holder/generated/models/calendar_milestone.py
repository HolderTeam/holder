from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING, Any, TypeVar, cast

from attrs import define as _attrs_define
from typing_extensions import Self

if TYPE_CHECKING:
    from ..models.milestone_date import MilestoneDate


T = TypeVar("T", bound="CalendarMilestone")


@_attrs_define
class CalendarMilestone:
    """A milestone on a calendar, with the title of its card.

    Attributes:
        milestone_id (str):
        project_id (str):
        card_id (str):
        dates (list[MilestoneDate]): The milestone's dates, in the order they were given.
        kind (None | str): What kind of milestone it is; null when it has none.
        description (None | str): A note on the milestone; null when it has none.
        created_at (int): When the milestone was made.
        updated_at (int): When the milestone was last changed.
        card_title (str): The title of the card the milestone is on.
    """

    milestone_id: str
    project_id: str
    card_id: str
    dates: list[MilestoneDate]
    kind: None | str
    description: None | str
    created_at: int
    updated_at: int
    card_title: str

    def to_dict(self) -> dict[str, Any]:
        milestone_id = self.milestone_id

        project_id = self.project_id

        card_id = self.card_id

        dates = []
        for dates_item_data in self.dates:
            dates_item = dates_item_data.to_dict()
            dates.append(dates_item)

        kind: None | str
        kind = self.kind

        description: None | str
        description = self.description

        created_at = self.created_at

        updated_at = self.updated_at

        card_title = self.card_title

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "milestone_id": milestone_id,
                "project_id": project_id,
                "card_id": card_id,
                "dates": dates,
                "kind": kind,
                "description": description,
                "created_at": created_at,
                "updated_at": updated_at,
                "card_title": card_title,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        from ..models.milestone_date import MilestoneDate

        d = dict(src_dict)
        milestone_id = d.pop("milestone_id")

        project_id = d.pop("project_id")

        card_id = d.pop("card_id")

        dates = []
        _dates = d.pop("dates")
        for dates_item_data in _dates:
            dates_item = MilestoneDate.from_dict(dates_item_data)

            dates.append(dates_item)

        def _parse_kind(data: object) -> None | str:
            if data is None:
                return data
            return cast(None | str, data)

        kind = _parse_kind(d.pop("kind"))

        def _parse_description(data: object) -> None | str:
            if data is None:
                return data
            return cast(None | str, data)

        description = _parse_description(d.pop("description"))

        created_at = d.pop("created_at")

        updated_at = d.pop("updated_at")

        card_title = d.pop("card_title")

        calendar_milestone = cls(
            milestone_id=milestone_id,
            project_id=project_id,
            card_id=card_id,
            dates=dates,
            kind=kind,
            description=description,
            created_at=created_at,
            updated_at=updated_at,
            card_title=card_title,
        )

        return calendar_milestone
