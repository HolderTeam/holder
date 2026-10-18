"""Readable project activity and card history backed by saved revisions."""

from __future__ import annotations

from dataclasses import dataclass, field
from datetime import UTC, datetime
from typing import TYPE_CHECKING, Any, Self

from ..collections import Collection
from ..exceptions import ProtocolError
from ..generated.api.default import (
    get_projects_project_id_history,
    get_projects_project_id_history_cards_card_id,
    get_projects_project_id_history_cards_card_id_compare,
    get_projects_project_id_history_cards_card_id_snapshot,
    post_projects_project_id_history_cards_card_id_restore,
)
from ..generated.models import (
    GetProjectsProjectIdHistoryCardsCardIdCompareMode as CompareMode,
)
from ..generated.models import (
    GetProjectsProjectIdHistoryKind as ActivityKind,
)
from ..generated.types import UNSET, Unset

if TYPE_CHECKING:
    from .project import Project


def _revision(value: str | CardChange | CardSave | CardVersion) -> str:
    if isinstance(value, str):
        revision = value
    elif isinstance(value, (CardChange, CardSave, CardVersion)):
        revision = value.revision
    else:
        raise TypeError(
            "revision must be a string, CardChange, CardSave, or CardVersion"
        )
    if not revision:
        raise ValueError("revision must be nonempty")
    return revision


@dataclass(frozen=True)
class ActivityItem:
    path: str
    title: str | None = None
    detail: str | None = None


@dataclass(frozen=True)
class AffectedObject:
    kind: str
    paths: tuple[str, ...]
    items: tuple[ActivityItem, ...]


@dataclass(frozen=True)
class ProjectActivity:
    revision: str
    parent_revisions: tuple[str, ...]
    author: str
    author_email: str
    authored_at: int
    committed_at: int
    message: str
    affected_objects: tuple[AffectedObject, ...]
    is_merge: bool

    @property
    def when(self) -> datetime:
        return datetime.fromtimestamp(self.committed_at, UTC)

    @classmethod
    def _from(cls, row: Any) -> Self:
        return cls(
            revision=row.oid,
            parent_revisions=tuple(row.parent_oids),
            author=row.author.name,
            author_email=row.author.email,
            authored_at=row.authored_at,
            committed_at=row.committed_at,
            message=row.message,
            affected_objects=tuple(
                AffectedObject(
                    kind=str(obj.kind),
                    paths=tuple(obj.paths),
                    items=(
                        tuple(ActivityItem(path) for path in obj.paths)
                        if isinstance(obj.items, Unset)
                        else tuple(
                            ActivityItem(
                                path=item.path,
                                title=None
                                if isinstance(item.title, Unset)
                                else item.title,
                                detail=None
                                if isinstance(item.detail, Unset)
                                else item.detail,
                            )
                            for item in obj.items
                        )
                    ),
                )
                for obj in row.affected_objects
            ),
            is_merge=row.is_merge,
        )


class ProjectActivityCollection(Collection[ProjectActivity]):
    def __init__(self, project: Project, query: dict[str, Any] | None = None) -> None:
        self._project = project
        super().__init__(project._holder, query)

    def _clone(self, query: dict[str, Any]) -> Self:
        return type(self)(self._project, query)

    def filter(self, *, kind: str) -> Self:
        if not isinstance(kind, str):
            raise TypeError("kind must be a string")
        try:
            selected = ActivityKind(kind)
        except ValueError:
            raise ValueError(f"Unsupported activity kind: {kind}") from None
        return self._clone({**self._query, "kind": selected})

    def _fetch_page(self) -> tuple[list[ProjectActivity], str | None]:
        cursor = self._next
        while True:
            data = self._holder._call(
                get_projects_project_id_history,
                self._project.id,
                limit=50,
                cursor=cursor or UNSET,
                **self._query,
            )
            rows = [ProjectActivity._from(row) for row in data.activities]
            _check_cursor(cursor, data.next_cursor)
            if rows or data.next_cursor is None:
                return rows, data.next_cursor
            cursor = data.next_cursor


@dataclass(frozen=True)
class CardVersion:
    """The card's saved content at one revision; never a live Card."""

    revision: str
    exists: bool
    title: str
    content: str

    @classmethod
    def _from(cls, row: Any) -> Self:
        return cls(row.oid, row.exists, row.title, row.body)


@dataclass(frozen=True)
class DiffLine:
    origin: str
    text: str
    old_line: int | None
    new_line: int | None


@dataclass(frozen=True)
class CardComparison:
    before: CardVersion
    after: CardVersion
    summary: str
    lines: tuple[DiffLine, ...]
    truncated: bool

    @property
    def text(self) -> str:
        """A simple line diff; structured lines retain their line numbers."""
        return "\n".join(f"{line.origin}{line.text}" for line in self.lines)

    @classmethod
    def _from(cls, row: Any) -> Self:
        return cls(
            before=CardVersion._from(row.from_),
            after=CardVersion._from(row.to),
            summary=row.summary,
            lines=tuple(
                DiffLine(str(line.origin), line.text, line.old_line, line.new_line)
                for line in row.lines
            ),
            truncated=row.truncated,
        )


@dataclass(frozen=True)
class CardSave:
    revision: str
    parent_revisions: tuple[str, ...]
    authored_at: int
    committed_at: int
    message: str
    _history: CardHistoryCollection = field(repr=False, compare=False)

    @property
    def when(self) -> datetime:
        return datetime.fromtimestamp(self.committed_at, UTC)

    def version(self) -> CardVersion:
        return self._history.version(self)

    def restore(self) -> CardRestoreResult:
        return self._history.restore_version(self)


@dataclass(frozen=True)
class CardChange:
    """One displayed history entry, possibly containing several exact saves."""

    revision: str
    first_revision: str
    parent_revisions: tuple[str, ...]
    visible_parent_revisions: tuple[str, ...]
    author: str
    author_email: str
    started_at: int
    ended_at: int
    kind: str
    summary: str
    commit_count: int
    is_merge: bool
    saves: tuple[CardSave, ...]
    _history: CardHistoryCollection = field(repr=False, compare=False)

    @property
    def when(self) -> datetime:
        return datetime.fromtimestamp(self.ended_at, UTC)

    def version(self) -> CardVersion:
        return self._history.version(self)

    def diff(self) -> CardComparison:
        """Compare the state before this entire displayed entry with its final save."""
        if self.parent_revisions:
            return self._history.compare(self.parent_revisions[0], self)
        return self._history._change(self.first_revision)

    def restore(self) -> CardRestoreResult:
        return self._history.restore_version(self)

    @classmethod
    def _from(cls, history: CardHistoryCollection, row: Any) -> Self:
        return cls(
            revision=row.last_oid,
            first_revision=row.first_oid,
            parent_revisions=tuple(row.parent_oids),
            visible_parent_revisions=tuple(row.visible_parent_oids),
            author=row.author.name,
            author_email=row.author.email,
            started_at=row.started_at,
            ended_at=row.ended_at,
            kind=str(row.kind),
            summary=row.summary,
            commit_count=row.commit_count,
            is_merge=row.is_merge,
            saves=tuple(
                CardSave(
                    save.oid,
                    tuple(save.parent_oids),
                    save.authored_at,
                    save.committed_at,
                    save.message,
                    history,
                )
                for save in row.saves
            ),
            _history=history,
        )


@dataclass(frozen=True)
class CardRestoreResult:
    """The new revision written by a historical restore."""

    card_id: str
    restored_from_revision: str
    result_revision: str
    title: str
    updated_at: int
    deleted_at: int | None


class CardHistoryCollection(Collection[CardChange]):
    def __init__(self, project: Project, card_id: str) -> None:
        if not isinstance(card_id, str):
            raise TypeError("card_id must be a string")
        if not card_id:
            raise ValueError("card_id must be nonempty")
        self._project = project
        self.card_id = card_id
        super().__init__(project._holder)

    def _clone(self, query: dict[str, Any]) -> Self:
        return type(self)(self._project, self.card_id)

    def _fetch_page(self) -> tuple[list[CardChange], str | None]:
        cursor = self._next
        while True:
            data = self._holder._call(
                get_projects_project_id_history_cards_card_id,
                self._project.id,
                self.card_id,
                limit=50,
                cursor=cursor or UNSET,
            )
            rows = [CardChange._from(self, row) for row in data.entries]
            _check_cursor(cursor, data.next_cursor)
            if rows or data.next_cursor is None:
                return rows, data.next_cursor
            cursor = data.next_cursor

    def version(
        self, revision: str | CardChange | CardSave | CardVersion
    ) -> CardVersion:
        data = self._holder._call(
            get_projects_project_id_history_cards_card_id_snapshot,
            self._project.id,
            self.card_id,
            oid=_revision(revision),
        )
        if data.card_id != self.card_id:
            raise ProtocolError("Snapshot returned a different card")
        return CardVersion._from(data.snapshot)

    def compare(
        self,
        before: str | CardChange | CardSave | CardVersion,
        after: str | CardChange | CardSave | CardVersion,
    ) -> CardComparison:
        data = self._holder._call(
            get_projects_project_id_history_cards_card_id_compare,
            self._project.id,
            self.card_id,
            from_=_revision(before),
            to=_revision(after),
            mode=CompareMode.SINCE,
        )
        return CardComparison._from(data)

    def _change(self, revision: str) -> CardComparison:
        data = self._holder._call(
            get_projects_project_id_history_cards_card_id_compare,
            self._project.id,
            self.card_id,
            to=revision,
            mode=CompareMode.CHANGE,
        )
        return CardComparison._from(data)

    def restore_version(
        self, revision: str | CardChange | CardSave | CardVersion
    ) -> CardRestoreResult:
        """Write a new revision containing the selected historical card state."""
        data = self._holder._call(
            post_projects_project_id_history_cards_card_id_restore,
            self._project.id,
            self.card_id,
            oid=_revision(revision),
            write=True,
        )
        if data.card_id != self.card_id:
            raise ProtocolError("Restore returned a different card")
        return CardRestoreResult(
            data.card_id,
            data.restored_from_oid,
            data.result_oid,
            data.title,
            data.updated_at,
            data.deleted_at,
        )


def _check_cursor(previous: int | str | None, cursor: str | None) -> None:
    if cursor is not None and (
        not isinstance(cursor, str) or not cursor or cursor == previous
    ):
        raise ProtocolError("History pagination did not advance")
