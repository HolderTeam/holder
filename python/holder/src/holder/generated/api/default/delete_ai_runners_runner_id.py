from http import HTTPStatus
from typing import Any
from urllib.parse import quote

import httpx

from ... import errors
from ...client import AuthenticatedClient, Client
from ...models.ai_runner_delete_response import AiRunnerDeleteResponse
from ...models.error_response import ErrorResponse
from ...types import Response


def _get_kwargs(
    runner_id: str,
) -> dict[str, Any]:

    _kwargs: dict[str, Any] = {
        "method": "delete",
        "url": "/ai/runners/{runner_id}".format(
            runner_id=quote(str(runner_id), safe=""),
        ),
    }

    return _kwargs


def _parse_response(
    *, client: AuthenticatedClient | Client, response: httpx.Response
) -> AiRunnerDeleteResponse | ErrorResponse | None:
    if response.status_code == 200:
        response_200 = AiRunnerDeleteResponse.from_dict(response.json())

        return response_200

    if response.status_code == 400:
        response_400 = ErrorResponse.from_dict(response.json())

        return response_400

    if response.status_code == 404:
        response_404 = ErrorResponse.from_dict(response.json())

        return response_404

    if client.raise_on_unexpected_status:
        raise errors.UnexpectedStatus(response.status_code, response.content)
    else:
        return None


def _build_response(
    *, client: AuthenticatedClient | Client, response: httpx.Response
) -> Response[AiRunnerDeleteResponse | ErrorResponse]:
    return Response(
        status_code=HTTPStatus(response.status_code),
        content=response.content,
        headers=response.headers,
        parsed=_parse_response(client=client, response=response),
    )


def sync_detailed(
    runner_id: str,
    *,
    client: AuthenticatedClient | Client,
) -> Response[AiRunnerDeleteResponse | ErrorResponse]:
    """Remove a configured local model runner

    Args:
        runner_id (str):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[AiRunnerDeleteResponse | ErrorResponse]
    """

    kwargs = _get_kwargs(
        runner_id=runner_id,
    )

    response = client.get_httpx_client().request(
        **kwargs,
    )

    return _build_response(client=client, response=response)


def sync(
    runner_id: str,
    *,
    client: AuthenticatedClient | Client,
) -> AiRunnerDeleteResponse | ErrorResponse | None:
    """Remove a configured local model runner

    Args:
        runner_id (str):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        AiRunnerDeleteResponse | ErrorResponse
    """

    return sync_detailed(
        runner_id=runner_id,
        client=client,
    ).parsed


async def asyncio_detailed(
    runner_id: str,
    *,
    client: AuthenticatedClient | Client,
) -> Response[AiRunnerDeleteResponse | ErrorResponse]:
    """Remove a configured local model runner

    Args:
        runner_id (str):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[AiRunnerDeleteResponse | ErrorResponse]
    """

    kwargs = _get_kwargs(
        runner_id=runner_id,
    )

    response = await client.get_async_httpx_client().request(**kwargs)

    return _build_response(client=client, response=response)


async def asyncio(
    runner_id: str,
    *,
    client: AuthenticatedClient | Client,
) -> AiRunnerDeleteResponse | ErrorResponse | None:
    """Remove a configured local model runner

    Args:
        runner_id (str):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        AiRunnerDeleteResponse | ErrorResponse
    """

    return (
        await asyncio_detailed(
            runner_id=runner_id,
            client=client,
        )
    ).parsed
