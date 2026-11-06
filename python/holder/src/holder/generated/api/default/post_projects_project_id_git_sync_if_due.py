from http import HTTPStatus
from typing import Any
from urllib.parse import quote

import httpx

from ... import errors
from ...client import AuthenticatedClient, Client
from ...models.due_sync_result_response import DueSyncResultResponse
from ...models.error_response import ErrorResponse
from ...types import UNSET, Response, Unset


def _get_kwargs(
    project_id: str,
    *,
    push_interval_seconds: int | Unset = 0,
    pull_interval_seconds: int | Unset = 0,
) -> dict[str, Any]:

    params: dict[str, Any] = {}

    params["push_interval_seconds"] = push_interval_seconds

    params["pull_interval_seconds"] = pull_interval_seconds

    params = {k: v for k, v in params.items() if v is not UNSET and v is not None}

    _kwargs: dict[str, Any] = {
        "method": "post",
        "url": "/projects/{project_id}/git/sync-if-due".format(
            project_id=quote(str(project_id), safe=""),
        ),
        "params": params,
    }

    return _kwargs


def _parse_response(
    *, client: AuthenticatedClient | Client, response: httpx.Response
) -> DueSyncResultResponse | ErrorResponse | None:
    if response.status_code == 200:
        response_200 = DueSyncResultResponse.from_dict(response.json())

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
) -> Response[DueSyncResultResponse | ErrorResponse]:
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
    push_interval_seconds: int | Unset = 0,
    pull_interval_seconds: int | Unset = 0,
) -> Response[DueSyncResultResponse | ErrorResponse]:
    """Try the project's pull and push phases only when their cadence and retry policy say they are due.

     Zero uses the built-in intervals (20 minutes for push, 5 minutes for pull). A project without a
    remote and
    a project with nothing due return both phases as not attempted without changing sync state. A due
    push can
    run after a failed pull; this keeps scheduled pushes independent from the pull retry schedule.
    Git failures are phase outcomes and are recorded in the project's sync state.

    Args:
        project_id (str):
        push_interval_seconds (int | Unset):  Default: 0.
        pull_interval_seconds (int | Unset):  Default: 0.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[DueSyncResultResponse | ErrorResponse]
    """

    kwargs = _get_kwargs(
        project_id=project_id,
        push_interval_seconds=push_interval_seconds,
        pull_interval_seconds=pull_interval_seconds,
    )

    response = client.get_httpx_client().request(
        **kwargs,
    )

    return _build_response(client=client, response=response)


def sync(
    project_id: str,
    *,
    client: AuthenticatedClient | Client,
    push_interval_seconds: int | Unset = 0,
    pull_interval_seconds: int | Unset = 0,
) -> DueSyncResultResponse | ErrorResponse | None:
    """Try the project's pull and push phases only when their cadence and retry policy say they are due.

     Zero uses the built-in intervals (20 minutes for push, 5 minutes for pull). A project without a
    remote and
    a project with nothing due return both phases as not attempted without changing sync state. A due
    push can
    run after a failed pull; this keeps scheduled pushes independent from the pull retry schedule.
    Git failures are phase outcomes and are recorded in the project's sync state.

    Args:
        project_id (str):
        push_interval_seconds (int | Unset):  Default: 0.
        pull_interval_seconds (int | Unset):  Default: 0.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        DueSyncResultResponse | ErrorResponse
    """

    return sync_detailed(
        project_id=project_id,
        client=client,
        push_interval_seconds=push_interval_seconds,
        pull_interval_seconds=pull_interval_seconds,
    ).parsed


async def asyncio_detailed(
    project_id: str,
    *,
    client: AuthenticatedClient | Client,
    push_interval_seconds: int | Unset = 0,
    pull_interval_seconds: int | Unset = 0,
) -> Response[DueSyncResultResponse | ErrorResponse]:
    """Try the project's pull and push phases only when their cadence and retry policy say they are due.

     Zero uses the built-in intervals (20 minutes for push, 5 minutes for pull). A project without a
    remote and
    a project with nothing due return both phases as not attempted without changing sync state. A due
    push can
    run after a failed pull; this keeps scheduled pushes independent from the pull retry schedule.
    Git failures are phase outcomes and are recorded in the project's sync state.

    Args:
        project_id (str):
        push_interval_seconds (int | Unset):  Default: 0.
        pull_interval_seconds (int | Unset):  Default: 0.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[DueSyncResultResponse | ErrorResponse]
    """

    kwargs = _get_kwargs(
        project_id=project_id,
        push_interval_seconds=push_interval_seconds,
        pull_interval_seconds=pull_interval_seconds,
    )

    response = await client.get_async_httpx_client().request(**kwargs)

    return _build_response(client=client, response=response)


async def asyncio(
    project_id: str,
    *,
    client: AuthenticatedClient | Client,
    push_interval_seconds: int | Unset = 0,
    pull_interval_seconds: int | Unset = 0,
) -> DueSyncResultResponse | ErrorResponse | None:
    """Try the project's pull and push phases only when their cadence and retry policy say they are due.

     Zero uses the built-in intervals (20 minutes for push, 5 minutes for pull). A project without a
    remote and
    a project with nothing due return both phases as not attempted without changing sync state. A due
    push can
    run after a failed pull; this keeps scheduled pushes independent from the pull retry schedule.
    Git failures are phase outcomes and are recorded in the project's sync state.

    Args:
        project_id (str):
        push_interval_seconds (int | Unset):  Default: 0.
        pull_interval_seconds (int | Unset):  Default: 0.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        DueSyncResultResponse | ErrorResponse
    """

    return (
        await asyncio_detailed(
            project_id=project_id,
            client=client,
            push_interval_seconds=push_interval_seconds,
            pull_interval_seconds=pull_interval_seconds,
        )
    ).parsed
