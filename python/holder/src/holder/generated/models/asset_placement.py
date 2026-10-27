from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar

from attrs import define as _attrs_define
from typing_extensions import Self

from ..models.asset_placement_encoding import AssetPlacementEncoding

T = TypeVar("T", bound="AssetPlacement")


@_attrs_define
class AssetPlacement:
    """One stored copy of an asset.

    Attributes:
        placement_id (str): The copy's ID.
        location_id (str): The storage location that holds the copy.
        encoding (AssetPlacementEncoding): How the copy is stored: as the file is, or encrypted for the project.
        stored_byte_size (int): The size of the stored copy, in bytes.
        created_at (int): When the copy was stored.
    """

    placement_id: str
    location_id: str
    encoding: AssetPlacementEncoding
    stored_byte_size: int
    created_at: int

    def to_dict(self) -> dict[str, Any]:
        placement_id = self.placement_id

        location_id = self.location_id

        encoding = self.encoding.value

        stored_byte_size = self.stored_byte_size

        created_at = self.created_at

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "placement_id": placement_id,
                "location_id": location_id,
                "encoding": encoding,
                "stored_byte_size": stored_byte_size,
                "created_at": created_at,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        placement_id = d.pop("placement_id")

        location_id = d.pop("location_id")

        encoding = AssetPlacementEncoding(d.pop("encoding"))

        stored_byte_size = d.pop("stored_byte_size")

        created_at = d.pop("created_at")

        asset_placement = cls(
            placement_id=placement_id,
            location_id=location_id,
            encoding=encoding,
            stored_byte_size=stored_byte_size,
            created_at=created_at,
        )

        return asset_placement
