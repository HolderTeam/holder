from http import HTTPStatus
from typing import Any
from urllib.parse import quote

import httpx

from ... import errors
from ...client import AuthenticatedClient, Client
from ...models.error_response import ErrorResponse
from ...models.milestone_list_response import MilestoneListResponse
from ...types import UNSET, Response, Unset


def _get_kwargs(
    project_id: str,
    *,
    first_day: str,
    last_day: str,
    role: None | str | Unset = UNSET,
) -> dict[str, Any]:

    params: dict[str, Any] = {}

    params["first_day"] = first_day

    params["last_day"] = last_day

    json_role: None | str | Unset
    if isinstance(role, Unset):
        json_role = UNSET
    else:
        json_role = role
    params["role"] = json_role

    params = {k: v for k, v in params.items() if v is not UNSET and v is not None}

    _kwargs: dict[str, Any] = {
        "method": "get",
        "url": "/projects/{project_id}/milestones/days".format(
            project_id=quote(str(project_id), safe=""),
        ),
        "params": params,
    }

    return _kwargs


def _parse_response(
    *, client: AuthenticatedClient | Client, response: httpx.Response
) -> ErrorResponse | MilestoneListResponse | None:
    if response.status_code == 200:
        response_200 = MilestoneListResponse.from_dict(response.json())

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

    if client.raise_on_unexpected_status:
        raise errors.UnexpectedStatus(response.status_code, response.content)
    else:
        return None


def _build_response(
    *, client: AuthenticatedClient | Client, response: httpx.Response
) -> Response[ErrorResponse | MilestoneListResponse]:
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
    first_day: str,
    last_day: str,
    role: None | str | Unset = UNSET,
) -> Response[ErrorResponse | MilestoneListResponse]:
    """List a project's milestones with a day in a range.

     The range holds first_day, last_day and the days between; it is empty when first_day is after
    last_day. A
    milestone is listed once, by its first day in the range and then by ID. Milestones on cards in the
    trash are
    left out.

    Args:
        project_id (str):
        first_day (str):
        last_day (str):
        role (None | str | Unset): Only dates with this role; omit for any.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[ErrorResponse | MilestoneListResponse]
    """

    kwargs = _get_kwargs(
        project_id=project_id,
        first_day=first_day,
        last_day=last_day,
        role=role,
    )

    response = client.get_httpx_client().request(
        **kwargs,
    )

    return _build_response(client=client, response=response)


def sync(
    project_id: str,
    *,
    client: AuthenticatedClient | Client,
    first_day: str,
    last_day: str,
    role: None | str | Unset = UNSET,
) -> ErrorResponse | MilestoneListResponse | None:
    """List a project's milestones with a day in a range.

     The range holds first_day, last_day and the days between; it is empty when first_day is after
    last_day. A
    milestone is listed once, by its first day in the range and then by ID. Milestones on cards in the
    trash are
    left out.

    Args:
        project_id (str):
        first_day (str):
        last_day (str):
        role (None | str | Unset): Only dates with this role; omit for any.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        ErrorResponse | MilestoneListResponse
    """

    return sync_detailed(
        project_id=project_id,
        client=client,
        first_day=first_day,
        last_day=last_day,
        role=role,
    ).parsed


async def asyncio_detailed(
    project_id: str,
    *,
    client: AuthenticatedClient | Client,
    first_day: str,
    last_day: str,
    role: None | str | Unset = UNSET,
) -> Response[ErrorResponse | MilestoneListResponse]:
    """List a project's milestones with a day in a range.

     The range holds first_day, last_day and the days between; it is empty when first_day is after
    last_day. A
    milestone is listed once, by its first day in the range and then by ID. Milestones on cards in the
    trash are
    left out.

    Args:
        project_id (str):
        first_day (str):
        last_day (str):
        role (None | str | Unset): Only dates with this role; omit for any.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[ErrorResponse | MilestoneListResponse]
    """

    kwargs = _get_kwargs(
        project_id=project_id,
        first_day=first_day,
        last_day=last_day,
        role=role,
    )

    response = await client.get_async_httpx_client().request(**kwargs)

    return _build_response(client=client, response=response)


async def asyncio(
    project_id: str,
    *,
    client: AuthenticatedClient | Client,
    first_day: str,
    last_day: str,
    role: None | str | Unset = UNSET,
) -> ErrorResponse | MilestoneListResponse | None:
    """List a project's milestones with a day in a range.

     The range holds first_day, last_day and the days between; it is empty when first_day is after
    last_day. A
    milestone is listed once, by its first day in the range and then by ID. Milestones on cards in the
    trash are
    left out.

    Args:
        project_id (str):
        first_day (str):
        last_day (str):
        role (None | str | Unset): Only dates with this role; omit for any.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        ErrorResponse | MilestoneListResponse
    """

    return (
        await asyncio_detailed(
            project_id=project_id,
            client=client,
            first_day=first_day,
            last_day=last_day,
            role=role,
        )
    ).parsed
