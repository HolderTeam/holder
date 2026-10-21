from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar, cast
from uuid import UUID

from attrs import define as _attrs_define
from attrs import field as _attrs_field
from typing_extensions import Self

from ..types import UNSET, Unset

T = TypeVar("T", bound="Milestone")


@_attrs_define
class Milestone:
    """
    Attributes:
        milestone_id (UUID): Canonical milestone identifier; this is not a card ID and is never abbreviated.
        card_id (str):
        start_at (int): Unix epoch seconds. For an all-day milestone, this encodes the originating local date at local
            midnight.
        end_at (int | None): Inclusive milestone end in Unix epoch seconds. For an all-day milestone, this encodes the
            inclusive ending local date at local midnight.
        all_day (bool): When true, start_at and end_at carry local calendar-date semantics and clients should render
            dates rather than ordinary timed instants.
        kind (None | str):
        description (None | str):
        created_at (int):
        updated_at (int):
        card_title (str | Unset): Included in project calendar responses.
    """

    milestone_id: UUID
    card_id: str
    start_at: int
    end_at: int | None
    all_day: bool
    kind: None | str
    description: None | str
    created_at: int
    updated_at: int
    card_title: str | Unset = UNSET
    additional_properties: dict[str, Any] = _attrs_field(init=False, factory=dict)

    def to_dict(self) -> dict[str, Any]:
        milestone_id = str(self.milestone_id)

        card_id = self.card_id

        start_at = self.start_at

        end_at: int | None
        end_at = self.end_at

        all_day = self.all_day

        kind: None | str
        kind = self.kind

        description: None | str
        description = self.description

        created_at = self.created_at

        updated_at = self.updated_at

        card_title = self.card_title

        field_dict: dict[str, Any] = {}
        field_dict.update(self.additional_properties)
        field_dict.update(
            {
                "milestone_id": milestone_id,
                "card_id": card_id,
                "start_at": start_at,
                "end_at": end_at,
                "all_day": all_day,
                "kind": kind,
                "description": description,
                "created_at": created_at,
                "updated_at": updated_at,
            }
        )
        if card_title is not UNSET:
            field_dict["card_title"] = card_title

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        milestone_id = UUID(d.pop("milestone_id"))

        card_id = d.pop("card_id")

        start_at = d.pop("start_at")

        def _parse_end_at(data: object) -> int | None:
            if data is None:
                return data
            return cast(int | None, data)

        end_at = _parse_end_at(d.pop("end_at"))

        all_day = d.pop("all_day")

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

        card_title = d.pop("card_title", UNSET)

        milestone = cls(
            milestone_id=milestone_id,
            card_id=card_id,
            start_at=start_at,
            end_at=end_at,
            all_day=all_day,
            kind=kind,
            description=description,
            created_at=created_at,
            updated_at=updated_at,
            card_title=card_title,
        )

        milestone.additional_properties = d
        return milestone

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
