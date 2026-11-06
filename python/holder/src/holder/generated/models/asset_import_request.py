from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar

from attrs import define as _attrs_define
from typing_extensions import Self

T = TypeVar("T", bound="AssetImportRequest")


@_attrs_define
class AssetImportRequest:
    """A local file to import into a project resource and attach to a card.

    Attributes:
        project_id (str):
        card_id (str): The card to attach the imported resource to.
        location_id (str): The storage location to receive the asset.
        source_path (str): A local readable regular file; its path is never saved in project metadata.
    """

    project_id: str
    card_id: str
    location_id: str
    source_path: str

    def to_dict(self) -> dict[str, Any]:
        project_id = self.project_id

        card_id = self.card_id

        location_id = self.location_id

        source_path = self.source_path

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "project_id": project_id,
                "card_id": card_id,
                "location_id": location_id,
                "source_path": source_path,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        project_id = d.pop("project_id")

        card_id = d.pop("card_id")

        location_id = d.pop("location_id")

        source_path = d.pop("source_path")

        asset_import_request = cls(
            project_id=project_id,
            card_id=card_id,
            location_id=location_id,
            source_path=source_path,
        )

        return asset_import_request
