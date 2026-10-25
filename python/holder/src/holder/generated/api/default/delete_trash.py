from http import HTTPStatus
from typing import Any

import httpx

from ... import errors
from ...client import AuthenticatedClient, Client
from ...models.delete_trash_type import DeleteTrashType
from ...models.error_response import ErrorResponse
from ...models.trash_empty_response import TrashEmptyResponse
from ...types import UNSET, Response, Unset


def _get_kwargs(
    *,
    project_id: str,
    type_: DeleteTrashType | Unset = DeleteTrashType.ALL,
) -> dict[str, Any]:

    params: dict[str, Any] = {}

    params["project_id"] = project_id

    json_type_: str | Unset = UNSET
    if not isinstance(type_, Unset):
        json_type_ = type_.value

    params["type"] = json_type_

    params = {k: v for k, v in params.items() if v is not UNSET and v is not None}

    _kwargs: dict[str, Any] = {
        "method": "delete",
        "url": "/trash",
        "params": params,
    }

    return _kwargs


def _parse_response(
    *, client: AuthenticatedClient | Client, response: httpx.Response
) -> ErrorResponse | TrashEmptyResponse | None:
    if response.status_code == 200:
        response_200 = TrashEmptyResponse.from_dict(response.json())

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

    if response.status_code == 501:
        response_501 = ErrorResponse.from_dict(response.json())

        return response_501

    if client.raise_on_unexpected_status:
        raise errors.UnexpectedStatus(response.status_code, response.content)
    else:
        return None


def _build_response(
    *, client: AuthenticatedClient | Client, response: httpx.Response
) -> Response[ErrorResponse | TrashEmptyResponse]:
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
    type_: DeleteTrashType | Unset = DeleteTrashType.ALL,
) -> Response[ErrorResponse | TrashEmptyResponse]:
    """Empty trash by project

     Permanently removes the project's trashed cards, AI messages, or both (the default). A project with
    nothing in Trash, or one that is not registered, succeeds without changes.

    Args:
        project_id (str):
        type_ (DeleteTrashType | Unset):  Default: DeleteTrashType.ALL.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[ErrorResponse | TrashEmptyResponse]
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
    type_: DeleteTrashType | Unset = DeleteTrashType.ALL,
) -> ErrorResponse | TrashEmptyResponse | None:
    """Empty trash by project

     Permanently removes the project's trashed cards, AI messages, or both (the default). A project with
    nothing in Trash, or one that is not registered, succeeds without changes.

    Args:
        project_id (str):
        type_ (DeleteTrashType | Unset):  Default: DeleteTrashType.ALL.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        ErrorResponse | TrashEmptyResponse
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
    type_: DeleteTrashType | Unset = DeleteTrashType.ALL,
) -> Response[ErrorResponse | TrashEmptyResponse]:
    """Empty trash by project

     Permanently removes the project's trashed cards, AI messages, or both (the default). A project with
    nothing in Trash, or one that is not registered, succeeds without changes.

    Args:
        project_id (str):
        type_ (DeleteTrashType | Unset):  Default: DeleteTrashType.ALL.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[ErrorResponse | TrashEmptyResponse]
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
    type_: DeleteTrashType | Unset = DeleteTrashType.ALL,
) -> ErrorResponse | TrashEmptyResponse | None:
    """Empty trash by project

     Permanently removes the project's trashed cards, AI messages, or both (the default). A project with
    nothing in Trash, or one that is not registered, succeeds without changes.

    Args:
        project_id (str):
        type_ (DeleteTrashType | Unset):  Default: DeleteTrashType.ALL.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        ErrorResponse | TrashEmptyResponse
    """

    return (
        await asyncio_detailed(
            client=client,
            project_id=project_id,
            type_=type_,
        )
    ).parsed
