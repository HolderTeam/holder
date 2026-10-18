"""Project-scoped Trash browsing and removal."""

from __future__ import annotations

from dataclasses import dataclass, field
from datetime import UTC, datetime
from typing import TYPE_CHECKING, Any, Literal, Self

from ..collections import Collection
from ..exceptions import ProtocolError
from ..generated.api.default import (
    delete_projects_project_id_trash_type_id,
    delete_trash,
    get_trash,
    post_ai_messages_message_id_restore,
    post_cards_card_id_restore,
)
from ..generated.models import (
    DeleteProjectsProjectIdTrashTypeIdType as TrashType,
)
from ..generated.types import Unset

if TYPE_CHECKING:
    from .card import Card
    from .project import Project


def _optional(value: Any) -> Any:
    return None if isinstance(value, Unset) else value


@dataclass(frozen=True, eq=False)
class TrashItem:
    """A card or AI message currently listed in a project's Trash."""

    type: Literal["card", "ai_message"]
    id: str
    project_id: str
    deleted_at: int
    title: str | None
    role: str | None
    thread_id: str | None
    created_at: int | None
    rel_path: str | None
    _project: Project = field(repr=False, compare=False)

    @property
    def when(self) -> datetime:
        return datetime.fromtimestamp(self.deleted_at, UTC)

    @classmethod
    def _from(cls, project: Project, row: Any) -> Self:
        if row.type_ not in ("card", "ai_message") or row.project_id != project.id:
            raise ProtocolError("Trash listing returned an invalid item")
        item_id = _optional(row.card_id if row.type_ == "card" else row.message_id)
        deleted_at = _optional(row.deleted_at)
        if (
            not isinstance(item_id, str)
            or not item_id
            or not isinstance(deleted_at, int)
        ):
            raise ProtocolError("Trash listing omitted an item ID or deletion time")
        return cls(
            type=row.type_,
            id=item_id,
            project_id=project.id,
            deleted_at=deleted_at,
            title=_optional(row.title),
            role=_optional(row.role),
            thread_id=_optional(row.thread_id),
            created_at=_optional(row.created_at),
            rel_path=_optional(row.rel_path),
            _project=project,
        )

    def restore(self) -> Card | None:
        """Restore this item; return the live Card for cards.

        AI messages have no high-level live object, so restoring one returns
        None. A cached Trash listing needs refresh() to reflect the change.
        """
        if self.type == "card":
            self._project._holder._call(post_cards_card_id_restore, self.id, write=True)
            return self._project.cards.get(self.id)
        self._project._holder._call(
            post_ai_messages_message_id_restore, self.id, write=True
        )
        return None

    def delete_permanently(self) -> None:
        """Remove this trashed item from current storage.

        Card revisions already saved in Git history remain available. This
        operation cannot be undone through Trash.
        """
        self._project._holder._call(
            delete_projects_project_id_trash_type_id,
            self.project_id,
            TrashType(self.type),
            self.id,
            write=True,
        )


class TrashCollection(Collection[TrashItem]):
    """Lazy project Trash listing, cached until refresh()."""

    def __init__(self, project: Project, query: dict[str, Any] | None = None) -> None:
        self._project = project
        super().__init__(project._holder, query)

    def _clone(self, query: dict[str, Any]) -> Self:
        return type(self)(self._project, query)

    def filter(self, *, type: Literal["card", "ai_message"]) -> Self:
        if type not in ("card", "ai_message"):
            raise ValueError('type must be "card" or "ai_message"')
        return self._clone({"type_": type})

    def cards(self) -> Self:
        return self.filter(type="card")

    def messages(self) -> Self:
        return self.filter(type="ai_message")

    def _fetch_page(self) -> tuple[list[TrashItem], None]:
        data = self._holder._call(get_trash, project_id=self._project.id, **self._query)
        return [TrashItem._from(self._project, row) for row in data], None

    def empty(self) -> None:
        """Permanently remove matching items from this project's Trash.

        Call cards() or messages() first to limit the operation. Saved card
        revisions remain in Git history. Refresh cached listings afterwards.
        """
        self._holder._call(
            delete_trash, project_id=self._project.id, write=True, **self._query
        )
