from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar

from attrs import define as _attrs_define
from typing_extensions import Self

T = TypeVar("T", bound="ConflictCopyBacklinkSource")


@_attrs_define
class ConflictCopyBacklinkSource:
    """A card's link to the copy, as the cleanup review listed it in backlinks.

    Attributes:
        source_project_id (str):
        source_id (str):
        kind (str):
    """

    source_project_id: str
    source_id: str
    kind: str

    def to_dict(self) -> dict[str, Any]:
        source_project_id = self.source_project_id

        source_id = self.source_id

        kind = self.kind

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "source_project_id": source_project_id,
                "source_id": source_id,
                "kind": kind,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        source_project_id = d.pop("source_project_id")

        source_id = d.pop("source_id")

        kind = d.pop("kind")

        conflict_copy_backlink_source = cls(
            source_project_id=source_project_id,
            source_id=source_id,
            kind=kind,
        )

        return conflict_copy_backlink_source
