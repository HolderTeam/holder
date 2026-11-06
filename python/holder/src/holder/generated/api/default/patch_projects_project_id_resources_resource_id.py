from http import HTTPStatus
from typing import Any
from urllib.parse import quote

import httpx

from ... import errors
from ...client import AuthenticatedClient, Client
from ...models.error_response import ErrorResponse
from ...models.resource_response import ResourceResponse
from ...models.resource_update import ResourceUpdate
from ...types import UNSET, Response, Unset


def _get_kwargs(
    project_id: str,
    resource_id: str,
    *,
    body: ResourceUpdate | Unset = UNSET,
) -> dict[str, Any]:
    headers: dict[str, Any] = {}

    _kwargs: dict[str, Any] = {
        "method": "patch",
        "url": "/projects/{project_id}/resources/{resource_id}".format(
            project_id=quote(str(project_id), safe=""),
            resource_id=quote(str(resource_id), safe=""),
        ),
    }

    if not isinstance(body, Unset):
        _kwargs["json"] = body.to_dict()

    headers["Content-Type"] = "application/json"

    _kwargs["headers"] = headers
    return _kwargs


def _parse_response(
    *, client: AuthenticatedClient | Client, response: httpx.Response
) -> ErrorResponse | ResourceResponse | None:
    if response.status_code == 200:
        response_200 = ResourceResponse.from_dict(response.json())

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
) -> Response[ErrorResponse | ResourceResponse]:
    return Response(
        status_code=HTTPStatus(response.status_code),
        content=response.content,
        headers=response.headers,
        parsed=_parse_response(client=client, response=response),
    )


def sync_detailed(
    project_id: str,
    resource_id: str,
    *,
    client: AuthenticatedClient | Client,
    body: ResourceUpdate | Unset = UNSET,
) -> Response[ErrorResponse | ResourceResponse]:
    """Change a resource.

     Each field left out keeps its value. A resource in another project is not found.

    Args:
        project_id (str):
        resource_id (str):
        body (ResourceUpdate | Unset): A change to a resource. Each field left out keeps its
            value.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[ErrorResponse | ResourceResponse]
    """

    kwargs = _get_kwargs(
        project_id=project_id,
        resource_id=resource_id,
        body=body,
    )

    response = client.get_httpx_client().request(
        **kwargs,
    )

    return _build_response(client=client, response=response)


def sync(
    project_id: str,
    resource_id: str,
    *,
    client: AuthenticatedClient | Client,
    body: ResourceUpdate | Unset = UNSET,
) -> ErrorResponse | ResourceResponse | None:
    """Change a resource.

     Each field left out keeps its value. A resource in another project is not found.

    Args:
        project_id (str):
        resource_id (str):
        body (ResourceUpdate | Unset): A change to a resource. Each field left out keeps its
            value.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        ErrorResponse | ResourceResponse
    """

    return sync_detailed(
        project_id=project_id,
        resource_id=resource_id,
        client=client,
        body=body,
    ).parsed


async def asyncio_detailed(
    project_id: str,
    resource_id: str,
    *,
    client: AuthenticatedClient | Client,
    body: ResourceUpdate | Unset = UNSET,
) -> Response[ErrorResponse | ResourceResponse]:
    """Change a resource.

     Each field left out keeps its value. A resource in another project is not found.

    Args:
        project_id (str):
        resource_id (str):
        body (ResourceUpdate | Unset): A change to a resource. Each field left out keeps its
            value.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[ErrorResponse | ResourceResponse]
    """

    kwargs = _get_kwargs(
        project_id=project_id,
        resource_id=resource_id,
        body=body,
    )

    response = await client.get_async_httpx_client().request(**kwargs)

    return _build_response(client=client, response=response)


async def asyncio(
    project_id: str,
    resource_id: str,
    *,
    client: AuthenticatedClient | Client,
    body: ResourceUpdate | Unset = UNSET,
) -> ErrorResponse | ResourceResponse | None:
    """Change a resource.

     Each field left out keeps its value. A resource in another project is not found.

    Args:
        project_id (str):
        resource_id (str):
        body (ResourceUpdate | Unset): A change to a resource. Each field left out keeps its
            value.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        ErrorResponse | ResourceResponse
    """

    return (
        await asyncio_detailed(
            project_id=project_id,
            resource_id=resource_id,
            client=client,
            body=body,
        )
    ).parsed
