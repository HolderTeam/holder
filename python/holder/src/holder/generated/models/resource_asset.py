from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING, Any, TypeVar

from attrs import define as _attrs_define
from typing_extensions import Self

if TYPE_CHECKING:
    from ..models.asset_placement import AssetPlacement


T = TypeVar("T", bound="ResourceAsset")


@_attrs_define
class ResourceAsset:
    """A file that belongs to a resource.

    Attributes:
        asset_id (str): The asset's ID.
        original_filename (str): The file's name when it was imported.
        media_type (str): The file's media type, such as application/pdf.
        byte_size (int): The file's size, in bytes.
        plaintext_sha256 (str): The SHA-256 of the file's content, in lower-case hex.
        placements (list[AssetPlacement]): The stored copies of the file.
        created_at (int): When the asset was imported.
        updated_at (int): When the asset was last changed.
    """

    asset_id: str
    original_filename: str
    media_type: str
    byte_size: int
    plaintext_sha256: str
    placements: list[AssetPlacement]
    created_at: int
    updated_at: int

    def to_dict(self) -> dict[str, Any]:
        asset_id = self.asset_id

        original_filename = self.original_filename

        media_type = self.media_type

        byte_size = self.byte_size

        plaintext_sha256 = self.plaintext_sha256

        placements = []
        for placements_item_data in self.placements:
            placements_item = placements_item_data.to_dict()
            placements.append(placements_item)

        created_at = self.created_at

        updated_at = self.updated_at

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "asset_id": asset_id,
                "original_filename": original_filename,
                "media_type": media_type,
                "byte_size": byte_size,
                "plaintext_sha256": plaintext_sha256,
                "placements": placements,
                "created_at": created_at,
                "updated_at": updated_at,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        from ..models.asset_placement import AssetPlacement

        d = dict(src_dict)
        asset_id = d.pop("asset_id")

        original_filename = d.pop("original_filename")

        media_type = d.pop("media_type")

        byte_size = d.pop("byte_size")

        plaintext_sha256 = d.pop("plaintext_sha256")

        placements = []
        _placements = d.pop("placements")
        for placements_item_data in _placements:
            placements_item = AssetPlacement.from_dict(placements_item_data)

            placements.append(placements_item)

        created_at = d.pop("created_at")

        updated_at = d.pop("updated_at")

        resource_asset = cls(
            asset_id=asset_id,
            original_filename=original_filename,
            media_type=media_type,
            byte_size=byte_size,
            plaintext_sha256=plaintext_sha256,
            placements=placements,
            created_at=created_at,
            updated_at=updated_at,
        )

        return resource_asset
