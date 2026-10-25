from http import HTTPStatus
from typing import Any
from urllib.parse import quote

import httpx

from ... import errors
from ...client import AuthenticatedClient, Client
from ...models.error_response import ErrorResponse
from ...models.milestone_remove_response import MilestoneRemoveResponse
from ...types import Response


def _get_kwargs(
    card_id: str,
    milestone_id: str,
) -> dict[str, Any]:

    _kwargs: dict[str, Any] = {
        "method": "delete",
        "url": "/cards/{card_id}/milestones/{milestone_id}".format(
            card_id=quote(str(card_id), safe=""),
            milestone_id=quote(str(milestone_id), safe=""),
        ),
    }

    return _kwargs


def _parse_response(
    *, client: AuthenticatedClient | Client, response: httpx.Response
) -> ErrorResponse | MilestoneRemoveResponse | None:
    if response.status_code == 200:
        response_200 = MilestoneRemoveResponse.from_dict(response.json())

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

    if response.status_code == 501:
        response_501 = ErrorResponse.from_dict(response.json())

        return response_501

    if response.status_code == 503:
        response_503 = ErrorResponse.from_dict(response.json())

        return response_503

    if client.raise_on_unexpected_status:
        raise errors.UnexpectedStatus(response.status_code, response.content)
    else:
        return None


def _build_response(
    *, client: AuthenticatedClient | Client, response: httpx.Response
) -> Response[ErrorResponse | MilestoneRemoveResponse]:
    return Response(
        status_code=HTTPStatus(response.status_code),
        content=response.content,
        headers=response.headers,
        parsed=_parse_response(client=client, response=response),
    )


def sync_detailed(
    card_id: str,
    milestone_id: str,
    *,
    client: AuthenticatedClient | Client,
) -> Response[ErrorResponse | MilestoneRemoveResponse]:
    """Remove a milestone from a card

     Removing a milestone that is already absent succeeds with removed false.

    Args:
        card_id (str):
        milestone_id (str):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[ErrorResponse | MilestoneRemoveResponse]
    """

    kwargs = _get_kwargs(
        card_id=card_id,
        milestone_id=milestone_id,
    )

    response = client.get_httpx_client().request(
        **kwargs,
    )

    return _build_response(client=client, response=response)


def sync(
    card_id: str,
    milestone_id: str,
    *,
    client: AuthenticatedClient | Client,
) -> ErrorResponse | MilestoneRemoveResponse | None:
    """Remove a milestone from a card

     Removing a milestone that is already absent succeeds with removed false.

    Args:
        card_id (str):
        milestone_id (str):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        ErrorResponse | MilestoneRemoveResponse
    """

    return sync_detailed(
        card_id=card_id,
        milestone_id=milestone_id,
        client=client,
    ).parsed


async def asyncio_detailed(
    card_id: str,
    milestone_id: str,
    *,
    client: AuthenticatedClient | Client,
) -> Response[ErrorResponse | MilestoneRemoveResponse]:
    """Remove a milestone from a card

     Removing a milestone that is already absent succeeds with removed false.

    Args:
        card_id (str):
        milestone_id (str):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[ErrorResponse | MilestoneRemoveResponse]
    """

    kwargs = _get_kwargs(
        card_id=card_id,
        milestone_id=milestone_id,
    )

    response = await client.get_async_httpx_client().request(**kwargs)

    return _build_response(client=client, response=response)


async def asyncio(
    card_id: str,
    milestone_id: str,
    *,
    client: AuthenticatedClient | Client,
) -> ErrorResponse | MilestoneRemoveResponse | None:
    """Remove a milestone from a card

     Removing a milestone that is already absent succeeds with removed false.

    Args:
        card_id (str):
        milestone_id (str):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        ErrorResponse | MilestoneRemoveResponse
    """

    return (
        await asyncio_detailed(
            card_id=card_id,
            milestone_id=milestone_id,
            client=client,
        )
    ).parsed
