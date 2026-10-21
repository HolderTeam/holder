from http import HTTPStatus
from typing import Any
from urllib.parse import quote

import httpx

from ... import errors
from ...client import AuthenticatedClient, Client
from ...models.error_response import ErrorResponse
from ...models.hierarchy_repair_request import HierarchyRepairRequest
from ...models.hierarchy_repair_response import HierarchyRepairResponse
from ...types import UNSET, Response, Unset


def _get_kwargs(
    project_id: str,
    *,
    body: HierarchyRepairRequest | Unset = UNSET,
) -> dict[str, Any]:
    headers: dict[str, Any] = {}

    _kwargs: dict[str, Any] = {
        "method": "post",
        "url": "/projects/{project_id}/repair-hierarchy".format(
            project_id=quote(str(project_id), safe=""),
        ),
    }

    if not isinstance(body, Unset):
        _kwargs["json"] = body.to_dict()

    headers["Content-Type"] = "application/json"

    _kwargs["headers"] = headers
    return _kwargs


def _parse_response(
    *, client: AuthenticatedClient | Client, response: httpx.Response
) -> ErrorResponse | HierarchyRepairResponse | None:
    if response.status_code == 200:
        response_200 = HierarchyRepairResponse.from_dict(response.json())

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

    if response.status_code == 501:
        response_501 = ErrorResponse.from_dict(response.json())

        return response_501

    if response.status_code == 503:
        response_503 = ErrorResponse.from_dict(response.json())

        return response_503

    if client.raise_on_unexpected_status:
        raise errors.UnexpectedStatus(response.status_code, response.content)
    else:
        return None


def _build_response(
    *, client: AuthenticatedClient | Client, response: httpx.Response
) -> Response[ErrorResponse | HierarchyRepairResponse]:
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
    body: HierarchyRepairRequest | Unset = UNSET,
) -> Response[ErrorResponse | HierarchyRepairResponse]:
    """Find, and optionally fix, cards hidden under missing or trashed parents

     A live card whose parent is missing or trashed cannot be reached by browsing from the project's top
    level. This can happen after an older client trashed a parent, a card file was removed by hand, or a
    sync brought either in. Without apply, this only reports, reading the database alone (so it works on
    a locked encrypted project). With apply, each hidden branch moves to its nearest live, reachable
    ancestor (or the top level), into the place its trashed ancestor held, with its descendants still
    attached, in one commit. Applying twice changes nothing the second time. Cards in parent cycles are
    reported and left alone. The daemon also repairs at startup and core repairs after every pull.

    Args:
        project_id (str):
        body (HierarchyRepairRequest | Unset):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[ErrorResponse | HierarchyRepairResponse]
    """

    kwargs = _get_kwargs(
        project_id=project_id,
        body=body,
    )

    response = client.get_httpx_client().request(
        **kwargs,
    )

    return _build_response(client=client, response=response)


def sync(
    project_id: str,
    *,
    client: AuthenticatedClient | Client,
    body: HierarchyRepairRequest | Unset = UNSET,
) -> ErrorResponse | HierarchyRepairResponse | None:
    """Find, and optionally fix, cards hidden under missing or trashed parents

     A live card whose parent is missing or trashed cannot be reached by browsing from the project's top
    level. This can happen after an older client trashed a parent, a card file was removed by hand, or a
    sync brought either in. Without apply, this only reports, reading the database alone (so it works on
    a locked encrypted project). With apply, each hidden branch moves to its nearest live, reachable
    ancestor (or the top level), into the place its trashed ancestor held, with its descendants still
    attached, in one commit. Applying twice changes nothing the second time. Cards in parent cycles are
    reported and left alone. The daemon also repairs at startup and core repairs after every pull.

    Args:
        project_id (str):
        body (HierarchyRepairRequest | Unset):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        ErrorResponse | HierarchyRepairResponse
    """

    return sync_detailed(
        project_id=project_id,
        client=client,
        body=body,
    ).parsed


async def asyncio_detailed(
    project_id: str,
    *,
    client: AuthenticatedClient | Client,
    body: HierarchyRepairRequest | Unset = UNSET,
) -> Response[ErrorResponse | HierarchyRepairResponse]:
    """Find, and optionally fix, cards hidden under missing or trashed parents

     A live card whose parent is missing or trashed cannot be reached by browsing from the project's top
    level. This can happen after an older client trashed a parent, a card file was removed by hand, or a
    sync brought either in. Without apply, this only reports, reading the database alone (so it works on
    a locked encrypted project). With apply, each hidden branch moves to its nearest live, reachable
    ancestor (or the top level), into the place its trashed ancestor held, with its descendants still
    attached, in one commit. Applying twice changes nothing the second time. Cards in parent cycles are
    reported and left alone. The daemon also repairs at startup and core repairs after every pull.

    Args:
        project_id (str):
        body (HierarchyRepairRequest | Unset):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[ErrorResponse | HierarchyRepairResponse]
    """

    kwargs = _get_kwargs(
        project_id=project_id,
        body=body,
    )

    response = await client.get_async_httpx_client().request(**kwargs)

    return _build_response(client=client, response=response)


async def asyncio(
    project_id: str,
    *,
    client: AuthenticatedClient | Client,
    body: HierarchyRepairRequest | Unset = UNSET,
) -> ErrorResponse | HierarchyRepairResponse | None:
    """Find, and optionally fix, cards hidden under missing or trashed parents

     A live card whose parent is missing or trashed cannot be reached by browsing from the project's top
    level. This can happen after an older client trashed a parent, a card file was removed by hand, or a
    sync brought either in. Without apply, this only reports, reading the database alone (so it works on
    a locked encrypted project). With apply, each hidden branch moves to its nearest live, reachable
    ancestor (or the top level), into the place its trashed ancestor held, with its descendants still
    attached, in one commit. Applying twice changes nothing the second time. Cards in parent cycles are
    reported and left alone. The daemon also repairs at startup and core repairs after every pull.

    Args:
        project_id (str):
        body (HierarchyRepairRequest | Unset):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        ErrorResponse | HierarchyRepairResponse
    """

    return (
        await asyncio_detailed(
            project_id=project_id,
            client=client,
            body=body,
        )
    ).parsed
