"""Qualified card links and card-scoped link queries."""

from __future__ import annotations

from dataclasses import dataclass
from typing import TYPE_CHECKING, Any, Self

from ..collections import Collection
from ..exceptions import NotFoundError, ProtocolError
from ..generated.api.default import (
    delete_cards_card_id_links,
    get_cards_card_id_backlinks,
    get_cards_card_id_links,
    post_cards_card_id_links,
)
from ..generated.models import CardLinkCreateRequest, CardLinkDeleteRequest
from ..generated.models.get_cards_card_id_backlinks_include_deleted import (
    GetCardsCardIdBacklinksIncludeDeleted,
)
from ..generated.models.get_cards_card_id_backlinks_scope import (
    GetCardsCardIdBacklinksScope,
)
from ..generated.models.get_cards_card_id_links_include_deleted import (
    GetCardsCardIdLinksIncludeDeleted,
)
from ..generated.types import UNSET, Unset

if TYPE_CHECKING:
    from .card import Card


@dataclass(frozen=True)
class CardLink:
    """A link identity and its metadata; the target need not be available locally."""

    source_id: str
    source_project_id: str
    target_id: str
    target_project_id: str
    target_type: str
    kind: str
    label: str | None
    created_at: int

    @classmethod
    def _from(cls, row: Any, project_id: str) -> Self:
        source_project = row.source_project_id
        if isinstance(source_project, Unset):
            source_project = project_id
        target_project = row.target_project_id
        if isinstance(target_project, Unset):
            target_project = source_project
        label = None if isinstance(row.label, Unset) else row.label
        if (
            not all(
                isinstance(value, str) and value
                for value in (
                    row.from_card_id,
                    source_project,
                    row.to_card_id,
                    target_project,
                    row.to_type,
                    row.kind,
                )
            )
            or not isinstance(label, (str, type(None)))
            or not isinstance(row.created_at, int)
        ):
            raise ProtocolError("Invalid card link response")
        return cls(
            source_id=row.from_card_id,
            source_project_id=source_project,
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

    def _check_scope(self) -> None:
        current = self._card._project.cards.get(self._card.id)
        if current.project_id != self._card.project_id:
            raise NotFoundError(404, "not_found", "Card not found in this project")

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
        self._check_scope()
        data = self._holder._call(
            get_cards_card_id_links,
            self._card.id,
            include_deleted=GetCardsCardIdLinksIncludeDeleted(
                int(self._include_deleted)
            ),
        )
        return [CardLink._from(row, self._card.project_id) for row in data], None

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
        self._check_scope()
        row = self._holder._call(
            post_cards_card_id_links,
            self._card.id,
            body=CardLinkCreateRequest(
                to_card_id=to_card_id,
                target_project_id=target_project_id or UNSET,
                to_type=to_type,
                kind=kind,
                label=label,
            ),
            write=True,
        )
        self.refresh()
        return CardLink._from(row, self._card.project_id)

    def remove(
        self,
        to_card_id: str,
        *,
        target_project_id: str | None = None,
        to_type: str = "card",
        kind: str = "ref",
    ) -> Self:
        """Remove exactly one qualified target/type/kind combination."""
        _validate_identity(to_card_id, target_project_id, to_type, kind)
        self._check_scope()
        self._holder._call(
            delete_cards_card_id_links,
            self._card.id,
            body=CardLinkDeleteRequest(
                to_card_id=to_card_id,
                target_project_id=target_project_id or self._card.project_id,
                to_type=to_type,
                kind=kind,
            ),
            write=True,
        )
        self.refresh()
        return self


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
    """Backlinks from the chosen daemon-visible source projects."""

    def __init__(
        self, card: Card, *, scope: str = "local", include_deleted: bool = False
    ) -> None:
        try:
            self._scope = GetCardsCardIdBacklinksScope(scope)
        except ValueError:
            raise ValueError(
                "scope must be local, external, or all_available"
            ) from None
        self._coverage: str | None = None
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

    @property
    def coverage(self) -> str:
        """Target project or available projects, as reported by the daemon."""
        if self._coverage is None:
            list(self)
        assert self._coverage is not None
        return self._coverage

    def refresh(self) -> Self:
        super().refresh()
        self._coverage = None
        return self

    def _fetch_page(self) -> tuple[list[CardLink], None]:
        self._check_scope()
        data = self._holder._call(
            get_cards_card_id_backlinks,
            self._card.id,
            scope=self._scope,
            include_deleted=GetCardsCardIdBacklinksIncludeDeleted(
                int(self._include_deleted)
            ),
            return_envelope=True,
        )
        if data.scope.value != self._scope.value:
            raise ProtocolError("Backlink response has the wrong scope")
        self._coverage = data.coverage.value
        return [CardLink._from(row, self._card.project_id) for row in data.data], None
