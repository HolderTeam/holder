from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar

from attrs import define as _attrs_define
from typing_extensions import Self

T = TypeVar("T", bound="ConflictResolution")


@_attrs_define
class ConflictResolution:
    """Title and body to keep on the original card.

    Attributes:
        review_token (str): The token returned with the reviewed comparison.
        title (str): The replacement title.
        content (str): The replacement card content, which can be empty.
    """

    review_token: str
    title: str
    content: str

    def to_dict(self) -> dict[str, Any]:
        review_token = self.review_token

        title = self.title

        content = self.content

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "review_token": review_token,
                "title": title,
                "content": content,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        review_token = d.pop("review_token")

        title = d.pop("title")

        content = d.pop("content")

        conflict_resolution = cls(
            review_token=review_token,
            title=title,
            content=content,
        )

        return conflict_resolution
