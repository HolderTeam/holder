from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar, cast

from attrs import define as _attrs_define
from typing_extensions import Self

from ..models.link_target_type import LinkTargetType
from ..types import UNSET, Unset

T = TypeVar("T", bound="NewLink")


@_attrs_define
class NewLink:
    """A link to make from a card. Its source is the card it is made from.

    Attributes:
        target_type (LinkTargetType):
        target_id (str):
        target_project_id (str | Unset):
        kind (str | Unset):
        label (None | str | Unset): A note on the link; null or omitted for none.
        created_at (int | Unset): When the link was made, for imports; omit for now. A link that already exists keeps
            its own.
    """

    target_type: LinkTargetType
    target_id: str
    target_project_id: str | Unset = UNSET
    kind: str | Unset = UNSET
    label: None | str | Unset = UNSET
    created_at: int | Unset = UNSET

    def to_dict(self) -> dict[str, Any]:
        target_type = self.target_type.value

        target_id = self.target_id

        target_project_id = self.target_project_id

        kind = self.kind

        label: None | str | Unset
        if isinstance(self.label, Unset):
            label = UNSET
        else:
            label = self.label

        created_at = self.created_at

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "target_type": target_type,
                "target_id": target_id,
            }
        )
        if target_project_id is not UNSET:
            field_dict["target_project_id"] = target_project_id
        if kind is not UNSET:
            field_dict["kind"] = kind
        if label is not UNSET:
            field_dict["label"] = label
        if created_at is not UNSET:
            field_dict["created_at"] = created_at

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        target_type = LinkTargetType(d.pop("target_type"))

        target_id = d.pop("target_id")

        target_project_id = d.pop("target_project_id", UNSET)

        kind = d.pop("kind", UNSET)

        def _parse_label(data: object) -> None | str | Unset:
            if data is None:
                return data
            if isinstance(data, Unset):
                return data
            return cast(None | str | Unset, data)

        label = _parse_label(d.pop("label", UNSET))

        created_at = d.pop("created_at", UNSET)

        new_link = cls(
            target_type=target_type,
            target_id=target_id,
            target_project_id=target_project_id,
            kind=kind,
            label=label,
            created_at=created_at,
        )

        return new_link
