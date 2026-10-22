from http import HTTPStatus
from typing import Any, cast
from urllib.parse import quote

import httpx

from ... import errors
from ...client import AuthenticatedClient, Client
from ...models.error_response import ErrorResponse
from ...streams import AsyncByteStream, ByteStream
from ...types import Response


def _get_kwargs(
    file: str,
) -> dict[str, Any]:

    _kwargs: dict[str, Any] = {
        "method": "get",
        "url": "/docs/{file}".format(
            file=quote(str(file), safe=""),
        ),
    }

    return _kwargs


def _parse_response(
    *, client: AuthenticatedClient | Client, response: httpx.Response
) -> Any | ErrorResponse | None:
    if response.status_code == 200:
        response_200 = cast(Any, None)
        return response_200

    if response.status_code == 404:
        response_404 = ErrorResponse.from_dict(response.json())

        return response_404

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
    file: str,
    *,
    client: AuthenticatedClient | Client,
) -> Response[Any | ErrorResponse]:
    """Download one of the API documentation's files

    Args:
        file (str):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[Any | ErrorResponse]
    """

    kwargs = _get_kwargs(
        file=file,
    )

    response = client.get_httpx_client().request(
        **kwargs,
    )

    return _build_response(client=client, response=response)


def sync(
    file: str,
    *,
    client: AuthenticatedClient | Client,
) -> Any | ErrorResponse | None:
    """Download one of the API documentation's files

    Args:
        file (str):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Any | ErrorResponse
    """

    return sync_detailed(
        file=file,
        client=client,
    ).parsed


async def asyncio_detailed(
    file: str,
    *,
    client: AuthenticatedClient | Client,
) -> Response[Any | ErrorResponse]:
    """Download one of the API documentation's files

    Args:
        file (str):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[Any | ErrorResponse]
    """

    kwargs = _get_kwargs(
        file=file,
    )

    response = await client.get_async_httpx_client().request(**kwargs)

    return _build_response(client=client, response=response)


async def asyncio(
    file: str,
    *,
    client: AuthenticatedClient | Client,
) -> Any | ErrorResponse | None:
    """Download one of the API documentation's files

    Args:
        file (str):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Any | ErrorResponse
    """

    return (
        await asyncio_detailed(
            file=file,
            client=client,
        )
    ).parsed


def stream(
    file: str,
    *,
    client: AuthenticatedClient | Client,
) -> ByteStream:
    """Download one of the API documentation's files

    Args:
        file (str):

    Raises:
        StreamError: If the server answers with an error status instead of the stream.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        ByteStream
    """

    return ByteStream(
        client.get_httpx_client(),
        _get_kwargs(
            file=file,
        ),
        lambda response: _parse_response(client=client, response=response),
    )


def astream(
    file: str,
    *,
    client: AuthenticatedClient | Client,
) -> AsyncByteStream:
    """Download one of the API documentation's files

    Args:
        file (str):

    Raises:
        StreamError: If the server answers with an error status instead of the stream.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        AsyncByteStream
    """

    return AsyncByteStream(
        client.get_async_httpx_client(),
        _get_kwargs(
            file=file,
        ),
        lambda response: _parse_response(client=client, response=response),
    )
