from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar, cast

from attrs import define as _attrs_define
from attrs import field as _attrs_field
from typing_extensions import Self

from ..models.asset_import_job_status import AssetImportJobStatus

T = TypeVar("T", bound="AssetImportJob")


@_attrs_define
class AssetImportJob:
    """
    Attributes:
        job_id (str):
        status (AssetImportJobStatus):
        resource_id (None | str):
        asset_id (None | str):
        duplicate_reused (bool):
        link_created (bool):
        error (None | str):
    """

    job_id: str
    status: AssetImportJobStatus
    resource_id: None | str
    asset_id: None | str
    duplicate_reused: bool
    link_created: bool
    error: None | str
    additional_properties: dict[str, Any] = _attrs_field(init=False, factory=dict)

    def to_dict(self) -> dict[str, Any]:
        job_id = self.job_id

        status = self.status.value

        resource_id: None | str
        resource_id = self.resource_id

        asset_id: None | str
        asset_id = self.asset_id

        duplicate_reused = self.duplicate_reused

        link_created = self.link_created

        error: None | str
        error = self.error

        field_dict: dict[str, Any] = {}
        field_dict.update(self.additional_properties)
        field_dict.update(
            {
                "job_id": job_id,
                "status": status,
                "resource_id": resource_id,
                "asset_id": asset_id,
                "duplicate_reused": duplicate_reused,
                "link_created": link_created,
                "error": error,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        job_id = d.pop("job_id")

        status = AssetImportJobStatus(d.pop("status"))

        def _parse_resource_id(data: object) -> None | str:
            if data is None:
                return data
            return cast(None | str, data)

        resource_id = _parse_resource_id(d.pop("resource_id"))

        def _parse_asset_id(data: object) -> None | str:
            if data is None:
                return data
            return cast(None | str, data)

        asset_id = _parse_asset_id(d.pop("asset_id"))

        duplicate_reused = d.pop("duplicate_reused")

        link_created = d.pop("link_created")

        def _parse_error(data: object) -> None | str:
            if data is None:
                return data
            return cast(None | str, data)

        error = _parse_error(d.pop("error"))

        asset_import_job = cls(
            job_id=job_id,
            status=status,
            resource_id=resource_id,
            asset_id=asset_id,
            duplicate_reused=duplicate_reused,
            link_created=link_created,
            error=error,
        )

        asset_import_job.additional_properties = d
        return asset_import_job

    @property
    def additional_keys(self) -> list[str]:
        return list(self.additional_properties.keys())

    def __getitem__(self, key: str) -> Any:
        return self.additional_properties[key]

    def __setitem__(self, key: str, value: Any) -> None:
        self.additional_properties[key] = value

    def __delitem__(self, key: str) -> None:
        del self.additional_properties[key]

    def __contains__(self, key: str) -> bool:
        return key in self.additional_properties
