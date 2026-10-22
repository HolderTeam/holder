from http import HTTPStatus
from typing import Any
from urllib.parse import quote

import httpx

from ... import errors
from ...client import AuthenticatedClient, Client
from ...models.error_response import ErrorResponse
from ...types import UNSET, Response, Unset


def _get_kwargs(
    location_id: str,
    *,
    code: str | Unset = UNSET,
    state: str | Unset = UNSET,
    error: str | Unset = UNSET,
) -> dict[str, Any]:

    params: dict[str, Any] = {}

    params["code"] = code

    params["state"] = state

    params["error"] = error

    params = {k: v for k, v in params.items() if v is not UNSET and v is not None}

    _kwargs: dict[str, Any] = {
        "method": "get",
        "url": "/locations/{location_id}/oauth/google-drive/callback".format(
            location_id=quote(str(location_id), safe=""),
        ),
        "params": params,
    }

    return _kwargs


def _parse_response(
    *, client: AuthenticatedClient | Client, response: httpx.Response
) -> ErrorResponse | str | None:
    if response.status_code == 200:
        response_200 = response.text
        return response_200

    if response.status_code == 500:
        response_500 = ErrorResponse.from_dict(response.json())

        return response_500

    if client.raise_on_unexpected_status:
        raise errors.UnexpectedStatus(response.status_code, response.content)
    else:
        return None


def _build_response(
    *, client: AuthenticatedClient | Client, response: httpx.Response
) -> Response[ErrorResponse | str]:
    return Response(
        status_code=HTTPStatus(response.status_code),
        content=response.content,
        headers=response.headers,
        parsed=_parse_response(client=client, response=response),
    )


def sync_detailed(
    location_id: str,
    *,
    client: AuthenticatedClient | Client,
    code: str | Unset = UNSET,
    state: str | Unset = UNSET,
    error: str | Unset = UNSET,
) -> Response[ErrorResponse | str]:
    """Finish connecting a Google Drive location

     Google redirects the user's browser here after authorization, so the request carries no token; the
    state from the authorize call authenticates it instead. The daemon exchanges the code, creates the
    location's Drive folder if needed, and binds the location. A failure is answered with 400 and an
    HTML page that explains it.

    Args:
        location_id (str):
        code (str | Unset):
        state (str | Unset):
        error (str | Unset):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[ErrorResponse | str]
    """

    kwargs = _get_kwargs(
        location_id=location_id,
        code=code,
        state=state,
        error=error,
    )

    response = client.get_httpx_client().request(
        **kwargs,
    )

    return _build_response(client=client, response=response)


def sync(
    location_id: str,
    *,
    client: AuthenticatedClient | Client,
    code: str | Unset = UNSET,
    state: str | Unset = UNSET,
    error: str | Unset = UNSET,
) -> ErrorResponse | str | None:
    """Finish connecting a Google Drive location

     Google redirects the user's browser here after authorization, so the request carries no token; the
    state from the authorize call authenticates it instead. The daemon exchanges the code, creates the
    location's Drive folder if needed, and binds the location. A failure is answered with 400 and an
    HTML page that explains it.

    Args:
        location_id (str):
        code (str | Unset):
        state (str | Unset):
        error (str | Unset):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        ErrorResponse | str
    """

    return sync_detailed(
        location_id=location_id,
        client=client,
        code=code,
        state=state,
        error=error,
    ).parsed


async def asyncio_detailed(
    location_id: str,
    *,
    client: AuthenticatedClient | Client,
    code: str | Unset = UNSET,
    state: str | Unset = UNSET,
    error: str | Unset = UNSET,
) -> Response[ErrorResponse | str]:
    """Finish connecting a Google Drive location

     Google redirects the user's browser here after authorization, so the request carries no token; the
    state from the authorize call authenticates it instead. The daemon exchanges the code, creates the
    location's Drive folder if needed, and binds the location. A failure is answered with 400 and an
    HTML page that explains it.

    Args:
        location_id (str):
        code (str | Unset):
        state (str | Unset):
        error (str | Unset):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[ErrorResponse | str]
    """

    kwargs = _get_kwargs(
        location_id=location_id,
        code=code,
        state=state,
        error=error,
    )

    response = await client.get_async_httpx_client().request(**kwargs)

    return _build_response(client=client, response=response)


async def asyncio(
    location_id: str,
    *,
    client: AuthenticatedClient | Client,
    code: str | Unset = UNSET,
    state: str | Unset = UNSET,
    error: str | Unset = UNSET,
) -> ErrorResponse | str | None:
    """Finish connecting a Google Drive location

     Google redirects the user's browser here after authorization, so the request carries no token; the
    state from the authorize call authenticates it instead. The daemon exchanges the code, creates the
    location's Drive folder if needed, and binds the location. A failure is answered with 400 and an
    HTML page that explains it.

    Args:
        location_id (str):
        code (str | Unset):
        state (str | Unset):
        error (str | Unset):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        ErrorResponse | str
    """

    return (
        await asyncio_detailed(
            location_id=location_id,
            client=client,
            code=code,
            state=state,
            error=error,
        )
    ).parsed
