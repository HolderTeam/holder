from http import HTTPStatus
from typing import Any

import httpx

from ... import errors
from ...client import AuthenticatedClient, Client
from ...models.error_response import ErrorResponse
from ...models.trash_item_list_response import TrashItemListResponse
from ...types import UNSET, Response, Unset


def _get_kwargs(
    *,
    project_id: str,
    type_: None | str | Unset = UNSET,
) -> dict[str, Any]:

    params: dict[str, Any] = {}

    params["project_id"] = project_id

    json_type_: None | str | Unset
    if isinstance(type_, Unset):
        json_type_ = UNSET
    else:
        json_type_ = type_
    params["type"] = json_type_

    params = {k: v for k, v in params.items() if v is not UNSET and v is not None}

    _kwargs: dict[str, Any] = {
        "method": "get",
        "url": "/trash",
        "params": params,
    }

    return _kwargs


def _parse_response(
    *, client: AuthenticatedClient | Client, response: httpx.Response
) -> ErrorResponse | TrashItemListResponse | None:
    if response.status_code == 200:
        response_200 = TrashItemListResponse.from_dict(response.json())

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
) -> Response[ErrorResponse | TrashItemListResponse]:
    return Response(
        status_code=HTTPStatus(response.status_code),
        content=response.content,
        headers=response.headers,
        parsed=_parse_response(client=client, response=response),
    )


def sync_detailed(
    *,
    client: AuthenticatedClient | Client,
    project_id: str,
    type_: None | str | Unset = UNSET,
) -> Response[ErrorResponse | TrashItemListResponse]:
    """List a project's trashed cards and AI messages.

     A project that is not registered has an empty Trash.

    Args:
        project_id (str):
        type_ (None | str | Unset): card, ai_message, or all; an unknown type lists nothing.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[ErrorResponse | TrashItemListResponse]
    """

    kwargs = _get_kwargs(
        project_id=project_id,
        type_=type_,
    )

    response = client.get_httpx_client().request(
        **kwargs,
    )

    return _build_response(client=client, response=response)


def sync(
    *,
    client: AuthenticatedClient | Client,
    project_id: str,
    type_: None | str | Unset = UNSET,
) -> ErrorResponse | TrashItemListResponse | None:
    """List a project's trashed cards and AI messages.

     A project that is not registered has an empty Trash.

    Args:
        project_id (str):
        type_ (None | str | Unset): card, ai_message, or all; an unknown type lists nothing.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        ErrorResponse | TrashItemListResponse
    """

    return sync_detailed(
        client=client,
        project_id=project_id,
        type_=type_,
    ).parsed


async def asyncio_detailed(
    *,
    client: AuthenticatedClient | Client,
    project_id: str,
    type_: None | str | Unset = UNSET,
) -> Response[ErrorResponse | TrashItemListResponse]:
    """List a project's trashed cards and AI messages.

     A project that is not registered has an empty Trash.

    Args:
        project_id (str):
        type_ (None | str | Unset): card, ai_message, or all; an unknown type lists nothing.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[ErrorResponse | TrashItemListResponse]
    """

    kwargs = _get_kwargs(
        project_id=project_id,
        type_=type_,
    )

    response = await client.get_async_httpx_client().request(**kwargs)

    return _build_response(client=client, response=response)


async def asyncio(
    *,
    client: AuthenticatedClient | Client,
    project_id: str,
    type_: None | str | Unset = UNSET,
) -> ErrorResponse | TrashItemListResponse | None:
    """List a project's trashed cards and AI messages.

     A project that is not registered has an empty Trash.

    Args:
        project_id (str):
        type_ (None | str | Unset): card, ai_message, or all; an unknown type lists nothing.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        ErrorResponse | TrashItemListResponse
    """

    return (
        await asyncio_detailed(
            client=client,
            project_id=project_id,
            type_=type_,
        )
    ).parsed
