from http import HTTPStatus
from typing import Any

import httpx

from ... import errors
from ...client import AuthenticatedClient, Client
from ...models.error_response import ErrorResponse
from ...models.get_projects_order_type_0 import GetProjectsOrderType0
from ...models.project_list_item_list_response import ProjectListItemListResponse
from ...types import UNSET, Response, Unset


def _get_kwargs(
    *,
    name: None | str | Unset = UNSET,
    updated_after: int | None | Unset = UNSET,
    updated_before: int | None | Unset = UNSET,
    order: GetProjectsOrderType0 | None | Unset = UNSET,
    limit: int | Unset = 100,
    offset: int | Unset = 0,
    include_card_counts: bool | Unset = False,
) -> dict[str, Any]:

    params: dict[str, Any] = {}

    json_name: None | str | Unset
    if isinstance(name, Unset):
        json_name = UNSET
    else:
        json_name = name
    params["name"] = json_name

    json_updated_after: int | None | Unset
    if isinstance(updated_after, Unset):
        json_updated_after = UNSET
    else:
        json_updated_after = updated_after
    params["updated_after"] = json_updated_after

    json_updated_before: int | None | Unset
    if isinstance(updated_before, Unset):
        json_updated_before = UNSET
    else:
        json_updated_before = updated_before
    params["updated_before"] = json_updated_before

    json_order: None | str | Unset
    if isinstance(order, Unset):
        json_order = UNSET
    elif isinstance(order, GetProjectsOrderType0):
        json_order = order.value
    else:
        json_order = order
    params["order"] = json_order

    params["limit"] = limit

    params["offset"] = offset

    params["include_card_counts"] = include_card_counts

    params = {k: v for k, v in params.items() if v is not UNSET and v is not None}

    _kwargs: dict[str, Any] = {
        "method": "get",
        "url": "/projects",
        "params": params,
    }

    return _kwargs


def _parse_response(
    *, client: AuthenticatedClient | Client, response: httpx.Response
) -> ErrorResponse | ProjectListItemListResponse | None:
    if response.status_code == 200:
        response_200 = ProjectListItemListResponse.from_dict(response.json())

        return response_200

    if response.status_code == 400:
        response_400 = ErrorResponse.from_dict(response.json())

        return response_400

    if response.status_code == 401:
        response_401 = ErrorResponse.from_dict(response.json())

        return response_401

    if response.status_code == 500:
        response_500 = ErrorResponse.from_dict(response.json())

        return response_500

    if client.raise_on_unexpected_status:
        raise errors.UnexpectedStatus(response.status_code, response.content)
    else:
        return None


def _build_response(
    *, client: AuthenticatedClient | Client, response: httpx.Response
) -> Response[ErrorResponse | ProjectListItemListResponse]:
    return Response(
        status_code=HTTPStatus(response.status_code),
        content=response.content,
        headers=response.headers,
        parsed=_parse_response(client=client, response=response),
    )


def sync_detailed(
    *,
    client: AuthenticatedClient | Client,
    name: None | str | Unset = UNSET,
    updated_after: int | None | Unset = UNSET,
    updated_before: int | None | Unset = UNSET,
    order: GetProjectsOrderType0 | None | Unset = UNSET,
    limit: int | Unset = 100,
    offset: int | Unset = 0,
    include_card_counts: bool | Unset = False,
) -> Response[ErrorResponse | ProjectListItemListResponse]:
    """List the projects on this device.

     Filters combine. Projects that tie in the order keep the order they were stored in.

    Args:
        name (None | str | Unset): Only projects whose name contains this text.
        updated_after (int | None | Unset): Only projects last changed at or after this instant.
        updated_before (int | None | Unset): Only projects last changed at or before this instant.
        order (GetProjectsOrderType0 | None | Unset): Omit for Home first, then the latest
            changed.
        limit (int | Unset):  Default: 100.
        offset (int | Unset): How many of the ordered projects to skip. Default: 0.
        include_card_counts (bool | Unset): Include each project's counts of live cards. Default:
            False.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[ErrorResponse | ProjectListItemListResponse]
    """

    kwargs = _get_kwargs(
        name=name,
        updated_after=updated_after,
        updated_before=updated_before,
        order=order,
        limit=limit,
        offset=offset,
        include_card_counts=include_card_counts,
    )

    response = client.get_httpx_client().request(
        **kwargs,
    )

    return _build_response(client=client, response=response)


def sync(
    *,
    client: AuthenticatedClient | Client,
    name: None | str | Unset = UNSET,
    updated_after: int | None | Unset = UNSET,
    updated_before: int | None | Unset = UNSET,
    order: GetProjectsOrderType0 | None | Unset = UNSET,
    limit: int | Unset = 100,
    offset: int | Unset = 0,
    include_card_counts: bool | Unset = False,
) -> ErrorResponse | ProjectListItemListResponse | None:
    """List the projects on this device.

     Filters combine. Projects that tie in the order keep the order they were stored in.

    Args:
        name (None | str | Unset): Only projects whose name contains this text.
        updated_after (int | None | Unset): Only projects last changed at or after this instant.
        updated_before (int | None | Unset): Only projects last changed at or before this instant.
        order (GetProjectsOrderType0 | None | Unset): Omit for Home first, then the latest
            changed.
        limit (int | Unset):  Default: 100.
        offset (int | Unset): How many of the ordered projects to skip. Default: 0.
        include_card_counts (bool | Unset): Include each project's counts of live cards. Default:
            False.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        ErrorResponse | ProjectListItemListResponse
    """

    return sync_detailed(
        client=client,
        name=name,
        updated_after=updated_after,
        updated_before=updated_before,
        order=order,
        limit=limit,
        offset=offset,
        include_card_counts=include_card_counts,
    ).parsed


async def asyncio_detailed(
    *,
    client: AuthenticatedClient | Client,
    name: None | str | Unset = UNSET,
    updated_after: int | None | Unset = UNSET,
    updated_before: int | None | Unset = UNSET,
    order: GetProjectsOrderType0 | None | Unset = UNSET,
    limit: int | Unset = 100,
    offset: int | Unset = 0,
    include_card_counts: bool | Unset = False,
) -> Response[ErrorResponse | ProjectListItemListResponse]:
    """List the projects on this device.

     Filters combine. Projects that tie in the order keep the order they were stored in.

    Args:
        name (None | str | Unset): Only projects whose name contains this text.
        updated_after (int | None | Unset): Only projects last changed at or after this instant.
        updated_before (int | None | Unset): Only projects last changed at or before this instant.
        order (GetProjectsOrderType0 | None | Unset): Omit for Home first, then the latest
            changed.
        limit (int | Unset):  Default: 100.
        offset (int | Unset): How many of the ordered projects to skip. Default: 0.
        include_card_counts (bool | Unset): Include each project's counts of live cards. Default:
            False.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[ErrorResponse | ProjectListItemListResponse]
    """

    kwargs = _get_kwargs(
        name=name,
        updated_after=updated_after,
        updated_before=updated_before,
        order=order,
        limit=limit,
        offset=offset,
        include_card_counts=include_card_counts,
    )

    response = await client.get_async_httpx_client().request(**kwargs)

    return _build_response(client=client, response=response)


async def asyncio(
    *,
    client: AuthenticatedClient | Client,
    name: None | str | Unset = UNSET,
    updated_after: int | None | Unset = UNSET,
    updated_before: int | None | Unset = UNSET,
    order: GetProjectsOrderType0 | None | Unset = UNSET,
    limit: int | Unset = 100,
    offset: int | Unset = 0,
    include_card_counts: bool | Unset = False,
) -> ErrorResponse | ProjectListItemListResponse | None:
    """List the projects on this device.

     Filters combine. Projects that tie in the order keep the order they were stored in.

    Args:
        name (None | str | Unset): Only projects whose name contains this text.
        updated_after (int | None | Unset): Only projects last changed at or after this instant.
        updated_before (int | None | Unset): Only projects last changed at or before this instant.
        order (GetProjectsOrderType0 | None | Unset): Omit for Home first, then the latest
            changed.
        limit (int | Unset):  Default: 100.
        offset (int | Unset): How many of the ordered projects to skip. Default: 0.
        include_card_counts (bool | Unset): Include each project's counts of live cards. Default:
            False.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        ErrorResponse | ProjectListItemListResponse
    """

    return (
        await asyncio_detailed(
            client=client,
            name=name,
            updated_after=updated_after,
            updated_before=updated_before,
            order=order,
            limit=limit,
            offset=offset,
            include_card_counts=include_card_counts,
        )
    ).parsed
