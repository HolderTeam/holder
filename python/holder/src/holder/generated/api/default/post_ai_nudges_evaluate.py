from http import HTTPStatus
from typing import Any

import httpx

from ... import errors
from ...client import AuthenticatedClient, Client
from ...models.ai_nudge_candidate import AiNudgeCandidate
from ...models.ai_nudge_evaluate_response import AiNudgeEvaluateResponse
from ...models.error_response import ErrorResponse
from ...types import Response


def _get_kwargs(
    *,
    body: AiNudgeCandidate,
) -> dict[str, Any]:
    headers: dict[str, Any] = {}

    _kwargs: dict[str, Any] = {
        "method": "post",
        "url": "/ai/nudges/evaluate",
    }

    _kwargs["json"] = body.to_dict()

    headers["Content-Type"] = "application/json"

    _kwargs["headers"] = headers
    return _kwargs


def _parse_response(
    *, client: AuthenticatedClient | Client, response: httpx.Response
) -> AiNudgeEvaluateResponse | ErrorResponse | None:
    if response.status_code == 200:
        response_200 = AiNudgeEvaluateResponse.from_dict(response.json())

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
) -> Response[AiNudgeEvaluateResponse | ErrorResponse]:
    return Response(
        status_code=HTTPStatus(response.status_code),
        content=response.content,
        headers=response.headers,
        parsed=_parse_response(client=client, response=response),
    )


def sync_detailed(
    *,
    client: AuthenticatedClient | Client,
    body: AiNudgeCandidate,
) -> Response[AiNudgeEvaluateResponse | ErrorResponse]:
    """Evaluate a nudge candidate and record the nudge it warrants

     Answers whether the candidate is accepted and whether it warrants a nudge, with the reason. A
    candidate the daemon does not know is answered, not rejected.

    Args:
        body (AiNudgeCandidate): AI NudgeCandidate.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[AiNudgeEvaluateResponse | ErrorResponse]
    """

    kwargs = _get_kwargs(
        body=body,
    )

    response = client.get_httpx_client().request(
        **kwargs,
    )

    return _build_response(client=client, response=response)


def sync(
    *,
    client: AuthenticatedClient | Client,
    body: AiNudgeCandidate,
) -> AiNudgeEvaluateResponse | ErrorResponse | None:
    """Evaluate a nudge candidate and record the nudge it warrants

     Answers whether the candidate is accepted and whether it warrants a nudge, with the reason. A
    candidate the daemon does not know is answered, not rejected.

    Args:
        body (AiNudgeCandidate): AI NudgeCandidate.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        AiNudgeEvaluateResponse | ErrorResponse
    """

    return sync_detailed(
        client=client,
        body=body,
    ).parsed


async def asyncio_detailed(
    *,
    client: AuthenticatedClient | Client,
    body: AiNudgeCandidate,
) -> Response[AiNudgeEvaluateResponse | ErrorResponse]:
    """Evaluate a nudge candidate and record the nudge it warrants

     Answers whether the candidate is accepted and whether it warrants a nudge, with the reason. A
    candidate the daemon does not know is answered, not rejected.

    Args:
        body (AiNudgeCandidate): AI NudgeCandidate.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[AiNudgeEvaluateResponse | ErrorResponse]
    """

    kwargs = _get_kwargs(
        body=body,
    )

    response = await client.get_async_httpx_client().request(**kwargs)

    return _build_response(client=client, response=response)


async def asyncio(
    *,
    client: AuthenticatedClient | Client,
    body: AiNudgeCandidate,
) -> AiNudgeEvaluateResponse | ErrorResponse | None:
    """Evaluate a nudge candidate and record the nudge it warrants

     Answers whether the candidate is accepted and whether it warrants a nudge, with the reason. A
    candidate the daemon does not know is answered, not rejected.

    Args:
        body (AiNudgeCandidate): AI NudgeCandidate.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        AiNudgeEvaluateResponse | ErrorResponse
    """

    return (
        await asyncio_detailed(
            client=client,
            body=body,
        )
    ).parsed
