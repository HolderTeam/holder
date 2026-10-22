import json
from http import HTTPStatus
from typing import Any, cast

import httpx

from ... import errors
from ...client import AuthenticatedClient, Client
from ...models.ai_run_create_request import AiRunCreateRequest
from ...models.ai_run_event_data import AiRunEventData
from ...models.error_response import ErrorResponse
from ...streams import AsyncEventStream, EventStream, ServerSentEvent
from ...types import Response


def _get_kwargs(
    *,
    body: AiRunCreateRequest,
) -> dict[str, Any]:
    headers: dict[str, Any] = {}

    _kwargs: dict[str, Any] = {
        "method": "post",
        "url": "/ai/runs",
    }

    _kwargs["json"] = body.to_dict()

    headers["Content-Type"] = "application/json"

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
    *,
    client: AuthenticatedClient | Client,
    body: AiRunCreateRequest,
) -> Response[Any | ErrorResponse]:
    """Create and execute an AI run with routing and streaming

     Creates one run and streams run_started (with run_id), provider-dependent
    progress/router/fallback/chunk events, then done or failed. Every frame has an opaque replay ID.
    Reconnect via GET /ai/runs/{run_id}/events rather than repeating this POST. Disconnecting does not
    cancel execution; terminal events follow persisted status.

    Args:
        body (AiRunCreateRequest):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[Any | ErrorResponse]
    """

    kwargs = _get_kwargs(
        body=body,
    )

    response = client.get_httpx_client().request(
        **kwargs,
    )

    return _build_response(client=client, response=response)


def sync(
    *,
    client: AuthenticatedClient | Client,
    body: AiRunCreateRequest,
) -> Any | ErrorResponse | None:
    """Create and execute an AI run with routing and streaming

     Creates one run and streams run_started (with run_id), provider-dependent
    progress/router/fallback/chunk events, then done or failed. Every frame has an opaque replay ID.
    Reconnect via GET /ai/runs/{run_id}/events rather than repeating this POST. Disconnecting does not
    cancel execution; terminal events follow persisted status.

    Args:
        body (AiRunCreateRequest):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Any | ErrorResponse
    """

    return sync_detailed(
        client=client,
        body=body,
    ).parsed


async def asyncio_detailed(
    *,
    client: AuthenticatedClient | Client,
    body: AiRunCreateRequest,
) -> Response[Any | ErrorResponse]:
    """Create and execute an AI run with routing and streaming

     Creates one run and streams run_started (with run_id), provider-dependent
    progress/router/fallback/chunk events, then done or failed. Every frame has an opaque replay ID.
    Reconnect via GET /ai/runs/{run_id}/events rather than repeating this POST. Disconnecting does not
    cancel execution; terminal events follow persisted status.

    Args:
        body (AiRunCreateRequest):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[Any | ErrorResponse]
    """

    kwargs = _get_kwargs(
        body=body,
    )

    response = await client.get_async_httpx_client().request(**kwargs)

    return _build_response(client=client, response=response)


async def asyncio(
    *,
    client: AuthenticatedClient | Client,
    body: AiRunCreateRequest,
) -> Any | ErrorResponse | None:
    """Create and execute an AI run with routing and streaming

     Creates one run and streams run_started (with run_id), provider-dependent
    progress/router/fallback/chunk events, then done or failed. Every frame has an opaque replay ID.
    Reconnect via GET /ai/runs/{run_id}/events rather than repeating this POST. Disconnecting does not
    cancel execution; terminal events follow persisted status.

    Args:
        body (AiRunCreateRequest):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Any | ErrorResponse
    """

    return (
        await asyncio_detailed(
            client=client,
            body=body,
        )
    ).parsed


def _decode_event(event: ServerSentEvent[str]) -> ServerSentEvent[AiRunEventData | str]:
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

    return ServerSentEvent(
        event=event.event, data=event.data, id=event.id, retry=event.retry
    )


def stream(
    *,
    client: AuthenticatedClient | Client,
    body: AiRunCreateRequest,
) -> EventStream[AiRunEventData | str]:
    """Create and execute an AI run with routing and streaming

     Creates one run and streams run_started (with run_id), provider-dependent
    progress/router/fallback/chunk events, then done or failed. Every frame has an opaque replay ID.
    Reconnect via GET /ai/runs/{run_id}/events rather than repeating this POST. Disconnecting does not
    cancel execution; terminal events follow persisted status.

    Args:
        body (AiRunCreateRequest):

    Raises:
        StreamError: If the server answers with an error status instead of the stream.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        EventStream[AiRunEventData | str]
    """

    return EventStream(
        client.get_httpx_client(),
        _get_kwargs(
            body=body,
        ),
        _decode_event,
        lambda response: _parse_response(client=client, response=response),
        reconnect=False,
    )


def astream(
    *,
    client: AuthenticatedClient | Client,
    body: AiRunCreateRequest,
) -> AsyncEventStream[AiRunEventData | str]:
    """Create and execute an AI run with routing and streaming

     Creates one run and streams run_started (with run_id), provider-dependent
    progress/router/fallback/chunk events, then done or failed. Every frame has an opaque replay ID.
    Reconnect via GET /ai/runs/{run_id}/events rather than repeating this POST. Disconnecting does not
    cancel execution; terminal events follow persisted status.

    Args:
        body (AiRunCreateRequest):

    Raises:
        StreamError: If the server answers with an error status instead of the stream.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        AsyncEventStream[AiRunEventData | str]
    """

    return AsyncEventStream(
        client.get_async_httpx_client(),
        _get_kwargs(
            body=body,
        ),
        _decode_event,
        lambda response: _parse_response(client=client, response=response),
        reconnect=False,
    )
