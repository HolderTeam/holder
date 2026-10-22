import json
from http import HTTPStatus
from typing import Any, cast

import httpx

from ... import errors
from ...client import AuthenticatedClient, Client
from ...models.change_event_data import ChangeEventData
from ...models.error_response import ErrorResponse
from ...models.event_checkpoint import EventCheckpoint
from ...models.event_resync_data import EventResyncData
from ...streams import AsyncEventStream, EventStream, ServerSentEvent
from ...types import UNSET, Response, Unset


def _get_kwargs(
    *,
    project_id: str | Unset = UNSET,
    last_revision: str | Unset = UNSET,
    last_event_id: str | Unset = UNSET,
) -> dict[str, Any]:
    headers: dict[str, Any] = {}
    if not isinstance(last_event_id, Unset):
        headers["Last-Event-ID"] = last_event_id

    params: dict[str, Any] = {}

    params["project_id"] = project_id

    params["last_revision"] = last_revision

    params = {k: v for k, v in params.items() if v is not UNSET and v is not None}

    _kwargs: dict[str, Any] = {
        "method": "get",
        "url": "/events",
        "params": params,
    }

    _kwargs["headers"] = headers
    return _kwargs


def _parse_response(
    *, client: AuthenticatedClient | Client, response: httpx.Response
) -> Any | ErrorResponse | None:
    if response.status_code == 200:
        response_200 = cast(Any, None)
        return response_200

    if response.status_code == 400:
        response_400 = ErrorResponse.from_dict(response.json())

        return response_400

    if response.status_code == 401:
        response_401 = ErrorResponse.from_dict(response.json())

        return response_401

    if response.status_code == 404:
        response_404 = ErrorResponse.from_dict(response.json())

        return response_404

    if response.status_code == 503:
        response_503 = ErrorResponse.from_dict(response.json())

        return response_503

    if client.raise_on_unexpected_status:
        raise errors.UnexpectedStatus(response.status_code, response.content)
    else:
        return None


def _build_response(
    *, client: AuthenticatedClient | Client, response: httpx.Response
) -> Response[Any | ErrorResponse]:
    return Response(
        status_code=HTTPStatus(response.status_code),
        content=response.content,
        headers=response.headers,
        parsed=_parse_response(client=client, response=response),
    )


def sync_detailed(
    *,
    client: AuthenticatedClient | Client,
    project_id: str | Unset = UNSET,
    last_revision: str | Unset = UNSET,
    last_event_id: str | Unset = UNSET,
) -> Response[Any | ErrorResponse]:
    """Subscribe to committed daemon state changes (SSE)

     Sends ready, then project.changed, card.changed, resource.changed,
    location.changed, thread.changed, message.changed and run.changed invalidations.
    Intermediate states can coalesce. Each frame has an opaque id and JSON data.
    Without a cursor starts at the current tail. Last-Event-ID replays bounded
    in-memory history. Restart, an expired cursor or last_revision without a
    replayable cursor returns resync_required with current Git revisions and
    history URLs, then closes. This endpoint does not synthesize events from Git.
    Refresh current state on resync; Git is the durable project history.
    Comments provide heartbeats. Disconnecting unsubscribes without cancelling jobs.

    Args:
        project_id (str | Unset):
        last_revision (str | Unset):
        last_event_id (str | Unset):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[Any | ErrorResponse]
    """

    kwargs = _get_kwargs(
        project_id=project_id,
        last_revision=last_revision,
        last_event_id=last_event_id,
    )

    response = client.get_httpx_client().request(
        **kwargs,
    )

    return _build_response(client=client, response=response)


def sync(
    *,
    client: AuthenticatedClient | Client,
    project_id: str | Unset = UNSET,
    last_revision: str | Unset = UNSET,
    last_event_id: str | Unset = UNSET,
) -> Any | ErrorResponse | None:
    """Subscribe to committed daemon state changes (SSE)

     Sends ready, then project.changed, card.changed, resource.changed,
    location.changed, thread.changed, message.changed and run.changed invalidations.
    Intermediate states can coalesce. Each frame has an opaque id and JSON data.
    Without a cursor starts at the current tail. Last-Event-ID replays bounded
    in-memory history. Restart, an expired cursor or last_revision without a
    replayable cursor returns resync_required with current Git revisions and
    history URLs, then closes. This endpoint does not synthesize events from Git.
    Refresh current state on resync; Git is the durable project history.
    Comments provide heartbeats. Disconnecting unsubscribes without cancelling jobs.

    Args:
        project_id (str | Unset):
        last_revision (str | Unset):
        last_event_id (str | Unset):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Any | ErrorResponse
    """

    return sync_detailed(
        client=client,
        project_id=project_id,
        last_revision=last_revision,
        last_event_id=last_event_id,
    ).parsed


async def asyncio_detailed(
    *,
    client: AuthenticatedClient | Client,
    project_id: str | Unset = UNSET,
    last_revision: str | Unset = UNSET,
    last_event_id: str | Unset = UNSET,
) -> Response[Any | ErrorResponse]:
    """Subscribe to committed daemon state changes (SSE)

     Sends ready, then project.changed, card.changed, resource.changed,
    location.changed, thread.changed, message.changed and run.changed invalidations.
    Intermediate states can coalesce. Each frame has an opaque id and JSON data.
    Without a cursor starts at the current tail. Last-Event-ID replays bounded
    in-memory history. Restart, an expired cursor or last_revision without a
    replayable cursor returns resync_required with current Git revisions and
    history URLs, then closes. This endpoint does not synthesize events from Git.
    Refresh current state on resync; Git is the durable project history.
    Comments provide heartbeats. Disconnecting unsubscribes without cancelling jobs.

    Args:
        project_id (str | Unset):
        last_revision (str | Unset):
        last_event_id (str | Unset):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[Any | ErrorResponse]
    """

    kwargs = _get_kwargs(
        project_id=project_id,
        last_revision=last_revision,
        last_event_id=last_event_id,
    )

    response = await client.get_async_httpx_client().request(**kwargs)

    return _build_response(client=client, response=response)


async def asyncio(
    *,
    client: AuthenticatedClient | Client,
    project_id: str | Unset = UNSET,
    last_revision: str | Unset = UNSET,
    last_event_id: str | Unset = UNSET,
) -> Any | ErrorResponse | None:
    """Subscribe to committed daemon state changes (SSE)

     Sends ready, then project.changed, card.changed, resource.changed,
    location.changed, thread.changed, message.changed and run.changed invalidations.
    Intermediate states can coalesce. Each frame has an opaque id and JSON data.
    Without a cursor starts at the current tail. Last-Event-ID replays bounded
    in-memory history. Restart, an expired cursor or last_revision without a
    replayable cursor returns resync_required with current Git revisions and
    history URLs, then closes. This endpoint does not synthesize events from Git.
    Refresh current state on resync; Git is the durable project history.
    Comments provide heartbeats. Disconnecting unsubscribes without cancelling jobs.

    Args:
        project_id (str | Unset):
        last_revision (str | Unset):
        last_event_id (str | Unset):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Any | ErrorResponse
    """

    return (
        await asyncio_detailed(
            client=client,
            project_id=project_id,
            last_revision=last_revision,
            last_event_id=last_event_id,
        )
    ).parsed


def _decode_event(
    event: ServerSentEvent[str],
) -> ServerSentEvent[ChangeEventData | EventCheckpoint | EventResyncData | str]:
    if event.event == "ready":
        data = json.loads(event.data)
        ready_data = EventCheckpoint.from_dict(data)

        return ServerSentEvent(
            event=event.event, data=ready_data, id=event.id, retry=event.retry
        )

    if event.event == "project.changed":
        data = json.loads(event.data)
        project_changed_data = ChangeEventData.from_dict(data)

        return ServerSentEvent(
            event=event.event, data=project_changed_data, id=event.id, retry=event.retry
        )

    if event.event == "card.changed":
        data = json.loads(event.data)
        card_changed_data = ChangeEventData.from_dict(data)

        return ServerSentEvent(
            event=event.event, data=card_changed_data, id=event.id, retry=event.retry
        )

    if event.event == "resource.changed":
        data = json.loads(event.data)
        resource_changed_data = ChangeEventData.from_dict(data)

        return ServerSentEvent(
            event=event.event,
            data=resource_changed_data,
            id=event.id,
            retry=event.retry,
        )

    if event.event == "location.changed":
        data = json.loads(event.data)
        location_changed_data = ChangeEventData.from_dict(data)

        return ServerSentEvent(
            event=event.event,
            data=location_changed_data,
            id=event.id,
            retry=event.retry,
        )

    if event.event == "thread.changed":
        data = json.loads(event.data)
        thread_changed_data = ChangeEventData.from_dict(data)

        return ServerSentEvent(
            event=event.event, data=thread_changed_data, id=event.id, retry=event.retry
        )

    if event.event == "message.changed":
        data = json.loads(event.data)
        message_changed_data = ChangeEventData.from_dict(data)

        return ServerSentEvent(
            event=event.event, data=message_changed_data, id=event.id, retry=event.retry
        )

    if event.event == "run.changed":
        data = json.loads(event.data)
        run_changed_data = ChangeEventData.from_dict(data)

        return ServerSentEvent(
            event=event.event, data=run_changed_data, id=event.id, retry=event.retry
        )

    if event.event == "resync_required":
        data = json.loads(event.data)
        resync_required_data = EventResyncData.from_dict(data)

        return ServerSentEvent(
            event=event.event, data=resync_required_data, id=event.id, retry=event.retry
        )

    return ServerSentEvent(
        event=event.event, data=event.data, id=event.id, retry=event.retry
    )


def stream(
    *,
    client: AuthenticatedClient | Client,
    project_id: str | Unset = UNSET,
    last_revision: str | Unset = UNSET,
    last_event_id: str | Unset = UNSET,
    reconnect: bool = True,
) -> EventStream[ChangeEventData | EventCheckpoint | EventResyncData | str]:
    """Subscribe to committed daemon state changes (SSE)

     Sends ready, then project.changed, card.changed, resource.changed,
    location.changed, thread.changed, message.changed and run.changed invalidations.
    Intermediate states can coalesce. Each frame has an opaque id and JSON data.
    Without a cursor starts at the current tail. Last-Event-ID replays bounded
    in-memory history. Restart, an expired cursor or last_revision without a
    replayable cursor returns resync_required with current Git revisions and
    history URLs, then closes. This endpoint does not synthesize events from Git.
    Refresh current state on resync; Git is the durable project history.
    Comments provide heartbeats. Disconnecting unsubscribes without cancelling jobs.

    Args:
        project_id (str | Unset):
        last_revision (str | Unset):
        last_event_id (str | Unset):

    Raises:
        StreamError: If the server answers with an error status instead of the stream.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        EventStream[ChangeEventData | EventCheckpoint | EventResyncData | str]
    """

    return EventStream(
        client.get_httpx_client(),
        _get_kwargs(
            project_id=project_id,
            last_revision=last_revision,
            last_event_id=last_event_id,
        ),
        _decode_event,
        lambda response: _parse_response(client=client, response=response),
        reconnect=reconnect,
    )


def astream(
    *,
    client: AuthenticatedClient | Client,
    project_id: str | Unset = UNSET,
    last_revision: str | Unset = UNSET,
    last_event_id: str | Unset = UNSET,
    reconnect: bool = True,
) -> AsyncEventStream[ChangeEventData | EventCheckpoint | EventResyncData | str]:
    """Subscribe to committed daemon state changes (SSE)

     Sends ready, then project.changed, card.changed, resource.changed,
    location.changed, thread.changed, message.changed and run.changed invalidations.
    Intermediate states can coalesce. Each frame has an opaque id and JSON data.
    Without a cursor starts at the current tail. Last-Event-ID replays bounded
    in-memory history. Restart, an expired cursor or last_revision without a
    replayable cursor returns resync_required with current Git revisions and
    history URLs, then closes. This endpoint does not synthesize events from Git.
    Refresh current state on resync; Git is the durable project history.
    Comments provide heartbeats. Disconnecting unsubscribes without cancelling jobs.

    Args:
        project_id (str | Unset):
        last_revision (str | Unset):
        last_event_id (str | Unset):

    Raises:
        StreamError: If the server answers with an error status instead of the stream.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        AsyncEventStream[ChangeEventData | EventCheckpoint | EventResyncData | str]
    """

    return AsyncEventStream(
        client.get_async_httpx_client(),
        _get_kwargs(
            project_id=project_id,
            last_revision=last_revision,
            last_event_id=last_event_id,
        ),
        _decode_event,
        lambda response: _parse_response(client=client, response=response),
        reconnect=reconnect,
    )
