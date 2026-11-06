from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING, Any, TypeVar, cast

from attrs import define as _attrs_define
from typing_extensions import Self

from ..models.location_provider import LocationProvider

if TYPE_CHECKING:
    from ..models.location_configuration import LocationConfiguration


T = TypeVar("T", bound="ProjectStorageLocation")


@_attrs_define
class ProjectStorageLocation:
    """A portable declaration with this device's binding status.

    Attributes:
        location_id (str):
        project_id (str):
        name (str): Display name.
        provider (LocationProvider):
        configuration (LocationConfiguration):
        bound (bool): Whether this device has a private binding.
        binding_preview (None | str): Safe binding display text, or null; credentials are never returned.
        created_at (int): When the declaration was created.
        updated_at (int): When the declaration was last updated.
    """

    location_id: str
    project_id: str
    name: str
    provider: LocationProvider
    configuration: LocationConfiguration
    bound: bool
    binding_preview: None | str
    created_at: int
    updated_at: int

    def to_dict(self) -> dict[str, Any]:
        location_id = self.location_id

        project_id = self.project_id

        name = self.name

        provider = self.provider.value

        configuration = self.configuration.to_dict()

        bound = self.bound

        binding_preview: None | str
        binding_preview = self.binding_preview

        created_at = self.created_at

        updated_at = self.updated_at

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "location_id": location_id,
                "project_id": project_id,
                "name": name,
                "provider": provider,
                "configuration": configuration,
                "bound": bound,
                "binding_preview": binding_preview,
                "created_at": created_at,
                "updated_at": updated_at,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        from ..models.location_configuration import (
            LocationConfiguration,
        )

        d = dict(src_dict)
        location_id = d.pop("location_id")

        project_id = d.pop("project_id")

        name = d.pop("name")

        provider = LocationProvider(d.pop("provider"))

        configuration = LocationConfiguration.from_dict(d.pop("configuration"))

        bound = d.pop("bound")

        def _parse_binding_preview(data: object) -> None | str:
            if data is None:
                return data
            return cast(None | str, data)

        binding_preview = _parse_binding_preview(d.pop("binding_preview"))

        created_at = d.pop("created_at")

        updated_at = d.pop("updated_at")

        project_storage_location = cls(
            location_id=location_id,
            project_id=project_id,
            name=name,
            provider=provider,
            configuration=configuration,
            bound=bound,
            binding_preview=binding_preview,
            created_at=created_at,
            updated_at=updated_at,
        )

        return project_storage_location
