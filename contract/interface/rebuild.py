"""Restore the local database projection and search indexes."""

from typing import Annotated

from authoring import Field, Int32, Record

from .cards.models import ProjectId


class RebuildRequest(Record):
    """The project whose files should be read into the database."""

    project_id: ProjectId
    """The project to rebuild."""


class RebuildResult(Record):
    """The numbers of entities restored from a project's files."""

    project_id: ProjectId
    """The project rebuilt."""
    cards: Annotated[Int32, Field(ge=0)]
    """Cards restored."""
    ai_messages: Annotated[Int32, Field(ge=0)]
    """AI messages restored."""
    ai_threads: Annotated[Int32, Field(ge=0)]
    """AI threads restored."""
    links: Annotated[Int32, Field(ge=0)]
    """Links restored."""


class ReindexResult(Record):
    """Completion of a full database search-index rebuild."""

    message: str
    """A short completion message."""
