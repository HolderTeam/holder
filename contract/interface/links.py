"""Links: a card's references to other cards, AI messages, AI threads and resources."""

from typing import Annotated, Literal

from holder_contract import MISSING, Field, Record, Timestamp

from .cards import ProjectId

type LinkKind = Annotated[str, Field(min_length=1)]
"""What a link means, such as ref or depends_on."""

type LinkTargetType = Literal["card", "ai_message", "ai_thread", "resource"]
"""What a link points at."""

type LinkTargetId = Annotated[str, Field(min_length=1)]
"""The ID of what a link points at: a card, an AI message, an AI thread or a resource."""


class Link(Record):
    """A link from a card, or from an AI message, to something in a project. A link names both of its ends in full:
    each end's project, type and ID."""

    source_project_id: ProjectId
    """The project of the card or AI message the link is from."""
    source_type: Literal["card", "ai_message"]
    """What the link is from."""
    source_id: Annotated[str, Field(min_length=1)]
    """The ID of the card or AI message the link is from."""
    target_project_id: ProjectId
    """The project of what the link points at: the source's own project, or, for a card, another project, which
    need not be on this device. A link grants no access to its target."""
    target_type: LinkTargetType
    """What the link points at."""
    target_id: LinkTargetId
    """The ID of what the link points at."""
    kind: LinkKind
    """What the link means."""
    label: str | None
    """A note on the link; null when it has none."""
    created_at: Timestamp
    """When the link was made."""


class NewLink(Record):
    """A link to make from a card. Its source is the card it is made from."""

    target_type: LinkTargetType
    """What the link points at."""
    target_id: LinkTargetId
    """The ID of what the link points at."""
    target_project_id: ProjectId | MISSING = MISSING
    """The project of a card the link points at; omit for the card's own project. Only a card can be in another
    project, which need not be on this device."""
    kind: LinkKind = "ref"
    """What the link means."""
    label: str | None = None
    """A note on the link; null or omitted for none."""
    created_at: Timestamp | MISSING = MISSING
    """When the link was made, for imports; omit for now. A link that already exists keeps its own."""
