from http import HTTPStatus
from typing import Any
from urllib.parse import quote

import httpx

from ... import errors
from ...client import AuthenticatedClient, Client
from ...models.conflict_resolution import ConflictResolution
from ...models.conflict_resolution_result_response import (
    ConflictResolutionResultResponse,
)
from ...models.error_response import ErrorResponse
from ...types import Response


def _get_kwargs(
    project_id: str,
    copy_id: str,
    *,
    body: ConflictResolution,
) -> dict[str, Any]:
    headers: dict[str, Any] = {}

    _kwargs: dict[str, Any] = {
        "method": "post",
        "url": "/projects/{project_id}/git/conflict-copies/{copy_id}/resolution".format(
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
) -> ConflictResolutionResultResponse | ErrorResponse | None:
    if response.status_code == 200:
        response_200 = ConflictResolutionResultResponse.from_dict(response.json())

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
) -> Response[ConflictResolutionResultResponse | ErrorResponse]:
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
    body: ConflictResolution,
) -> Response[ConflictResolutionResultResponse | ErrorResponse]:
    """Save reviewed text to the original card without changing the preserved copy.

     Holder verifies the token against both live cards before changing anything. A stale token is
    Conflict. A missing association or a card that is missing or in Trash is NotFound. Changes
    to the title and content make a new Git commit. The original's identity, hierarchy, links and
    milestones are retained. Saving the existing title and content makes no commit. The result
    includes a review for optional cleanup of the preserved copy.

    Args:
        project_id (str):
        copy_id (str):
        body (ConflictResolution): Title and body to keep on the original card.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[ConflictResolutionResultResponse | ErrorResponse]
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
    body: ConflictResolution,
) -> ConflictResolutionResultResponse | ErrorResponse | None:
    """Save reviewed text to the original card without changing the preserved copy.

     Holder verifies the token against both live cards before changing anything. A stale token is
    Conflict. A missing association or a card that is missing or in Trash is NotFound. Changes
    to the title and content make a new Git commit. The original's identity, hierarchy, links and
    milestones are retained. Saving the existing title and content makes no commit. The result
    includes a review for optional cleanup of the preserved copy.

    Args:
        project_id (str):
        copy_id (str):
        body (ConflictResolution): Title and body to keep on the original card.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        ConflictResolutionResultResponse | ErrorResponse
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
    body: ConflictResolution,
) -> Response[ConflictResolutionResultResponse | ErrorResponse]:
    """Save reviewed text to the original card without changing the preserved copy.

     Holder verifies the token against both live cards before changing anything. A stale token is
    Conflict. A missing association or a card that is missing or in Trash is NotFound. Changes
    to the title and content make a new Git commit. The original's identity, hierarchy, links and
    milestones are retained. Saving the existing title and content makes no commit. The result
    includes a review for optional cleanup of the preserved copy.

    Args:
        project_id (str):
        copy_id (str):
        body (ConflictResolution): Title and body to keep on the original card.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[ConflictResolutionResultResponse | ErrorResponse]
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
    body: ConflictResolution,
) -> ConflictResolutionResultResponse | ErrorResponse | None:
    """Save reviewed text to the original card without changing the preserved copy.

     Holder verifies the token against both live cards before changing anything. A stale token is
    Conflict. A missing association or a card that is missing or in Trash is NotFound. Changes
    to the title and content make a new Git commit. The original's identity, hierarchy, links and
    milestones are retained. Saving the existing title and content makes no commit. The result
    includes a review for optional cleanup of the preserved copy.

    Args:
        project_id (str):
        copy_id (str):
        body (ConflictResolution): Title and body to keep on the original card.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        ConflictResolutionResultResponse | ErrorResponse
    """

    return (
        await asyncio_detailed(
            project_id=project_id,
            copy_id=copy_id,
            client=client,
            body=body,
        )
    ).parsed
