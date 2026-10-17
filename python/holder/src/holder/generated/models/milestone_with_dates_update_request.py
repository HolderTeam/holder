from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING, Any, TypeVar, cast

from attrs import define as _attrs_define
from typing_extensions import Self

from ..types import UNSET, Unset

if TYPE_CHECKING:
    from ..models.milestone_date_value import MilestoneDateValue


T = TypeVar("T", bound="MilestoneWithDatesUpdateRequest")


@_attrs_define
class MilestoneWithDatesUpdateRequest:
    """Partial update. Supplying dates replaces the entire ordered list; an empty array clears all dates. Omitted fields
    remain unchanged.

        Attributes:
            dates (list[MilestoneDateValue] | Unset):
            kind (None | str | Unset):
            description (None | str | Unset):
    """

    dates: list[MilestoneDateValue] | Unset = UNSET
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
        from ..models.milestone_date_value import MilestoneDateValue

        d = dict(src_dict)
        _dates = d.pop("dates", UNSET)
        dates: list[MilestoneDateValue] | Unset = UNSET
        if _dates is not UNSET:
            dates = []
            for dates_item_data in _dates:
                dates_item = MilestoneDateValue.from_dict(dates_item_data)

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

        milestone_with_dates_update_request = cls(
            dates=dates,
            kind=kind,
            description=description,
        )

        return milestone_with_dates_update_request
