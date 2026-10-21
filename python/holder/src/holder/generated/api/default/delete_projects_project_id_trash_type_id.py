from http import HTTPStatus
from typing import Any
from urllib.parse import quote

import httpx

from ... import errors
from ...client import AuthenticatedClient, Client
from ...models.delete_projects_project_id_trash_type_id_type import (
    DeleteProjectsProjectIdTrashTypeIdType,
)
from ...models.error_response import ErrorResponse
from ...models.trash_delete_response import TrashDeleteResponse
from ...types import Response


def _get_kwargs(
    project_id: str,
    type_: DeleteProjectsProjectIdTrashTypeIdType,
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
) -> ErrorResponse | TrashDeleteResponse | None:
    if response.status_code == 200:
        response_200 = TrashDeleteResponse.from_dict(response.json())

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

    if response.status_code == 501:
        response_501 = ErrorResponse.from_dict(response.json())

        return response_501

    if client.raise_on_unexpected_status:
        raise errors.UnexpectedStatus(response.status_code, response.content)
    else:
        return None


def _build_response(
    *, client: AuthenticatedClient | Client, response: httpx.Response
) -> Response[ErrorResponse | TrashDeleteResponse]:
    return Response(
        status_code=HTTPStatus(response.status_code),
        content=response.content,
        headers=response.headers,
        parsed=_parse_response(client=client, response=response),
    )


def sync_detailed(
    project_id: str,
    type_: DeleteProjectsProjectIdTrashTypeIdType,
    id: str,
    *,
    client: AuthenticatedClient | Client,
) -> Response[ErrorResponse | TrashDeleteResponse]:
    """Permanently remove a trashed item from one project

     The item must belong to the named project and still be in Trash. For cards, older Git revisions
    remain available; this operation does not erase repository history.

    Args:
        project_id (str):
        type_ (DeleteProjectsProjectIdTrashTypeIdType):
        id (str):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[ErrorResponse | TrashDeleteResponse]
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
    type_: DeleteProjectsProjectIdTrashTypeIdType,
    id: str,
    *,
    client: AuthenticatedClient | Client,
) -> ErrorResponse | TrashDeleteResponse | None:
    """Permanently remove a trashed item from one project

     The item must belong to the named project and still be in Trash. For cards, older Git revisions
    remain available; this operation does not erase repository history.

    Args:
        project_id (str):
        type_ (DeleteProjectsProjectIdTrashTypeIdType):
        id (str):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        ErrorResponse | TrashDeleteResponse
    """

    return sync_detailed(
        project_id=project_id,
        type_=type_,
        id=id,
        client=client,
    ).parsed


async def asyncio_detailed(
    project_id: str,
    type_: DeleteProjectsProjectIdTrashTypeIdType,
    id: str,
    *,
    client: AuthenticatedClient | Client,
) -> Response[ErrorResponse | TrashDeleteResponse]:
    """Permanently remove a trashed item from one project

     The item must belong to the named project and still be in Trash. For cards, older Git revisions
    remain available; this operation does not erase repository history.

    Args:
        project_id (str):
        type_ (DeleteProjectsProjectIdTrashTypeIdType):
        id (str):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[ErrorResponse | TrashDeleteResponse]
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
    type_: DeleteProjectsProjectIdTrashTypeIdType,
    id: str,
    *,
    client: AuthenticatedClient | Client,
) -> ErrorResponse | TrashDeleteResponse | None:
    """Permanently remove a trashed item from one project

     The item must belong to the named project and still be in Trash. For cards, older Git revisions
    remain available; this operation does not erase repository history.

    Args:
        project_id (str):
        type_ (DeleteProjectsProjectIdTrashTypeIdType):
        id (str):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        ErrorResponse | TrashDeleteResponse
    """

    return (
        await asyncio_detailed(
            project_id=project_id,
            type_=type_,
            id=id,
            client=client,
        )
    ).parsed
