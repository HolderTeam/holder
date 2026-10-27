"""Cards and their project-scoped synchronous collection queries."""

from __future__ import annotations

import os
import time
from dataclasses import dataclass, field, fields
from functools import cached_property
from typing import TYPE_CHECKING, Any, Literal, Self

from ..collections import Collection
from ..exceptions import AmbiguousCardReferenceError, NotFoundError, ProtocolError
from ..generated.api.default import (
    delete_cards_card_id,
    get_cards_card_id,
    get_projects_project_id_cards,
    get_projects_project_id_cards_card_id_breadcrumbs,
    patch_cards_card_id,
    post_card_references_resolve,
    post_cards,
    post_cards_card_id_move,
    post_cards_card_id_restore,
)
from ..generated.models import (
    CardMoveIntent,
    CardMoveRequest,
    CardReferenceResolveRequest,
    CardReferenceResolveRequestScope,
    CardUpdateRequest,
    DeleteCardsCardIdChildren,
    GetProjectsProjectIdCardsOrder,
    NewCard,
)
from ..generated.types import UNSET, Unset
from .card_reference import CardReferenceResult

# The parent value that selects a project's top-level cards.
ROOTS = "roots"
from .history import (
    CardChange,
    CardComparison,
    CardHistoryCollection,
    CardRestoreResult,
    CardSave,
    CardVersion,
)
from .import_job import ImportJob, start_import
from .link import CardBacklinkCollection, CardLinkCollection
from .milestone import CardMilestoneCollection
from .resource import CardAttachmentCollection, Resource
from .search import CardSearchCollection
from .storage import StorageLocation
from .tag import CardTagCollection

if TYPE_CHECKING:
    from .project import Project


@dataclass(frozen=True)
class CardBreadcrumb:
    """One project or card step in a live card's path."""

    kind: Literal["project", "card"]
    id: str
    title: str


class CardCollection(Collection["Card"]):
    def __init__(
        self,
        project: Project,
        query: dict[str, Any] | None = None,
        max_items: int | None = None,
    ) -> None:
        self._project = project
        self._max_items = max_items
        super().__init__(project._holder, query)

    def create(self, *, title: str, content: str = "") -> Card:
        """Create a root card, or a child when called on a children collection.

        Tag filters do not add tags to the content. Cached listings remain
        unchanged until refreshed. The write is never retried.
        """
        if not isinstance(title, str):
            raise TypeError("title must be a string")
        if not isinstance(content, str):
            raise TypeError("content must be a string")
        parent = self._query.get("parent", UNSET)
        if parent == ROOTS:
            parent = UNSET
        data = self._holder._call(
            post_cards,
            body=NewCard(
                project_id=self._project.id,
                title=title,
                content=content,
                parent_card_id=parent,
            ),
            write=True,
        )
        return Card._from(self._project, data)

    def _clone(self, query: dict[str, Any]) -> Self:
        return type(self)(self._project, query, self._max_items)

    def filter(self, *, tag: str) -> Self:
        if not isinstance(tag, str) or not tag.strip():
            raise ValueError("tag must be a nonempty string")
        return self._clone({**self._query, "tag": tag})

    def with_child_counts(self) -> Self:
        """Include each returned card's number of live immediate children."""
        return self._clone({**self._query, "include_child_count": True})

    def with_content(self) -> Self:
        """Load card bodies with each page, avoiding a request per card."""
        return self._clone({**self._query, "include_content": True})

    def roots(self) -> Self:
        return self._clone(
            {
                **self._query,
                "parent": ROOTS,
                "order": GetProjectsProjectIdCardsOrder.SORT_KEY_ASC,
            }
        )

    def recent(self, limit: int = 20) -> Self:
        """Recently updated live cards, newest first, up to ``limit`` cards."""
        if not isinstance(limit, int) or isinstance(limit, bool) or limit < 1:
            raise ValueError("limit must be a positive integer")
        return type(self)(
            self._project,
            {**self._query, "order": GetProjectsProjectIdCardsOrder.UPDATED_DESC},
            limit,
        )

    def search(self, query: str) -> CardSearchCollection:
        """Search this project's cards using the daemon's full-text query syntax.

        Tag and hierarchy filters cannot be combined with the search endpoint.
        """
        if not isinstance(query, str):
            raise TypeError("query must be a string")
        if not query.strip():
            raise ValueError("query must be nonempty")
        if self._query:
            raise ValueError("Search does not support tag or hierarchy filters")
        return CardSearchCollection(self._project, query)

    def _fetch_page(self) -> tuple[list[Card], str | None]:
        remaining = (
            200
            if self._max_items is None
            else min(200, self._max_items - len(self._cache))
        )
        data = self._holder._call(
            get_projects_project_id_cards,
            self._project.id,
            limit=remaining,
            cursor=self._next or UNSET,
            **self._query,
        )
        if self._query.get("include_content") and any(
            isinstance(item.content, Unset) for item in data.items
        ):
            raise ProtocolError("Card listing omitted requested content")
        rows = [Card._from(self._project, item) for item in data.items]
        if any(card.project_id != self._project.id for card in rows):
            raise ProtocolError("Card listing returned an object from another project")
        cursor = data.next_cursor
        if (
            self._max_items is not None
            and len(self._cache) + len(rows) >= self._max_items
        ):
            cursor = None
        if cursor is not None and (
            not isinstance(cursor, str)
            or not cursor
            or not rows
            or cursor == self._next
        ):
            raise ProtocolError("Card pagination did not advance")
        return rows, cursor

    def get(self, card_id: str) -> Card:
        data = self._holder._call(get_cards_card_id, card_id)
        if data.project_id != self._project.id:
            raise NotFoundError(404, "not_found", "Card not found in this project")
        return Card._from(self._project, data)

    def lookup(
        self,
        reference: str,
        *,
        scope: Literal["live", "trashed", "either"] = "live",
    ) -> CardReferenceResult:
        """Inspect an ID, ID prefix, or exact title within this project.

        Like get(), this ignores filters on the collection. Ambiguous results
        contain two examples, not an exhaustive list of matching cards.
        """
        if not isinstance(reference, str) or not reference:
            raise ValueError("reference must be a nonempty string")
        try:
            selected_scope = CardReferenceResolveRequestScope(scope)
        except ValueError:
            raise ValueError('scope must be "live", "trashed", or "either"') from None
        data = self._holder._call(
            post_card_references_resolve,
            body=CardReferenceResolveRequest(
                project_id=self._project.id,
                reference=reference,
                scope=selected_scope,
            ),
        )
        return CardReferenceResult._from(data)

    def resolve(self, reference: str) -> Card | None:
        """Open one live card by ID, ID prefix, or exact title.

        Return None when there is no match. Raise AmbiguousCardReferenceError
        with two candidate examples when the reference is ambiguous.
        """
        result = self.lookup(reference)
        if result.status == "not_found":
            return None
        if result.status == "ambiguous":
            raise AmbiguousCardReferenceError(reference, result.candidates)
        if result.card is None:
            raise ProtocolError("Resolved card reference omitted the card")
        return self.get(result.card.id)

    def history(self, card_id: str) -> CardHistoryCollection:
        """History by ID, including a card no longer available as a live Card."""
        return CardHistoryCollection(self._project, card_id)


@dataclass(frozen=True, eq=False)
class Card:
    card_id: str
    project_id: str
    title: str
    rel_path: str
    sort_key: float
    created_at: int
    updated_at: int
    parent_card_id: str | None
    deleted_at: int | None
    child_count: int | None
    _project: Project = field(repr=False, compare=False)
    _content: str | None = field(default=None, repr=False, compare=False)

    @property
    def id(self) -> str:
        return self.card_id

    @property
    def content(self) -> str:
        self._project._holder._active()
        if self._content is None:
            self.refresh()
        if self._content is None:
            raise ProtocolError("Card response did not include content")
        return self._content

    @property
    def breadcrumbs(self) -> tuple[CardBreadcrumb, ...]:
        """The current live path from this project to this card."""
        rows = self._project._holder._call(
            get_projects_project_id_cards_card_id_breadcrumbs,
            self._project.id,
            self.id,
        )
        steps = tuple(CardBreadcrumb(str(row.kind), row.id, row.title) for row in rows)
        if (
            len(steps) < 2
            or steps[0].kind != "project"
            or steps[0].id != self._project.id
            or steps[-1].kind != "card"
            or steps[-1].id != self.id
            or any(step.kind != "card" for step in steps[1:])
        ):
            raise ProtocolError("Invalid card breadcrumb path")
        return steps

    @cached_property
    def children(self) -> CardCollection:
        return CardCollection(
            self._project,
            {"parent": self.id, "order": GetProjectsProjectIdCardsOrder.SORT_KEY_ASC},
        )

    @cached_property
    def tags(self) -> CardTagCollection:
        return CardTagCollection(self)

    @cached_property
    def links(self) -> CardLinkCollection:
        return CardLinkCollection(self)

    @cached_property
    def backlinks(self) -> CardBacklinkCollection:
        return CardBacklinkCollection(self)

    @cached_property
    def milestones(self) -> CardMilestoneCollection:
        return CardMilestoneCollection(self)

    @cached_property
    def attachments(self) -> CardAttachmentCollection:
        """Resources attached to this live card."""
        return CardAttachmentCollection(self)

    def attach(self, resource: Resource | str) -> bool:
        return self.attachments.attach(resource)

    def detach(self, resource: Resource | str) -> bool:
        return self.attachments.detach(resource)

    def import_file(
        self, source: str | os.PathLike[str], *, location: StorageLocation | None = None
    ) -> ImportJob:
        """Import a daemon-readable file using a location or this device's preference."""
        return start_import(self, source, location)

    @cached_property
    def history(self) -> CardHistoryCollection:
        return self._project.cards.history(self.id)

    def version(
        self, revision: str | CardChange | CardSave | CardVersion
    ) -> CardVersion:
        return self.history.version(revision)

    def compare(
        self,
        before: str | CardChange | CardSave | CardVersion,
        after: str | CardChange | CardSave | CardVersion,
    ) -> CardComparison:
        return self.history.compare(before, after)

    def restore_version(
        self, revision: str | CardChange | CardSave | CardVersion
    ) -> CardRestoreResult:
        """Restore a saved card state as a new revision, then refresh this card."""
        result = self.history.restore_version(revision)
        if result.deleted_at is None:
            self.refresh()
        return result

    @classmethod
    def _from(cls, project: Project, data: Any) -> Self:
        values: dict[str, Any] = {
            f.name: getattr(data, f.name, None)
            for f in fields(cls)
            if not f.name.startswith("_")
        }
        values = {k: None if isinstance(v, Unset) else v for k, v in values.items()}
        # Collection pages carry content only when asked; an absent field means "not loaded".
        content = getattr(data, "content", None)
        if isinstance(content, Unset):
            content = None
        return cls(**values, _project=project, _content=content)

    def refresh(self) -> Self:
        fresh = self._project.cards.get(self.id)
        for f in fields(self):
            if f.name != "_project":
                object.__setattr__(self, f.name, getattr(fresh, f.name))
        if "tags" in self.__dict__:
            self.tags.refresh()
        return self

    def update(
        self, *, title: str | None | Unset = UNSET, content: str | Unset = UNSET
    ) -> Self:
        if isinstance(title, Unset) and isinstance(content, Unset):
            raise TypeError("Supply title or content")
        if not isinstance(title, (str, Unset)) and title is not None:
            raise TypeError("title must be a string or None")
        if not isinstance(content, (str, Unset)):
            raise TypeError("content must be a string")
        # Verify membership and obtain fresh content before any write.
        current = self._project.cards.get(self.id)
        if isinstance(content, Unset):
            content = current.content
        body = CardUpdateRequest(
            updated_at=int(time.time()), title=title, content=content
        )
        self._project._holder._call(patch_cards_card_id, self.id, body=body, write=True)
        return self.refresh()

    def move(
        self,
        *,
        parent: Card | None | Unset = UNSET,
        before: Card | Unset = UNSET,
        after: Card | Unset = UNSET,
    ) -> Self:
        """Move into a parent or immediately before/after a target card.

        Supply exactly one Card, or parent=None to append the card to the
        project's top level. Relative moves adopt the target's parent.
        The daemon enforces project membership and prevents hierarchy cycles.
        The moved card is refreshed; evaluated listings need explicit refresh.
        Writes are never retried, even if the subsequent refresh fails.
        """
        supplied = [
            (intent, target)
            for intent, target in (
                (CardMoveIntent.INTO, parent),
                (CardMoveIntent.BEFORE, before),
                (CardMoveIntent.AFTER, after),
            )
            if not isinstance(target, Unset)
        ]
        if len(supplied) != 1:
            raise TypeError("Supply exactly one of parent, before or after")
        intent, target = supplied[0]
        if intent is CardMoveIntent.INTO and target is None:
            # An explicit null parent is the project's top level.
            body = CardMoveRequest(
                project_id=self._project.id,
                intent=CardMoveIntent.TO_END,
                parent_card_id=None,
            )
        elif isinstance(target, Card):
            body = CardMoveRequest(
                project_id=self._project.id,
                intent=intent,
                target_card_id=target.id,
            )
        else:
            raise TypeError("The move target must be a Card")
        self._project._holder._call(
            post_cards_card_id_move, self.id, body=body, write=True
        )
        return self.refresh()

    def trash(self, *, children: str = "promote") -> None:
        """Move this card to Trash.

        With children="promote" (the default), its live children take its place
        among its siblings, keeping their own descendants. With
        children="trash_subtree", every live descendant goes to Trash with it.
        A trashed card cannot be read again until restored, so this object is
        not refreshed. The write is never retried.
        """
        try:
            policy = DeleteCardsCardIdChildren(children)
        except ValueError:
            raise ValueError('children must be "promote" or "trash_subtree"') from None
        self._project._holder._call(
            delete_cards_card_id, self.id, children=policy, write=True
        )

    def restore(self) -> Self:
        """Restore this card from Trash, and refresh it.

        Restoring is single-card: it returns to its saved parent when that
        parent is live, otherwise to its nearest live ancestor or the top level.
        Children promoted when it was trashed stay where they are.
        """
        self._project._holder._call(post_cards_card_id_restore, self.id, write=True)
        return self.refresh()
