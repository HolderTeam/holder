from http import HTTPStatus
from typing import Any
from urllib.parse import quote

import httpx

from ... import errors
from ...client import AuthenticatedClient, Client
from ...models.error_response import ErrorResponse
from ...models.resource_response import ResourceResponse
from ...types import Response


def _get_kwargs(
    project_id: str,
    resource_id: str,
) -> dict[str, Any]:

    _kwargs: dict[str, Any] = {
        "method": "get",
        "url": "/projects/{project_id}/resources/{resource_id}".format(
            project_id=quote(str(project_id), safe=""),
            resource_id=quote(str(resource_id), safe=""),
        ),
    }

    return _kwargs


def _parse_response(
    *, client: AuthenticatedClient | Client, response: httpx.Response
) -> ErrorResponse | ResourceResponse | None:
    if response.status_code == 200:
        response_200 = ResourceResponse.from_dict(response.json())

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
) -> Response[ErrorResponse | ResourceResponse]:
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
) -> Response[ErrorResponse | ResourceResponse]:
    """Fetch a resource.

     A resource in another project is not found.

    Args:
        project_id (str):
        resource_id (str):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[ErrorResponse | ResourceResponse]
    """

    kwargs = _get_kwargs(
        project_id=project_id,
        resource_id=resource_id,
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
) -> ErrorResponse | ResourceResponse | None:
    """Fetch a resource.

     A resource in another project is not found.

    Args:
        project_id (str):
        resource_id (str):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        ErrorResponse | ResourceResponse
    """

    return sync_detailed(
        project_id=project_id,
        resource_id=resource_id,
        client=client,
    ).parsed


async def asyncio_detailed(
    project_id: str,
    resource_id: str,
    *,
    client: AuthenticatedClient | Client,
) -> Response[ErrorResponse | ResourceResponse]:
    """Fetch a resource.

     A resource in another project is not found.

    Args:
        project_id (str):
        resource_id (str):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[ErrorResponse | ResourceResponse]
    """

    kwargs = _get_kwargs(
        project_id=project_id,
        resource_id=resource_id,
    )

    response = await client.get_async_httpx_client().request(**kwargs)

    return _build_response(client=client, response=response)


async def asyncio(
    project_id: str,
    resource_id: str,
    *,
    client: AuthenticatedClient | Client,
) -> ErrorResponse | ResourceResponse | None:
    """Fetch a resource.

     A resource in another project is not found.

    Args:
        project_id (str):
        resource_id (str):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        ErrorResponse | ResourceResponse
    """

    return (
        await asyncio_detailed(
            project_id=project_id,
            resource_id=resource_id,
            client=client,
        )
    ).parsed
