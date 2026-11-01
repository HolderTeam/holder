"""Public locations operations."""

from typing import Annotated

from fastapi import Body, Path

from authoring import Contract

from ..errors import Conflict, NotFound
from ..locations import LocationDeleteResult, NewStorageLocation, ProjectStorageLocation, StorageLocationUpdate


def register(contract: Contract) -> None:
    @contract.route("GET", "/locations/{location_id}", errors=[NotFound])
    def get_storage_location(location_id: Annotated[str, Path(min_length=1)]) -> ProjectStorageLocation:
        """Get a storage declaration and this device's binding status."""


    @contract.route("POST", "/locations", status=201, errors=[NotFound])
    def create_storage_location(request: Annotated[NewStorageLocation, Body()]) -> ProjectStorageLocation:
        """Create a project's portable storage declaration."""


    @contract.route("PATCH", "/locations/{location_id}", errors=[NotFound])
    def update_storage_location(
        location_id: Annotated[str, Path(min_length=1)], request: Annotated[StorageLocationUpdate, Body()],
    ) -> ProjectStorageLocation:
        """Change a storage declaration; configuration replaces the whole map."""


    @contract.route("DELETE", "/locations/{location_id}", errors=[NotFound, Conflict])
    def delete_storage_location(location_id: Annotated[str, Path(min_length=1)]) -> LocationDeleteResult:
        """Delete an unused declaration, its binding and its preferred setting on this device."""
