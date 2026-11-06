from http import HTTPStatus
from typing import Any
from urllib.parse import quote

import httpx

from ... import errors
from ...client import AuthenticatedClient, Client
from ...models.calendar_response import CalendarResponse
from ...models.error_response import ErrorResponse
from ...types import UNSET, Response, Unset


def _get_kwargs(
    project_id: str,
    *,
    first_day: str,
    last_day: str,
    time_zone: None | str | Unset = UNSET,
) -> dict[str, Any]:

    params: dict[str, Any] = {}

    params["first_day"] = first_day

    params["last_day"] = last_day

    json_time_zone: None | str | Unset
    if isinstance(time_zone, Unset):
        json_time_zone = UNSET
    else:
        json_time_zone = time_zone
    params["time_zone"] = json_time_zone

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
    first_day: str,
    last_day: str,
    time_zone: None | str | Unset = UNSET,
) -> Response[CalendarResponse | ErrorResponse]:
    """What happens in a project over a window of days in a time zone.

     The window runs from the start of first_day to the end of last_day in the zone, so it is empty when
    first_day is
    after last_day. A milestone is in it when one of its days is in the days, or one of its instants
    falls in the
    window; a day has no time zone, so it is compared with the days, never turned into an instant. Cards
    are made and
    changed at instants. Core works out the window's instants from the zone, across daylight-saving
    changes, so every
    client gets the same answer. An unknown zone is an invalid argument.

    Args:
        project_id (str):
        first_day (str):
        last_day (str):
        time_zone (None | str | Unset): The zone the days are in, such as Europe/London; omit for
            this device's zone.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[CalendarResponse | ErrorResponse]
    """

    kwargs = _get_kwargs(
        project_id=project_id,
        first_day=first_day,
        last_day=last_day,
        time_zone=time_zone,
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
    time_zone: None | str | Unset = UNSET,
) -> CalendarResponse | ErrorResponse | None:
    """What happens in a project over a window of days in a time zone.

     The window runs from the start of first_day to the end of last_day in the zone, so it is empty when
    first_day is
    after last_day. A milestone is in it when one of its days is in the days, or one of its instants
    falls in the
    window; a day has no time zone, so it is compared with the days, never turned into an instant. Cards
    are made and
    changed at instants. Core works out the window's instants from the zone, across daylight-saving
    changes, so every
    client gets the same answer. An unknown zone is an invalid argument.

    Args:
        project_id (str):
        first_day (str):
        last_day (str):
        time_zone (None | str | Unset): The zone the days are in, such as Europe/London; omit for
            this device's zone.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        CalendarResponse | ErrorResponse
    """

    return sync_detailed(
        project_id=project_id,
        client=client,
        first_day=first_day,
        last_day=last_day,
        time_zone=time_zone,
    ).parsed


async def asyncio_detailed(
    project_id: str,
    *,
    client: AuthenticatedClient | Client,
    first_day: str,
    last_day: str,
    time_zone: None | str | Unset = UNSET,
) -> Response[CalendarResponse | ErrorResponse]:
    """What happens in a project over a window of days in a time zone.

     The window runs from the start of first_day to the end of last_day in the zone, so it is empty when
    first_day is
    after last_day. A milestone is in it when one of its days is in the days, or one of its instants
    falls in the
    window; a day has no time zone, so it is compared with the days, never turned into an instant. Cards
    are made and
    changed at instants. Core works out the window's instants from the zone, across daylight-saving
    changes, so every
    client gets the same answer. An unknown zone is an invalid argument.

    Args:
        project_id (str):
        first_day (str):
        last_day (str):
        time_zone (None | str | Unset): The zone the days are in, such as Europe/London; omit for
            this device's zone.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[CalendarResponse | ErrorResponse]
    """

    kwargs = _get_kwargs(
        project_id=project_id,
        first_day=first_day,
        last_day=last_day,
        time_zone=time_zone,
    )

    response = await client.get_async_httpx_client().request(**kwargs)

    return _build_response(client=client, response=response)


async def asyncio(
    project_id: str,
    *,
    client: AuthenticatedClient | Client,
    first_day: str,
    last_day: str,
    time_zone: None | str | Unset = UNSET,
) -> CalendarResponse | ErrorResponse | None:
    """What happens in a project over a window of days in a time zone.

     The window runs from the start of first_day to the end of last_day in the zone, so it is empty when
    first_day is
    after last_day. A milestone is in it when one of its days is in the days, or one of its instants
    falls in the
    window; a day has no time zone, so it is compared with the days, never turned into an instant. Cards
    are made and
    changed at instants. Core works out the window's instants from the zone, across daylight-saving
    changes, so every
    client gets the same answer. An unknown zone is an invalid argument.

    Args:
        project_id (str):
        first_day (str):
        last_day (str):
        time_zone (None | str | Unset): The zone the days are in, such as Europe/London; omit for
            this device's zone.

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
            first_day=first_day,
            last_day=last_day,
            time_zone=time_zone,
        )
    ).parsed
