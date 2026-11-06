import json
from http import HTTPStatus
from typing import Any, cast
from urllib.parse import quote

import httpx

from ... import errors
from ...client import AuthenticatedClient, Client
from ...models.error_response import ErrorResponse
from ...models.runner_pull_job import RunnerPullJob
from ...streams import AsyncEventStream, EventStream, ServerSentEvent
from ...types import UNSET, Response, Unset


def _get_kwargs(
    job_id: str,
    *,
    runner_id: None | str | Unset = UNSET,
    last_event_id: None | str | Unset = UNSET,
) -> dict[str, Any]:
    headers: dict[str, Any] = {}
    if not isinstance(last_event_id, Unset):
        headers["Last-Event-ID"] = last_event_id

    params: dict[str, Any] = {}

    json_runner_id: None | str | Unset
    if isinstance(runner_id, Unset):
        json_runner_id = UNSET
    else:
        json_runner_id = runner_id
    params["runner_id"] = json_runner_id

    params = {k: v for k, v in params.items() if v is not UNSET and v is not None}

    _kwargs: dict[str, Any] = {
        "method": "get",
        "url": "/ai/runner/pull/{job_id}/events".format(
            job_id=quote(str(job_id), safe=""),
        ),
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

    if response.status_code == 500:
        response_500 = ErrorResponse.from_dict(response.json())

        return response_500

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
    job_id: str,
    *,
    client: AuthenticatedClient | Client,
    runner_id: None | str | Unset = UNSET,
    last_event_id: None | str | Unset = UNSET,
) -> Response[Any | ErrorResponse]:
    """Stream pull job progress (SSE)

     Streams the job's progress snapshot whenever it changes; completed or failed closes the stream. A
    reconnect receives the current snapshot, and once the pull has finished it is answered with 204, the
    event-stream standard's signal to stop reconnecting. Disconnecting does not cancel the pull.

    Args:
        job_id (str):
        runner_id (None | str | Unset):
        last_event_id (None | str | Unset): Opaque SSE event ID, sent by a reconnecting client.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[Any | ErrorResponse]
    """

    kwargs = _get_kwargs(
        job_id=job_id,
        runner_id=runner_id,
        last_event_id=last_event_id,
    )

    response = client.get_httpx_client().request(
        **kwargs,
    )

    return _build_response(client=client, response=response)


def sync(
    job_id: str,
    *,
    client: AuthenticatedClient | Client,
    runner_id: None | str | Unset = UNSET,
    last_event_id: None | str | Unset = UNSET,
) -> Any | ErrorResponse | None:
    """Stream pull job progress (SSE)

     Streams the job's progress snapshot whenever it changes; completed or failed closes the stream. A
    reconnect receives the current snapshot, and once the pull has finished it is answered with 204, the
    event-stream standard's signal to stop reconnecting. Disconnecting does not cancel the pull.

    Args:
        job_id (str):
        runner_id (None | str | Unset):
        last_event_id (None | str | Unset): Opaque SSE event ID, sent by a reconnecting client.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Any | ErrorResponse
    """

    return sync_detailed(
        job_id=job_id,
        client=client,
        runner_id=runner_id,
        last_event_id=last_event_id,
    ).parsed


async def asyncio_detailed(
    job_id: str,
    *,
    client: AuthenticatedClient | Client,
    runner_id: None | str | Unset = UNSET,
    last_event_id: None | str | Unset = UNSET,
) -> Response[Any | ErrorResponse]:
    """Stream pull job progress (SSE)

     Streams the job's progress snapshot whenever it changes; completed or failed closes the stream. A
    reconnect receives the current snapshot, and once the pull has finished it is answered with 204, the
    event-stream standard's signal to stop reconnecting. Disconnecting does not cancel the pull.

    Args:
        job_id (str):
        runner_id (None | str | Unset):
        last_event_id (None | str | Unset): Opaque SSE event ID, sent by a reconnecting client.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[Any | ErrorResponse]
    """

    kwargs = _get_kwargs(
        job_id=job_id,
        runner_id=runner_id,
        last_event_id=last_event_id,
    )

    response = await client.get_async_httpx_client().request(**kwargs)

    return _build_response(client=client, response=response)


async def asyncio(
    job_id: str,
    *,
    client: AuthenticatedClient | Client,
    runner_id: None | str | Unset = UNSET,
    last_event_id: None | str | Unset = UNSET,
) -> Any | ErrorResponse | None:
    """Stream pull job progress (SSE)

     Streams the job's progress snapshot whenever it changes; completed or failed closes the stream. A
    reconnect receives the current snapshot, and once the pull has finished it is answered with 204, the
    event-stream standard's signal to stop reconnecting. Disconnecting does not cancel the pull.

    Args:
        job_id (str):
        runner_id (None | str | Unset):
        last_event_id (None | str | Unset): Opaque SSE event ID, sent by a reconnecting client.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Any | ErrorResponse
    """

    return (
        await asyncio_detailed(
            job_id=job_id,
            client=client,
            runner_id=runner_id,
            last_event_id=last_event_id,
        )
    ).parsed


def _decode_event(event: ServerSentEvent[str]) -> ServerSentEvent[RunnerPullJob | str]:
    if event.event == "progress":
        data = json.loads(event.data)
        progress_data = RunnerPullJob.from_dict(data)

        return ServerSentEvent(
            event=event.event, data=progress_data, id=event.id, retry=event.retry
        )

    if event.event == "completed":
        data = json.loads(event.data)
        completed_data = RunnerPullJob.from_dict(data)

        return ServerSentEvent(
            event=event.event, data=completed_data, id=event.id, retry=event.retry
        )

    if event.event == "failed":
        data = json.loads(event.data)
        failed_data = RunnerPullJob.from_dict(data)

        return ServerSentEvent(
            event=event.event, data=failed_data, id=event.id, retry=event.retry
        )

    return ServerSentEvent(
        event=event.event, data=event.data, id=event.id, retry=event.retry
    )


def stream(
    job_id: str,
    *,
    client: AuthenticatedClient | Client,
    runner_id: None | str | Unset = UNSET,
    last_event_id: None | str | Unset = UNSET,
    reconnect: bool = True,
) -> EventStream[RunnerPullJob | str]:
    """Stream pull job progress (SSE)

     Streams the job's progress snapshot whenever it changes; completed or failed closes the stream. A
    reconnect receives the current snapshot, and once the pull has finished it is answered with 204, the
    event-stream standard's signal to stop reconnecting. Disconnecting does not cancel the pull.

    Args:
        job_id (str):
        runner_id (None | str | Unset):
        last_event_id (None | str | Unset): Opaque SSE event ID, sent by a reconnecting client.

    Raises:
        StreamError: If the server answers with an error status instead of the stream.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        EventStream[RunnerPullJob | str]
    """

    return EventStream(
        client.get_httpx_client(),
        _get_kwargs(
            job_id=job_id,
            runner_id=runner_id,
            last_event_id=last_event_id,
        ),
        _decode_event,
        lambda response: _parse_response(client=client, response=response),
        reconnect=reconnect,
    )


def astream(
    job_id: str,
    *,
    client: AuthenticatedClient | Client,
    runner_id: None | str | Unset = UNSET,
    last_event_id: None | str | Unset = UNSET,
    reconnect: bool = True,
) -> AsyncEventStream[RunnerPullJob | str]:
    """Stream pull job progress (SSE)

     Streams the job's progress snapshot whenever it changes; completed or failed closes the stream. A
    reconnect receives the current snapshot, and once the pull has finished it is answered with 204, the
    event-stream standard's signal to stop reconnecting. Disconnecting does not cancel the pull.

    Args:
        job_id (str):
        runner_id (None | str | Unset):
        last_event_id (None | str | Unset): Opaque SSE event ID, sent by a reconnecting client.

    Raises:
        StreamError: If the server answers with an error status instead of the stream.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        AsyncEventStream[RunnerPullJob | str]
    """

    return AsyncEventStream(
        client.get_async_httpx_client(),
        _get_kwargs(
            job_id=job_id,
            runner_id=runner_id,
            last_event_id=last_event_id,
        ),
        _decode_event,
        lambda response: _parse_response(client=client, response=response),
        reconnect=reconnect,
    )
