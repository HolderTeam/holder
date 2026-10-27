from http import HTTPStatus
from typing import Any
from urllib.parse import quote

import httpx

from ... import errors
from ...client import AuthenticatedClient, Client
from ...models.error_response import ErrorResponse
from ...models.link_list_response import LinkListResponse
from ...models.link_target_type import LinkTargetType
from ...types import UNSET, Response, Unset


def _get_kwargs(
    project_id: str,
    card_id: str,
    *,
    target_type: LinkTargetType,
    target_id: str,
    kind: str | Unset = UNSET,
    target_project_id: None | str | Unset = UNSET,
) -> dict[str, Any]:

    params: dict[str, Any] = {}

    json_target_type = target_type.value
    params["target_type"] = json_target_type

    params["target_id"] = target_id

    params["kind"] = kind

    json_target_project_id: None | str | Unset
    if isinstance(target_project_id, Unset):
        json_target_project_id = UNSET
    else:
        json_target_project_id = target_project_id
    params["target_project_id"] = json_target_project_id

    params = {k: v for k, v in params.items() if v is not UNSET and v is not None}

    _kwargs: dict[str, Any] = {
        "method": "delete",
        "url": "/projects/{project_id}/cards/{card_id}/links".format(
            project_id=quote(str(project_id), safe=""),
            card_id=quote(str(card_id), safe=""),
        ),
        "params": params,
    }

    return _kwargs


def _parse_response(
    *, client: AuthenticatedClient | Client, response: httpx.Response
) -> ErrorResponse | LinkListResponse | None:
    if response.status_code == 200:
        response_200 = LinkListResponse.from_dict(response.json())

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
) -> Response[ErrorResponse | LinkListResponse]:
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
    target_type: LinkTargetType,
    target_id: str,
    kind: str | Unset = UNSET,
    target_project_id: None | str | Unset = UNSET,
) -> Response[ErrorResponse | LinkListResponse]:
    """Remove a link from a card.

     Removes the one link to that target, of that type in that project, with that kind, and returns the
    card's
    remaining links as list_card_links lists them by default. A card without that link is left as it is.
    A card in
    the trash or in another project is not found.

    Args:
        project_id (str):
        card_id (str):
        target_type (LinkTargetType):
        target_id (str):
        kind (str | Unset):
        target_project_id (None | str | Unset): Omit for the card's own project.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[ErrorResponse | LinkListResponse]
    """

    kwargs = _get_kwargs(
        project_id=project_id,
        card_id=card_id,
        target_type=target_type,
        target_id=target_id,
        kind=kind,
        target_project_id=target_project_id,
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
    target_type: LinkTargetType,
    target_id: str,
    kind: str | Unset = UNSET,
    target_project_id: None | str | Unset = UNSET,
) -> ErrorResponse | LinkListResponse | None:
    """Remove a link from a card.

     Removes the one link to that target, of that type in that project, with that kind, and returns the
    card's
    remaining links as list_card_links lists them by default. A card without that link is left as it is.
    A card in
    the trash or in another project is not found.

    Args:
        project_id (str):
        card_id (str):
        target_type (LinkTargetType):
        target_id (str):
        kind (str | Unset):
        target_project_id (None | str | Unset): Omit for the card's own project.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        ErrorResponse | LinkListResponse
    """

    return sync_detailed(
        project_id=project_id,
        card_id=card_id,
        client=client,
        target_type=target_type,
        target_id=target_id,
        kind=kind,
        target_project_id=target_project_id,
    ).parsed


async def asyncio_detailed(
    project_id: str,
    card_id: str,
    *,
    client: AuthenticatedClient | Client,
    target_type: LinkTargetType,
    target_id: str,
    kind: str | Unset = UNSET,
    target_project_id: None | str | Unset = UNSET,
) -> Response[ErrorResponse | LinkListResponse]:
    """Remove a link from a card.

     Removes the one link to that target, of that type in that project, with that kind, and returns the
    card's
    remaining links as list_card_links lists them by default. A card without that link is left as it is.
    A card in
    the trash or in another project is not found.

    Args:
        project_id (str):
        card_id (str):
        target_type (LinkTargetType):
        target_id (str):
        kind (str | Unset):
        target_project_id (None | str | Unset): Omit for the card's own project.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[ErrorResponse | LinkListResponse]
    """

    kwargs = _get_kwargs(
        project_id=project_id,
        card_id=card_id,
        target_type=target_type,
        target_id=target_id,
        kind=kind,
        target_project_id=target_project_id,
    )

    response = await client.get_async_httpx_client().request(**kwargs)

    return _build_response(client=client, response=response)


async def asyncio(
    project_id: str,
    card_id: str,
    *,
    client: AuthenticatedClient | Client,
    target_type: LinkTargetType,
    target_id: str,
    kind: str | Unset = UNSET,
    target_project_id: None | str | Unset = UNSET,
) -> ErrorResponse | LinkListResponse | None:
    """Remove a link from a card.

     Removes the one link to that target, of that type in that project, with that kind, and returns the
    card's
    remaining links as list_card_links lists them by default. A card without that link is left as it is.
    A card in
    the trash or in another project is not found.

    Args:
        project_id (str):
        card_id (str):
        target_type (LinkTargetType):
        target_id (str):
        kind (str | Unset):
        target_project_id (None | str | Unset): Omit for the card's own project.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        ErrorResponse | LinkListResponse
    """

    return (
        await asyncio_detailed(
            project_id=project_id,
            card_id=card_id,
            client=client,
            target_type=target_type,
            target_id=target_id,
            kind=kind,
            target_project_id=target_project_id,
        )
    ).parsed
