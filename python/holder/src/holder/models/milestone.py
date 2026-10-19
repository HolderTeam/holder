"""Card milestones with ordered, labelled calendar days and instants."""

from __future__ import annotations

from dataclasses import dataclass, field, fields
from datetime import date, datetime, timedelta
from functools import cached_property
from typing import TYPE_CHECKING, Any, Self
from uuid import UUID

from ..collections import Collection
from ..exceptions import NotFoundError, ProtocolError
from ..generated.api.default import (
    delete_cards_card_id_milestones_with_dates_milestone_id,
    get_cards_card_id_milestones_with_dates,
    get_projects_project_id_milestones_with_dates,
    patch_cards_card_id_milestones_with_dates_milestone_id,
    post_cards_card_id_milestones_with_dates,
)
from ..generated.models import (
    GetProjectsProjectIdMilestonesWithDatesValueType,
    MilestoneDateValue,
    MilestoneWithDatesCreateRequest,
    MilestoneWithDatesUpdateRequest,
)
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
        return MilestoneDateValue(role=self.role, date=self.value)

    @classmethod
    def _from(cls, row: MilestoneDateValue) -> Self:
        has_date = not isinstance(row.date, Unset)
        has_datetime = not isinstance(row.datetime_, Unset)
        if has_date == has_datetime:
            raise ProtocolError("Milestone date must have exactly one value")
        if has_date:
            value = row.date
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

    def _check_scope(self) -> None:
        # The route identifies cards globally; retain this Card's project scope.
        current = self._card._project.cards.get(self._card.id)
        if current.project_id != self._card.project_id:
            raise NotFoundError(404, "not_found", "Card not found in this project")

    def _fetch_page(self) -> tuple[list[Milestone], None]:
        self._check_scope()
        rows = self._holder._call(
            get_cards_card_id_milestones_with_dates, self._card.id
        )
        return [
            Milestone._from(self._card._project, self, row, card_id=self._card.id)
            for row in rows
        ], None

    def get(self, milestone_id: UUID | str) -> Milestone:
        """Fetch by stable ID independently of this collection's cache."""
        identity = _uuid(milestone_id)
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
        self._check_scope()
        row = self._holder._call(
            post_cards_card_id_milestones_with_dates,
            self._card.id,
            body=MilestoneWithDatesCreateRequest(
                dates=encoded, kind=kind, description=description
            ),
            write=True,
        )
        self.refresh()
        return Milestone._from(self._card._project, self, row, card_id=self._card.id)


class ProjectMilestones:
    """Search labelled milestone dates across one project."""

    def __init__(self, project: Project) -> None:
        self._project = project

    def between(
        self, start: date | datetime, end: date | datetime, *, role: str
    ) -> ProjectMilestoneResults:
        """Find milestones with a matching role and date in an inclusive range.

        Calendar days and offset instants are separate searches. Results are
        lazy, cached until refresh(), and omit milestones on trashed cards.
        """
        if not isinstance(role, str) or not role:
            raise ValueError("role must be a nonempty string")
        if isinstance(start, datetime) and isinstance(end, datetime):
            for value in (start, end):
                offset = value.utcoffset()
                if offset is None:
                    raise ValueError("datetime bounds must have explicit UTC offsets")
                if value.microsecond or offset % timedelta(minutes=1):
                    raise ValueError(
                        "datetime bounds must use whole seconds and minute offsets"
                    )
            if start > end:
                raise ValueError("start must not be after end")
            value_type = GetProjectsProjectIdMilestonesWithDatesValueType.DATETIME
            from_value, to_value = (
                str(int(start.timestamp())),
                str(int(end.timestamp())),
            )
        elif (
            isinstance(start, date)
            and not isinstance(start, datetime)
            and isinstance(end, date)
            and not isinstance(end, datetime)
        ):
            if start > end:
                raise ValueError("start must not be after end")
            value_type = GetProjectsProjectIdMilestonesWithDatesValueType.DATE
            from_value, to_value = start.isoformat(), end.isoformat()
        else:
            raise TypeError("start and end must both be dates or aware datetimes")
        return ProjectMilestoneResults(
            self._project, role, value_type, from_value, to_value
        )


class ProjectMilestoneResults(Collection["Milestone"]):
    """A cached project query; each result is the ordinary Milestone type."""

    def __init__(
        self,
        project: Project,
        role: str,
        value_type: GetProjectsProjectIdMilestonesWithDatesValueType,
        from_value: str,
        to_value: str,
    ) -> None:
        self._project = project
        self._role = role
        self._value_type = value_type
        self._from_value = from_value
        self._to_value = to_value
        super().__init__(project._holder)

    def _clone(self, query: dict[str, Any]) -> Self:
        return type(self)(
            self._project,
            self._role,
            self._value_type,
            self._from_value,
            self._to_value,
        )

    def _fetch_page(self) -> tuple[list[Milestone], None]:
        rows = self._holder._call(
            get_projects_project_id_milestones_with_dates,
            self._project.id,
            role=self._role,
            value_type=self._value_type,
            from_=self._from_value,
            to=self._to_value,
        )
        return [Milestone._from(self._project, self, row) for row in rows], None


def _uuid(value: UUID | str) -> UUID:
    if isinstance(value, UUID):
        return value
    if not isinstance(value, str):
        raise TypeError("milestone_id must be a UUID or UUID string")
    try:
        return UUID(value)
    except ValueError:
        raise ValueError("milestone_id must be a UUID") from None


@dataclass(frozen=True, eq=False)
class Milestone:
    """A card milestone; no completion state is inferred from its date roles."""

    id: UUID
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
        ):
            raise ProtocolError("Milestone belongs to another card")
        return cls(
            id=row.milestone_id,
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
        self._project.cards.get(self.card_id)
        row = self._project._holder._call(
            patch_cards_card_id_milestones_with_dates_milestone_id,
            self.card_id,
            self.id,
            body=MilestoneWithDatesUpdateRequest(
                dates=encoded, kind=kind, description=description
            ),
            write=True,
        )
        self._listing.refresh()
        self._replace(
            Milestone._from(self._project, self._listing, row, card_id=self.card_id)
        )
        return self

    def remove(self) -> bool:
        """Remove this milestone; return False if it was already absent."""
        self._project.cards.get(self.card_id)
        result = self._project._holder._call(
            delete_cards_card_id_milestones_with_dates_milestone_id,
            self.card_id,
            self.id,
            write=True,
        )
        if result.card_id != self.card_id or result.milestone_id != self.id:
            raise ProtocolError("Milestone removal returned another identity")
        self._listing.refresh()
        return result.removed
