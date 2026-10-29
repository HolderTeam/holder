from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING, Any, TypeVar, cast

from attrs import define as _attrs_define
from typing_extensions import Self

from ..models.storage_location_provider import StorageLocationProvider

if TYPE_CHECKING:
    from ..models.storage_location_configuration import StorageLocationConfiguration


T = TypeVar("T", bound="StorageLocation")


@_attrs_define
class StorageLocation:
    """A portable storage declaration with this device's binding status.

    Attributes:
        location_id (str): The location ID.
        project_id (str): The owning project.
        name (str): Display name.
        provider (StorageLocationProvider):
        configuration (StorageLocationConfiguration): Portable non-secret provider configuration.
        bound (bool): Whether this device has a binding.
        binding_preview (None | str): Safe binding display text; credentials are never returned.
        created_at (int): Creation time.
        updated_at (int): Last update time.
    """

    location_id: str
    project_id: str
    name: str
    provider: StorageLocationProvider
    configuration: StorageLocationConfiguration
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
        from ..models.storage_location_configuration import (
            StorageLocationConfiguration,
        )

        d = dict(src_dict)
        location_id = d.pop("location_id")

        project_id = d.pop("project_id")

        name = d.pop("name")

        provider = StorageLocationProvider(d.pop("provider"))

        configuration = StorageLocationConfiguration.from_dict(d.pop("configuration"))

        bound = d.pop("bound")

        def _parse_binding_preview(data: object) -> None | str:
            if data is None:
                return data
            return cast(None | str, data)

        binding_preview = _parse_binding_preview(d.pop("binding_preview"))

        created_at = d.pop("created_at")

        updated_at = d.pop("updated_at")

        storage_location = cls(
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

        return storage_location
