from http import HTTPStatus
from typing import Any
from urllib.parse import quote

import httpx

from ... import errors
from ...client import AuthenticatedClient, Client
from ...models.error_response import ErrorResponse
from ...models.trash_delete_result_response import TrashDeleteResultResponse
from ...models.trash_item_type import TrashItemType
from ...types import Response


def _get_kwargs(
    project_id: str,
    type_: TrashItemType,
    id: str,
) -> dict[str, Any]:

    _kwargs: dict[str, Any] = {
        "method": "delete",
        "url": "/projects/{project_id}/trash/{type_}/{id}".format(
            project_id=quote(str(project_id), safe=""),
            type_=quote(str(type_), safe=""),
            id=quote(str(id), safe=""),
        ),
    }

    return _kwargs


def _parse_response(
    *, client: AuthenticatedClient | Client, response: httpx.Response
) -> ErrorResponse | TrashDeleteResultResponse | None:
    if response.status_code == 200:
        response_200 = TrashDeleteResultResponse.from_dict(response.json())

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

    if response.status_code == 409:
        response_409 = ErrorResponse.from_dict(response.json())

        return response_409

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
) -> Response[ErrorResponse | TrashDeleteResultResponse]:
    return Response(
        status_code=HTTPStatus(response.status_code),
        content=response.content,
        headers=response.headers,
        parsed=_parse_response(client=client, response=response),
    )


def sync_detailed(
    project_id: str,
    type_: TrashItemType,
    id: str,
    *,
    client: AuthenticatedClient | Client,
) -> Response[ErrorResponse | TrashDeleteResultResponse]:
    """Permanently remove a trashed item from the named project.

     Old Git revisions of cards remain available.

    Args:
        project_id (str):
        type_ (TrashItemType):
        id (str):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[ErrorResponse | TrashDeleteResultResponse]
    """

    kwargs = _get_kwargs(
        project_id=project_id,
        type_=type_,
        id=id,
    )

    response = client.get_httpx_client().request(
        **kwargs,
    )

    return _build_response(client=client, response=response)


def sync(
    project_id: str,
    type_: TrashItemType,
    id: str,
    *,
    client: AuthenticatedClient | Client,
) -> ErrorResponse | TrashDeleteResultResponse | None:
    """Permanently remove a trashed item from the named project.

     Old Git revisions of cards remain available.

    Args:
        project_id (str):
        type_ (TrashItemType):
        id (str):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        ErrorResponse | TrashDeleteResultResponse
    """

    return sync_detailed(
        project_id=project_id,
        type_=type_,
        id=id,
        client=client,
    ).parsed


async def asyncio_detailed(
    project_id: str,
    type_: TrashItemType,
    id: str,
    *,
    client: AuthenticatedClient | Client,
) -> Response[ErrorResponse | TrashDeleteResultResponse]:
    """Permanently remove a trashed item from the named project.

     Old Git revisions of cards remain available.

    Args:
        project_id (str):
        type_ (TrashItemType):
        id (str):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[ErrorResponse | TrashDeleteResultResponse]
    """

    kwargs = _get_kwargs(
        project_id=project_id,
        type_=type_,
        id=id,
    )

    response = await client.get_async_httpx_client().request(**kwargs)

    return _build_response(client=client, response=response)


async def asyncio(
    project_id: str,
    type_: TrashItemType,
    id: str,
    *,
    client: AuthenticatedClient | Client,
) -> ErrorResponse | TrashDeleteResultResponse | None:
    """Permanently remove a trashed item from the named project.

     Old Git revisions of cards remain available.

    Args:
        project_id (str):
        type_ (TrashItemType):
        id (str):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        ErrorResponse | TrashDeleteResultResponse
    """

    return (
        await asyncio_detailed(
            project_id=project_id,
            type_=type_,
            id=id,
            client=client,
        )
    ).parsed
