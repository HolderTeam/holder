from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING, Any, TypeVar

from attrs import define as _attrs_define
from typing_extensions import Self

from ..types import UNSET, Unset

if TYPE_CHECKING:
    from ..models.resource_metadata import ResourceMetadata


T = TypeVar("T", bound="ResourceUpdate")


@_attrs_define
class ResourceUpdate:
    """A change to a resource. Each field left out keeps its value.

    Attributes:
        type_ (str | Unset):
        label (str | Unset): The resource's name.
        metadata (ResourceMetadata | Unset):
        updated_at (int | Unset): When the resource was changed, for imports; omit for now.
    """

    type_: str | Unset = UNSET
    label: str | Unset = UNSET
    metadata: ResourceMetadata | Unset = UNSET
    updated_at: int | Unset = UNSET

    def to_dict(self) -> dict[str, Any]:
        type_ = self.type_

        label = self.label

        metadata: dict[str, Any] | Unset = UNSET
        if not isinstance(self.metadata, Unset):
            metadata = self.metadata.to_dict()

        updated_at = self.updated_at

        field_dict: dict[str, Any] = {}

        field_dict.update({})
        if type_ is not UNSET:
            field_dict["type"] = type_
        if label is not UNSET:
            field_dict["label"] = label
        if metadata is not UNSET:
            field_dict["metadata"] = metadata
        if updated_at is not UNSET:
            field_dict["updated_at"] = updated_at

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        from ..models.resource_metadata import ResourceMetadata

        d = dict(src_dict)
        type_ = d.pop("type", UNSET)

        label = d.pop("label", UNSET)

        _metadata = d.pop("metadata", UNSET)
        metadata: ResourceMetadata | Unset
        if isinstance(_metadata, Unset):
            metadata = UNSET
        else:
            metadata = ResourceMetadata.from_dict(_metadata)

        updated_at = d.pop("updated_at", UNSET)

        resource_update = cls(
            type_=type_,
            label=label,
            metadata=metadata,
            updated_at=updated_at,
        )

        return resource_update
