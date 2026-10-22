from http import HTTPStatus
from typing import Any
from urllib.parse import quote

import httpx

from ... import errors
from ...client import AuthenticatedClient, Client
from ...models.card_patch_response import CardPatchResponse
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
) -> CardPatchResponse | ErrorResponse | None:
    if response.status_code == 200:
        response_200 = CardPatchResponse.from_dict(response.json())

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
) -> Response[CardPatchResponse | ErrorResponse]:
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
) -> Response[CardPatchResponse | ErrorResponse]:
    """Move card to trash

     The card and every changed card are written in one commit. With children=promote (the default), the
    card's immediate live children take its place among its siblings (or the project's top level) in
    their existing order, keeping their own descendants; children already in Trash stay there. With
    children=trash_subtree, every live descendant goes to Trash with the card and keeps its parent
    there, including live cards beneath descendants that were already trashed, which keep their own
    trash time. Restoring is always single-card: see POST /cards/{card_id}/restore.

    Args:
        card_id (str):
        children (DeleteCardsCardIdChildren | Unset):  Default: DeleteCardsCardIdChildren.PROMOTE.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[CardPatchResponse | ErrorResponse]
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
) -> CardPatchResponse | ErrorResponse | None:
    """Move card to trash

     The card and every changed card are written in one commit. With children=promote (the default), the
    card's immediate live children take its place among its siblings (or the project's top level) in
    their existing order, keeping their own descendants; children already in Trash stay there. With
    children=trash_subtree, every live descendant goes to Trash with the card and keeps its parent
    there, including live cards beneath descendants that were already trashed, which keep their own
    trash time. Restoring is always single-card: see POST /cards/{card_id}/restore.

    Args:
        card_id (str):
        children (DeleteCardsCardIdChildren | Unset):  Default: DeleteCardsCardIdChildren.PROMOTE.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        CardPatchResponse | ErrorResponse
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
) -> Response[CardPatchResponse | ErrorResponse]:
    """Move card to trash

     The card and every changed card are written in one commit. With children=promote (the default), the
    card's immediate live children take its place among its siblings (or the project's top level) in
    their existing order, keeping their own descendants; children already in Trash stay there. With
    children=trash_subtree, every live descendant goes to Trash with the card and keeps its parent
    there, including live cards beneath descendants that were already trashed, which keep their own
    trash time. Restoring is always single-card: see POST /cards/{card_id}/restore.

    Args:
        card_id (str):
        children (DeleteCardsCardIdChildren | Unset):  Default: DeleteCardsCardIdChildren.PROMOTE.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[CardPatchResponse | ErrorResponse]
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
) -> CardPatchResponse | ErrorResponse | None:
    """Move card to trash

     The card and every changed card are written in one commit. With children=promote (the default), the
    card's immediate live children take its place among its siblings (or the project's top level) in
    their existing order, keeping their own descendants; children already in Trash stay there. With
    children=trash_subtree, every live descendant goes to Trash with the card and keeps its parent
    there, including live cards beneath descendants that were already trashed, which keep their own
    trash time. Restoring is always single-card: see POST /cards/{card_id}/restore.

    Args:
        card_id (str):
        children (DeleteCardsCardIdChildren | Unset):  Default: DeleteCardsCardIdChildren.PROMOTE.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        CardPatchResponse | ErrorResponse
    """

    return (
        await asyncio_detailed(
            card_id=card_id,
            client=client,
            children=children,
        )
    ).parsed
