from http import HTTPStatus
from typing import Any
from urllib.parse import quote

import httpx

from ... import errors
from ...client import AuthenticatedClient, Client
from ...models.error_response import ErrorResponse
from ...models.google_drive_authorize_response import GoogleDriveAuthorizeResponse
from ...types import Response


def _get_kwargs(
    location_id: str,
) -> dict[str, Any]:

    _kwargs: dict[str, Any] = {
        "method": "post",
        "url": "/locations/{location_id}/oauth/google-drive/authorize".format(
            location_id=quote(str(location_id), safe=""),
        ),
    }

    return _kwargs


def _parse_response(
    *, client: AuthenticatedClient | Client, response: httpx.Response
) -> ErrorResponse | GoogleDriveAuthorizeResponse | None:
    if response.status_code == 200:
        response_200 = GoogleDriveAuthorizeResponse.from_dict(response.json())

        return response_200

    if response.status_code == 400:
        response_400 = ErrorResponse.from_dict(response.json())

        return response_400

    if response.status_code == 401:
        response_401 = ErrorResponse.from_dict(response.json())

        return response_401

    if client.raise_on_unexpected_status:
        raise errors.UnexpectedStatus(response.status_code, response.content)
    else:
        return None


def _build_response(
    *, client: AuthenticatedClient | Client, response: httpx.Response
) -> Response[ErrorResponse | GoogleDriveAuthorizeResponse]:
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
) -> Response[ErrorResponse | GoogleDriveAuthorizeResponse]:
    """Start connecting a Google Drive location

     Returns the Google authorization URL for the user to open in a browser. Google then redirects the
    browser to this location's callback, on the host and port this request used.

    Args:
        location_id (str):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[ErrorResponse | GoogleDriveAuthorizeResponse]
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
) -> ErrorResponse | GoogleDriveAuthorizeResponse | None:
    """Start connecting a Google Drive location

     Returns the Google authorization URL for the user to open in a browser. Google then redirects the
    browser to this location's callback, on the host and port this request used.

    Args:
        location_id (str):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        ErrorResponse | GoogleDriveAuthorizeResponse
    """

    return sync_detailed(
        location_id=location_id,
        client=client,
    ).parsed


async def asyncio_detailed(
    location_id: str,
    *,
    client: AuthenticatedClient | Client,
) -> Response[ErrorResponse | GoogleDriveAuthorizeResponse]:
    """Start connecting a Google Drive location

     Returns the Google authorization URL for the user to open in a browser. Google then redirects the
    browser to this location's callback, on the host and port this request used.

    Args:
        location_id (str):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[ErrorResponse | GoogleDriveAuthorizeResponse]
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
) -> ErrorResponse | GoogleDriveAuthorizeResponse | None:
    """Start connecting a Google Drive location

     Returns the Google authorization URL for the user to open in a browser. Google then redirects the
    browser to this location's callback, on the host and port this request used.

    Args:
        location_id (str):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        ErrorResponse | GoogleDriveAuthorizeResponse
    """

    return (
        await asyncio_detailed(
            location_id=location_id,
            client=client,
        )
    ).parsed
