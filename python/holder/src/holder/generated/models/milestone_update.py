from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING, Any, TypeVar, cast

from attrs import define as _attrs_define
from typing_extensions import Self

from ..types import UNSET, Unset

if TYPE_CHECKING:
    from ..models.milestone_date import MilestoneDate


T = TypeVar("T", bound="MilestoneUpdate")


@_attrs_define
class MilestoneUpdate:
    """A change to a milestone. Each field left out keeps its value.

    Attributes:
        dates (list[MilestoneDate] | Unset): The milestone's new dates, replacing all of them; empty for none.
        kind (None | str | Unset): What kind of milestone it is; null for none.
        description (None | str | Unset): A note on the milestone; null for none.
    """

    dates: list[MilestoneDate] | Unset = UNSET
    kind: None | str | Unset = UNSET
    description: None | str | Unset = UNSET

    def to_dict(self) -> dict[str, Any]:
        dates: list[dict[str, Any]] | Unset = UNSET
        if not isinstance(self.dates, Unset):
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

        field_dict.update({})
        if dates is not UNSET:
            field_dict["dates"] = dates
        if kind is not UNSET:
            field_dict["kind"] = kind
        if description is not UNSET:
            field_dict["description"] = description

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        from ..models.milestone_date import MilestoneDate

        d = dict(src_dict)
        _dates = d.pop("dates", UNSET)
        dates: list[MilestoneDate] | Unset = UNSET
        if _dates is not UNSET:
            dates = []
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

        milestone_update = cls(
            dates=dates,
            kind=kind,
            description=description,
        )

        return milestone_update
