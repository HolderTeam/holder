"""Card milestones with ordered, labelled calendar days and instants."""

from __future__ import annotations

from dataclasses import dataclass, field, fields
from datetime import date, datetime, timedelta
from functools import cached_property
from typing import TYPE_CHECKING, Any, Self

from ..collections import Collection
from ..exceptions import NotFoundError, ProtocolError
from ..generated.api.default import (
    delete_projects_project_id_cards_card_id_milestones_milestone_id,
    get_projects_project_id_cards_card_id_milestones,
    get_projects_project_id_milestones_days,
    get_projects_project_id_milestones_instants,
    patch_projects_project_id_cards_card_id_milestones_milestone_id,
    post_projects_project_id_cards_card_id_milestones,
)
from ..generated.models import MilestoneDate as MilestoneDateValue
from ..generated.models import MilestoneUpdate, NewMilestone
from ..generated.types import UNSET, Unset

if TYPE_CHECKING:
    from .card import Card
    from .project import Project


@dataclass(frozen=True)
class MilestoneDate:
    """One labelled day or offset instant; roles may repeat and order matters."""

    role: str
    value: date | datetime

    def __post_init__(self) -> None:
        if not isinstance(self.role, str) or not self.role:
            raise ValueError("role must be a nonempty string")
        if isinstance(self.value, datetime):
            offset = self.value.utcoffset()
            if offset is None:
                raise ValueError("datetime must have an explicit UTC offset")
            if self.value.microsecond or offset % timedelta(minutes=1):
                raise ValueError("datetime must use whole seconds and a minute offset")
        elif not isinstance(self.value, date):
            raise TypeError("value must be a date or timezone-aware datetime")

    def _generated(self) -> MilestoneDateValue:
        if isinstance(self.value, datetime):
            return MilestoneDateValue(
                role=self.role, datetime_=self.value.isoformat(timespec="seconds")
            )
        return MilestoneDateValue(role=self.role, date=self.value.isoformat())

    @classmethod
    def _from(cls, row: MilestoneDateValue) -> Self:
        has_date = not isinstance(row.date, Unset)
        has_datetime = not isinstance(row.datetime_, Unset)
        if has_date == has_datetime:
            raise ProtocolError("Milestone date must have exactly one value")
        if has_date:
            try:
                value = date.fromisoformat(row.date)
            except (TypeError, ValueError):
                raise ProtocolError("Invalid milestone date") from None
        else:
            try:
                value = datetime.fromisoformat(row.datetime_)
            except (TypeError, ValueError):
                raise ProtocolError("Invalid milestone datetime") from None
        try:
            return cls(role=row.role, value=value)
        except (TypeError, ValueError):
            raise ProtocolError("Invalid milestone date") from None


def _dates(
    values: list[MilestoneDate] | tuple[MilestoneDate, ...],
) -> list[MilestoneDateValue]:
    if not isinstance(values, (list, tuple)) or any(
        not isinstance(value, MilestoneDate) for value in values
    ):
        raise TypeError("dates must be a list or tuple of MilestoneDate values")
    return [value._generated() for value in values]


def _optional_text(name: str, value: str | None | Unset) -> None:
    if not isinstance(value, (str, type(None), Unset)):
        raise TypeError(f"{name} must be a string or None")


class CardMilestoneCollection(Collection["Milestone"]):
    """All milestones on one live card, including undated ones."""

    def __init__(self, card: Card) -> None:
        self._card = card
        super().__init__(card._project._holder)

    def _clone(self, query: dict[str, Any]) -> Self:
        return type(self)(self._card)

    def _fetch_page(self) -> tuple[list[Milestone], None]:
        rows = self._holder._call(
            get_projects_project_id_cards_card_id_milestones,
            self._card.project_id,
            self._card.id,
        )
        return [
            Milestone._from(self._card._project, self, row, card_id=self._card.id)
            for row in rows
        ], None

    def get(self, milestone_id: str) -> Milestone:
        """Fetch by stable ID independently of this collection's cache."""
        identity = _identity(milestone_id)
        for milestone in self.all():
            if milestone.id == identity:
                return milestone
        raise NotFoundError(404, "not_found", "Milestone not found on this card")

    def create(
        self,
        *,
        dates: list[MilestoneDate] | tuple[MilestoneDate, ...] = (),
        kind: str | None = None,
        description: str | None = None,
    ) -> Milestone:
        """Create a milestone; an empty dates sequence makes it undated."""
        encoded = _dates(dates)
        _optional_text("kind", kind)
        _optional_text("description", description)
        row = self._holder._call(
            post_projects_project_id_cards_card_id_milestones,
            self._card.project_id,
            self._card.id,
            body=NewMilestone(dates=encoded, kind=kind, description=description),
            write=True,
        )
        self.refresh()
        return Milestone._from(self._card._project, self, row, card_id=self._card.id)


class ProjectMilestones:
    """Search labelled milestone dates across one project."""

    def __init__(self, project: Project) -> None:
        self._project = project

    def between(
        self, start: date | datetime, end: date | datetime, *, role: str | None = None
    ) -> ProjectMilestoneResults:
        """Find milestones with a date in an inclusive range, of the role if given.

        Calendar days and offset instants are separate searches. A range whose
        start is after its end is empty. Results are lazy, cached until
        refresh(), and omit milestones on trashed cards.
        """
        if role is not None and (not isinstance(role, str) or not role):
            raise ValueError("role must be a nonempty string or None")
        if isinstance(start, datetime) and isinstance(end, datetime):
            for value in (start, end):
                offset = value.utcoffset()
                if offset is None:
                    raise ValueError("datetime bounds must have explicit UTC offsets")
                if value.microsecond:
                    raise ValueError("datetime bounds must use whole seconds")
            return ProjectMilestoneResults(
                self._project,
                get_projects_project_id_milestones_instants,
                {
                    "first_instant": int(start.timestamp()),
                    "last_instant": int(end.timestamp()),
                    "role": UNSET if role is None else role,
                },
            )
        if (
            isinstance(start, date)
            and not isinstance(start, datetime)
            and isinstance(end, date)
            and not isinstance(end, datetime)
        ):
            return ProjectMilestoneResults(
                self._project,
                get_projects_project_id_milestones_days,
                {
                    "first_day": start.isoformat(),
                    "last_day": end.isoformat(),
                    "role": UNSET if role is None else role,
                },
            )
        raise TypeError("start and end must both be dates or aware datetimes")


class ProjectMilestoneResults(Collection["Milestone"]):
    """A cached project query; each result is the ordinary Milestone type."""

    def __init__(self, project: Project, route: Any, params: dict[str, Any]) -> None:
        self._project = project
        self._route = route
        self._params = params
        super().__init__(project._holder)

    def _clone(self, query: dict[str, Any]) -> Self:
        return type(self)(self._project, self._route, self._params)

    def _fetch_page(self) -> tuple[list[Milestone], None]:
        rows = self._holder._call(self._route, self._project.id, **self._params)
        return [Milestone._from(self._project, self, row) for row in rows], None


def _identity(value: str) -> str:
    if not isinstance(value, str) or not value:
        raise ValueError("milestone_id must be a nonempty string")
    return value


@dataclass(frozen=True, eq=False)
class Milestone:
    """A card milestone; no completion state is inferred from its date roles."""

    id: str
    project_id: str
    card_id: str
    dates: tuple[MilestoneDate, ...]
    kind: str | None
    description: str | None
    created_at: int
    updated_at: int
    _project: Project = field(repr=False, compare=False)
    _listing: Collection[Milestone] = field(repr=False, compare=False)

    @cached_property
    def card(self) -> Card:
        """Fetch and cache the live card only when requested."""
        return self._project.cards.get(self.card_id)

    @classmethod
    def _from(
        cls,
        project: Project,
        listing: Collection[Milestone],
        row: Any,
        *,
        card_id: str | None = None,
    ) -> Self:
        if (
            not isinstance(row.card_id, str)
            or not row.card_id
            or (card_id is not None and row.card_id != card_id)
            or row.project_id != project.id
        ):
            raise ProtocolError("Milestone belongs to another card")
        if not isinstance(row.milestone_id, str) or not row.milestone_id:
            raise ProtocolError("Invalid milestone ID")
        return cls(
            id=row.milestone_id,
            project_id=row.project_id,
            card_id=row.card_id,
            dates=tuple(MilestoneDate._from(value) for value in row.dates),
            kind=row.kind,
            description=row.description,
            created_at=row.created_at,
            updated_at=row.updated_at,
            _project=project,
            _listing=listing,
        )

    def _replace(self, fresh: Milestone) -> None:
        for member in fields(self):
            if member.name not in ("_project", "_listing"):
                object.__setattr__(self, member.name, getattr(fresh, member.name))

    def refresh(self) -> Self:
        self._replace(self._project.cards.get(self.card_id).milestones.get(self.id))
        return self

    def update(
        self,
        *,
        dates: list[MilestoneDate] | tuple[MilestoneDate, ...] | Unset = UNSET,
        kind: str | None | Unset = UNSET,
        description: str | None | Unset = UNSET,
    ) -> Self:
        """Partially update; dates replace the full sequence when supplied."""
        if all(isinstance(value, Unset) for value in (dates, kind, description)):
            raise TypeError("Supply dates, kind, or description")
        encoded = UNSET if isinstance(dates, Unset) else _dates(dates)
        _optional_text("kind", kind)
        _optional_text("description", description)
        row = self._project._holder._call(
            patch_projects_project_id_cards_card_id_milestones_milestone_id,
            self.project_id,
            self.card_id,
            self.id,
            body=MilestoneUpdate(dates=encoded, kind=kind, description=description),
            write=True,
        )
        self._listing.refresh()
        self._replace(
            Milestone._from(self._project, self._listing, row, card_id=self.card_id)
        )
        return self

    def remove(self) -> None:
        """Remove this milestone; removing one already gone changes nothing."""
        remaining = self._project._holder._call(
            delete_projects_project_id_cards_card_id_milestones_milestone_id,
            self.project_id,
            self.card_id,
            self.id,
            write=True,
        )
        if any(row.milestone_id == self.id for row in remaining):
            raise ProtocolError("Milestone removal left the milestone in place")
        self._listing.refresh()
