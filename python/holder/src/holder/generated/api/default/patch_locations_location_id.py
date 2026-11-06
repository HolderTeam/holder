from http import HTTPStatus
from typing import Any
from urllib.parse import quote

import httpx

from ... import errors
from ...client import AuthenticatedClient, Client
from ...models.error_response import ErrorResponse
from ...models.project_storage_location_response import ProjectStorageLocationResponse
from ...models.storage_location_update import StorageLocationUpdate
from ...types import UNSET, Response, Unset


def _get_kwargs(
    location_id: str,
    *,
    body: StorageLocationUpdate | Unset = UNSET,
) -> dict[str, Any]:
    headers: dict[str, Any] = {}

    _kwargs: dict[str, Any] = {
        "method": "patch",
        "url": "/locations/{location_id}".format(
            location_id=quote(str(location_id), safe=""),
        ),
    }

    if not isinstance(body, Unset):
        _kwargs["json"] = body.to_dict()

    headers["Content-Type"] = "application/json"

    _kwargs["headers"] = headers
    return _kwargs


def _parse_response(
    *, client: AuthenticatedClient | Client, response: httpx.Response
) -> ErrorResponse | ProjectStorageLocationResponse | None:
    if response.status_code == 200:
        response_200 = ProjectStorageLocationResponse.from_dict(response.json())

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
) -> Response[ErrorResponse | ProjectStorageLocationResponse]:
    return Response(
        status_code=HTTPStatus(response.status_code),
        content=response.content,
        headers=response.headers,
        parsed=_parse_response(client=client, response=response),
    )


def sync_detailed(
    location_id: str,
    *,
    client: AuthenticatedClient | Client,
    body: StorageLocationUpdate | Unset = UNSET,
) -> Response[ErrorResponse | ProjectStorageLocationResponse]:
    """Change a storage declaration; configuration replaces the whole map.

     Change a storage declaration; configuration replaces the whole map.

    Args:
        location_id (str):
        body (StorageLocationUpdate | Unset): Changes to a portable location declaration.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[ErrorResponse | ProjectStorageLocationResponse]
    """

    kwargs = _get_kwargs(
        location_id=location_id,
        body=body,
    )

    response = client.get_httpx_client().request(
        **kwargs,
    )

    return _build_response(client=client, response=response)


def sync(
    location_id: str,
    *,
    client: AuthenticatedClient | Client,
    body: StorageLocationUpdate | Unset = UNSET,
) -> ErrorResponse | ProjectStorageLocationResponse | None:
    """Change a storage declaration; configuration replaces the whole map.

     Change a storage declaration; configuration replaces the whole map.

    Args:
        location_id (str):
        body (StorageLocationUpdate | Unset): Changes to a portable location declaration.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        ErrorResponse | ProjectStorageLocationResponse
    """

    return sync_detailed(
        location_id=location_id,
        client=client,
        body=body,
    ).parsed


async def asyncio_detailed(
    location_id: str,
    *,
    client: AuthenticatedClient | Client,
    body: StorageLocationUpdate | Unset = UNSET,
) -> Response[ErrorResponse | ProjectStorageLocationResponse]:
    """Change a storage declaration; configuration replaces the whole map.

     Change a storage declaration; configuration replaces the whole map.

    Args:
        location_id (str):
        body (StorageLocationUpdate | Unset): Changes to a portable location declaration.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[ErrorResponse | ProjectStorageLocationResponse]
    """

    kwargs = _get_kwargs(
        location_id=location_id,
        body=body,
    )

    response = await client.get_async_httpx_client().request(**kwargs)

    return _build_response(client=client, response=response)


async def asyncio(
    location_id: str,
    *,
    client: AuthenticatedClient | Client,
    body: StorageLocationUpdate | Unset = UNSET,
) -> ErrorResponse | ProjectStorageLocationResponse | None:
    """Change a storage declaration; configuration replaces the whole map.

     Change a storage declaration; configuration replaces the whole map.

    Args:
        location_id (str):
        body (StorageLocationUpdate | Unset): Changes to a portable location declaration.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        ErrorResponse | ProjectStorageLocationResponse
    """

    return (
        await asyncio_detailed(
            location_id=location_id,
            client=client,
            body=body,
        )
    ).parsed
