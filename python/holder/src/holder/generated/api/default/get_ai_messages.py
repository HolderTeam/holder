from http import HTTPStatus
from typing import Any

import httpx

from ... import errors
from ...client import AuthenticatedClient, Client
from ...models.ai_message_list_response import AiMessageListResponse
from ...models.error_response import ErrorResponse
from ...models.get_ai_messages_include_deleted import GetAiMessagesIncludeDeleted
from ...types import UNSET, Response, Unset


def _get_kwargs(
    *,
    thread_id: str,
    include_deleted: GetAiMessagesIncludeDeleted | Unset = UNSET,
) -> dict[str, Any]:

    params: dict[str, Any] = {}

    params["thread_id"] = thread_id

    json_include_deleted: int | Unset = UNSET
    if not isinstance(include_deleted, Unset):
        json_include_deleted = include_deleted.value

    params["include_deleted"] = json_include_deleted

    params = {k: v for k, v in params.items() if v is not UNSET and v is not None}

    _kwargs: dict[str, Any] = {
        "method": "get",
        "url": "/ai/messages",
        "params": params,
    }

    return _kwargs


def _parse_response(
    *, client: AuthenticatedClient | Client, response: httpx.Response
) -> AiMessageListResponse | ErrorResponse | None:
    if response.status_code == 200:
        response_200 = AiMessageListResponse.from_dict(response.json())

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

    if client.raise_on_unexpected_status:
        raise errors.UnexpectedStatus(response.status_code, response.content)
    else:
        return None


def _build_response(
    *, client: AuthenticatedClient | Client, response: httpx.Response
) -> Response[AiMessageListResponse | ErrorResponse]:
    return Response(
        status_code=HTTPStatus(response.status_code),
        content=response.content,
        headers=response.headers,
        parsed=_parse_response(client=client, response=response),
    )


def sync_detailed(
    *,
    client: AuthenticatedClient | Client,
    thread_id: str,
    include_deleted: GetAiMessagesIncludeDeleted | Unset = UNSET,
) -> Response[AiMessageListResponse | ErrorResponse]:
    """List AI messages by thread

     List AI messages by thread

    Args:
        thread_id (str):
        include_deleted (GetAiMessagesIncludeDeleted | Unset): Set to 1 to include deleted
            messages.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[AiMessageListResponse | ErrorResponse]
    """

    kwargs = _get_kwargs(
        thread_id=thread_id,
        include_deleted=include_deleted,
    )

    response = client.get_httpx_client().request(
        **kwargs,
    )

    return _build_response(client=client, response=response)


def sync(
    *,
    client: AuthenticatedClient | Client,
    thread_id: str,
    include_deleted: GetAiMessagesIncludeDeleted | Unset = UNSET,
) -> AiMessageListResponse | ErrorResponse | None:
    """List AI messages by thread

     List AI messages by thread

    Args:
        thread_id (str):
        include_deleted (GetAiMessagesIncludeDeleted | Unset): Set to 1 to include deleted
            messages.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        AiMessageListResponse | ErrorResponse
    """

    return sync_detailed(
        client=client,
        thread_id=thread_id,
        include_deleted=include_deleted,
    ).parsed


async def asyncio_detailed(
    *,
    client: AuthenticatedClient | Client,
    thread_id: str,
    include_deleted: GetAiMessagesIncludeDeleted | Unset = UNSET,
) -> Response[AiMessageListResponse | ErrorResponse]:
    """List AI messages by thread

     List AI messages by thread

    Args:
        thread_id (str):
        include_deleted (GetAiMessagesIncludeDeleted | Unset): Set to 1 to include deleted
            messages.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[AiMessageListResponse | ErrorResponse]
    """

    kwargs = _get_kwargs(
        thread_id=thread_id,
        include_deleted=include_deleted,
    )

    response = await client.get_async_httpx_client().request(**kwargs)

    return _build_response(client=client, response=response)


async def asyncio(
    *,
    client: AuthenticatedClient | Client,
    thread_id: str,
    include_deleted: GetAiMessagesIncludeDeleted | Unset = UNSET,
) -> AiMessageListResponse | ErrorResponse | None:
    """List AI messages by thread

     List AI messages by thread

    Args:
        thread_id (str):
        include_deleted (GetAiMessagesIncludeDeleted | Unset): Set to 1 to include deleted
            messages.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        AiMessageListResponse | ErrorResponse
    """

    return (
        await asyncio_detailed(
            client=client,
            thread_id=thread_id,
            include_deleted=include_deleted,
        )
    ).parsed
