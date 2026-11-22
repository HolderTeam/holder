from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar

from attrs import define as _attrs_define
from typing_extensions import Self

from ..models.link_target_type import LinkTargetType

T = TypeVar("T", bound="ConflictCopyLinkTarget")


@_attrs_define
class ConflictCopyLinkTarget:
    """One of the copy's own links, as the cleanup review listed it in copy_links.

    Attributes:
        target_project_id (str):
        target_type (LinkTargetType):
        target_id (str):
        kind (str):
    """

    target_project_id: str
    target_type: LinkTargetType
    target_id: str
    kind: str

    def to_dict(self) -> dict[str, Any]:
        target_project_id = self.target_project_id

        target_type = self.target_type.value

        target_id = self.target_id

        kind = self.kind

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "target_project_id": target_project_id,
                "target_type": target_type,
                "target_id": target_id,
                "kind": kind,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        target_project_id = d.pop("target_project_id")

        target_type = LinkTargetType(d.pop("target_type"))

        target_id = d.pop("target_id")

        kind = d.pop("kind")

        conflict_copy_link_target = cls(
            target_project_id=target_project_id,
            target_type=target_type,
            target_id=target_id,
            kind=kind,
        )

        return conflict_copy_link_target
