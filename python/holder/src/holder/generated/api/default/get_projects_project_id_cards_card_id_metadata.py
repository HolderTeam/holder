from http import HTTPStatus
from typing import Any
from urllib.parse import quote

import httpx

from ... import errors
from ...client import AuthenticatedClient, Client
from ...models.card_metadata_response import CardMetadataResponse
from ...models.error_response import ErrorResponse
from ...types import Response


def _get_kwargs(
    project_id: str,
    card_id: str,
) -> dict[str, Any]:

    _kwargs: dict[str, Any] = {
        "method": "get",
        "url": "/projects/{project_id}/cards/{card_id}/metadata".format(
            project_id=quote(str(project_id), safe=""),
            card_id=quote(str(card_id), safe=""),
        ),
    }

    return _kwargs


def _parse_response(
    *, client: AuthenticatedClient | Client, response: httpx.Response
) -> CardMetadataResponse | ErrorResponse | None:
    if response.status_code == 200:
        response_200 = CardMetadataResponse.from_dict(response.json())

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
) -> Response[CardMetadataResponse | ErrorResponse]:
    return Response(
        status_code=HTTPStatus(response.status_code),
        content=response.content,
        headers=response.headers,
        parsed=_parse_response(client=client, response=response),
    )


def sync_detailed(
    project_id: str,
    card_id: str,
    *,
    client: AuthenticatedClient | Client,
) -> Response[CardMetadataResponse | ErrorResponse]:
    """Fetch a live card's metadata without reading its content.

     A card in the trash or in another project is not found.

    Args:
        project_id (str):
        card_id (str):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[CardMetadataResponse | ErrorResponse]
    """

    kwargs = _get_kwargs(
        project_id=project_id,
        card_id=card_id,
    )

    response = client.get_httpx_client().request(
        **kwargs,
    )

    return _build_response(client=client, response=response)


def sync(
    project_id: str,
    card_id: str,
    *,
    client: AuthenticatedClient | Client,
) -> CardMetadataResponse | ErrorResponse | None:
    """Fetch a live card's metadata without reading its content.

     A card in the trash or in another project is not found.

    Args:
        project_id (str):
        card_id (str):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        CardMetadataResponse | ErrorResponse
    """

    return sync_detailed(
        project_id=project_id,
        card_id=card_id,
        client=client,
    ).parsed


async def asyncio_detailed(
    project_id: str,
    card_id: str,
    *,
    client: AuthenticatedClient | Client,
) -> Response[CardMetadataResponse | ErrorResponse]:
    """Fetch a live card's metadata without reading its content.

     A card in the trash or in another project is not found.

    Args:
        project_id (str):
        card_id (str):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[CardMetadataResponse | ErrorResponse]
    """

    kwargs = _get_kwargs(
        project_id=project_id,
        card_id=card_id,
    )

    response = await client.get_async_httpx_client().request(**kwargs)

    return _build_response(client=client, response=response)


async def asyncio(
    project_id: str,
    card_id: str,
    *,
    client: AuthenticatedClient | Client,
) -> CardMetadataResponse | ErrorResponse | None:
    """Fetch a live card's metadata without reading its content.

     A card in the trash or in another project is not found.

    Args:
        project_id (str):
        card_id (str):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        CardMetadataResponse | ErrorResponse
    """

    return (
        await asyncio_detailed(
            project_id=project_id,
            card_id=card_id,
            client=client,
        )
    ).parsed
