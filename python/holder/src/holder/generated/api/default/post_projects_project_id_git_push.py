from http import HTTPStatus
from typing import Any
from urllib.parse import quote

import httpx

from ... import errors
from ...client import AuthenticatedClient, Client
from ...models.error_response import ErrorResponse
from ...models.push_options import PushOptions
from ...models.sync_result_response import SyncResultResponse
from ...types import UNSET, Response, Unset


def _get_kwargs(
    project_id: str,
    *,
    body: PushOptions | Unset = UNSET,
) -> dict[str, Any]:
    headers: dict[str, Any] = {}

    _kwargs: dict[str, Any] = {
        "method": "post",
        "url": "/projects/{project_id}/git/push".format(
            project_id=quote(str(project_id), safe=""),
        ),
    }

    if not isinstance(body, Unset):
        _kwargs["json"] = body.to_dict()

    headers["Content-Type"] = "application/json"

    _kwargs["headers"] = headers
    return _kwargs


def _parse_response(
    *, client: AuthenticatedClient | Client, response: httpx.Response
) -> ErrorResponse | SyncResultResponse | None:
    if response.status_code == 200:
        response_200 = SyncResultResponse.from_dict(response.json())

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
) -> Response[ErrorResponse | SyncResultResponse]:
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
    body: PushOptions | Unset = UNSET,
) -> Response[ErrorResponse | SyncResultResponse]:
    """Push a project to its Git remote.

     An encrypted project is checked first, so no plaintext card leaves the device: one whose files are
    not safe
    to push is DataCorrupted, and nothing is pushed. A Git failure is an outcome in the result, recorded
    in the
    sync state.

    Args:
        project_id (str):
        body (PushOptions | Unset): How to push.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[ErrorResponse | SyncResultResponse]
    """

    kwargs = _get_kwargs(
        project_id=project_id,
        body=body,
    )

    response = client.get_httpx_client().request(
        **kwargs,
    )

    return _build_response(client=client, response=response)


def sync(
    project_id: str,
    *,
    client: AuthenticatedClient | Client,
    body: PushOptions | Unset = UNSET,
) -> ErrorResponse | SyncResultResponse | None:
    """Push a project to its Git remote.

     An encrypted project is checked first, so no plaintext card leaves the device: one whose files are
    not safe
    to push is DataCorrupted, and nothing is pushed. A Git failure is an outcome in the result, recorded
    in the
    sync state.

    Args:
        project_id (str):
        body (PushOptions | Unset): How to push.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        ErrorResponse | SyncResultResponse
    """

    return sync_detailed(
        project_id=project_id,
        client=client,
        body=body,
    ).parsed


async def asyncio_detailed(
    project_id: str,
    *,
    client: AuthenticatedClient | Client,
    body: PushOptions | Unset = UNSET,
) -> Response[ErrorResponse | SyncResultResponse]:
    """Push a project to its Git remote.

     An encrypted project is checked first, so no plaintext card leaves the device: one whose files are
    not safe
    to push is DataCorrupted, and nothing is pushed. A Git failure is an outcome in the result, recorded
    in the
    sync state.

    Args:
        project_id (str):
        body (PushOptions | Unset): How to push.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[ErrorResponse | SyncResultResponse]
    """

    kwargs = _get_kwargs(
        project_id=project_id,
        body=body,
    )

    response = await client.get_async_httpx_client().request(**kwargs)

    return _build_response(client=client, response=response)


async def asyncio(
    project_id: str,
    *,
    client: AuthenticatedClient | Client,
    body: PushOptions | Unset = UNSET,
) -> ErrorResponse | SyncResultResponse | None:
    """Push a project to its Git remote.

     An encrypted project is checked first, so no plaintext card leaves the device: one whose files are
    not safe
    to push is DataCorrupted, and nothing is pushed. A Git failure is an outcome in the result, recorded
    in the
    sync state.

    Args:
        project_id (str):
        body (PushOptions | Unset): How to push.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        ErrorResponse | SyncResultResponse
    """

    return (
        await asyncio_detailed(
            project_id=project_id,
            client=client,
            body=body,
        )
    ).parsed
