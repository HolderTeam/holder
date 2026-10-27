from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING, Any, TypeVar

from attrs import define as _attrs_define
from typing_extensions import Self

if TYPE_CHECKING:
    from ..models.calendar_milestone import CalendarMilestone
    from ..models.card_metadata import CardMetadata


T = TypeVar("T", bound="Calendar")


@_attrs_define
class Calendar:
    """What happens in a project over a window of time: its milestones, and the cards made and changed.

    Attributes:
        milestones (list[CalendarMilestone]): The milestones with a date in the window, as a day in its days or an
            instant in its instants, each once, by
            when they were made and then by ID.
        created_cards (list[CardMetadata]): The live cards made in the window, by when they were made and then by ID.
        updated_cards (list[CardMetadata]): The live cards last changed in the window, other than by being made, by when
            and then by ID.
    """

    milestones: list[CalendarMilestone]
    created_cards: list[CardMetadata]
    updated_cards: list[CardMetadata]

    def to_dict(self) -> dict[str, Any]:
        milestones = []
        for milestones_item_data in self.milestones:
            milestones_item = milestones_item_data.to_dict()
            milestones.append(milestones_item)

        created_cards = []
        for created_cards_item_data in self.created_cards:
            created_cards_item = created_cards_item_data.to_dict()
            created_cards.append(created_cards_item)

        updated_cards = []
        for updated_cards_item_data in self.updated_cards:
            updated_cards_item = updated_cards_item_data.to_dict()
            updated_cards.append(updated_cards_item)

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "milestones": milestones,
                "created_cards": created_cards,
                "updated_cards": updated_cards,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        from ..models.calendar_milestone import CalendarMilestone
        from ..models.card_metadata import CardMetadata

        d = dict(src_dict)
        milestones = []
        _milestones = d.pop("milestones")
        for milestones_item_data in _milestones:
            milestones_item = CalendarMilestone.from_dict(milestones_item_data)

            milestones.append(milestones_item)

        created_cards = []
        _created_cards = d.pop("created_cards")
        for created_cards_item_data in _created_cards:
            created_cards_item = CardMetadata.from_dict(created_cards_item_data)

            created_cards.append(created_cards_item)

        updated_cards = []
        _updated_cards = d.pop("updated_cards")
        for updated_cards_item_data in _updated_cards:
            updated_cards_item = CardMetadata.from_dict(updated_cards_item_data)

            updated_cards.append(updated_cards_item)

        calendar = cls(
            milestones=milestones,
            created_cards=created_cards,
            updated_cards=updated_cards,
        )

        return calendar
