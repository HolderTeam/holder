"""Portable storage declarations and this device's private bindings."""

from __future__ import annotations

from dataclasses import dataclass, field, fields
from typing import TYPE_CHECKING, Any, Self

from ..collections import Collection
from ..exceptions import NotFoundError, ProtocolError
from ..generated.api.default import (
    delete_locations_location_id,
    delete_locations_location_id_binding,
    get_locations,
    get_locations_location_id,
    patch_locations_location_id,
    post_locations,
    post_locations_location_id_test,
    put_locations_location_id_binding,
    put_locations_preferred,
)
from ..generated.models import (
    LocationBindingRequest,
    LocationBindingRequestValues,
    PatchLocationsLocationIdBody,
    PatchLocationsLocationIdBodyConfiguration,
    PutLocationsPreferredBody,
    StorageLocationRequest,
    StorageLocationRequestConfiguration,
    StorageLocationRequestProvider,
)
from ..generated.types import UNSET, Unset

if TYPE_CHECKING:
    from .project import Project


def _values(value: dict[str, str]) -> dict[str, str]:
    if not isinstance(value, dict) or any(
        not isinstance(k, str) or not isinstance(v, str) for k, v in value.items()
    ):
        raise TypeError("configuration and binding values must map strings to strings")
    return dict(value)


@dataclass(frozen=True, eq=False)
class StorageLocation:
    id: str
    project_id: str
    name: str
    provider: str
    configuration: dict[str, str]
    bound: bool
    binding_preview: str | None
    created_at: int
    updated_at: int
    _project: Project = field(repr=False, compare=False)

    @classmethod
    def _from(cls, project: Project, row: Any) -> Self:
        if row.project_id != project.id:
            raise ProtocolError("Storage location belongs to another project")
        preview = (
            None if isinstance(row.binding_preview, Unset) else row.binding_preview
        )
        return cls(
            id=row.location_id,
            project_id=row.project_id,
            name=row.name,
            provider=str(row.provider),
            configuration=dict(row.configuration.additional_properties),
            bound=row.bound,
            binding_preview=preview,
            created_at=row.created_at,
            updated_at=row.updated_at,
            _project=project,
        )

    def refresh(self) -> Self:
        fresh = self._project.storage_locations.get(self.id)
        for item in fields(self):
            if item.name != "_project":
                object.__setattr__(self, item.name, getattr(fresh, item.name))
        return self

    def update(
        self,
        *,
        name: str | Unset = UNSET,
        configuration: dict[str, str] | Unset = UNSET,
    ) -> Self:
        if isinstance(name, Unset) and isinstance(configuration, Unset):
            raise TypeError("Supply name or configuration")
        if not isinstance(name, (str, Unset)):
            raise TypeError("name must be a string")
        config = UNSET
        if not isinstance(configuration, Unset):
            config = PatchLocationsLocationIdBodyConfiguration.from_dict(
                _values(configuration)
            )
        self._project.storage_locations.get(self.id)
        self._project._holder._call(
            patch_locations_location_id,
            self.id,
            body=PatchLocationsLocationIdBody(name=name, configuration=config),
            write=True,
        )
        return self.refresh()

    def bind(
        self, values: dict[str, str], *, preview: str | Unset = UNSET
    ) -> StorageLocation:
        """Save a device-local path or credentials; values are never returned."""
        if not isinstance(preview, (str, Unset)):
            raise TypeError("preview must be a string")
        self._project.storage_locations.get(self.id)
        self._project._holder._call(
            put_locations_location_id_binding,
            self.id,
            body=LocationBindingRequest(
                values=LocationBindingRequestValues.from_dict(_values(values)),
                preview=preview,
            ),
            write=True,
        )
        return self.refresh()

    def unbind(self) -> StorageLocation:
        self._project.storage_locations.get(self.id)
        self._project._holder._call(
            delete_locations_location_id_binding, self.id, write=True
        )
        return self.refresh()

    def test(self) -> bool:
        """Probe storage with an upload, verification, and removal."""
        self._project.storage_locations.get(self.id)
        result = self._project._holder._call(
            post_locations_location_id_test, self.id, write=True
        )
        return result.available

    def make_preferred(self) -> StorageLocation:
        self._project.storage_locations.get(self.id)
        self._project._holder._call(
            put_locations_preferred,
            body=PutLocationsPreferredBody(
                project_id=self.project_id, location_id=self.id
            ),
            write=True,
        )
        return self

    def delete(self) -> None:
        """Delete an unused declaration; locations with placements cannot be removed."""
        self._project.storage_locations.get(self.id)
        self._project._holder._call(delete_locations_location_id, self.id, write=True)


class StorageLocationCollection(Collection[StorageLocation]):
    def __init__(self, project: Project) -> None:
        self._project = project
        super().__init__(project._holder)

    def _clone(self, query: dict[str, Any]) -> Self:
        return type(self)(self._project)

    def _fetch_page(self) -> tuple[list[StorageLocation], None]:
        rows = self._holder._call(get_locations, project_id=self._project.id)
        return [StorageLocation._from(self._project, row) for row in rows], None

    def get(self, location_id: str) -> StorageLocation:
        row = self._holder._call(get_locations_location_id, location_id)
        if row.project_id != self._project.id:
            raise NotFoundError(404, "not_found", "Location not found in this project")
        return StorageLocation._from(self._project, row)

    def create(
        self,
        *,
        name: str,
        provider: str,
        configuration: dict[str, str] | None = None,
    ) -> StorageLocation:
        if not isinstance(name, str) or not name:
            raise ValueError("name must be a nonempty string")
        try:
            selected = StorageLocationRequestProvider(provider)
        except ValueError:
            raise ValueError(f"Unsupported storage provider: {provider}") from None
        row = self._holder._call(
            post_locations,
            body=StorageLocationRequest(
                project_id=self._project.id,
                name=name,
                provider=selected,
                configuration=(
                    StorageLocationRequestConfiguration.from_dict(
                        _values(configuration)
                    )
                    if configuration is not None
                    else UNSET
                ),
            ),
            write=True,
        )
        return StorageLocation._from(self._project, row)

    @property
    def preferred_id(self) -> str | None:
        envelope = self._holder._call(
            get_locations, project_id=self._project.id, return_envelope=True
        )
        return envelope.preferred_location_id
