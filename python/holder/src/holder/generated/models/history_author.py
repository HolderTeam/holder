from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar

from attrs import define as _attrs_define
from typing_extensions import Self

T = TypeVar("T", bound="HistoryAuthor")


@_attrs_define
class HistoryAuthor:
    """The author recorded on a Git commit.

    Attributes:
        name (str): The author's name.
        email (str): The author's email address.
    """

    name: str
    email: str

    def to_dict(self) -> dict[str, Any]:
        name = self.name

        email = self.email

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "name": name,
                "email": email,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        name = d.pop("name")

        email = d.pop("email")

        history_author = cls(
            name=name,
            email=email,
        )

        return history_author
