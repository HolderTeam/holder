"""Storage location operations."""

from typing import Annotated

from fastapi import Body, Path, Query

from authoring import Contract
from authoring.routes import ErrorResponse

from ..errors import Conflict, NotFound
from ..projects.ids import ProjectId
from .models import (
    GoogleDriveAuthorizeResponse,
    LocationActionResponse,
    LocationBindingRequest,
    LocationDeleteResult,
    LocationTestResponse,
    NewStorageLocation,
    PreferredLocationRequest,
    ProjectStorageLocation,
    StorageLocationList,
    StorageLocationListResponse,
    StorageLocationUpdate,
)


def register(contract: Contract) -> None:
    @contract.core_operation()
    def list_storage_locations(project_id: ProjectId) -> StorageLocationList:
        """List a project's storage declarations, with this device's binding status and preferred location.

        Locations are most recently updated first. A project with none, or one this device does not have, lists none.
        The daemon serves this as GET /locations.
        """

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


    @contract.route("GET", "/locations", core=False, raw_media_type="application/json")
    def get_locations(project_id: Annotated[str, Query(min_length=1)]) -> StorageLocationListResponse:
        """List project storage locations and this device's preferred location."""


    @contract.route("PUT", "/locations/{location_id}/binding", core=False, raw_media_type="application/json",
                    responses={404: {"model": ErrorResponse, "description": "Location not found."},
                               503: {"model": ErrorResponse, "description": "Secret store unavailable."}})
    def put_location_binding(location_id: Annotated[str, Path()],
                             request: Annotated[LocationBindingRequest, Body()]) -> LocationActionResponse:
        """Bind private device credentials or path; secret values are never returned."""


    @contract.route("DELETE", "/locations/{location_id}/binding", core=False, raw_media_type="application/json",
                    responses={404: {"model": ErrorResponse, "description": "Location not found."},
                               503: {"model": ErrorResponse, "description": "Secret store unavailable."}})
    def delete_location_binding(location_id: Annotated[str, Path()]) -> LocationActionResponse:
        """Remove this device's private binding.

        Also clears the preferred location if it was this one.
        """


    @contract.route("POST", "/locations/{location_id}/test", core=False, raw_media_type="application/json",
                    responses={404: {"model": ErrorResponse, "description": "Location not found."},
                               409: {"model": ErrorResponse, "description": "Binding required or storage conflict."},
                               422: {"model": ErrorResponse, "description": "Probe verification failed."},
                               502: {"model": ErrorResponse, "description": "Storage rejected credentials or permissions."},
                               503: {"model": ErrorResponse, "description": "Storage or secret store unavailable."},
                               507: {"model": ErrorResponse, "description": "Storage is full."}})
    def test_location(location_id: Annotated[str, Path()]) -> LocationTestResponse:
        """Upload, verify, and remove a storage probe."""


    @contract.route("PUT", "/locations/preferred", core=False, raw_media_type="application/json",
                    responses={404: {"model": ErrorResponse, "description": "Location not found in the project."},
                               503: {"model": ErrorResponse, "description": "Secret store unavailable."}})
    def put_preferred_location(request: Annotated[PreferredLocationRequest, Body()]) -> LocationActionResponse:
        """Set this device's preferred project storage location."""


    @contract.route("POST", "/locations/{location_id}/oauth/google-drive/authorize", core=False,
                    raw_media_type="application/json")
    def authorize_google_drive_location(location_id: Annotated[str, Path()]) -> GoogleDriveAuthorizeResponse:
        """Start connecting a Google Drive location.

        The returned URL redirects to this daemon's callback on the host and port the request used.
        """


    @contract.route("GET", "/locations/{location_id}/oauth/google-drive/callback", core=False, public=True,
                    raw_media_type="text/html", responses={400: {"description": "Authorization failed.",
                                                            "content": {"text/html": {},
                                                                        "application/json": {"schema": {
                                                                            "$ref": "#/components/schemas/ErrorResponse"}}}}})
    def finish_google_drive_location(
        location_id: Annotated[str, Path()], code: Annotated[str | None, Query()] = None,
        state: Annotated[str | None, Query()] = None,
        error: Annotated[str | None, Query(description="Set by Google on denial or failure.")] = None,
    ) -> str:
        """Finish connecting a Google Drive location.

        Google redirects the browser here without a bearer token. The authorize state authenticates the callback.
        A failure returns an explanatory HTML page.
        """
