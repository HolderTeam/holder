from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING, Any, TypeVar, cast

from attrs import define as _attrs_define
from typing_extensions import Self

if TYPE_CHECKING:
    from ..models.project_storage_location import ProjectStorageLocation


T = TypeVar("T", bound="StorageLocationListResponse")


@_attrs_define
class StorageLocationListResponse:
    """Project storage locations and this device's preference.

    Attributes:
        ok (bool): Whether the request succeeded.
        data (list[ProjectStorageLocation]): The project's locations.
        preferred_location_id (None | str): This device's preferred location, when set.
    """

    ok: bool
    data: list[ProjectStorageLocation]
    preferred_location_id: None | str

    def to_dict(self) -> dict[str, Any]:
        ok = self.ok

        data = []
        for data_item_data in self.data:
            data_item = data_item_data.to_dict()
            data.append(data_item)

        preferred_location_id: None | str
        preferred_location_id = self.preferred_location_id

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "ok": ok,
                "data": data,
                "preferred_location_id": preferred_location_id,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        from ..models.project_storage_location import (
            ProjectStorageLocation,
        )

        d = dict(src_dict)
        ok = d.pop("ok")

        data = []
        _data = d.pop("data")
        for data_item_data in _data:
            data_item = ProjectStorageLocation.from_dict(data_item_data)

            data.append(data_item)

        def _parse_preferred_location_id(data: object) -> None | str:
            if data is None:
                return data
            return cast(None | str, data)

        preferred_location_id = _parse_preferred_location_id(
            d.pop("preferred_location_id")
        )

        storage_location_list_response = cls(
            ok=ok,
            data=data,
            preferred_location_id=preferred_location_id,
        )

        return storage_location_list_response
