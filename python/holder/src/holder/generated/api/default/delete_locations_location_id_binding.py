from http import HTTPStatus
from typing import Any
from urllib.parse import quote

import httpx

from ... import errors
from ...client import AuthenticatedClient, Client
from ...models.location_action_response import LocationActionResponse
from ...types import Response


def _get_kwargs(
    location_id: str,
) -> dict[str, Any]:

    _kwargs: dict[str, Any] = {
        "method": "delete",
        "url": "/locations/{location_id}/binding".format(
            location_id=quote(str(location_id), safe=""),
        ),
    }

    return _kwargs


def _parse_response(
    *, client: AuthenticatedClient | Client, response: httpx.Response
) -> LocationActionResponse | None:
    if response.status_code == 200:
        response_200 = LocationActionResponse.from_dict(response.json())

        return response_200

    if client.raise_on_unexpected_status:
        raise errors.UnexpectedStatus(response.status_code, response.content)
    else:
        return None


def _build_response(
    *, client: AuthenticatedClient | Client, response: httpx.Response
) -> Response[LocationActionResponse]:
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
) -> Response[LocationActionResponse]:
    """Remove this device's private binding

    Args:
        location_id (str):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[LocationActionResponse]
    """

    kwargs = _get_kwargs(
        location_id=location_id,
    )

    response = client.get_httpx_client().request(
        **kwargs,
    )

    return _build_response(client=client, response=response)


def sync(
    location_id: str,
    *,
    client: AuthenticatedClient | Client,
) -> LocationActionResponse | None:
    """Remove this device's private binding

    Args:
        location_id (str):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        LocationActionResponse
    """

    return sync_detailed(
        location_id=location_id,
        client=client,
    ).parsed


async def asyncio_detailed(
    location_id: str,
    *,
    client: AuthenticatedClient | Client,
) -> Response[LocationActionResponse]:
    """Remove this device's private binding

    Args:
        location_id (str):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[LocationActionResponse]
    """

    kwargs = _get_kwargs(
        location_id=location_id,
    )

    response = await client.get_async_httpx_client().request(**kwargs)

    return _build_response(client=client, response=response)


async def asyncio(
    location_id: str,
    *,
    client: AuthenticatedClient | Client,
) -> LocationActionResponse | None:
    """Remove this device's private binding

    Args:
        location_id (str):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        LocationActionResponse
    """

    return (
        await asyncio_detailed(
            location_id=location_id,
            client=client,
        )
    ).parsed
