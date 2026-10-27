from http import HTTPStatus
from typing import Any
from urllib.parse import quote

import httpx

from ... import errors
from ...client import AuthenticatedClient, Client
from ...models.error_response import ErrorResponse
from ...models.link_response import LinkResponse
from ...models.new_link import NewLink
from ...types import Response


def _get_kwargs(
    project_id: str,
    card_id: str,
    *,
    body: NewLink,
) -> dict[str, Any]:
    headers: dict[str, Any] = {}

    _kwargs: dict[str, Any] = {
        "method": "post",
        "url": "/projects/{project_id}/cards/{card_id}/links".format(
            project_id=quote(str(project_id), safe=""),
            card_id=quote(str(card_id), safe=""),
        ),
    }

    _kwargs["json"] = body.to_dict()

    headers["Content-Type"] = "application/json"

    _kwargs["headers"] = headers
    return _kwargs


def _parse_response(
    *, client: AuthenticatedClient | Client, response: httpx.Response
) -> ErrorResponse | LinkResponse | None:
    if response.status_code == 200:
        response_200 = LinkResponse.from_dict(response.json())

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
) -> Response[ErrorResponse | LinkResponse]:
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
    body: NewLink,
) -> Response[ErrorResponse | LinkResponse]:
    """Link a card to something.

     A link is one target, of one type in one project, with one kind. Making a link that already exists
    sets its
    label and keeps when it was made. A target in the card's project must exist and not be in the trash;
    a card in
    another project may be one this device does not have. A card in the trash or in another project is
    not found,
    and neither is a target that does not exist or is in the trash.

    Args:
        project_id (str):
        card_id (str):
        body (NewLink): A link to make from a card. Its source is the card it is made from.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[ErrorResponse | LinkResponse]
    """

    kwargs = _get_kwargs(
        project_id=project_id,
        card_id=card_id,
        body=body,
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
    body: NewLink,
) -> ErrorResponse | LinkResponse | None:
    """Link a card to something.

     A link is one target, of one type in one project, with one kind. Making a link that already exists
    sets its
    label and keeps when it was made. A target in the card's project must exist and not be in the trash;
    a card in
    another project may be one this device does not have. A card in the trash or in another project is
    not found,
    and neither is a target that does not exist or is in the trash.

    Args:
        project_id (str):
        card_id (str):
        body (NewLink): A link to make from a card. Its source is the card it is made from.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        ErrorResponse | LinkResponse
    """

    return sync_detailed(
        project_id=project_id,
        card_id=card_id,
        client=client,
        body=body,
    ).parsed


async def asyncio_detailed(
    project_id: str,
    card_id: str,
    *,
    client: AuthenticatedClient | Client,
    body: NewLink,
) -> Response[ErrorResponse | LinkResponse]:
    """Link a card to something.

     A link is one target, of one type in one project, with one kind. Making a link that already exists
    sets its
    label and keeps when it was made. A target in the card's project must exist and not be in the trash;
    a card in
    another project may be one this device does not have. A card in the trash or in another project is
    not found,
    and neither is a target that does not exist or is in the trash.

    Args:
        project_id (str):
        card_id (str):
        body (NewLink): A link to make from a card. Its source is the card it is made from.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[ErrorResponse | LinkResponse]
    """

    kwargs = _get_kwargs(
        project_id=project_id,
        card_id=card_id,
        body=body,
    )

    response = await client.get_async_httpx_client().request(**kwargs)

    return _build_response(client=client, response=response)


async def asyncio(
    project_id: str,
    card_id: str,
    *,
    client: AuthenticatedClient | Client,
    body: NewLink,
) -> ErrorResponse | LinkResponse | None:
    """Link a card to something.

     A link is one target, of one type in one project, with one kind. Making a link that already exists
    sets its
    label and keeps when it was made. A target in the card's project must exist and not be in the trash;
    a card in
    another project may be one this device does not have. A card in the trash or in another project is
    not found,
    and neither is a target that does not exist or is in the trash.

    Args:
        project_id (str):
        card_id (str):
        body (NewLink): A link to make from a card. Its source is the card it is made from.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        ErrorResponse | LinkResponse
    """

    return (
        await asyncio_detailed(
            project_id=project_id,
            card_id=card_id,
            client=client,
            body=body,
        )
    ).parsed
