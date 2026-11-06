from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar

from attrs import define as _attrs_define
from typing_extensions import Self

from ..types import UNSET, Unset

T = TypeVar("T", bound="MilestoneDate")


@_attrs_define
class MilestoneDate:
    """One of a milestone's dates: a role and exactly one of a day or an instant.

    Attributes:
        role (str):
        date (str | Unset):
        datetime_ (str | Unset):
    """

    role: str
    date: str | Unset = UNSET
    datetime_: str | Unset = UNSET

    def to_dict(self) -> dict[str, Any]:
        role = self.role

        date = self.date

        datetime_ = self.datetime_

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "role": role,
            }
        )
        if date is not UNSET:
            field_dict["date"] = date
        if datetime_ is not UNSET:
            field_dict["datetime"] = datetime_

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        role = d.pop("role")

        date = d.pop("date", UNSET)

        datetime_ = d.pop("datetime", UNSET)

        milestone_date = cls(
            role=role,
            date=date,
            datetime_=datetime_,
        )

        return milestone_date
