from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar

from attrs import define as _attrs_define
from typing_extensions import Self

T = TypeVar("T", bound="TagCount")


@_attrs_define
class TagCount:
    """A tag and how many of a project's live cards carry it.

    Attributes:
        tag (str):
        card_count (int): The live cards in the project that carry the tag.
    """

    tag: str
    card_count: int

    def to_dict(self) -> dict[str, Any]:
        tag = self.tag

        card_count = self.card_count

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "tag": tag,
                "card_count": card_count,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        tag = d.pop("tag")

        card_count = d.pop("card_count")

        tag_count = cls(
            tag=tag,
            card_count=card_count,
        )

        return tag_count
