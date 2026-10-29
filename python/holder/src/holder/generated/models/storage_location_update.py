from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING, Any, TypeVar

from attrs import define as _attrs_define
from typing_extensions import Self

from ..types import UNSET, Unset

if TYPE_CHECKING:
    from ..models.location_configuration import LocationConfiguration


T = TypeVar("T", bound="StorageLocationUpdate")


@_attrs_define
class StorageLocationUpdate:
    """Changes to a portable location declaration.

    Attributes:
        name (str | Unset): Replacement display name.
        configuration (LocationConfiguration | Unset):
        updated_at (int | Unset): Update time; current time when omitted.
    """

    name: str | Unset = UNSET
    configuration: LocationConfiguration | Unset = UNSET
    updated_at: int | Unset = UNSET

    def to_dict(self) -> dict[str, Any]:
        name = self.name

        configuration: dict[str, Any] | Unset = UNSET
        if not isinstance(self.configuration, Unset):
            configuration = self.configuration.to_dict()

        updated_at = self.updated_at

        field_dict: dict[str, Any] = {}

        field_dict.update({})
        if name is not UNSET:
            field_dict["name"] = name
        if configuration is not UNSET:
            field_dict["configuration"] = configuration
        if updated_at is not UNSET:
            field_dict["updated_at"] = updated_at

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        from ..models.location_configuration import (
            LocationConfiguration,
        )

        d = dict(src_dict)
        name = d.pop("name", UNSET)

        _configuration = d.pop("configuration", UNSET)
        configuration: LocationConfiguration | Unset
        if isinstance(_configuration, Unset):
            configuration = UNSET
        else:
            configuration = LocationConfiguration.from_dict(_configuration)

        updated_at = d.pop("updated_at", UNSET)

        storage_location_update = cls(
            name=name,
            configuration=configuration,
            updated_at=updated_at,
        )

        return storage_location_update
