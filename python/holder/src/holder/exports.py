"""Detached project records for analysis and export."""

from collections.abc import Iterator
from typing import Literal, Protocol, TypedDict

from ._workspace_contract import CardListItem, CardOperations, Link, Milestone, TagCount
from ._workspace_contract import Project as ProjectRecord
from .exceptions import ProtocolError

_CardOrder = Literal["card_id_asc", "updated_desc", "sort_key_asc"]


class TagMembership(TypedDict):
    """A normalized tag carried by a live card in a project."""

    project_id: str
    card_id: str
    tag: str


class ProjectRecords(TypedDict):
    """Detached records for one project and its live cards."""

    project: ProjectRecord
    cards: list[CardListItem]
    links: list[Link]
    tag_memberships: list[TagMembership]
    milestones: list[Milestone]


class _ExportSession(Protocol):
    card_operations: CardOperations

    def get_project(self, project_id: str) -> ProjectRecord: ...
    def list_project_tags(self, project_id: str) -> list[TagCount]: ...


def iter_card_records(
    cards: CardOperations,
    project_id: str,
    *,
    include_content: bool = False,
    parent: str | None = None,
    tag: str | None = None,
    order: _CardOrder = "card_id_asc",
) -> Iterator[CardListItem]:
    """Yield detached live card records from a project, in card ID order by default.

    Pass ``parent="roots"`` for root cards or a parent card ID for its children.
    Parent and tag filters combine. Keep the session open while iterating.
    Each page reads current data, so edits can appear in the export.
    """
    return _iter_card_records(
        cards,
        project_id,
        include_content=include_content,
        parent=parent,
        tag=tag,
        order=order,
    )


def iter_card_batches(
    cards: CardOperations,
    project_id: str,
    *,
    batch_size: int = 256,
    include_content: bool = False,
    parent: str | None = None,
    tag: str | None = None,
    order: _CardOrder = "card_id_asc",
) -> Iterator[list[CardListItem]]:
    """Yield detached batches of live cards with the same filters as iter_card_records.

    Only the last batch may contain fewer than ``batch_size`` records. The
    underlying backend may serve several pages per batch.
    """
    if isinstance(batch_size, bool) or not isinstance(batch_size, int):
        raise TypeError("batch_size must be a positive integer")
    if batch_size < 1:
        raise ValueError("batch_size must be a positive integer")

    records = _iter_card_records(
        cards,
        project_id,
        include_content=include_content,
        parent=parent,
        tag=tag,
        order=order,
        # This is the shared card listing limit when bodies are requested.
        limit=min(batch_size, 1000),
    )
    batch: list[CardListItem] = []
    for record in records:
        batch.append(record)
        if len(batch) == batch_size:
            yield batch
            batch = []
    if batch:
        yield batch


def _iter_card_records(
    cards: CardOperations,
    project_id: str,
    *,
    include_content: bool,
    parent: str | None,
    tag: str | None,
    order: _CardOrder,
    limit: int = 200,
) -> Iterator[CardListItem]:
    cursor = None
    seen_cursors: set[str] = set()
    while True:
        page = cards.list_cards(
            project_id,
            cursor=cursor,
            include_content=include_content,
            parent=parent,
            tag=tag,
            order=order,
            limit=limit,
        )
        items, cursor = page["items"], page["next_cursor"]
        if cursor is not None:
            if not cursor or not items or cursor in seen_cursors:
                raise ProtocolError("Card export pagination did not advance")
            seen_cursors.add(cursor)
        yield from items
        if cursor is None:
            return


def _export_project_records(
    session: _ExportSession, project_id: str, *, include_content: bool
) -> ProjectRecords:
    project = session.get_project(project_id)
    cards = session.card_operations
    card_records = list(
        iter_card_records(cards, project_id, include_content=include_content)
    )
    links: list[Link] = []
    milestones: list[Milestone] = []
    for card in card_records:
        card_id = card["card_id"]
        links.extend(cards.list_card_links(project_id, card_id))
        milestones.extend(cards.list_card_milestones(project_id, card_id))

    card_ids = {card["card_id"] for card in card_records}
    memberships: list[TagMembership] = []
    if card_ids:
        for count in session.list_project_tags(project_id):
            tag = count["tag"]
            memberships.extend(
                {"project_id": project_id, "card_id": card["card_id"], "tag": tag}
                for card in iter_card_records(cards, project_id, tag=tag)
                if card["card_id"] in card_ids
            )

    return {
        "project": project,
        "cards": card_records,
        "links": links,
        "tag_memberships": memberships,
        "milestones": milestones,
    }
