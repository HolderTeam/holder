from http import HTTPStatus
from typing import Any
from urllib.parse import quote

import httpx

from ... import errors
from ...client import AuthenticatedClient, Client
from ...models.card_metadata_response import CardMetadataResponse
from ...models.error_response import ErrorResponse
from ...models.trash_conflict_copy_request import TrashConflictCopyRequest
from ...types import Response


def _get_kwargs(
    project_id: str,
    copy_id: str,
    *,
    body: TrashConflictCopyRequest,
) -> dict[str, Any]:
    headers: dict[str, Any] = {}

    _kwargs: dict[str, Any] = {
        "method": "post",
        "url": "/projects/{project_id}/git/conflict-copies/{copy_id}/trash".format(
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
) -> CardMetadataResponse | ErrorResponse | None:
    if response.status_code == 200:
        response_200 = CardMetadataResponse.from_dict(response.json())

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
) -> Response[CardMetadataResponse | ErrorResponse]:
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
    body: TrashConflictCopyRequest,
) -> Response[CardMetadataResponse | ErrorResponse]:
    """Move a reviewed conflict copy to Trash after choosing a result.

     Holder verifies the cleanup token against both live cards and the reviewed effects.
    A stale token or cleanup that would move the original is Conflict. A missing association
    or a card missing or in Trash is NotFound.
    The original is untouched. The copy can be restored and its live children move up a level.

    Args:
        project_id (str):
        copy_id (str):
        body (TrashConflictCopyRequest): Confirmation of the cleanup effects returned when the
            resolution was saved.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[CardMetadataResponse | ErrorResponse]
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
    body: TrashConflictCopyRequest,
) -> CardMetadataResponse | ErrorResponse | None:
    """Move a reviewed conflict copy to Trash after choosing a result.

     Holder verifies the cleanup token against both live cards and the reviewed effects.
    A stale token or cleanup that would move the original is Conflict. A missing association
    or a card missing or in Trash is NotFound.
    The original is untouched. The copy can be restored and its live children move up a level.

    Args:
        project_id (str):
        copy_id (str):
        body (TrashConflictCopyRequest): Confirmation of the cleanup effects returned when the
            resolution was saved.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        CardMetadataResponse | ErrorResponse
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
    body: TrashConflictCopyRequest,
) -> Response[CardMetadataResponse | ErrorResponse]:
    """Move a reviewed conflict copy to Trash after choosing a result.

     Holder verifies the cleanup token against both live cards and the reviewed effects.
    A stale token or cleanup that would move the original is Conflict. A missing association
    or a card missing or in Trash is NotFound.
    The original is untouched. The copy can be restored and its live children move up a level.

    Args:
        project_id (str):
        copy_id (str):
        body (TrashConflictCopyRequest): Confirmation of the cleanup effects returned when the
            resolution was saved.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[CardMetadataResponse | ErrorResponse]
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
    body: TrashConflictCopyRequest,
) -> CardMetadataResponse | ErrorResponse | None:
    """Move a reviewed conflict copy to Trash after choosing a result.

     Holder verifies the cleanup token against both live cards and the reviewed effects.
    A stale token or cleanup that would move the original is Conflict. A missing association
    or a card missing or in Trash is NotFound.
    The original is untouched. The copy can be restored and its live children move up a level.

    Args:
        project_id (str):
        copy_id (str):
        body (TrashConflictCopyRequest): Confirmation of the cleanup effects returned when the
            resolution was saved.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        CardMetadataResponse | ErrorResponse
    """

    return (
        await asyncio_detailed(
            project_id=project_id,
            copy_id=copy_id,
            client=client,
            body=body,
        )
    ).parsed
