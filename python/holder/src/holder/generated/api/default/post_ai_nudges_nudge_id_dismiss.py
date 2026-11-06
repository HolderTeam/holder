from http import HTTPStatus
from typing import Any
from urllib.parse import quote

import httpx

from ... import errors
from ...client import AuthenticatedClient, Client
from ...models.ai_nudge_dismiss_response import AiNudgeDismissResponse
from ...models.error_response import ErrorResponse
from ...types import Response


def _get_kwargs(
    nudge_id: str,
) -> dict[str, Any]:

    _kwargs: dict[str, Any] = {
        "method": "post",
        "url": "/ai/nudges/{nudge_id}/dismiss".format(
            nudge_id=quote(str(nudge_id), safe=""),
        ),
    }

    return _kwargs


def _parse_response(
    *, client: AuthenticatedClient | Client, response: httpx.Response
) -> AiNudgeDismissResponse | ErrorResponse | None:
    if response.status_code == 200:
        response_200 = AiNudgeDismissResponse.from_dict(response.json())

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
) -> Response[AiNudgeDismissResponse | ErrorResponse]:
    return Response(
        status_code=HTTPStatus(response.status_code),
        content=response.content,
        headers=response.headers,
        parsed=_parse_response(client=client, response=response),
    )


def sync_detailed(
    nudge_id: str,
    *,
    client: AuthenticatedClient | Client,
) -> Response[AiNudgeDismissResponse | ErrorResponse]:
    """Dismiss a nudge

     Dismiss a nudge

    Args:
        nudge_id (str):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[AiNudgeDismissResponse | ErrorResponse]
    """

    kwargs = _get_kwargs(
        nudge_id=nudge_id,
    )

    response = client.get_httpx_client().request(
        **kwargs,
    )

    return _build_response(client=client, response=response)


def sync(
    nudge_id: str,
    *,
    client: AuthenticatedClient | Client,
) -> AiNudgeDismissResponse | ErrorResponse | None:
    """Dismiss a nudge

     Dismiss a nudge

    Args:
        nudge_id (str):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        AiNudgeDismissResponse | ErrorResponse
    """

    return sync_detailed(
        nudge_id=nudge_id,
        client=client,
    ).parsed


async def asyncio_detailed(
    nudge_id: str,
    *,
    client: AuthenticatedClient | Client,
) -> Response[AiNudgeDismissResponse | ErrorResponse]:
    """Dismiss a nudge

     Dismiss a nudge

    Args:
        nudge_id (str):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[AiNudgeDismissResponse | ErrorResponse]
    """

    kwargs = _get_kwargs(
        nudge_id=nudge_id,
    )

    response = await client.get_async_httpx_client().request(**kwargs)

    return _build_response(client=client, response=response)


async def asyncio(
    nudge_id: str,
    *,
    client: AuthenticatedClient | Client,
) -> AiNudgeDismissResponse | ErrorResponse | None:
    """Dismiss a nudge

     Dismiss a nudge

    Args:
        nudge_id (str):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        AiNudgeDismissResponse | ErrorResponse
    """

    return (
        await asyncio_detailed(
            nudge_id=nudge_id,
            client=client,
        )
    ).parsed
