"""Project maintenance: inspect encryption safety and repair hidden card branches."""

from typing import Annotated

from authoring import Field, Int32, Record

from .cards.models import CardId, ProjectId
from .projects import PrivacyMode


class EncryptionSafetyCheck(Record):
    """Whether an encrypted project's card files are safe to push."""

    ok: bool
    """Whether every checked file passed."""
    checked_files: Annotated[Int32, Field(ge=0)]
    """Card files examined."""
    unsafe_files: Annotated[Int32, Field(ge=0)]
    """Card files that failed the safety check."""
    unsafe_paths: list[str]
    """Project-relative paths of unsafe files."""
    message: str
    """A short explanation of the result."""


class ProjectEncryptionCheck(Record):
    """A project's privacy mode and the result of checking its files."""

    project_id: ProjectId
    """The project checked."""
    privacy_mode: PrivacyMode
    """How the project's files are stored."""
    check: EncryptionSafetyCheck
    """The safety check's result."""


class HierarchyRepairRequest(Record):
    """Ask for a repair plan, or apply it."""

    apply: bool = False
    """Apply the repair. Without it, only report."""


class HierarchyRepairMove(Record):
    """A hidden branch's top card and its new parent."""

    card_id: CardId
    """The hidden branch's top card."""
    parent_card_id: CardId | None
    """Its new parent, or null for a project root."""


class HierarchyRepair(Record):
    """A plan to make hidden live cards reachable, and whether it was applied."""

    applied: bool
    """Whether changes were written."""
    moved: list[HierarchyRepairMove]
    """The hidden branches whose top cards move."""
    changed_card_count: Annotated[Int32, Field(ge=0)]
    """Cards moved or re-spaced to make room."""
    cycle_card_ids: list[str]
    """Cards in parent cycles; left for manual repair."""
