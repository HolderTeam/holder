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
    project_id: str,
    *,
    stream: bool | Unset = False,
) -> dict[str, Any]:

    params: dict[str, Any] = {}

    params["stream"] = stream

    params = {k: v for k, v in params.items() if v is not UNSET and v is not None}

    _kwargs: dict[str, Any] = {
        "method": "get",
        "url": "/projects/{project_id}/recovery-snapshot".format(
            project_id=quote(str(project_id), safe=""),
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
) -> Response[Any | ErrorResponse]:
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
    stream: bool | Unset = False,
) -> Response[Any | ErrorResponse]:
    """Download a consistent recovery snapshot of the project's live cards.

     Returns UTF-8 recovery snapshot lines, one versioned JSON record per card. The default
    response is capped at 2 MiB. With stream=true, the daemon captures to a temporary file
    before transferring it, and the total size is limited by available storage.
    An empty project returns zero bytes. The download includes readable Markdown bodies,
    including plaintext from encrypted projects. It includes linked resource declarations
    but omits asset bytes, credentials, Git history, trash, and AI threads and messages.

    Args:
        project_id (str):
        stream (bool | Unset):  Default: False.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[Any | ErrorResponse]
    """

    kwargs = _get_kwargs(
        project_id=project_id,
        stream=stream,
    )

    response = client.get_httpx_client().request(
        **kwargs,
    )

    return _build_response(client=client, response=response)


def sync(
    project_id: str,
    *,
    client: AuthenticatedClient | Client,
    stream: bool | Unset = False,
) -> Any | ErrorResponse | None:
    """Download a consistent recovery snapshot of the project's live cards.

     Returns UTF-8 recovery snapshot lines, one versioned JSON record per card. The default
    response is capped at 2 MiB. With stream=true, the daemon captures to a temporary file
    before transferring it, and the total size is limited by available storage.
    An empty project returns zero bytes. The download includes readable Markdown bodies,
    including plaintext from encrypted projects. It includes linked resource declarations
    but omits asset bytes, credentials, Git history, trash, and AI threads and messages.

    Args:
        project_id (str):
        stream (bool | Unset):  Default: False.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Any | ErrorResponse
    """

    return sync_detailed(
        project_id=project_id,
        client=client,
        stream=stream,
    ).parsed


async def asyncio_detailed(
    project_id: str,
    *,
    client: AuthenticatedClient | Client,
    stream: bool | Unset = False,
) -> Response[Any | ErrorResponse]:
    """Download a consistent recovery snapshot of the project's live cards.

     Returns UTF-8 recovery snapshot lines, one versioned JSON record per card. The default
    response is capped at 2 MiB. With stream=true, the daemon captures to a temporary file
    before transferring it, and the total size is limited by available storage.
    An empty project returns zero bytes. The download includes readable Markdown bodies,
    including plaintext from encrypted projects. It includes linked resource declarations
    but omits asset bytes, credentials, Git history, trash, and AI threads and messages.

    Args:
        project_id (str):
        stream (bool | Unset):  Default: False.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[Any | ErrorResponse]
    """

    kwargs = _get_kwargs(
        project_id=project_id,
        stream=stream,
    )

    response = await client.get_async_httpx_client().request(**kwargs)

    return _build_response(client=client, response=response)


async def asyncio(
    project_id: str,
    *,
    client: AuthenticatedClient | Client,
    stream: bool | Unset = False,
) -> Any | ErrorResponse | None:
    """Download a consistent recovery snapshot of the project's live cards.

     Returns UTF-8 recovery snapshot lines, one versioned JSON record per card. The default
    response is capped at 2 MiB. With stream=true, the daemon captures to a temporary file
    before transferring it, and the total size is limited by available storage.
    An empty project returns zero bytes. The download includes readable Markdown bodies,
    including plaintext from encrypted projects. It includes linked resource declarations
    but omits asset bytes, credentials, Git history, trash, and AI threads and messages.

    Args:
        project_id (str):
        stream (bool | Unset):  Default: False.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Any | ErrorResponse
    """

    return (
        await asyncio_detailed(
            project_id=project_id,
            client=client,
            stream=stream,
        )
    ).parsed


def stream(
    project_id: str,
    *,
    client: AuthenticatedClient | Client,
    stream: bool | Unset = False,
) -> ByteStream:
    """Download a consistent recovery snapshot of the project's live cards.

     Returns UTF-8 recovery snapshot lines, one versioned JSON record per card. The default
    response is capped at 2 MiB. With stream=true, the daemon captures to a temporary file
    before transferring it, and the total size is limited by available storage.
    An empty project returns zero bytes. The download includes readable Markdown bodies,
    including plaintext from encrypted projects. It includes linked resource declarations
    but omits asset bytes, credentials, Git history, trash, and AI threads and messages.

    Args:
        project_id (str):
        stream (bool | Unset):  Default: False.

    Raises:
        StreamError: If the server answers with an error status instead of the stream.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        ByteStream
    """

    return ByteStream(
        client.get_httpx_client(),
        _get_kwargs(
            project_id=project_id,
            stream=stream,
        ),
        lambda response: _parse_response(client=client, response=response),
    )


def astream(
    project_id: str,
    *,
    client: AuthenticatedClient | Client,
    stream: bool | Unset = False,
) -> AsyncByteStream:
    """Download a consistent recovery snapshot of the project's live cards.

     Returns UTF-8 recovery snapshot lines, one versioned JSON record per card. The default
    response is capped at 2 MiB. With stream=true, the daemon captures to a temporary file
    before transferring it, and the total size is limited by available storage.
    An empty project returns zero bytes. The download includes readable Markdown bodies,
    including plaintext from encrypted projects. It includes linked resource declarations
    but omits asset bytes, credentials, Git history, trash, and AI threads and messages.

    Args:
        project_id (str):
        stream (bool | Unset):  Default: False.

    Raises:
        StreamError: If the server answers with an error status instead of the stream.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        AsyncByteStream
    """

    return AsyncByteStream(
        client.get_async_httpx_client(),
        _get_kwargs(
            project_id=project_id,
            stream=stream,
        ),
        lambda response: _parse_response(client=client, response=response),
    )
