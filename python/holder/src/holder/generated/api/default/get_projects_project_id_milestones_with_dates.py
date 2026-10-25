from http import HTTPStatus
from typing import Any
from urllib.parse import quote

import httpx

from ... import errors
from ...client import AuthenticatedClient, Client
from ...models.error_response import ErrorResponse
from ...models.get_projects_project_id_milestones_with_dates_value_type import (
    GetProjectsProjectIdMilestonesWithDatesValueType,
)
from ...models.milestone_with_dates_list_response import MilestoneWithDatesListResponse
from ...types import UNSET, Response


def _get_kwargs(
    project_id: str,
    *,
    role: str,
    value_type: GetProjectsProjectIdMilestonesWithDatesValueType,
    from_: str,
    to: str,
) -> dict[str, Any]:

    params: dict[str, Any] = {}

    params["role"] = role

    json_value_type = value_type.value
    params["value_type"] = json_value_type

    params["from"] = from_

    params["to"] = to

    params = {k: v for k, v in params.items() if v is not UNSET and v is not None}

    _kwargs: dict[str, Any] = {
        "method": "get",
        "url": "/projects/{project_id}/milestones-with-dates".format(
            project_id=quote(str(project_id), safe=""),
        ),
        "params": params,
    }

    return _kwargs


def _parse_response(
    *, client: AuthenticatedClient | Client, response: httpx.Response
) -> ErrorResponse | MilestoneWithDatesListResponse | None:
    if response.status_code == 200:
        response_200 = MilestoneWithDatesListResponse.from_dict(response.json())

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

    if response.status_code == 500:
        response_500 = ErrorResponse.from_dict(response.json())

        return response_500

    if client.raise_on_unexpected_status:
        raise errors.UnexpectedStatus(response.status_code, response.content)
    else:
        return None


def _build_response(
    *, client: AuthenticatedClient | Client, response: httpx.Response
) -> Response[ErrorResponse | MilestoneWithDatesListResponse]:
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
    role: str,
    value_type: GetProjectsProjectIdMilestonesWithDatesValueType,
    from_: str,
    to: str,
) -> Response[ErrorResponse | MilestoneWithDatesListResponse]:
    """Query project milestones by a labelled date role

     Returns each matching milestone once with its complete ordered dates array. Calendar days and
    instants are queried separately; both range bounds are inclusive. Milestones on trashed cards are
    excluded. Roles are free-form, and this endpoint does not infer outstanding or completion state.

    Args:
        project_id (str):
        role (str):
        value_type (GetProjectsProjectIdMilestonesWithDatesValueType):
        from_ (str):
        to (str):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[ErrorResponse | MilestoneWithDatesListResponse]
    """

    kwargs = _get_kwargs(
        project_id=project_id,
        role=role,
        value_type=value_type,
        from_=from_,
        to=to,
    )

    response = client.get_httpx_client().request(
        **kwargs,
    )

    return _build_response(client=client, response=response)


def sync(
    project_id: str,
    *,
    client: AuthenticatedClient | Client,
    role: str,
    value_type: GetProjectsProjectIdMilestonesWithDatesValueType,
    from_: str,
    to: str,
) -> ErrorResponse | MilestoneWithDatesListResponse | None:
    """Query project milestones by a labelled date role

     Returns each matching milestone once with its complete ordered dates array. Calendar days and
    instants are queried separately; both range bounds are inclusive. Milestones on trashed cards are
    excluded. Roles are free-form, and this endpoint does not infer outstanding or completion state.

    Args:
        project_id (str):
        role (str):
        value_type (GetProjectsProjectIdMilestonesWithDatesValueType):
        from_ (str):
        to (str):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        ErrorResponse | MilestoneWithDatesListResponse
    """

    return sync_detailed(
        project_id=project_id,
        client=client,
        role=role,
        value_type=value_type,
        from_=from_,
        to=to,
    ).parsed


async def asyncio_detailed(
    project_id: str,
    *,
    client: AuthenticatedClient | Client,
    role: str,
    value_type: GetProjectsProjectIdMilestonesWithDatesValueType,
    from_: str,
    to: str,
) -> Response[ErrorResponse | MilestoneWithDatesListResponse]:
    """Query project milestones by a labelled date role

     Returns each matching milestone once with its complete ordered dates array. Calendar days and
    instants are queried separately; both range bounds are inclusive. Milestones on trashed cards are
    excluded. Roles are free-form, and this endpoint does not infer outstanding or completion state.

    Args:
        project_id (str):
        role (str):
        value_type (GetProjectsProjectIdMilestonesWithDatesValueType):
        from_ (str):
        to (str):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[ErrorResponse | MilestoneWithDatesListResponse]
    """

    kwargs = _get_kwargs(
        project_id=project_id,
        role=role,
        value_type=value_type,
        from_=from_,
        to=to,
    )

    response = await client.get_async_httpx_client().request(**kwargs)

    return _build_response(client=client, response=response)


async def asyncio(
    project_id: str,
    *,
    client: AuthenticatedClient | Client,
    role: str,
    value_type: GetProjectsProjectIdMilestonesWithDatesValueType,
    from_: str,
    to: str,
) -> ErrorResponse | MilestoneWithDatesListResponse | None:
    """Query project milestones by a labelled date role

     Returns each matching milestone once with its complete ordered dates array. Calendar days and
    instants are queried separately; both range bounds are inclusive. Milestones on trashed cards are
    excluded. Roles are free-form, and this endpoint does not infer outstanding or completion state.

    Args:
        project_id (str):
        role (str):
        value_type (GetProjectsProjectIdMilestonesWithDatesValueType):
        from_ (str):
        to (str):

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        ErrorResponse | MilestoneWithDatesListResponse
    """

    return (
        await asyncio_detailed(
            project_id=project_id,
            client=client,
            role=role,
            value_type=value_type,
            from_=from_,
            to=to,
        )
    ).parsed
