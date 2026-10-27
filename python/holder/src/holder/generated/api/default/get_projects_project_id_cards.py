from http import HTTPStatus
from typing import Any, Literal
from urllib.parse import quote

import httpx

from ... import errors
from ...client import AuthenticatedClient, Client
from ...models.card_list_item_page_response import CardListItemPageResponse
from ...models.error_response import ErrorResponse
from ...models.get_projects_project_id_cards_order import GetProjectsProjectIdCardsOrder
from ...types import UNSET, Response, Unset


def _get_kwargs(
    project_id: str,
    *,
    parent: Literal["roots"] | None | str | Unset = UNSET,
    tag: None | str | Unset = UNSET,
    order: GetProjectsProjectIdCardsOrder
    | Unset = GetProjectsProjectIdCardsOrder.CARD_ID_ASC,
    limit: int | Unset = 200,
    cursor: None | str | Unset = UNSET,
    include_content: bool | Unset = False,
    include_child_count: bool | Unset = False,
) -> dict[str, Any]:

    params: dict[str, Any] = {}

    json_parent: Literal["roots"] | None | str | Unset
    if isinstance(parent, Unset):
        json_parent = UNSET
    else:
        json_parent = parent
    params["parent"] = json_parent

    json_tag: None | str | Unset
    if isinstance(tag, Unset):
        json_tag = UNSET
    else:
        json_tag = tag
    params["tag"] = json_tag

    json_order: str | Unset = UNSET
    if not isinstance(order, Unset):
        json_order = order.value

    params["order"] = json_order

    params["limit"] = limit

    json_cursor: None | str | Unset
    if isinstance(cursor, Unset):
        json_cursor = UNSET
    else:
        json_cursor = cursor
    params["cursor"] = json_cursor

    params["include_content"] = include_content

    params["include_child_count"] = include_child_count

    params = {k: v for k, v in params.items() if v is not UNSET and v is not None}

    _kwargs: dict[str, Any] = {
        "method": "get",
        "url": "/projects/{project_id}/cards".format(
            project_id=quote(str(project_id), safe=""),
        ),
        "params": params,
    }

    return _kwargs


def _parse_response(
    *, client: AuthenticatedClient | Client, response: httpx.Response
) -> CardListItemPageResponse | ErrorResponse | None:
    if response.status_code == 200:
        response_200 = CardListItemPageResponse.from_dict(response.json())

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
) -> Response[CardListItemPageResponse | ErrorResponse]:
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
    parent: Literal["roots"] | None | str | Unset = UNSET,
    tag: None | str | Unset = UNSET,
    order: GetProjectsProjectIdCardsOrder
    | Unset = GetProjectsProjectIdCardsOrder.CARD_ID_ASC,
    limit: int | Unset = 200,
    cursor: None | str | Unset = UNSET,
    include_content: bool | Unset = False,
    include_child_count: bool | Unset = False,
) -> Response[CardListItemPageResponse | ErrorResponse]:
    """Page through a project's live cards.

     Filters combine. Each page reads current data; iterating is not a snapshot.

    Args:
        project_id (str):
        parent (Literal['roots'] | None | str | Unset): Omit for every level; roots selects root
            cards; a card ID selects its children.
        tag (None | str | Unset): Only cards with this tag.
        order (GetProjectsProjectIdCardsOrder | Unset): updated_desc breaks ties by descending
            card ID; sort_key_asc follows sibling order. Default:
            GetProjectsProjectIdCardsOrder.CARD_ID_ASC.
        limit (int | Unset): At most 1000 when include_content is true. Default: 200.
        cursor (None | str | Unset): The previous page's next_cursor. Keep the project, filters
            and order unchanged.
        include_content (bool | Unset): Include each card's body. Default: False.
        include_child_count (bool | Unset): Include each card's count of live children. Default:
            False.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[CardListItemPageResponse | ErrorResponse]
    """

    kwargs = _get_kwargs(
        project_id=project_id,
        parent=parent,
        tag=tag,
        order=order,
        limit=limit,
        cursor=cursor,
        include_content=include_content,
        include_child_count=include_child_count,
    )

    response = client.get_httpx_client().request(
        **kwargs,
    )

    return _build_response(client=client, response=response)


def sync(
    project_id: str,
    *,
    client: AuthenticatedClient | Client,
    parent: Literal["roots"] | None | str | Unset = UNSET,
    tag: None | str | Unset = UNSET,
    order: GetProjectsProjectIdCardsOrder
    | Unset = GetProjectsProjectIdCardsOrder.CARD_ID_ASC,
    limit: int | Unset = 200,
    cursor: None | str | Unset = UNSET,
    include_content: bool | Unset = False,
    include_child_count: bool | Unset = False,
) -> CardListItemPageResponse | ErrorResponse | None:
    """Page through a project's live cards.

     Filters combine. Each page reads current data; iterating is not a snapshot.

    Args:
        project_id (str):
        parent (Literal['roots'] | None | str | Unset): Omit for every level; roots selects root
            cards; a card ID selects its children.
        tag (None | str | Unset): Only cards with this tag.
        order (GetProjectsProjectIdCardsOrder | Unset): updated_desc breaks ties by descending
            card ID; sort_key_asc follows sibling order. Default:
            GetProjectsProjectIdCardsOrder.CARD_ID_ASC.
        limit (int | Unset): At most 1000 when include_content is true. Default: 200.
        cursor (None | str | Unset): The previous page's next_cursor. Keep the project, filters
            and order unchanged.
        include_content (bool | Unset): Include each card's body. Default: False.
        include_child_count (bool | Unset): Include each card's count of live children. Default:
            False.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        CardListItemPageResponse | ErrorResponse
    """

    return sync_detailed(
        project_id=project_id,
        client=client,
        parent=parent,
        tag=tag,
        order=order,
        limit=limit,
        cursor=cursor,
        include_content=include_content,
        include_child_count=include_child_count,
    ).parsed


async def asyncio_detailed(
    project_id: str,
    *,
    client: AuthenticatedClient | Client,
    parent: Literal["roots"] | None | str | Unset = UNSET,
    tag: None | str | Unset = UNSET,
    order: GetProjectsProjectIdCardsOrder
    | Unset = GetProjectsProjectIdCardsOrder.CARD_ID_ASC,
    limit: int | Unset = 200,
    cursor: None | str | Unset = UNSET,
    include_content: bool | Unset = False,
    include_child_count: bool | Unset = False,
) -> Response[CardListItemPageResponse | ErrorResponse]:
    """Page through a project's live cards.

     Filters combine. Each page reads current data; iterating is not a snapshot.

    Args:
        project_id (str):
        parent (Literal['roots'] | None | str | Unset): Omit for every level; roots selects root
            cards; a card ID selects its children.
        tag (None | str | Unset): Only cards with this tag.
        order (GetProjectsProjectIdCardsOrder | Unset): updated_desc breaks ties by descending
            card ID; sort_key_asc follows sibling order. Default:
            GetProjectsProjectIdCardsOrder.CARD_ID_ASC.
        limit (int | Unset): At most 1000 when include_content is true. Default: 200.
        cursor (None | str | Unset): The previous page's next_cursor. Keep the project, filters
            and order unchanged.
        include_content (bool | Unset): Include each card's body. Default: False.
        include_child_count (bool | Unset): Include each card's count of live children. Default:
            False.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        Response[CardListItemPageResponse | ErrorResponse]
    """

    kwargs = _get_kwargs(
        project_id=project_id,
        parent=parent,
        tag=tag,
        order=order,
        limit=limit,
        cursor=cursor,
        include_content=include_content,
        include_child_count=include_child_count,
    )

    response = await client.get_async_httpx_client().request(**kwargs)

    return _build_response(client=client, response=response)


async def asyncio(
    project_id: str,
    *,
    client: AuthenticatedClient | Client,
    parent: Literal["roots"] | None | str | Unset = UNSET,
    tag: None | str | Unset = UNSET,
    order: GetProjectsProjectIdCardsOrder
    | Unset = GetProjectsProjectIdCardsOrder.CARD_ID_ASC,
    limit: int | Unset = 200,
    cursor: None | str | Unset = UNSET,
    include_content: bool | Unset = False,
    include_child_count: bool | Unset = False,
) -> CardListItemPageResponse | ErrorResponse | None:
    """Page through a project's live cards.

     Filters combine. Each page reads current data; iterating is not a snapshot.

    Args:
        project_id (str):
        parent (Literal['roots'] | None | str | Unset): Omit for every level; roots selects root
            cards; a card ID selects its children.
        tag (None | str | Unset): Only cards with this tag.
        order (GetProjectsProjectIdCardsOrder | Unset): updated_desc breaks ties by descending
            card ID; sort_key_asc follows sibling order. Default:
            GetProjectsProjectIdCardsOrder.CARD_ID_ASC.
        limit (int | Unset): At most 1000 when include_content is true. Default: 200.
        cursor (None | str | Unset): The previous page's next_cursor. Keep the project, filters
            and order unchanged.
        include_content (bool | Unset): Include each card's body. Default: False.
        include_child_count (bool | Unset): Include each card's count of live children. Default:
            False.

    Raises:
        errors.UnexpectedStatus: If the server returns an undocumented status code and Client.raise_on_unexpected_status is True.
        httpx.TimeoutException: If the request takes longer than Client.timeout.

    Returns:
        CardListItemPageResponse | ErrorResponse
    """

    return (
        await asyncio_detailed(
            project_id=project_id,
            client=client,
            parent=parent,
            tag=tag,
            order=order,
            limit=limit,
            cursor=cursor,
            include_content=include_content,
            include_child_count=include_child_count,
        )
    ).parsed
