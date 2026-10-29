from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING, Any, TypeVar

from attrs import define as _attrs_define
from typing_extensions import Self

from ..models.location_provider import LocationProvider
from ..types import UNSET, Unset

if TYPE_CHECKING:
    from ..models.location_configuration import LocationConfiguration


T = TypeVar("T", bound="NewStorageLocation")


@_attrs_define
class NewStorageLocation:
    """A portable location declaration to create.

    Attributes:
        project_id (str):
        name (str): Display name.
        provider (LocationProvider):
        location_id (str | Unset): Optional imported ID; Holder makes one when omitted or empty.
        configuration (LocationConfiguration | Unset):
        created_at (int | Unset): Imported creation time; current time when omitted.
        updated_at (int | Unset): Imported update time; creation time when omitted.
    """

    project_id: str
    name: str
    provider: LocationProvider
    location_id: str | Unset = UNSET
    configuration: LocationConfiguration | Unset = UNSET
    created_at: int | Unset = UNSET
    updated_at: int | Unset = UNSET

    def to_dict(self) -> dict[str, Any]:
        project_id = self.project_id

        name = self.name

        provider = self.provider.value

        location_id = self.location_id

        configuration: dict[str, Any] | Unset = UNSET
        if not isinstance(self.configuration, Unset):
            configuration = self.configuration.to_dict()

        created_at = self.created_at

        updated_at = self.updated_at

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "project_id": project_id,
                "name": name,
                "provider": provider,
            }
        )
        if location_id is not UNSET:
            field_dict["location_id"] = location_id
        if configuration is not UNSET:
            field_dict["configuration"] = configuration
        if created_at is not UNSET:
            field_dict["created_at"] = created_at
        if updated_at is not UNSET:
            field_dict["updated_at"] = updated_at

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        from ..models.location_configuration import (
            LocationConfiguration,
        )

        d = dict(src_dict)
        project_id = d.pop("project_id")

        name = d.pop("name")

        provider = LocationProvider(d.pop("provider"))

        location_id = d.pop("location_id", UNSET)

        _configuration = d.pop("configuration", UNSET)
        configuration: LocationConfiguration | Unset
        if isinstance(_configuration, Unset):
            configuration = UNSET
        else:
            configuration = LocationConfiguration.from_dict(_configuration)

        created_at = d.pop("created_at", UNSET)

        updated_at = d.pop("updated_at", UNSET)

        new_storage_location = cls(
            project_id=project_id,
            name=name,
            provider=provider,
            location_id=location_id,
            configuration=configuration,
            created_at=created_at,
            updated_at=updated_at,
        )

        return new_storage_location
