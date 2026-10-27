from http import HTTPStatus
from typing import Any
from urllib.parse import quote

import httpx

from ... import errors
from ...client import AuthenticatedClient, Client
from ...models.card_response import CardResponse
from ...models.delete_cards_card_id_children import DeleteCardsCardIdChildren
from ...models.error_response import ErrorResponse
from ...types import UNSET, Response, Unset


def _get_kwargs(
    card_id: str,
    *,
    children: DeleteCardsCardIdChildren | Unset = DeleteCardsCardIdChildren.PROMOTE,
) -> dict[str, Any]:

    params: dict[str, Any] = {}

    json_children: str | Unset = UNSET
    if not isinstance(children, Unset):
        json_children = children.value

    params["children"] = json_children

    params = {k: v for k, v in params.items() if v is not UNSET and v is not None}

    _kwargs: dict[str, Any] = {
        "method": "delete",
        "url": "/cards/{card_id}".format(
            card_id=quote(str(card_id), safe=""),
        ),
        "params": params,
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
    card_id: str,
    *,
    client: AuthenticatedClient | Client,
    children: DeleteCardsCardIdChildren | Unset = DeleteCardsCardIdChildren.PROMOTE,
) -> Response[CardResponse | ErrorResponse]:
    """Move a card to the trash.

     The card and every card that changes with it are written as one change in the project's history. A
    card already
    in the trash is not found. Returns the card as it is in the trash.

    Args:
        card_id (str):
        children (DeleteCardsCardIdChildren | Unset): promote moves the card's live children into
            its place among its siblings, keeping their own descendants; trash_subtree moves every
            live descendant to the trash with it. Default: DeleteCardsCardIdChildren.PROMOTE.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[CardResponse | ErrorResponse]
    """

    kwargs = _get_kwargs(
        card_id=card_id,
        children=children,
    )

    response = client.get_httpx_client().request(
        **kwargs,
    )

    return _build_response(client=client, response=response)


def sync(
    card_id: str,
    *,
    client: AuthenticatedClient | Client,
    children: DeleteCardsCardIdChildren | Unset = DeleteCardsCardIdChildren.PROMOTE,
) -> CardResponse | ErrorResponse | None:
    """Move a card to the trash.

     The card and every card that changes with it are written as one change in the project's history. A
    card already
    in the trash is not found. Returns the card as it is in the trash.

    Args:
        card_id (str):
        children (DeleteCardsCardIdChildren | Unset): promote moves the card's live children into
            its place among its siblings, keeping their own descendants; trash_subtree moves every
            live descendant to the trash with it. Default: DeleteCardsCardIdChildren.PROMOTE.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        CardResponse | ErrorResponse
    """

    return sync_detailed(
        card_id=card_id,
        client=client,
        children=children,
    ).parsed


async def asyncio_detailed(
    card_id: str,
    *,
    client: AuthenticatedClient | Client,
    children: DeleteCardsCardIdChildren | Unset = DeleteCardsCardIdChildren.PROMOTE,
) -> Response[CardResponse | ErrorResponse]:
    """Move a card to the trash.

     The card and every card that changes with it are written as one change in the project's history. A
    card already
    in the trash is not found. Returns the card as it is in the trash.

    Args:
        card_id (str):
        children (DeleteCardsCardIdChildren | Unset): promote moves the card's live children into
            its place among its siblings, keeping their own descendants; trash_subtree moves every
            live descendant to the trash with it. Default: DeleteCardsCardIdChildren.PROMOTE.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[CardResponse | ErrorResponse]
    """

    kwargs = _get_kwargs(
        card_id=card_id,
        children=children,
    )

    response = await client.get_async_httpx_client().request(**kwargs)

    return _build_response(client=client, response=response)


async def asyncio(
    card_id: str,
    *,
    client: AuthenticatedClient | Client,
    children: DeleteCardsCardIdChildren | Unset = DeleteCardsCardIdChildren.PROMOTE,
) -> CardResponse | ErrorResponse | None:
    """Move a card to the trash.

     The card and every card that changes with it are written as one change in the project's history. A
    card already
    in the trash is not found. Returns the card as it is in the trash.

    Args:
        card_id (str):
        children (DeleteCardsCardIdChildren | Unset): promote moves the card's live children into
            its place among its siblings, keeping their own descendants; trash_subtree moves every
            live descendant to the trash with it. Default: DeleteCardsCardIdChildren.PROMOTE.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        CardResponse | ErrorResponse
    """

    return (
        await asyncio_detailed(
            card_id=card_id,
            client=client,
            children=children,
        )
    ).parsed
