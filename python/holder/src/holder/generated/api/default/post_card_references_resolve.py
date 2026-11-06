from http import HTTPStatus
from typing import Any

import httpx

from ... import errors
from ...client import AuthenticatedClient, Client
from ...models.card_reference_resolve_data_response import (
    CardReferenceResolveDataResponse,
)
from ...models.card_reference_resolve_request import CardReferenceResolveRequest
from ...models.error_response import ErrorResponse
from ...types import Response


def _get_kwargs(
    *,
    body: CardReferenceResolveRequest,
) -> dict[str, Any]:
    headers: dict[str, Any] = {}

    _kwargs: dict[str, Any] = {
        "method": "post",
        "url": "/card-references/resolve",
    }

    _kwargs["json"] = body.to_dict()

    headers["Content-Type"] = "application/json"

    _kwargs["headers"] = headers
    return _kwargs


def _parse_response(
    *, client: AuthenticatedClient | Client, response: httpx.Response
) -> CardReferenceResolveDataResponse | ErrorResponse | None:
    if response.status_code == 200:
        response_200 = CardReferenceResolveDataResponse.from_dict(response.json())

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
) -> Response[CardReferenceResolveDataResponse | ErrorResponse]:
    return Response(
        status_code=HTTPStatus(response.status_code),
        content=response.content,
        headers=response.headers,
        parsed=_parse_response(client=client, response=response),
    )


def sync_detailed(
    *,
    client: AuthenticatedClient | Client,
    body: CardReferenceResolveRequest,
) -> Response[CardReferenceResolveDataResponse | ErrorResponse]:
    """Resolve a card reference within a project and deletion scope.

     Resolve a card reference within a project and deletion scope.

    Args:
        body (CardReferenceResolveRequest): A reference to resolve among one project's cards.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[CardReferenceResolveDataResponse | ErrorResponse]
    """

    kwargs = _get_kwargs(
        body=body,
    )

    response = client.get_httpx_client().request(
        **kwargs,
    )

    return _build_response(client=client, response=response)


def sync(
    *,
    client: AuthenticatedClient | Client,
    body: CardReferenceResolveRequest,
) -> CardReferenceResolveDataResponse | ErrorResponse | None:
    """Resolve a card reference within a project and deletion scope.

     Resolve a card reference within a project and deletion scope.

    Args:
        body (CardReferenceResolveRequest): A reference to resolve among one project's cards.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        CardReferenceResolveDataResponse | ErrorResponse
    """

    return sync_detailed(
        client=client,
        body=body,
    ).parsed


async def asyncio_detailed(
    *,
    client: AuthenticatedClient | Client,
    body: CardReferenceResolveRequest,
) -> Response[CardReferenceResolveDataResponse | ErrorResponse]:
    """Resolve a card reference within a project and deletion scope.

     Resolve a card reference within a project and deletion scope.

    Args:
        body (CardReferenceResolveRequest): A reference to resolve among one project's cards.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[CardReferenceResolveDataResponse | ErrorResponse]
    """

    kwargs = _get_kwargs(
        body=body,
    )

    response = await client.get_async_httpx_client().request(**kwargs)

    return _build_response(client=client, response=response)


async def asyncio(
    *,
    client: AuthenticatedClient | Client,
    body: CardReferenceResolveRequest,
) -> CardReferenceResolveDataResponse | ErrorResponse | None:
    """Resolve a card reference within a project and deletion scope.

     Resolve a card reference within a project and deletion scope.

    Args:
        body (CardReferenceResolveRequest): A reference to resolve among one project's cards.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        CardReferenceResolveDataResponse | ErrorResponse
    """

    return (
        await asyncio_detailed(
            client=client,
            body=body,
        )
    ).parsed
