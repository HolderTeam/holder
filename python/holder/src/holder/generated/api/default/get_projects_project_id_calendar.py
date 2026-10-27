from http import HTTPStatus
from typing import Any
from urllib.parse import quote

import httpx

from ... import errors
from ...client import AuthenticatedClient, Client
from ...models.calendar_response import CalendarResponse
from ...models.error_response import ErrorResponse
from ...types import UNSET, Response


def _get_kwargs(
    project_id: str,
    *,
    first_instant: int,
    last_instant: int,
    first_day: str,
    last_day: str,
) -> dict[str, Any]:

    params: dict[str, Any] = {}

    params["first_instant"] = first_instant

    params["last_instant"] = last_instant

    params["first_day"] = first_day

    params["last_day"] = last_day

    params = {k: v for k, v in params.items() if v is not UNSET and v is not None}

    _kwargs: dict[str, Any] = {
        "method": "get",
        "url": "/projects/{project_id}/calendar".format(
            project_id=quote(str(project_id), safe=""),
        ),
        "params": params,
    }

    return _kwargs


def _parse_response(
    *, client: AuthenticatedClient | Client, response: httpx.Response
) -> CalendarResponse | ErrorResponse | None:
    if response.status_code == 200:
        response_200 = CalendarResponse.from_dict(response.json())

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
) -> Response[CalendarResponse | ErrorResponse]:
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
    first_instant: int,
    last_instant: int,
    first_day: str,
    last_day: str,
) -> Response[CalendarResponse | ErrorResponse]:
    """What happens in a project over a window of time.

     The caller gives the window twice, as instants and as the days they fall on where the caller is,
    because a day
    has no time zone: a milestone is in the window when one of its days is in the days, or one of its
    instants in
    the instants. Cards are made and changed at instants. Either range is empty when its start is after
    its end.

    Args:
        project_id (str):
        first_instant (int):
        last_instant (int):
        first_day (str):
        last_day (str):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[CalendarResponse | ErrorResponse]
    """

    kwargs = _get_kwargs(
        project_id=project_id,
        first_instant=first_instant,
        last_instant=last_instant,
        first_day=first_day,
        last_day=last_day,
    )

    response = client.get_httpx_client().request(
        **kwargs,
    )

    return _build_response(client=client, response=response)


def sync(
    project_id: str,
    *,
    client: AuthenticatedClient | Client,
    first_instant: int,
    last_instant: int,
    first_day: str,
    last_day: str,
) -> CalendarResponse | ErrorResponse | None:
    """What happens in a project over a window of time.

     The caller gives the window twice, as instants and as the days they fall on where the caller is,
    because a day
    has no time zone: a milestone is in the window when one of its days is in the days, or one of its
    instants in
    the instants. Cards are made and changed at instants. Either range is empty when its start is after
    its end.

    Args:
        project_id (str):
        first_instant (int):
        last_instant (int):
        first_day (str):
        last_day (str):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        CalendarResponse | ErrorResponse
    """

    return sync_detailed(
        project_id=project_id,
        client=client,
        first_instant=first_instant,
        last_instant=last_instant,
        first_day=first_day,
        last_day=last_day,
    ).parsed


async def asyncio_detailed(
    project_id: str,
    *,
    client: AuthenticatedClient | Client,
    first_instant: int,
    last_instant: int,
    first_day: str,
    last_day: str,
) -> Response[CalendarResponse | ErrorResponse]:
    """What happens in a project over a window of time.

     The caller gives the window twice, as instants and as the days they fall on where the caller is,
    because a day
    has no time zone: a milestone is in the window when one of its days is in the days, or one of its
    instants in
    the instants. Cards are made and changed at instants. Either range is empty when its start is after
    its end.

    Args:
        project_id (str):
        first_instant (int):
        last_instant (int):
        first_day (str):
        last_day (str):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[CalendarResponse | ErrorResponse]
    """

    kwargs = _get_kwargs(
        project_id=project_id,
        first_instant=first_instant,
        last_instant=last_instant,
        first_day=first_day,
        last_day=last_day,
    )

    response = await client.get_async_httpx_client().request(**kwargs)

    return _build_response(client=client, response=response)


async def asyncio(
    project_id: str,
    *,
    client: AuthenticatedClient | Client,
    first_instant: int,
    last_instant: int,
    first_day: str,
    last_day: str,
) -> CalendarResponse | ErrorResponse | None:
    """What happens in a project over a window of time.

     The caller gives the window twice, as instants and as the days they fall on where the caller is,
    because a day
    has no time zone: a milestone is in the window when one of its days is in the days, or one of its
    instants in
    the instants. Cards are made and changed at instants. Either range is empty when its start is after
    its end.

    Args:
        project_id (str):
        first_instant (int):
        last_instant (int):
        first_day (str):
        last_day (str):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        CalendarResponse | ErrorResponse
    """

    return (
        await asyncio_detailed(
            project_id=project_id,
            client=client,
            first_instant=first_instant,
            last_instant=last_instant,
            first_day=first_day,
            last_day=last_day,
        )
    ).parsed
