"""Detached project records for analysis and export."""

from collections.abc import Iterator

from ._workspace_contract import CardListItem, CardOperations
from .exceptions import ProtocolError


def iter_card_records(
    cards: CardOperations, project_id: str, *, include_content: bool = False
) -> Iterator[CardListItem]:
    """Yield detached live Card records from a project in card ID order.

    Card bodies are optional. Keep the session open while iterating. Each page
    reads current data, so edits during iteration can appear in the export.
    """
    cursor = None
    seen_cursors: set[str] = set()
    while True:
        page = cards.list_cards(
            project_id, cursor=cursor, include_content=include_content
        )
        items, cursor = page["items"], page["next_cursor"]
        if cursor is not None:
            if not cursor or not items or cursor in seen_cursors:
                raise ProtocolError("Card export pagination did not advance")
            seen_cursors.add(cursor)
        yield from items
        if cursor is None:
            return
