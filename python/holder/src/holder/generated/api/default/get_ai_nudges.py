from http import HTTPStatus
from typing import Any

import httpx

from ... import errors
from ...client import AuthenticatedClient, Client
from ...models.ai_nudge_list_response import AiNudgeListResponse
from ...models.error_response import ErrorResponse
from ...types import UNSET, Response, Unset


def _get_kwargs(
    *,
    project_id: str,
    card_id: None | str | Unset = UNSET,
) -> dict[str, Any]:

    params: dict[str, Any] = {}

    params["project_id"] = project_id

    json_card_id: None | str | Unset
    if isinstance(card_id, Unset):
        json_card_id = UNSET
    else:
        json_card_id = card_id
    params["card_id"] = json_card_id

    params = {k: v for k, v in params.items() if v is not UNSET and v is not None}

    _kwargs: dict[str, Any] = {
        "method": "get",
        "url": "/ai/nudges",
        "params": params,
    }

    return _kwargs


def _parse_response(
    *, client: AuthenticatedClient | Client, response: httpx.Response
) -> AiNudgeListResponse | ErrorResponse | None:
    if response.status_code == 200:
        response_200 = AiNudgeListResponse.from_dict(response.json())

        return response_200

    if response.status_code == 400:
        response_400 = ErrorResponse.from_dict(response.json())

        return response_400

    if response.status_code == 401:
        response_401 = ErrorResponse.from_dict(response.json())

        return response_401

    if response.status_code == 500:
        response_500 = ErrorResponse.from_dict(response.json())

        return response_500

    if client.raise_on_unexpected_status:
        raise errors.UnexpectedStatus(response.status_code, response.content)
    else:
        return None


def _build_response(
    *, client: AuthenticatedClient | Client, response: httpx.Response
) -> Response[AiNudgeListResponse | ErrorResponse]:
    return Response(
        status_code=HTTPStatus(response.status_code),
        content=response.content,
        headers=response.headers,
        parsed=_parse_response(client=client, response=response),
    )


def sync_detailed(
    *,
    client: AuthenticatedClient | Client,
    project_id: str,
    card_id: None | str | Unset = UNSET,
) -> Response[AiNudgeListResponse | ErrorResponse]:
    """List a project's current nudges

     Nudges whose evidence no longer holds are removed before listing, and dismissed nudges are left out.

    Args:
        project_id (str):
        card_id (None | str | Unset): Only this card's nudges; empty means every card's.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[AiNudgeListResponse | ErrorResponse]
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
    *,
    client: AuthenticatedClient | Client,
    project_id: str,
    card_id: None | str | Unset = UNSET,
) -> AiNudgeListResponse | ErrorResponse | None:
    """List a project's current nudges

     Nudges whose evidence no longer holds are removed before listing, and dismissed nudges are left out.

    Args:
        project_id (str):
        card_id (None | str | Unset): Only this card's nudges; empty means every card's.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        AiNudgeListResponse | ErrorResponse
    """

    return sync_detailed(
        client=client,
        project_id=project_id,
        card_id=card_id,
    ).parsed


async def asyncio_detailed(
    *,
    client: AuthenticatedClient | Client,
    project_id: str,
    card_id: None | str | Unset = UNSET,
) -> Response[AiNudgeListResponse | ErrorResponse]:
    """List a project's current nudges

     Nudges whose evidence no longer holds are removed before listing, and dismissed nudges are left out.

    Args:
        project_id (str):
        card_id (None | str | Unset): Only this card's nudges; empty means every card's.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[AiNudgeListResponse | ErrorResponse]
    """

    kwargs = _get_kwargs(
        project_id=project_id,
        card_id=card_id,
    )

    response = await client.get_async_httpx_client().request(**kwargs)

    return _build_response(client=client, response=response)


async def asyncio(
    *,
    client: AuthenticatedClient | Client,
    project_id: str,
    card_id: None | str | Unset = UNSET,
) -> AiNudgeListResponse | ErrorResponse | None:
    """List a project's current nudges

     Nudges whose evidence no longer holds are removed before listing, and dismissed nudges are left out.

    Args:
        project_id (str):
        card_id (None | str | Unset): Only this card's nudges; empty means every card's.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        AiNudgeListResponse | ErrorResponse
    """

    return (
        await asyncio_detailed(
            client=client,
            project_id=project_id,
            card_id=card_id,
        )
    ).parsed
