"""Detached project records for analysis and export."""

from collections.abc import Iterator
from typing import Protocol, TypedDict

from ._workspace_contract import CardListItem, CardOperations, Link, Milestone, TagCount
from ._workspace_contract import Project as ProjectRecord
from .exceptions import ProtocolError


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
    cards: CardOperations, project_id: str, *, include_content: bool = False
) -> Iterator[CardListItem]:
    """Yield detached live Card records from a project in card ID order.

    Card bodies are optional. Keep the session open while iterating. Each page
    reads current data, so edits during iteration can appear in the export.
    """
    return _iter_card_records(cards, project_id, include_content=include_content)


def _iter_card_records(
    cards: CardOperations,
    project_id: str,
    *,
    include_content: bool = False,
    tag: str | None = None,
) -> Iterator[CardListItem]:
    cursor = None
    seen_cursors: set[str] = set()
    while True:
        page = cards.list_cards(
            project_id, cursor=cursor, include_content=include_content, tag=tag
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
                for card in _iter_card_records(cards, project_id, tag=tag)
                if card["card_id"] in card_ids
            )

    return {
        "project": project,
        "cards": card_records,
        "links": links,
        "tag_memberships": memberships,
        "milestones": milestones,
    }
