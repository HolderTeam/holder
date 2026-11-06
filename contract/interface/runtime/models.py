"""What core's runtime operations report."""

from typing import Annotated

from authoring import Field, Int32, Record


class CoreVersion(Record):
    """The version of the core library loaded."""

    version: str
    """The whole version, such as 1.4.2."""
    major: Int32
    """Its major number."""
    minor: Int32
    """Its minor number."""
    patch: Int32
    """Its patch number."""


class DatabaseRebuildReport(Record):
    """What rebuilding a data directory's database from its project files found, or would find."""

    dry_run: bool
    """Whether the database was left as it was and only counted."""
    previous_health: str
    """The old database's state: healthy, missing, corrupt or stale."""
    projects: Annotated[Int32, Field(ge=0)]
    """Projects read."""
    cards: Annotated[Int32, Field(ge=0)]
    """Cards read."""
    ai_threads: Annotated[Int32, Field(ge=0)]
    """AI threads read."""
    ai_messages: Annotated[Int32, Field(ge=0)]
    """AI messages read."""
    resources: Annotated[Int32, Field(ge=0)]
    """Resources read."""
    assets: Annotated[Int32, Field(ge=0)]
    """Assets read."""
    placements: Annotated[Int32, Field(ge=0)]
    """Asset placements read."""
    locations: Annotated[Int32, Field(ge=0)]
    """Storage locations read."""
    quarantined_cards: Annotated[Int32, Field(ge=0)]
    """Cards left out because their file is missing; each is logged beside the database."""
    backup_path: str | None
    """Where the old database was kept; null when there was none to keep, or on a dry run."""
