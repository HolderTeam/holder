"""Event operations."""

from typing import Annotated

from fastapi import Header, Query

from authoring import Contract, Stream
from authoring.routes import ErrorResponse

from .models import ChangeEventData, EventCheckpoint, EventCheckpointResponse, EventResyncData, LastRevision


def register(contract: Contract) -> None:
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
