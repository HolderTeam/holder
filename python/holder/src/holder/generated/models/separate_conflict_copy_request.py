from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar

from attrs import define as _attrs_define
from typing_extensions import Self

T = TypeVar("T", bound="SeparateConflictCopyRequest")


@_attrs_define
class SeparateConflictCopyRequest:
    """The reviewed pair and the title the preserved copy keeps as a card of its own.

    Attributes:
        review_token (str): The token returned with the reviewed comparison.
        title (str): The copy's new title, usually without the "(conflicted copy)" suffix.
    """

    review_token: str
    title: str

    def to_dict(self) -> dict[str, Any]:
        review_token = self.review_token

        title = self.title

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "review_token": review_token,
                "title": title,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        review_token = d.pop("review_token")

        title = d.pop("title")

        separate_conflict_copy_request = cls(
            review_token=review_token,
            title=title,
        )

        return separate_conflict_copy_request
