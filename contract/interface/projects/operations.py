"""Public projects operations."""

from typing import Annotated, Literal

from fastapi import Body, Path, Query

from authoring import Contract, Field, Int32, Timestamp

from ..errors import Conflict, NotFound
from ..operations.common import KEYS
from .ids import ProjectId
from .models import NewProject, Project, ProjectListItem, ProjectUpdate


def register(contract: Contract) -> None:
    @contract.route("GET", "/projects", errors=[])
    def list_projects(
        name: Annotated[Annotated[str, Field(min_length=1)] | None, Query(
            description="Only projects whose name contains this text.")] = None,
        updated_after: Annotated[Timestamp | None, Query(
            description="Only projects last changed at or after this instant.")] = None,
        updated_before: Annotated[Timestamp | None, Query(
            description="Only projects last changed at or before this instant.")] = None,
        order: Annotated[Literal["updated_at_desc", "updated_at_asc", "created_at_desc", "created_at_asc", "name_desc",
                                 "name_asc"] | None, Query(
            description="Omit for Home first, then the latest changed.")] = None,
        limit: Annotated[Int32, Query(ge=1, le=1000)] = 100,
        offset: Annotated[Int32, Query(ge=0, description="How many of the ordered projects to skip.")] = 0,
        include_card_counts: Annotated[bool, Query(description="Include each project's counts of live cards.")] = False,
    ) -> list[ProjectListItem]:
        """List the projects on this device.

        Filters combine. Projects that tie in the order keep the order they were stored in.
        """


    @contract.route("GET", "/projects/{project_id}", errors=[NotFound])
    def get_project(project_id: Annotated[ProjectId, Path()]) -> Project:
        """Fetch a project."""


    @contract.route("POST", "/projects", status=201, errors=[Conflict, *KEYS])
    def create_project(project: Annotated[NewProject, Body()]) -> Project:
        """Make a project.

        Its repository is made, with its settings, and an encrypted project's key. An ID a project on this device has
        is a Conflict. Making a project that was removed from this device makes it known here again.
        """


    @contract.route("PATCH", "/projects/{project_id}", errors=[NotFound, *KEYS])
    def update_project(project_id: Annotated[ProjectId, Path()], update: Annotated[ProjectUpdate, Body()]) -> Project:
        """Change a project's settings.

        Each field left out keeps its value, and an update that changes nothing writes nothing. The settings are
        committed to the project's repository; a Git remote is set there before it is stored, and one Git does not
        accept is an invalid argument.
        """


    @contract.route("DELETE", "/projects/{project_id}", errors=[NotFound])
    def delete_project(project_id: Annotated[ProjectId, Path()]) -> Project:
        """Remove a project from this device.

        The project's files and keys are kept, and its removal is remembered, so that looking for projects does not
        bring it back; making it again does. Returns the project as it was.
        """
