"""Project identifiers shared by domain models and operations."""

from typing import Annotated

from authoring import Field

type ProjectId = Annotated[str, Field(min_length=1)]
"""A project's ID."""
