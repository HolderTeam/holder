from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING, Any, TypeVar, cast
from uuid import UUID

from attrs import define as _attrs_define
from attrs import field as _attrs_field
from typing_extensions import Self

if TYPE_CHECKING:
    from ..models.milestone_date_value import MilestoneDateValue


T = TypeVar("T", bound="MilestoneWithDates")


@_attrs_define
class MilestoneWithDates:
    """
    Attributes:
        milestone_id (UUID):
        card_id (str):
        dates (list[MilestoneDateValue]):
        kind (None | str):
        description (None | str):
        created_at (int):
        updated_at (int):
    """

    milestone_id: UUID
    card_id: str
    dates: list[MilestoneDateValue]
    kind: None | str
    description: None | str
    created_at: int
    updated_at: int
    additional_properties: dict[str, Any] = _attrs_field(init=False, factory=dict)

    def to_dict(self) -> dict[str, Any]:
        milestone_id = str(self.milestone_id)

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

        field_dict: dict[str, Any] = {}
        field_dict.update(self.additional_properties)
        field_dict.update(
            {
                "milestone_id": milestone_id,
                "card_id": card_id,
                "dates": dates,
                "kind": kind,
                "description": description,
                "created_at": created_at,
                "updated_at": updated_at,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        from ..models.milestone_date_value import MilestoneDateValue

        d = dict(src_dict)
        milestone_id = UUID(d.pop("milestone_id"))

        card_id = d.pop("card_id")

        dates = []
        _dates = d.pop("dates")
        for dates_item_data in _dates:
            dates_item = MilestoneDateValue.from_dict(dates_item_data)

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

        milestone_with_dates = cls(
            milestone_id=milestone_id,
            card_id=card_id,
            dates=dates,
            kind=kind,
            description=description,
            created_at=created_at,
            updated_at=updated_at,
        )

        milestone_with_dates.additional_properties = d
        return milestone_with_dates

    @property
    def additional_keys(self) -> list[str]:
        return list(self.additional_properties.keys())

    def __getitem__(self, key: str) -> Any:
        return self.additional_properties[key]

    def __setitem__(self, key: str, value: Any) -> None:
        self.additional_properties[key] = value

    def __delitem__(self, key: str) -> None:
        del self.additional_properties[key]

    def __contains__(self, key: str) -> bool:
        return key in self.additional_properties
