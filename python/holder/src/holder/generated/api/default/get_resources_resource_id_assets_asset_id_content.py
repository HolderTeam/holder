from http import HTTPStatus
from typing import Any, cast
from urllib.parse import quote

import httpx

from ... import errors
from ...client import AuthenticatedClient, Client
from ...models.error_response import ErrorResponse
from ...streams import AsyncByteStream, ByteStream
from ...types import UNSET, Response, Unset


def _get_kwargs(
    resource_id: str,
    asset_id: str,
    *,
    placement_id: None | str | Unset = UNSET,
) -> dict[str, Any]:

    params: dict[str, Any] = {}

    json_placement_id: None | str | Unset
    if isinstance(placement_id, Unset):
        json_placement_id = UNSET
    else:
        json_placement_id = placement_id
    params["placement_id"] = json_placement_id

    params = {k: v for k, v in params.items() if v is not UNSET and v is not None}

    _kwargs: dict[str, Any] = {
        "method": "get",
        "url": "/resources/{resource_id}/assets/{asset_id}/content".format(
            resource_id=quote(str(resource_id), safe=""),
            asset_id=quote(str(asset_id), safe=""),
        ),
        "params": params,
    }

    return _kwargs


def _parse_response(
    *, client: AuthenticatedClient | Client, response: httpx.Response
) -> Any | ErrorResponse | None:
    if response.status_code == 200:
        response_200 = cast(Any, None)
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

    if response.status_code == 422:
        response_422 = ErrorResponse.from_dict(response.json())

        return response_422

    if response.status_code == 500:
        response_500 = ErrorResponse.from_dict(response.json())

        return response_500

    if response.status_code == 502:
        response_502 = ErrorResponse.from_dict(response.json())

        return response_502

    if response.status_code == 503:
        response_503 = ErrorResponse.from_dict(response.json())

        return response_503

    if response.status_code == 507:
        response_507 = ErrorResponse.from_dict(response.json())

        return response_507

    if client.raise_on_unexpected_status:
        raise errors.UnexpectedStatus(response.status_code, response.content)
    else:
        return None


def _build_response(
    *, client: AuthenticatedClient | Client, response: httpx.Response
) -> Response[Any | ErrorResponse]:
    return Response(
        status_code=HTTPStatus(response.status_code),
        content=response.content,
        headers=response.headers,
        parsed=_parse_response(client=client, response=response),
    )


def sync_detailed(
    resource_id: str,
    asset_id: str,
    *,
    client: AuthenticatedClient | Client,
    placement_id: None | str | Unset = UNSET,
) -> Response[Any | ErrorResponse]:
    """Stream verified asset content.

     This authenticated download returns raw bytes. Content-Type preserves the asset media type and
    Content-Disposition contains a sanitized original filename. Without placement_id the daemon selects
    a
    stored placement.

    Args:
        resource_id (str):
        asset_id (str):
        placement_id (None | str | Unset):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[Any | ErrorResponse]
    """

    kwargs = _get_kwargs(
        resource_id=resource_id,
        asset_id=asset_id,
        placement_id=placement_id,
    )

    response = client.get_httpx_client().request(
        **kwargs,
    )

    return _build_response(client=client, response=response)


def sync(
    resource_id: str,
    asset_id: str,
    *,
    client: AuthenticatedClient | Client,
    placement_id: None | str | Unset = UNSET,
) -> Any | ErrorResponse | None:
    """Stream verified asset content.

     This authenticated download returns raw bytes. Content-Type preserves the asset media type and
    Content-Disposition contains a sanitized original filename. Without placement_id the daemon selects
    a
    stored placement.

    Args:
        resource_id (str):
        asset_id (str):
        placement_id (None | str | Unset):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Any | ErrorResponse
    """

    return sync_detailed(
        resource_id=resource_id,
        asset_id=asset_id,
        client=client,
        placement_id=placement_id,
    ).parsed


async def asyncio_detailed(
    resource_id: str,
    asset_id: str,
    *,
    client: AuthenticatedClient | Client,
    placement_id: None | str | Unset = UNSET,
) -> Response[Any | ErrorResponse]:
    """Stream verified asset content.

     This authenticated download returns raw bytes. Content-Type preserves the asset media type and
    Content-Disposition contains a sanitized original filename. Without placement_id the daemon selects
    a
    stored placement.

    Args:
        resource_id (str):
        asset_id (str):
        placement_id (None | str | Unset):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[Any | ErrorResponse]
    """

    kwargs = _get_kwargs(
        resource_id=resource_id,
        asset_id=asset_id,
        placement_id=placement_id,
    )

    response = await client.get_async_httpx_client().request(**kwargs)

    return _build_response(client=client, response=response)


async def asyncio(
    resource_id: str,
    asset_id: str,
    *,
    client: AuthenticatedClient | Client,
    placement_id: None | str | Unset = UNSET,
) -> Any | ErrorResponse | None:
    """Stream verified asset content.

     This authenticated download returns raw bytes. Content-Type preserves the asset media type and
    Content-Disposition contains a sanitized original filename. Without placement_id the daemon selects
    a
    stored placement.

    Args:
        resource_id (str):
        asset_id (str):
        placement_id (None | str | Unset):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Any | ErrorResponse
    """

    return (
        await asyncio_detailed(
            resource_id=resource_id,
            asset_id=asset_id,
            client=client,
            placement_id=placement_id,
        )
    ).parsed


def stream(
    resource_id: str,
    asset_id: str,
    *,
    client: AuthenticatedClient | Client,
    placement_id: None | str | Unset = UNSET,
) -> ByteStream:
    """Stream verified asset content.

     This authenticated download returns raw bytes. Content-Type preserves the asset media type and
    Content-Disposition contains a sanitized original filename. Without placement_id the daemon selects
    a
    stored placement.

    Args:
        resource_id (str):
        asset_id (str):
        placement_id (None | str | Unset):

    Raises:
        StreamError: If the server answers with an error status instead of the stream.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        ByteStream
    """

    return ByteStream(
        client.get_httpx_client(),
        _get_kwargs(
            resource_id=resource_id,
            asset_id=asset_id,
            placement_id=placement_id,
        ),
        lambda response: _parse_response(client=client, response=response),
    )


def astream(
    resource_id: str,
    asset_id: str,
    *,
    client: AuthenticatedClient | Client,
    placement_id: None | str | Unset = UNSET,
) -> AsyncByteStream:
    """Stream verified asset content.

     This authenticated download returns raw bytes. Content-Type preserves the asset media type and
    Content-Disposition contains a sanitized original filename. Without placement_id the daemon selects
    a
    stored placement.

    Args:
        resource_id (str):
        asset_id (str):
        placement_id (None | str | Unset):

    Raises:
        StreamError: If the server answers with an error status instead of the stream.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        AsyncByteStream
    """

    return AsyncByteStream(
        client.get_async_httpx_client(),
        _get_kwargs(
            resource_id=resource_id,
            asset_id=asset_id,
            placement_id=placement_id,
        ),
        lambda response: _parse_response(client=client, response=response),
    )
