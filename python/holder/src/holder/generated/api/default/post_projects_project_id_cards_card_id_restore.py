from http import HTTPStatus
from typing import Any
from urllib.parse import quote

import httpx

from ... import errors
from ...client import AuthenticatedClient, Client
from ...models.card_response import CardResponse
from ...models.error_response import ErrorResponse
from ...types import Response


def _get_kwargs(
    project_id: str,
    card_id: str,
) -> dict[str, Any]:

    _kwargs: dict[str, Any] = {
        "method": "post",
        "url": "/projects/{project_id}/cards/{card_id}/restore".format(
            project_id=quote(str(project_id), safe=""),
            card_id=quote(str(card_id), safe=""),
        ),
    }

    return _kwargs


def _parse_response(
    *, client: AuthenticatedClient | Client, response: httpx.Response
) -> CardResponse | ErrorResponse | None:
    if response.status_code == 200:
        response_200 = CardResponse.from_dict(response.json())

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
) -> Response[CardResponse | ErrorResponse]:
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
) -> Response[CardResponse | ErrorResponse]:
    """Restore a card from the trash.

     Restores this card only. It returns to its parent when that parent is live and reachable, otherwise
    to its
    nearest live, reachable ancestor or the top level. Children promoted when it was trashed stay where
    they are;
    descendants trashed with it stay in the trash, and come back under it when restored. A card in
    another
    project is not found, and one that is not in the trash is a Conflict.

    Args:
        project_id (str):
        card_id (str):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[CardResponse | ErrorResponse]
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
) -> CardResponse | ErrorResponse | None:
    """Restore a card from the trash.

     Restores this card only. It returns to its parent when that parent is live and reachable, otherwise
    to its
    nearest live, reachable ancestor or the top level. Children promoted when it was trashed stay where
    they are;
    descendants trashed with it stay in the trash, and come back under it when restored. A card in
    another
    project is not found, and one that is not in the trash is a Conflict.

    Args:
        project_id (str):
        card_id (str):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        CardResponse | ErrorResponse
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
) -> Response[CardResponse | ErrorResponse]:
    """Restore a card from the trash.

     Restores this card only. It returns to its parent when that parent is live and reachable, otherwise
    to its
    nearest live, reachable ancestor or the top level. Children promoted when it was trashed stay where
    they are;
    descendants trashed with it stay in the trash, and come back under it when restored. A card in
    another
    project is not found, and one that is not in the trash is a Conflict.

    Args:
        project_id (str):
        card_id (str):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[CardResponse | ErrorResponse]
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
) -> CardResponse | ErrorResponse | None:
    """Restore a card from the trash.

     Restores this card only. It returns to its parent when that parent is live and reachable, otherwise
    to its
    nearest live, reachable ancestor or the top level. Children promoted when it was trashed stay where
    they are;
    descendants trashed with it stay in the trash, and come back under it when restored. A card in
    another
    project is not found, and one that is not in the trash is a Conflict.

    Args:
        project_id (str):
        card_id (str):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        CardResponse | ErrorResponse
    """

    return (
        await asyncio_detailed(
            project_id=project_id,
            card_id=card_id,
            client=client,
        )
    ).parsed
