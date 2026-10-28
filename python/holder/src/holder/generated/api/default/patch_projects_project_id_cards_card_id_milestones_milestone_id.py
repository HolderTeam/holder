from http import HTTPStatus
from typing import Any
from urllib.parse import quote

import httpx

from ... import errors
from ...client import AuthenticatedClient, Client
from ...models.error_response import ErrorResponse
from ...models.milestone_response import MilestoneResponse
from ...models.milestone_update import MilestoneUpdate
from ...types import UNSET, Response, Unset


def _get_kwargs(
    project_id: str,
    card_id: str,
    milestone_id: str,
    *,
    body: MilestoneUpdate | Unset = UNSET,
) -> dict[str, Any]:
    headers: dict[str, Any] = {}

    _kwargs: dict[str, Any] = {
        "method": "patch",
        "url": "/projects/{project_id}/cards/{card_id}/milestones/{milestone_id}".format(
            project_id=quote(str(project_id), safe=""),
            card_id=quote(str(card_id), safe=""),
            milestone_id=quote(str(milestone_id), safe=""),
        ),
    }

    if not isinstance(body, Unset):
        _kwargs["json"] = body.to_dict()

    headers["Content-Type"] = "application/json"

    _kwargs["headers"] = headers
    return _kwargs


def _parse_response(
    *, client: AuthenticatedClient | Client, response: httpx.Response
) -> ErrorResponse | MilestoneResponse | None:
    if response.status_code == 200:
        response_200 = MilestoneResponse.from_dict(response.json())

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
) -> Response[ErrorResponse | MilestoneResponse]:
    return Response(
        status_code=HTTPStatus(response.status_code),
        content=response.content,
        headers=response.headers,
        parsed=_parse_response(client=client, response=response),
    )


def sync_detailed(
    project_id: str,
    card_id: str,
    milestone_id: str,
    *,
    client: AuthenticatedClient | Client,
    body: MilestoneUpdate | Unset = UNSET,
) -> Response[ErrorResponse | MilestoneResponse]:
    """Change a milestone.

     Each field left out keeps its value, and an update that changes nothing writes nothing. Each date
    must be a real
    day or instant. A card in the trash or in another project is not found, and neither is a milestone
    that is not
    on the card.

    Args:
        project_id (str):
        card_id (str):
        milestone_id (str):
        body (MilestoneUpdate | Unset): A change to a milestone. Each field left out keeps its
            value.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[ErrorResponse | MilestoneResponse]
    """

    kwargs = _get_kwargs(
        project_id=project_id,
        card_id=card_id,
        milestone_id=milestone_id,
        body=body,
    )

    response = client.get_httpx_client().request(
        **kwargs,
    )

    return _build_response(client=client, response=response)


def sync(
    project_id: str,
    card_id: str,
    milestone_id: str,
    *,
    client: AuthenticatedClient | Client,
    body: MilestoneUpdate | Unset = UNSET,
) -> ErrorResponse | MilestoneResponse | None:
    """Change a milestone.

     Each field left out keeps its value, and an update that changes nothing writes nothing. Each date
    must be a real
    day or instant. A card in the trash or in another project is not found, and neither is a milestone
    that is not
    on the card.

    Args:
        project_id (str):
        card_id (str):
        milestone_id (str):
        body (MilestoneUpdate | Unset): A change to a milestone. Each field left out keeps its
            value.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        ErrorResponse | MilestoneResponse
    """

    return sync_detailed(
        project_id=project_id,
        card_id=card_id,
        milestone_id=milestone_id,
        client=client,
        body=body,
    ).parsed


async def asyncio_detailed(
    project_id: str,
    card_id: str,
    milestone_id: str,
    *,
    client: AuthenticatedClient | Client,
    body: MilestoneUpdate | Unset = UNSET,
) -> Response[ErrorResponse | MilestoneResponse]:
    """Change a milestone.

     Each field left out keeps its value, and an update that changes nothing writes nothing. Each date
    must be a real
    day or instant. A card in the trash or in another project is not found, and neither is a milestone
    that is not
    on the card.

    Args:
        project_id (str):
        card_id (str):
        milestone_id (str):
        body (MilestoneUpdate | Unset): A change to a milestone. Each field left out keeps its
            value.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[ErrorResponse | MilestoneResponse]
    """

    kwargs = _get_kwargs(
        project_id=project_id,
        card_id=card_id,
        milestone_id=milestone_id,
        body=body,
    )

    response = await client.get_async_httpx_client().request(**kwargs)

    return _build_response(client=client, response=response)


async def asyncio(
    project_id: str,
    card_id: str,
    milestone_id: str,
    *,
    client: AuthenticatedClient | Client,
    body: MilestoneUpdate | Unset = UNSET,
) -> ErrorResponse | MilestoneResponse | None:
    """Change a milestone.

     Each field left out keeps its value, and an update that changes nothing writes nothing. Each date
    must be a real
    day or instant. A card in the trash or in another project is not found, and neither is a milestone
    that is not
    on the card.

    Args:
        project_id (str):
        card_id (str):
        milestone_id (str):
        body (MilestoneUpdate | Unset): A change to a milestone. Each field left out keeps its
            value.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        ErrorResponse | MilestoneResponse
    """

    return (
        await asyncio_detailed(
            project_id=project_id,
            card_id=card_id,
            milestone_id=milestone_id,
            client=client,
            body=body,
        )
    ).parsed
