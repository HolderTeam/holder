import json
from http import HTTPStatus
from typing import Any, cast
from urllib.parse import quote

import httpx

from ... import errors
from ...client import AuthenticatedClient, Client
from ...models.ai_run_event_data import AiRunEventData
from ...models.ai_stream_resync_data import AiStreamResyncData
from ...models.error_response import ErrorResponse
from ...streams import AsyncEventStream, EventStream, ServerSentEvent
from ...types import UNSET, Response, Unset


def _get_kwargs(
    run_id: str,
    *,
    last_event_id: str | Unset = UNSET,
) -> dict[str, Any]:
    headers: dict[str, Any] = {}
    if not isinstance(last_event_id, Unset):
        headers["Last-Event-ID"] = last_event_id

    _kwargs: dict[str, Any] = {
        "method": "get",
        "url": "/ai/runs/{run_id}/events".format(
            run_id=quote(str(run_id), safe=""),
        ),
    }

    _kwargs["headers"] = headers
    return _kwargs


def _parse_response(
    *, client: AuthenticatedClient | Client, response: httpx.Response
) -> Any | ErrorResponse | None:
    if response.status_code == 200:
        response_200 = cast(Any, None)
        return response_200

    if response.status_code == 204:
        response_204 = cast(Any, None)
        return response_204

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
    run_id: str,
    *,
    client: AuthenticatedClient | Client,
    last_event_id: str | Unset = UNSET,
) -> Response[Any | ErrorResponse]:
    """Stream run events (SSE)

     Replays bounded retained run events with absolute event IDs; done or failed closes the stream.
    Missing in-memory history can recover persisted terminal status. An expired or foreign cursor emits
    resync_required; fetch the run before reconnecting. A reconnect after the stream's last event is
    answered with 204, the event-stream standard's signal to stop reconnecting.

    Args:
        run_id (str):
        last_event_id (str | Unset):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[Any | ErrorResponse]
    """

    kwargs = _get_kwargs(
        run_id=run_id,
        last_event_id=last_event_id,
    )

    response = client.get_httpx_client().request(
        **kwargs,
    )

    return _build_response(client=client, response=response)


def sync(
    run_id: str,
    *,
    client: AuthenticatedClient | Client,
    last_event_id: str | Unset = UNSET,
) -> Any | ErrorResponse | None:
    """Stream run events (SSE)

     Replays bounded retained run events with absolute event IDs; done or failed closes the stream.
    Missing in-memory history can recover persisted terminal status. An expired or foreign cursor emits
    resync_required; fetch the run before reconnecting. A reconnect after the stream's last event is
    answered with 204, the event-stream standard's signal to stop reconnecting.

    Args:
        run_id (str):
        last_event_id (str | Unset):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Any | ErrorResponse
    """

    return sync_detailed(
        run_id=run_id,
        client=client,
        last_event_id=last_event_id,
    ).parsed


async def asyncio_detailed(
    run_id: str,
    *,
    client: AuthenticatedClient | Client,
    last_event_id: str | Unset = UNSET,
) -> Response[Any | ErrorResponse]:
    """Stream run events (SSE)

     Replays bounded retained run events with absolute event IDs; done or failed closes the stream.
    Missing in-memory history can recover persisted terminal status. An expired or foreign cursor emits
    resync_required; fetch the run before reconnecting. A reconnect after the stream's last event is
    answered with 204, the event-stream standard's signal to stop reconnecting.

    Args:
        run_id (str):
        last_event_id (str | Unset):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[Any | ErrorResponse]
    """

    kwargs = _get_kwargs(
        run_id=run_id,
        last_event_id=last_event_id,
    )

    response = await client.get_async_httpx_client().request(**kwargs)

    return _build_response(client=client, response=response)


async def asyncio(
    run_id: str,
    *,
    client: AuthenticatedClient | Client,
    last_event_id: str | Unset = UNSET,
) -> Any | ErrorResponse | None:
    """Stream run events (SSE)

     Replays bounded retained run events with absolute event IDs; done or failed closes the stream.
    Missing in-memory history can recover persisted terminal status. An expired or foreign cursor emits
    resync_required; fetch the run before reconnecting. A reconnect after the stream's last event is
    answered with 204, the event-stream standard's signal to stop reconnecting.

    Args:
        run_id (str):
        last_event_id (str | Unset):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Any | ErrorResponse
    """

    return (
        await asyncio_detailed(
            run_id=run_id,
            client=client,
            last_event_id=last_event_id,
        )
    ).parsed


def _decode_event(
    event: ServerSentEvent[str],
) -> ServerSentEvent[AiRunEventData | AiStreamResyncData | str]:
    if event.event == "run_started":
        data = json.loads(event.data)
        run_started_data = AiRunEventData.from_dict(data)

        return ServerSentEvent(
            event=event.event, data=run_started_data, id=event.id, retry=event.retry
        )

    if event.event == "progress":
        data = json.loads(event.data)
        progress_data = AiRunEventData.from_dict(data)

        return ServerSentEvent(
            event=event.event, data=progress_data, id=event.id, retry=event.retry
        )

    if event.event == "router":
        data = json.loads(event.data)
        router_data = AiRunEventData.from_dict(data)

        return ServerSentEvent(
            event=event.event, data=router_data, id=event.id, retry=event.retry
        )

    if event.event == "router_result":
        data = json.loads(event.data)
        router_result_data = AiRunEventData.from_dict(data)

        return ServerSentEvent(
            event=event.event, data=router_result_data, id=event.id, retry=event.retry
        )

    if event.event == "chunk":
        data = json.loads(event.data)
        chunk_data = AiRunEventData.from_dict(data)

        return ServerSentEvent(
            event=event.event, data=chunk_data, id=event.id, retry=event.retry
        )

    if event.event == "fallback":
        data = json.loads(event.data)
        fallback_data = AiRunEventData.from_dict(data)

        return ServerSentEvent(
            event=event.event, data=fallback_data, id=event.id, retry=event.retry
        )

    if event.event == "done":
        data = json.loads(event.data)
        done_data = AiRunEventData.from_dict(data)

        return ServerSentEvent(
            event=event.event, data=done_data, id=event.id, retry=event.retry
        )

    if event.event == "failed":
        data = json.loads(event.data)
        failed_data = AiRunEventData.from_dict(data)

        return ServerSentEvent(
            event=event.event, data=failed_data, id=event.id, retry=event.retry
        )

    if event.event == "resync_required":
        data = json.loads(event.data)
        resync_required_data = AiStreamResyncData.from_dict(data)

        return ServerSentEvent(
            event=event.event, data=resync_required_data, id=event.id, retry=event.retry
        )

    return ServerSentEvent(
        event=event.event, data=event.data, id=event.id, retry=event.retry
    )


def stream(
    run_id: str,
    *,
    client: AuthenticatedClient | Client,
    last_event_id: str | Unset = UNSET,
    reconnect: bool = True,
) -> EventStream[AiRunEventData | AiStreamResyncData | str]:
    """Stream run events (SSE)

     Replays bounded retained run events with absolute event IDs; done or failed closes the stream.
    Missing in-memory history can recover persisted terminal status. An expired or foreign cursor emits
    resync_required; fetch the run before reconnecting. A reconnect after the stream's last event is
    answered with 204, the event-stream standard's signal to stop reconnecting.

    Args:
        run_id (str):
        last_event_id (str | Unset):

    Raises:
        StreamError: If the server answers with an error status instead of the stream.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        EventStream[AiRunEventData | AiStreamResyncData | str]
    """

    return EventStream(
        client.get_httpx_client(),
        _get_kwargs(
            run_id=run_id,
            last_event_id=last_event_id,
        ),
        _decode_event,
        lambda response: _parse_response(client=client, response=response),
        reconnect=reconnect,
    )


def astream(
    run_id: str,
    *,
    client: AuthenticatedClient | Client,
    last_event_id: str | Unset = UNSET,
    reconnect: bool = True,
) -> AsyncEventStream[AiRunEventData | AiStreamResyncData | str]:
    """Stream run events (SSE)

     Replays bounded retained run events with absolute event IDs; done or failed closes the stream.
    Missing in-memory history can recover persisted terminal status. An expired or foreign cursor emits
    resync_required; fetch the run before reconnecting. A reconnect after the stream's last event is
    answered with 204, the event-stream standard's signal to stop reconnecting.

    Args:
        run_id (str):
        last_event_id (str | Unset):

    Raises:
        StreamError: If the server answers with an error status instead of the stream.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        AsyncEventStream[AiRunEventData | AiStreamResyncData | str]
    """

    return AsyncEventStream(
        client.get_async_httpx_client(),
        _get_kwargs(
            run_id=run_id,
            last_event_id=last_event_id,
        ),
        _decode_event,
        lambda response: _parse_response(client=client, response=response),
        reconnect=reconnect,
    )
