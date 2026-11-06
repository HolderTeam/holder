from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar, cast

from attrs import define as _attrs_define
from typing_extensions import Self

from ..models.link_source_type import LinkSourceType
from ..models.link_target_type import LinkTargetType

T = TypeVar("T", bound="Link")


@_attrs_define
class Link:
    """A link from a card, or from an AI message, to something in a project. A link names both of its ends in full:
    each end's project, type and ID.

        Attributes:
            source_project_id (str):
            source_type (LinkSourceType): What the link is from.
            source_id (str): The ID of the card or AI message the link is from.
            target_project_id (str):
            target_type (LinkTargetType):
            target_id (str):
            kind (str):
            label (None | str): A note on the link; null when it has none.
            created_at (int): When the link was made.
    """

    source_project_id: str
    source_type: LinkSourceType
    source_id: str
    target_project_id: str
    target_type: LinkTargetType
    target_id: str
    kind: str
    label: None | str
    created_at: int

    def to_dict(self) -> dict[str, Any]:
        source_project_id = self.source_project_id

        source_type = self.source_type.value

        source_id = self.source_id

        target_project_id = self.target_project_id

        target_type = self.target_type.value

        target_id = self.target_id

        kind = self.kind

        label: None | str
        label = self.label

        created_at = self.created_at

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "source_project_id": source_project_id,
                "source_type": source_type,
                "source_id": source_id,
                "target_project_id": target_project_id,
                "target_type": target_type,
                "target_id": target_id,
                "kind": kind,
                "label": label,
                "created_at": created_at,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        source_project_id = d.pop("source_project_id")

        source_type = LinkSourceType(d.pop("source_type"))

        source_id = d.pop("source_id")

        target_project_id = d.pop("target_project_id")

        target_type = LinkTargetType(d.pop("target_type"))

        target_id = d.pop("target_id")

        kind = d.pop("kind")

        def _parse_label(data: object) -> None | str:
            if data is None:
                return data
            return cast(None | str, data)

        label = _parse_label(d.pop("label"))

        created_at = d.pop("created_at")

        link = cls(
            source_project_id=source_project_id,
            source_type=source_type,
            source_id=source_id,
            target_project_id=target_project_id,
            target_type=target_type,
            target_id=target_id,
            kind=kind,
            label=label,
            created_at=created_at,
        )

        return link
