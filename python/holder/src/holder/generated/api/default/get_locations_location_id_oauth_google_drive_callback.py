from http import HTTPStatus
from typing import Any, cast
from urllib.parse import quote

import httpx

from ... import errors
from ...client import AuthenticatedClient, Client
from ...models.error_response import ErrorResponse
from ...types import UNSET, Response, Unset


def _get_kwargs(
    location_id: str,
    *,
    code: None | str | Unset = UNSET,
    state: None | str | Unset = UNSET,
    error: None | str | Unset = UNSET,
) -> dict[str, Any]:

    params: dict[str, Any] = {}

    json_code: None | str | Unset
    if isinstance(code, Unset):
        json_code = UNSET
    else:
        json_code = code
    params["code"] = json_code

    json_state: None | str | Unset
    if isinstance(state, Unset):
        json_state = UNSET
    else:
        json_state = state
    params["state"] = json_state

    json_error: None | str | Unset
    if isinstance(error, Unset):
        json_error = UNSET
    else:
        json_error = error
    params["error"] = json_error

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
) -> Any | ErrorResponse | str | None:
    if response.status_code == 200:
        response_200 = response.text
        return response_200

    if response.status_code == 400:
        response_400 = cast(Any, None)
        return response_400

    if response.status_code == 500:
        response_500 = ErrorResponse.from_dict(response.json())

        return response_500

    if client.raise_on_unexpected_status:
        raise errors.UnexpectedStatus(response.status_code, response.content)
    else:
        return None


def _build_response(
    *, client: AuthenticatedClient | Client, response: httpx.Response
) -> Response[Any | ErrorResponse | str]:
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
    code: None | str | Unset = UNSET,
    state: None | str | Unset = UNSET,
    error: None | str | Unset = UNSET,
) -> Response[Any | ErrorResponse | str]:
    """Finish connecting a Google Drive location.

     Google redirects the browser here without a bearer token. The authorize state authenticates the
    callback.
    A failure returns an explanatory HTML page.

    Args:
        location_id (str):
        code (None | str | Unset):
        state (None | str | Unset):
        error (None | str | Unset): Set by Google on denial or failure.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[Any | ErrorResponse | str]
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
    code: None | str | Unset = UNSET,
    state: None | str | Unset = UNSET,
    error: None | str | Unset = UNSET,
) -> Any | ErrorResponse | str | None:
    """Finish connecting a Google Drive location.

     Google redirects the browser here without a bearer token. The authorize state authenticates the
    callback.
    A failure returns an explanatory HTML page.

    Args:
        location_id (str):
        code (None | str | Unset):
        state (None | str | Unset):
        error (None | str | Unset): Set by Google on denial or failure.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Any | ErrorResponse | str
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
    code: None | str | Unset = UNSET,
    state: None | str | Unset = UNSET,
    error: None | str | Unset = UNSET,
) -> Response[Any | ErrorResponse | str]:
    """Finish connecting a Google Drive location.

     Google redirects the browser here without a bearer token. The authorize state authenticates the
    callback.
    A failure returns an explanatory HTML page.

    Args:
        location_id (str):
        code (None | str | Unset):
        state (None | str | Unset):
        error (None | str | Unset): Set by Google on denial or failure.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[Any | ErrorResponse | str]
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
    code: None | str | Unset = UNSET,
    state: None | str | Unset = UNSET,
    error: None | str | Unset = UNSET,
) -> Any | ErrorResponse | str | None:
    """Finish connecting a Google Drive location.

     Google redirects the browser here without a bearer token. The authorize state authenticates the
    callback.
    A failure returns an explanatory HTML page.

    Args:
        location_id (str):
        code (None | str | Unset):
        state (None | str | Unset):
        error (None | str | Unset): Set by Google on denial or failure.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Any | ErrorResponse | str
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
