"""Public system operations."""

from typing import Annotated, Literal

from fastapi import Body, Header, Path, Query

from authoring import Bytes, Contract, JsonValue, Stream
from authoring.routes import ErrorResponse

from ..daemon_routes import (
    AliveResponse,
    ByeResponse,
    ChangeEventData,
    EventCheckpoint,
    EventCheckpointResponse,
    EventResyncData,
    GoogleDriveAuthorizeResponse,
    HealthResponse,
    LastRevision,
    LocationActionResponse,
    LocationBindingRequest,
    LocationTestResponse,
    PreferredLocationRequest,
    StorageLocationListResponse,
)


def register(contract: Contract) -> None:
    @contract.route("GET", "/alive", core=False, public=True, raw_media_type="application/json")
    def get_alive() -> AliveResponse:
        """Liveness check returning JSON, unauthenticated."""


    @contract.route("GET", "/ping", core=False, public=True, raw_media_type="text/plain")
    def get_ping() -> Literal["pong"]:
        """Liveness check returning plain text, unauthenticated."""


    @contract.route("GET", "/health", core=False, raw_media_type="application/json")
    def get_health() -> HealthResponse:
        """Health check."""


    @contract.route("POST", "/bye", core=False, raw_media_type="application/json")
    def post_bye() -> ByeResponse:
        """Say that this client is leaving.

        A hint to a daemon started with --idle-exit: it shortens the idle wait only while nothing else happens.
        Running work, a stream, or another request keeps the daemon running. Other daemon modes ignore it.
        """


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


    @contract.route("GET", "/events/cursor", core=False, raw_media_type="application/json",
                    operation_id="get_event_cursor",
                    responses={404: {"model": ErrorResponse, "description": "Project not found."},
                               503: {"model": ErrorResponse, "description": "Feed or project unavailable."}})
    def get_event_cursor(project_id: Annotated[str | None, Query(min_length=1)] = None) -> EventCheckpointResponse:
        """Get a change-feed checkpoint before fetching a live snapshot.

        Fetch this checkpoint, read current entities, then subscribe using its cursor as Last-Event-ID.
        """


    @contract.route("GET", "/events", core=False, operation_id="stream_changes",
                    sse_events={"ready": EventCheckpoint,
                                "project.changed": ChangeEventData,
                                "card.changed": ChangeEventData,
                                "resource.changed": ChangeEventData,
                                "location.changed": ChangeEventData,
                                "thread.changed": ChangeEventData,
                                "message.changed": ChangeEventData,
                                "run.changed": ChangeEventData,
                                "resync_required": EventResyncData},
                    responses={404: {"model": ErrorResponse, "description": "Project not found."},
                               503: {"model": ErrorResponse, "description": "Observer or project unavailable."}})
    def stream_changes(
        project_id: Annotated[str | None, Query(min_length=1)] = None,
        last_revision: Annotated[LastRevision | None, Query()] = None,
        last_event_id: Annotated[str | None, Header(alias="Last-Event-ID")] = None,
    ) -> Stream[EventCheckpoint | ChangeEventData | EventResyncData]:
        """Subscribe to committed daemon state changes.

        The stream begins with ready or resync_required, then sends project, card, resource, location, thread,
        message, and run invalidations. Its opaque event ID supports bounded replay; resync_required calls for a
        fresh snapshot. Disconnecting does not cancel running work.
        """


    @contract.route("GET", "/openapi.yaml", core=False, public=True, raw_media_type="application/yaml",
                    raw_schema={})
    def get_openapi_yaml() -> str:
        """Download the OpenAPI specification."""


    @contract.route("GET", "/openapi.json", core=False, public=True, raw_media_type="application/json",
                    raw_schema={})
    def get_openapi_json() -> JsonValue:
        """Download the OpenAPI specification as JSON."""


    @contract.route("GET", "/ai_catalog.yaml", core=False, public=True, raw_media_type="application/yaml",
                    raw_schema={},
                    responses={404: {"model": ErrorResponse, "description": "Catalog not found."}})
    def get_ai_catalog_yaml() -> str:
        """Download the canonical AI catalog config."""


    @contract.route("GET", "/ai_catalog.json", core=False, public=True, raw_media_type="application/json",
                    raw_schema={},
                    responses={404: {"model": ErrorResponse, "description": "Catalog not found."}})
    def get_ai_catalog_json() -> JsonValue:
        """Download the canonical AI catalog config as JSON."""


    @contract.route("GET", "/git_providers.yaml", core=False, public=True, raw_media_type="application/yaml",
                    raw_schema={},
                    responses={404: {"model": ErrorResponse, "description": "Config not found."}})
    def get_git_providers_yaml() -> str:
        """Download the Git provider UI and runtime config."""


    @contract.route("GET", "/git_providers.json", core=False, public=True, raw_media_type="application/json",
                    raw_schema={},
                    responses={404: {"model": ErrorResponse, "description": "Config not found."}})
    def get_git_providers_json() -> JsonValue:
        """Download the Git provider UI and runtime config as JSON."""


    @contract.route("GET", "/docs", core=False, public=True, raw_media_type="text/html",
                    responses={404: {"model": ErrorResponse, "description": "Documentation assets not installed."}})
    def get_docs() -> str:
        """Browse the API documentation."""


    @contract.route("GET", "/docs/{file}", core=False, public=True, raw_media_type="*/*",
                    responses={404: {"model": ErrorResponse, "description": "Documentation file not found."}})
    def get_docs_file(file: Annotated[str, Path()]) -> Bytes:
        """Download an API documentation file with its media type."""
    # AI routes served by the daemon. Their bodies are implemented in daemon/src/api/.
