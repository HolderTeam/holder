from http import HTTPStatus
from typing import Any
from urllib.parse import quote

import httpx

from ... import errors
from ...client import AuthenticatedClient, Client
from ...models.error_response import ErrorResponse
from ...models.milestone_with_dates_list_response import MilestoneWithDatesListResponse
from ...types import Response


def _get_kwargs(
    card_id: str,
) -> dict[str, Any]:

    _kwargs: dict[str, Any] = {
        "method": "get",
        "url": "/cards/{card_id}/milestones-with-dates".format(
            card_id=quote(str(card_id), safe=""),
        ),
    }

    return _kwargs


def _parse_response(
    *, client: AuthenticatedClient | Client, response: httpx.Response
) -> ErrorResponse | MilestoneWithDatesListResponse | None:
    if response.status_code == 200:
        response_200 = MilestoneWithDatesListResponse.from_dict(response.json())

        return response_200

    if response.status_code == 401:
        response_401 = ErrorResponse.from_dict(response.json())

        return response_401

    if response.status_code == 404:
        response_404 = ErrorResponse.from_dict(response.json())

        return response_404

    if client.raise_on_unexpected_status:
        raise errors.UnexpectedStatus(response.status_code, response.content)
    else:
        return None


def _build_response(
    *, client: AuthenticatedClient | Client, response: httpx.Response
) -> Response[ErrorResponse | MilestoneWithDatesListResponse]:
    return Response(
        status_code=HTTPStatus(response.status_code),
        content=response.content,
        headers=response.headers,
        parsed=_parse_response(client=client, response=response),
    )


def sync_detailed(
    card_id: str,
    *,
    client: AuthenticatedClient | Client,
) -> Response[ErrorResponse | MilestoneWithDatesListResponse]:
    """List all milestones on a live card with their labelled dates

     Includes undated milestones and repeated roles. Dates remain in stored order. The existing
    /cards/{card_id}/milestones route remains a scheduled timestamp view.

    Args:
        card_id (str):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[ErrorResponse | MilestoneWithDatesListResponse]
    """

    kwargs = _get_kwargs(
        card_id=card_id,
    )

    response = client.get_httpx_client().request(
        **kwargs,
    )

    return _build_response(client=client, response=response)


def sync(
    card_id: str,
    *,
    client: AuthenticatedClient | Client,
) -> ErrorResponse | MilestoneWithDatesListResponse | None:
    """List all milestones on a live card with their labelled dates

     Includes undated milestones and repeated roles. Dates remain in stored order. The existing
    /cards/{card_id}/milestones route remains a scheduled timestamp view.

    Args:
        card_id (str):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        ErrorResponse | MilestoneWithDatesListResponse
    """

    return sync_detailed(
        card_id=card_id,
        client=client,
    ).parsed


async def asyncio_detailed(
    card_id: str,
    *,
    client: AuthenticatedClient | Client,
) -> Response[ErrorResponse | MilestoneWithDatesListResponse]:
    """List all milestones on a live card with their labelled dates

     Includes undated milestones and repeated roles. Dates remain in stored order. The existing
    /cards/{card_id}/milestones route remains a scheduled timestamp view.

    Args:
        card_id (str):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[ErrorResponse | MilestoneWithDatesListResponse]
    """

    kwargs = _get_kwargs(
        card_id=card_id,
    )

    response = await client.get_async_httpx_client().request(**kwargs)

    return _build_response(client=client, response=response)


async def asyncio(
    card_id: str,
    *,
    client: AuthenticatedClient | Client,
) -> ErrorResponse | MilestoneWithDatesListResponse | None:
    """List all milestones on a live card with their labelled dates

     Includes undated milestones and repeated roles. Dates remain in stored order. The existing
    /cards/{card_id}/milestones route remains a scheduled timestamp view.

    Args:
        card_id (str):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        ErrorResponse | MilestoneWithDatesListResponse
    """

    return (
        await asyncio_detailed(
            card_id=card_id,
            client=client,
        )
    ).parsed
