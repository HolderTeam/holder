from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar

from attrs import define as _attrs_define
from typing_extensions import Self

T = TypeVar("T", bound="TagOccurrence")


@_attrs_define
class TagOccurrence:
    """A tag written in a card's content.

    Attributes:
        tag (str): The normalized tag.
        byte_start (int): Inclusive byte offset of the leading # in the content.
        byte_end (int): Exclusive byte offset after the tag's name.
    """

    tag: str
    byte_start: int
    byte_end: int

    def to_dict(self) -> dict[str, Any]:
        tag = self.tag

        byte_start = self.byte_start

        byte_end = self.byte_end

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "tag": tag,
                "byte_start": byte_start,
                "byte_end": byte_end,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        tag = d.pop("tag")

        byte_start = d.pop("byte_start")

        byte_end = d.pop("byte_end")

        tag_occurrence = cls(
            tag=tag,
            byte_start=byte_start,
            byte_end=byte_end,
        )

        return tag_occurrence
