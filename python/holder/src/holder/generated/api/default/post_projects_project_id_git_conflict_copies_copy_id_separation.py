from http import HTTPStatus
from typing import Any
from urllib.parse import quote

import httpx

from ... import errors
from ...client import AuthenticatedClient, Client
from ...models.card_response import CardResponse
from ...models.error_response import ErrorResponse
from ...models.separate_conflict_copy_request import SeparateConflictCopyRequest
from ...types import Response


def _get_kwargs(
    project_id: str,
    copy_id: str,
    *,
    body: SeparateConflictCopyRequest,
) -> dict[str, Any]:
    headers: dict[str, Any] = {}

    _kwargs: dict[str, Any] = {
        "method": "post",
        "url": "/projects/{project_id}/git/conflict-copies/{copy_id}/separation".format(
            project_id=quote(str(project_id), safe=""),
            copy_id=quote(str(copy_id), safe=""),
        ),
    }

    _kwargs["json"] = body.to_dict()

    headers["Content-Type"] = "application/json"

    _kwargs["headers"] = headers
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
    copy_id: str,
    *,
    client: AuthenticatedClient | Client,
    body: SeparateConflictCopyRequest,
) -> Response[CardResponse | ErrorResponse]:
    """Keep a preserved copy as an ordinary card instead of resolving it into the original.

     Holder verifies the token against both live cards before changing anything. A stale token is
    Conflict. A missing association or a card that is missing or in Trash is NotFound. The copy takes
    the new title and loses its recorded association with the original, in one Git commit, so it is
    no longer listed as a conflict copy. Both cards keep their content, place, links and milestones.

    Args:
        project_id (str):
        copy_id (str):
        body (SeparateConflictCopyRequest): The reviewed pair and the title the preserved copy
            keeps as a card of its own.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[CardResponse | ErrorResponse]
    """

    kwargs = _get_kwargs(
        project_id=project_id,
        copy_id=copy_id,
        body=body,
    )

    response = client.get_httpx_client().request(
        **kwargs,
    )

    return _build_response(client=client, response=response)


def sync(
    project_id: str,
    copy_id: str,
    *,
    client: AuthenticatedClient | Client,
    body: SeparateConflictCopyRequest,
) -> CardResponse | ErrorResponse | None:
    """Keep a preserved copy as an ordinary card instead of resolving it into the original.

     Holder verifies the token against both live cards before changing anything. A stale token is
    Conflict. A missing association or a card that is missing or in Trash is NotFound. The copy takes
    the new title and loses its recorded association with the original, in one Git commit, so it is
    no longer listed as a conflict copy. Both cards keep their content, place, links and milestones.

    Args:
        project_id (str):
        copy_id (str):
        body (SeparateConflictCopyRequest): The reviewed pair and the title the preserved copy
            keeps as a card of its own.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        CardResponse | ErrorResponse
    """

    return sync_detailed(
        project_id=project_id,
        copy_id=copy_id,
        client=client,
        body=body,
    ).parsed


async def asyncio_detailed(
    project_id: str,
    copy_id: str,
    *,
    client: AuthenticatedClient | Client,
    body: SeparateConflictCopyRequest,
) -> Response[CardResponse | ErrorResponse]:
    """Keep a preserved copy as an ordinary card instead of resolving it into the original.

     Holder verifies the token against both live cards before changing anything. A stale token is
    Conflict. A missing association or a card that is missing or in Trash is NotFound. The copy takes
    the new title and loses its recorded association with the original, in one Git commit, so it is
    no longer listed as a conflict copy. Both cards keep their content, place, links and milestones.

    Args:
        project_id (str):
        copy_id (str):
        body (SeparateConflictCopyRequest): The reviewed pair and the title the preserved copy
            keeps as a card of its own.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[CardResponse | ErrorResponse]
    """

    kwargs = _get_kwargs(
        project_id=project_id,
        copy_id=copy_id,
        body=body,
    )

    response = await client.get_async_httpx_client().request(**kwargs)

    return _build_response(client=client, response=response)


async def asyncio(
    project_id: str,
    copy_id: str,
    *,
    client: AuthenticatedClient | Client,
    body: SeparateConflictCopyRequest,
) -> CardResponse | ErrorResponse | None:
    """Keep a preserved copy as an ordinary card instead of resolving it into the original.

     Holder verifies the token against both live cards before changing anything. A stale token is
    Conflict. A missing association or a card that is missing or in Trash is NotFound. The copy takes
    the new title and loses its recorded association with the original, in one Git commit, so it is
    no longer listed as a conflict copy. Both cards keep their content, place, links and milestones.

    Args:
        project_id (str):
        copy_id (str):
        body (SeparateConflictCopyRequest): The reviewed pair and the title the preserved copy
            keeps as a card of its own.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        CardResponse | ErrorResponse
    """

    return (
        await asyncio_detailed(
            project_id=project_id,
            copy_id=copy_id,
            client=client,
            body=body,
        )
    ).parsed
