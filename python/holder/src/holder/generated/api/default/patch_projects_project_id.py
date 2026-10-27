from http import HTTPStatus
from typing import Any
from urllib.parse import quote

import httpx

from ... import errors
from ...client import AuthenticatedClient, Client
from ...models.error_response import ErrorResponse
from ...models.project_response import ProjectResponse
from ...models.project_update import ProjectUpdate
from ...types import Response


def _get_kwargs(
    project_id: str,
    *,
    body: ProjectUpdate,
) -> dict[str, Any]:
    headers: dict[str, Any] = {}

    _kwargs: dict[str, Any] = {
        "method": "patch",
        "url": "/projects/{project_id}".format(
            project_id=quote(str(project_id), safe=""),
        ),
    }

    _kwargs["json"] = body.to_dict()

    headers["Content-Type"] = "application/json"

    _kwargs["headers"] = headers
    return _kwargs


def _parse_response(
    *, client: AuthenticatedClient | Client, response: httpx.Response
) -> ErrorResponse | ProjectResponse | None:
    if response.status_code == 200:
        response_200 = ProjectResponse.from_dict(response.json())

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
) -> Response[ErrorResponse | ProjectResponse]:
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
    body: ProjectUpdate,
) -> Response[ErrorResponse | ProjectResponse]:
    """Change a project's settings.

     Each field left out keeps its value, and an update that changes nothing writes nothing. The settings
    are
    committed to the project's repository; a Git remote is set there before it is stored, and one Git
    does not
    accept is an invalid argument.

    Args:
        project_id (str):
        body (ProjectUpdate): A change to a project's settings. Each field left out keeps its
            value.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[ErrorResponse | ProjectResponse]
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
    body: ProjectUpdate,
) -> ErrorResponse | ProjectResponse | None:
    """Change a project's settings.

     Each field left out keeps its value, and an update that changes nothing writes nothing. The settings
    are
    committed to the project's repository; a Git remote is set there before it is stored, and one Git
    does not
    accept is an invalid argument.

    Args:
        project_id (str):
        body (ProjectUpdate): A change to a project's settings. Each field left out keeps its
            value.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        ErrorResponse | ProjectResponse
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
    body: ProjectUpdate,
) -> Response[ErrorResponse | ProjectResponse]:
    """Change a project's settings.

     Each field left out keeps its value, and an update that changes nothing writes nothing. The settings
    are
    committed to the project's repository; a Git remote is set there before it is stored, and one Git
    does not
    accept is an invalid argument.

    Args:
        project_id (str):
        body (ProjectUpdate): A change to a project's settings. Each field left out keeps its
            value.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[ErrorResponse | ProjectResponse]
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
    body: ProjectUpdate,
) -> ErrorResponse | ProjectResponse | None:
    """Change a project's settings.

     Each field left out keeps its value, and an update that changes nothing writes nothing. The settings
    are
    committed to the project's repository; a Git remote is set there before it is stored, and one Git
    does not
    accept is an invalid argument.

    Args:
        project_id (str):
        body (ProjectUpdate): A change to a project's settings. Each field left out keeps its
            value.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        ErrorResponse | ProjectResponse
    """

    return (
        await asyncio_detailed(
            project_id=project_id,
            client=client,
            body=body,
        )
    ).parsed
