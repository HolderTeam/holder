from http import HTTPStatus
from typing import Any
from urllib.parse import quote

import httpx

from ... import errors
from ...client import AuthenticatedClient, Client
from ...models.error_response import ErrorResponse
from ...models.link_list_response import LinkListResponse
from ...types import UNSET, Response, Unset


def _get_kwargs(
    project_id: str,
    resource_id: str,
    *,
    include_deleted: bool | Unset = False,
) -> dict[str, Any]:

    params: dict[str, Any] = {}

    params["include_deleted"] = include_deleted

    params = {k: v for k, v in params.items() if v is not UNSET and v is not None}

    _kwargs: dict[str, Any] = {
        "method": "get",
        "url": "/projects/{project_id}/resources/{resource_id}/backlinks".format(
            project_id=quote(str(project_id), safe=""),
            resource_id=quote(str(resource_id), safe=""),
        ),
        "params": params,
    }

    return _kwargs


def _parse_response(
    *, client: AuthenticatedClient | Client, response: httpx.Response
) -> ErrorResponse | LinkListResponse | None:
    if response.status_code == 200:
        response_200 = LinkListResponse.from_dict(response.json())

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

    if response.status_code == 500:
        response_500 = ErrorResponse.from_dict(response.json())

        return response_500

    if client.raise_on_unexpected_status:
        raise errors.UnexpectedStatus(response.status_code, response.content)
    else:
        return None


def _build_response(
    *, client: AuthenticatedClient | Client, response: httpx.Response
) -> Response[ErrorResponse | LinkListResponse]:
    return Response(
        status_code=HTTPStatus(response.status_code),
        content=response.content,
        headers=response.headers,
        parsed=_parse_response(client=client, response=response),
    )


def sync_detailed(
    project_id: str,
    resource_id: str,
    *,
    client: AuthenticatedClient | Client,
    include_deleted: bool | Unset = False,
) -> Response[ErrorResponse | LinkListResponse]:
    """List the links to a resource.

     Links of every kind, attachments among them, from cards and AI messages in the resource's project,
    newest
    first. A resource in another project is not found.

    Args:
        project_id (str):
        resource_id (str):
        include_deleted (bool | Unset): Include links from cards and AI messages in the trash.
            Default: False.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[ErrorResponse | LinkListResponse]
    """

    kwargs = _get_kwargs(
        project_id=project_id,
        resource_id=resource_id,
        include_deleted=include_deleted,
    )

    response = client.get_httpx_client().request(
        **kwargs,
    )

    return _build_response(client=client, response=response)


def sync(
    project_id: str,
    resource_id: str,
    *,
    client: AuthenticatedClient | Client,
    include_deleted: bool | Unset = False,
) -> ErrorResponse | LinkListResponse | None:
    """List the links to a resource.

     Links of every kind, attachments among them, from cards and AI messages in the resource's project,
    newest
    first. A resource in another project is not found.

    Args:
        project_id (str):
        resource_id (str):
        include_deleted (bool | Unset): Include links from cards and AI messages in the trash.
            Default: False.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        ErrorResponse | LinkListResponse
    """

    return sync_detailed(
        project_id=project_id,
        resource_id=resource_id,
        client=client,
        include_deleted=include_deleted,
    ).parsed


async def asyncio_detailed(
    project_id: str,
    resource_id: str,
    *,
    client: AuthenticatedClient | Client,
    include_deleted: bool | Unset = False,
) -> Response[ErrorResponse | LinkListResponse]:
    """List the links to a resource.

     Links of every kind, attachments among them, from cards and AI messages in the resource's project,
    newest
    first. A resource in another project is not found.

    Args:
        project_id (str):
        resource_id (str):
        include_deleted (bool | Unset): Include links from cards and AI messages in the trash.
            Default: False.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[ErrorResponse | LinkListResponse]
    """

    kwargs = _get_kwargs(
        project_id=project_id,
        resource_id=resource_id,
        include_deleted=include_deleted,
    )

    response = await client.get_async_httpx_client().request(**kwargs)

    return _build_response(client=client, response=response)


async def asyncio(
    project_id: str,
    resource_id: str,
    *,
    client: AuthenticatedClient | Client,
    include_deleted: bool | Unset = False,
) -> ErrorResponse | LinkListResponse | None:
    """List the links to a resource.

     Links of every kind, attachments among them, from cards and AI messages in the resource's project,
    newest
    first. A resource in another project is not found.

    Args:
        project_id (str):
        resource_id (str):
        include_deleted (bool | Unset): Include links from cards and AI messages in the trash.
            Default: False.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        ErrorResponse | LinkListResponse
    """

    return (
        await asyncio_detailed(
            project_id=project_id,
            resource_id=resource_id,
            client=client,
            include_deleted=include_deleted,
        )
    ).parsed
