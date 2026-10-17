from __future__ import annotations

import datetime
from collections.abc import Mapping
from typing import Any, TypeVar

from attrs import define as _attrs_define
from typing_extensions import Self

from ..types import UNSET, Unset

T = TypeVar("T", bound="MilestoneDateValue")


@_attrs_define
class MilestoneDateValue:
    """Exactly one of date or datetime must be present. Roles are free-form and may repeat within a milestone. Array order
    is preserved.

        Attributes:
            role (str):
            date (datetime.date | Unset): Calendar day with no timezone; YYYY-MM-DD.
            datetime_ (str | Unset): RFC 3339 timestamp with seconds and an explicit Z or numeric offset.
    """

    role: str
    date: datetime.date | Unset = UNSET
    datetime_: str | Unset = UNSET

    def to_dict(self) -> dict[str, Any]:
        role = self.role

        date: str | Unset = UNSET
        if not isinstance(self.date, Unset):
            date = self.date.isoformat()

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

        _date = d.pop("date", UNSET)
        date: datetime.date | Unset
        if isinstance(_date, Unset):
            date = UNSET
        else:
            date = datetime.date.fromisoformat(_date)

        datetime_ = d.pop("datetime", UNSET)

        milestone_date_value = cls(
            role=role,
            date=date,
            datetime_=datetime_,
        )

        return milestone_date_value
