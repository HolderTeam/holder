"""Qualified card links and card-scoped link queries."""

from __future__ import annotations

from dataclasses import dataclass
from typing import TYPE_CHECKING, Any, Self

from ..collections import Collection
from ..exceptions import ProtocolError
from ..generated.api.default import (
    delete_projects_project_id_cards_card_id_links,
    get_projects_project_id_cards_card_id_backlinks,
    get_projects_project_id_cards_card_id_links,
    post_projects_project_id_cards_card_id_links,
)
from ..generated.models import LinkTargetType, NewLink
from ..generated.models.get_projects_project_id_cards_card_id_backlinks_scope import (
    GetProjectsProjectIdCardsCardIdBacklinksScope,
)
from ..generated.types import UNSET, Unset

if TYPE_CHECKING:
    from .card import Card


@dataclass(frozen=True)
class CardLink:
    """A link's two ends, each a project, a type and an ID, and its metadata.

    The target need not be available locally. A backlink's source is a card or
    an AI message.
    """

    source_id: str
    source_project_id: str
    source_type: str
    target_id: str
    target_project_id: str
    target_type: str
    kind: str
    label: str | None
    created_at: int

    @classmethod
    def _from(cls, row: Any) -> Self:
        label = None if isinstance(row.label, Unset) else row.label
        names = (
            row.source_id,
            row.source_project_id,
            row.target_id,
            row.target_project_id,
            row.kind,
        )
        if (
            not all(isinstance(value, str) and value for value in names)
            or not isinstance(label, (str, type(None)))
            or not isinstance(row.created_at, int)
        ):
            raise ProtocolError("Invalid card link response")
        return cls(
            source_id=row.source_id,
            source_project_id=row.source_project_id,
            source_type=row.source_type.value,
            target_id=row.target_id,
            target_project_id=row.target_project_id,
            target_type=row.target_type.value,
            kind=row.kind,
            label=label,
            created_at=row.created_at,
        )

    @classmethod
    def _from_ai_message_route(
        cls, row: Any, project_id: str, source_type: str
    ) -> Self:
        """A link as the AI message routes answer.

        Those routes leave out the project of a local end, and do not say what a
        backlink's source is, so the caller says.
        """
        source_project = row.source_project_id
        if isinstance(source_project, Unset):
            source_project = project_id
        target_project = row.target_project_id
        if isinstance(target_project, Unset):
            target_project = source_project
        label = None if isinstance(row.label, Unset) else row.label
        names = (row.from_card_id, source_project, row.to_card_id, target_project)
        if (
            not all(isinstance(value, str) and value for value in names)
            or not all(
                isinstance(value, str) and value for value in (row.to_type, row.kind)
            )
            or not isinstance(label, (str, type(None)))
            or not isinstance(row.created_at, int)
        ):
            raise ProtocolError("Invalid card link response")
        return cls(
            source_id=row.from_card_id,
            source_project_id=source_project,
            source_type=source_type,
            target_id=row.to_card_id,
            target_project_id=target_project,
            target_type=row.to_type,
            kind=row.kind,
            label=label,
            created_at=row.created_at,
        )


class _CardLinkCollection(Collection[CardLink]):
    def __init__(self, card: Card, *, include_deleted: bool = False) -> None:
        self._card = card
        self._include_deleted = include_deleted
        super().__init__(card._project._holder)

    def including_deleted(self) -> Self:
        """Return a fresh query that includes deleted local targets or sources."""
        return self._clone({"include_deleted": True})


class CardLinkCollection(_CardLinkCollection):
    """Outgoing links; evaluated results stay cached until refresh()."""

    def _clone(self, query: dict[str, Any]) -> Self:
        return type(self)(
            self._card,
            include_deleted=query.get("include_deleted", self._include_deleted),
        )

    def _fetch_page(self) -> tuple[list[CardLink], None]:
        data = self._holder._call(
            get_projects_project_id_cards_card_id_links,
            self._card.project_id,
            self._card.id,
            include_deleted=self._include_deleted,
        )
        return [CardLink._from(row) for row in data], None

    def add(
        self,
        to_card_id: str,
        *,
        target_project_id: str | None = None,
        to_type: str = "card",
        kind: str = "ref",
        label: str | None = None,
    ) -> CardLink:
        """Create or update one qualified link; validation belongs to the daemon."""
        _validate_identity(to_card_id, target_project_id, to_type, kind)
        if label is not None and not isinstance(label, str):
            raise TypeError("label must be a string or None")
        row = self._holder._call(
            post_projects_project_id_cards_card_id_links,
            self._card.project_id,
            self._card.id,
            body=NewLink(
                target_type=_target_type(to_type),
                target_id=to_card_id,
                target_project_id=target_project_id or UNSET,
                kind=kind,
                label=label,
            ),
            write=True,
        )
        self.refresh()
        return CardLink._from(row)

    def remove(
        self,
        to_card_id: str,
        *,
        target_project_id: str | None = None,
        to_type: str = "card",
        kind: str = "ref",
    ) -> Self:
        """Remove exactly one qualified target/type/kind combination.

        Removing a link the card does not have changes nothing.
        """
        _validate_identity(to_card_id, target_project_id, to_type, kind)
        self._holder._call(
            delete_projects_project_id_cards_card_id_links,
            self._card.project_id,
            self._card.id,
            target_type=_target_type(to_type),
            target_id=to_card_id,
            kind=kind,
            target_project_id=target_project_id or UNSET,
            write=True,
        )
        self.refresh()
        return self


def _target_type(to_type: str) -> LinkTargetType:
    try:
        return LinkTargetType(to_type)
    except ValueError:
        raise ValueError(
            "to_type must be card, ai_message, ai_thread or resource"
        ) from None


def _validate_identity(
    to_card_id: str, target_project_id: str | None, to_type: str, kind: str
) -> None:
    for name, value in (
        ("to_card_id", to_card_id),
        ("to_type", to_type),
        ("kind", kind),
    ):
        if not isinstance(value, str) or not value:
            raise ValueError(f"{name} must be a nonempty string")
    if target_project_id is not None and (
        not isinstance(target_project_id, str) or not target_project_id
    ):
        raise ValueError("target_project_id must be a nonempty string or None")


class CardBacklinkCollection(_CardLinkCollection):
    """Backlinks from the chosen daemon-visible source projects.

    External backlinks come only from projects available to the daemon, so they
    are never all there may be.
    """

    def __init__(
        self, card: Card, *, scope: str = "local", include_deleted: bool = False
    ) -> None:
        try:
            self._scope = GetProjectsProjectIdCardsCardIdBacklinksScope(scope)
        except ValueError:
            raise ValueError(
                "scope must be local, external, or all_available"
            ) from None
        super().__init__(card, include_deleted=include_deleted)

    def _clone(self, query: dict[str, Any]) -> Self:
        return type(self)(
            self._card,
            scope=query.get("scope", self._scope.value),
            include_deleted=query.get("include_deleted", self._include_deleted),
        )

    def filter(self, *, scope: str) -> Self:
        """Choose local, external, or all_available source projects."""
        return self._clone({"scope": scope})

    @property
    def scope(self) -> str:
        return self._scope.value

    def _fetch_page(self) -> tuple[list[CardLink], None]:
        data = self._holder._call(
            get_projects_project_id_cards_card_id_backlinks,
            self._card.project_id,
            self._card.id,
            scope=self._scope,
            include_deleted=self._include_deleted,
        )
        return [CardLink._from(row) for row in data], None
