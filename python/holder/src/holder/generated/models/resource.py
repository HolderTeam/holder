from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING, Any, TypeVar

from attrs import define as _attrs_define
from typing_extensions import Self

if TYPE_CHECKING:
    from ..models.resource_asset import ResourceAsset
    from ..models.resource_metadata import ResourceMetadata


T = TypeVar("T", bound="Resource")


@_attrs_define
class Resource:
    """A resource in a project, with its files.

    Attributes:
        resource_id (str):
        project_id (str):
        type_ (str):
        label (str): The resource's name.
        metadata (ResourceMetadata):
        assets (list[ResourceAsset]): The resource's files.
        created_at (int): When the resource was made.
        updated_at (int): When the resource was last changed.
    """

    resource_id: str
    project_id: str
    type_: str
    label: str
    metadata: ResourceMetadata
    assets: list[ResourceAsset]
    created_at: int
    updated_at: int

    def to_dict(self) -> dict[str, Any]:
        resource_id = self.resource_id

        project_id = self.project_id

        type_ = self.type_

        label = self.label

        metadata = self.metadata.to_dict()

        assets = []
        for assets_item_data in self.assets:
            assets_item = assets_item_data.to_dict()
            assets.append(assets_item)

        created_at = self.created_at

        updated_at = self.updated_at

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "resource_id": resource_id,
                "project_id": project_id,
                "type": type_,
                "label": label,
                "metadata": metadata,
                "assets": assets,
                "created_at": created_at,
                "updated_at": updated_at,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        from ..models.resource_asset import ResourceAsset
        from ..models.resource_metadata import ResourceMetadata

        d = dict(src_dict)
        resource_id = d.pop("resource_id")

        project_id = d.pop("project_id")

        type_ = d.pop("type")

        label = d.pop("label")

        metadata = ResourceMetadata.from_dict(d.pop("metadata"))

        assets = []
        _assets = d.pop("assets")
        for assets_item_data in _assets:
            assets_item = ResourceAsset.from_dict(assets_item_data)

            assets.append(assets_item)

        created_at = d.pop("created_at")

        updated_at = d.pop("updated_at")

        resource = cls(
            resource_id=resource_id,
            project_id=project_id,
            type_=type_,
            label=label,
            metadata=metadata,
            assets=assets,
            created_at=created_at,
            updated_at=updated_at,
        )

        return resource
