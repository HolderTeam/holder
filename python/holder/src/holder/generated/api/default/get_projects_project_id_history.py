from http import HTTPStatus
from typing import Any
from urllib.parse import quote

import httpx

from ... import errors
from ...client import AuthenticatedClient, Client
from ...models.error_response import ErrorResponse
from ...models.history_object_kind import HistoryObjectKind
from ...models.project_history_page_response import ProjectHistoryPageResponse
from ...types import UNSET, Response, Unset


def _get_kwargs(
    project_id: str,
    *,
    limit: int | Unset = 50,
    cursor: None | str | Unset = UNSET,
    kind: HistoryObjectKind | None | Unset = UNSET,
) -> dict[str, Any]:

    params: dict[str, Any] = {}

    params["limit"] = limit

    json_cursor: None | str | Unset
    if isinstance(cursor, Unset):
        json_cursor = UNSET
    else:
        json_cursor = cursor
    params["cursor"] = json_cursor

    json_kind: None | str | Unset
    if isinstance(kind, Unset):
        json_kind = UNSET
    elif isinstance(kind, HistoryObjectKind):
        json_kind = kind.value
    else:
        json_kind = kind
    params["kind"] = json_kind

    params = {k: v for k, v in params.items() if v is not UNSET and v is not None}

    _kwargs: dict[str, Any] = {
        "method": "get",
        "url": "/projects/{project_id}/history".format(
            project_id=quote(str(project_id), safe=""),
        ),
        "params": params,
    }

    return _kwargs


def _parse_response(
    *, client: AuthenticatedClient | Client, response: httpx.Response
) -> ErrorResponse | ProjectHistoryPageResponse | None:
    if response.status_code == 200:
        response_200 = ProjectHistoryPageResponse.from_dict(response.json())

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

    if response.status_code == 413:
        response_413 = ErrorResponse.from_dict(response.json())

        return response_413

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
) -> Response[ErrorResponse | ProjectHistoryPageResponse]:
    return Response(
        status_code=HTTPStatus(response.status_code),
        content=response.content,
        headers=response.headers,
        parsed=_parse_response(client=client, response=response),
    )


def sync_detailed(
    project_id: str,
    *,
    client: AuthenticatedClient | Client,
    limit: int | Unset = 50,
    cursor: None | str | Unset = UNSET,
    kind: HistoryObjectKind | None | Unset = UNSET,
) -> Response[ErrorResponse | ProjectHistoryPageResponse]:
    """Return bounded, cursor-paginated Git activities for a project.

     Each commit groups its changed paths by Holder object kind. Unrecognised paths remain visible as
    unknown.
    A missing decryption key is HistoryKeyUnavailable; other history read failures are
    HistoryUnavailable.

    Args:
        project_id (str):
        limit (int | Unset):  Default: 50.
        cursor (None | str | Unset): A full commit OID from next_cursor.
        kind (HistoryObjectKind | None | Unset): Only activities affecting this object kind.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[ErrorResponse | ProjectHistoryPageResponse]
    """

    kwargs = _get_kwargs(
        project_id=project_id,
        limit=limit,
        cursor=cursor,
        kind=kind,
    )

    response = client.get_httpx_client().request(
        **kwargs,
    )

    return _build_response(client=client, response=response)


def sync(
    project_id: str,
    *,
    client: AuthenticatedClient | Client,
    limit: int | Unset = 50,
    cursor: None | str | Unset = UNSET,
    kind: HistoryObjectKind | None | Unset = UNSET,
) -> ErrorResponse | ProjectHistoryPageResponse | None:
    """Return bounded, cursor-paginated Git activities for a project.

     Each commit groups its changed paths by Holder object kind. Unrecognised paths remain visible as
    unknown.
    A missing decryption key is HistoryKeyUnavailable; other history read failures are
    HistoryUnavailable.

    Args:
        project_id (str):
        limit (int | Unset):  Default: 50.
        cursor (None | str | Unset): A full commit OID from next_cursor.
        kind (HistoryObjectKind | None | Unset): Only activities affecting this object kind.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        ErrorResponse | ProjectHistoryPageResponse
    """

    return sync_detailed(
        project_id=project_id,
        client=client,
        limit=limit,
        cursor=cursor,
        kind=kind,
    ).parsed


async def asyncio_detailed(
    project_id: str,
    *,
    client: AuthenticatedClient | Client,
    limit: int | Unset = 50,
    cursor: None | str | Unset = UNSET,
    kind: HistoryObjectKind | None | Unset = UNSET,
) -> Response[ErrorResponse | ProjectHistoryPageResponse]:
    """Return bounded, cursor-paginated Git activities for a project.

     Each commit groups its changed paths by Holder object kind. Unrecognised paths remain visible as
    unknown.
    A missing decryption key is HistoryKeyUnavailable; other history read failures are
    HistoryUnavailable.

    Args:
        project_id (str):
        limit (int | Unset):  Default: 50.
        cursor (None | str | Unset): A full commit OID from next_cursor.
        kind (HistoryObjectKind | None | Unset): Only activities affecting this object kind.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[ErrorResponse | ProjectHistoryPageResponse]
    """

    kwargs = _get_kwargs(
        project_id=project_id,
        limit=limit,
        cursor=cursor,
        kind=kind,
    )

    response = await client.get_async_httpx_client().request(**kwargs)

    return _build_response(client=client, response=response)


async def asyncio(
    project_id: str,
    *,
    client: AuthenticatedClient | Client,
    limit: int | Unset = 50,
    cursor: None | str | Unset = UNSET,
    kind: HistoryObjectKind | None | Unset = UNSET,
) -> ErrorResponse | ProjectHistoryPageResponse | None:
    """Return bounded, cursor-paginated Git activities for a project.

     Each commit groups its changed paths by Holder object kind. Unrecognised paths remain visible as
    unknown.
    A missing decryption key is HistoryKeyUnavailable; other history read failures are
    HistoryUnavailable.

    Args:
        project_id (str):
        limit (int | Unset):  Default: 50.
        cursor (None | str | Unset): A full commit OID from next_cursor.
        kind (HistoryObjectKind | None | Unset): Only activities affecting this object kind.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        ErrorResponse | ProjectHistoryPageResponse
    """

    return (
        await asyncio_detailed(
            project_id=project_id,
            client=client,
            limit=limit,
            cursor=cursor,
            kind=kind,
        )
    ).parsed
