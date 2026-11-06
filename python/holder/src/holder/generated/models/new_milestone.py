from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING, Any, TypeVar, cast

from attrs import define as _attrs_define
from typing_extensions import Self

from ..types import UNSET, Unset

if TYPE_CHECKING:
    from ..models.milestone_date import MilestoneDate


T = TypeVar("T", bound="NewMilestone")


@_attrs_define
class NewMilestone:
    """A milestone to put on a card.

    Attributes:
        dates (list[MilestoneDate]): The milestone's dates, in order; empty for a milestone without a date.
        kind (None | str | Unset): What kind of milestone it is; null or omitted for none.
        description (None | str | Unset): A note on the milestone; null or omitted for none.
    """

    dates: list[MilestoneDate]
    kind: None | str | Unset = UNSET
    description: None | str | Unset = UNSET

    def to_dict(self) -> dict[str, Any]:
        dates = []
        for dates_item_data in self.dates:
            dates_item = dates_item_data.to_dict()
            dates.append(dates_item)

        kind: None | str | Unset
        if isinstance(self.kind, Unset):
            kind = UNSET
        else:
            kind = self.kind

        description: None | str | Unset
        if isinstance(self.description, Unset):
            description = UNSET
        else:
            description = self.description

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "dates": dates,
            }
        )
        if kind is not UNSET:
            field_dict["kind"] = kind
        if description is not UNSET:
            field_dict["description"] = description

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        from ..models.milestone_date import MilestoneDate

        d = dict(src_dict)
        dates = []
        _dates = d.pop("dates")
        for dates_item_data in _dates:
            dates_item = MilestoneDate.from_dict(dates_item_data)

            dates.append(dates_item)

        def _parse_kind(data: object) -> None | str | Unset:
            if data is None:
                return data
            if isinstance(data, Unset):
                return data
            return cast(None | str | Unset, data)

        kind = _parse_kind(d.pop("kind", UNSET))

        def _parse_description(data: object) -> None | str | Unset:
            if data is None:
                return data
            if isinstance(data, Unset):
                return data
            return cast(None | str | Unset, data)

        description = _parse_description(d.pop("description", UNSET))

        new_milestone = cls(
            dates=dates,
            kind=kind,
            description=description,
        )

        return new_milestone
