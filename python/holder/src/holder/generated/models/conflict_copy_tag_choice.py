from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar

from attrs import define as _attrs_define
from typing_extensions import Self

T = TypeVar("T", bound="ConflictCopyTagChoice")


@_attrs_define
class ConflictCopyTagChoice:
    """One tag from the cleanup review's copy_tags.

    Attributes:
        tag (str): The tag, without its #.
    """

    tag: str

    def to_dict(self) -> dict[str, Any]:
        tag = self.tag

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "tag": tag,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        tag = d.pop("tag")

        conflict_copy_tag_choice = cls(
            tag=tag,
        )

        return conflict_copy_tag_choice
