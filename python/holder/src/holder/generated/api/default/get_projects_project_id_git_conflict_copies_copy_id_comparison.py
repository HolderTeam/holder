from http import HTTPStatus
from typing import Any
from urllib.parse import quote

import httpx

from ... import errors
from ...client import AuthenticatedClient, Client
from ...models.conflict_comparison_response import ConflictComparisonResponse
from ...models.error_response import ErrorResponse
from ...types import Response


def _get_kwargs(
    project_id: str,
    copy_id: str,
) -> dict[str, Any]:

    _kwargs: dict[str, Any] = {
        "method": "get",
        "url": "/projects/{project_id}/git/conflict-copies/{copy_id}/comparison".format(
            project_id=quote(str(project_id), safe=""),
            copy_id=quote(str(copy_id), safe=""),
        ),
    }

    return _kwargs


def _parse_response(
    *, client: AuthenticatedClient | Client, response: httpx.Response
) -> ConflictComparisonResponse | ErrorResponse | None:
    if response.status_code == 200:
        response_200 = ConflictComparisonResponse.from_dict(response.json())

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
) -> Response[ConflictComparisonResponse | ErrorResponse]:
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
) -> Response[ConflictComparisonResponse | ErrorResponse]:
    """Compare a preserved copy with its recorded original.

     Reads both saved cards together. A missing association, an original or copy in Trash, or a card
    that no longer exists is NotFound. A pair changed during the read is Conflict. The operation does
    not
    change either card.

    Args:
        project_id (str):
        copy_id (str):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[ConflictComparisonResponse | ErrorResponse]
    """

    kwargs = _get_kwargs(
        project_id=project_id,
        copy_id=copy_id,
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
) -> ConflictComparisonResponse | ErrorResponse | None:
    """Compare a preserved copy with its recorded original.

     Reads both saved cards together. A missing association, an original or copy in Trash, or a card
    that no longer exists is NotFound. A pair changed during the read is Conflict. The operation does
    not
    change either card.

    Args:
        project_id (str):
        copy_id (str):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        ConflictComparisonResponse | ErrorResponse
    """

    return sync_detailed(
        project_id=project_id,
        copy_id=copy_id,
        client=client,
    ).parsed


async def asyncio_detailed(
    project_id: str,
    copy_id: str,
    *,
    client: AuthenticatedClient | Client,
) -> Response[ConflictComparisonResponse | ErrorResponse]:
    """Compare a preserved copy with its recorded original.

     Reads both saved cards together. A missing association, an original or copy in Trash, or a card
    that no longer exists is NotFound. A pair changed during the read is Conflict. The operation does
    not
    change either card.

    Args:
        project_id (str):
        copy_id (str):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[ConflictComparisonResponse | ErrorResponse]
    """

    kwargs = _get_kwargs(
        project_id=project_id,
        copy_id=copy_id,
    )

    response = await client.get_async_httpx_client().request(**kwargs)

    return _build_response(client=client, response=response)


async def asyncio(
    project_id: str,
    copy_id: str,
    *,
    client: AuthenticatedClient | Client,
) -> ConflictComparisonResponse | ErrorResponse | None:
    """Compare a preserved copy with its recorded original.

     Reads both saved cards together. A missing association, an original or copy in Trash, or a card
    that no longer exists is NotFound. A pair changed during the read is Conflict. The operation does
    not
    change either card.

    Args:
        project_id (str):
        copy_id (str):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        ConflictComparisonResponse | ErrorResponse
    """

    return (
        await asyncio_detailed(
            project_id=project_id,
            copy_id=copy_id,
            client=client,
        )
    ).parsed
