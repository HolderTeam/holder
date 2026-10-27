from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING, Any, TypeVar

from attrs import define as _attrs_define
from typing_extensions import Self

from ..types import UNSET, Unset

if TYPE_CHECKING:
    from ..models.resource_metadata import ResourceMetadata


T = TypeVar("T", bound="NewResource")


@_attrs_define
class NewResource:
    """A resource to make.

    Attributes:
        type_ (str):
        label (str): The resource's name.
        resource_id (str | Unset):
        metadata (ResourceMetadata | Unset):
        created_at (int | Unset): When the resource was made, for imports; omit for now.
        updated_at (int | Unset): When the resource was last changed, for imports; omit for when it was made.
    """

    type_: str
    label: str
    resource_id: str | Unset = UNSET
    metadata: ResourceMetadata | Unset = UNSET
    created_at: int | Unset = UNSET
    updated_at: int | Unset = UNSET

    def to_dict(self) -> dict[str, Any]:
        type_ = self.type_

        label = self.label

        resource_id = self.resource_id

        metadata: dict[str, Any] | Unset = UNSET
        if not isinstance(self.metadata, Unset):
            metadata = self.metadata.to_dict()

        created_at = self.created_at

        updated_at = self.updated_at

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "type": type_,
                "label": label,
            }
        )
        if resource_id is not UNSET:
            field_dict["resource_id"] = resource_id
        if metadata is not UNSET:
            field_dict["metadata"] = metadata
        if created_at is not UNSET:
            field_dict["created_at"] = created_at
        if updated_at is not UNSET:
            field_dict["updated_at"] = updated_at

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        from ..models.resource_metadata import ResourceMetadata

        d = dict(src_dict)
        type_ = d.pop("type")

        label = d.pop("label")

        resource_id = d.pop("resource_id", UNSET)

        _metadata = d.pop("metadata", UNSET)
        metadata: ResourceMetadata | Unset
        if isinstance(_metadata, Unset):
            metadata = UNSET
        else:
            metadata = ResourceMetadata.from_dict(_metadata)

        created_at = d.pop("created_at", UNSET)

        updated_at = d.pop("updated_at", UNSET)

        new_resource = cls(
            type_=type_,
            label=label,
            resource_id=resource_id,
            metadata=metadata,
            created_at=created_at,
            updated_at=updated_at,
        )

        return new_resource
