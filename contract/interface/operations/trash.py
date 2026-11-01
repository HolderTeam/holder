"""Public trash operations."""

from typing import Annotated

from fastapi import Path, Query

from authoring import Contract

from ..cards import ProjectId
from ..errors import DataCorrupted, KeyMissing, KeyringUnavailable, NotFound, NotTrashed
from ..trash import TrashDeleteResult, TrashEmptyResult, TrashFilter, TrashItem, TrashItemType


def register(contract: Contract) -> None:
    @contract.route("GET", "/trash")
    def list_trash(
        project_id: Annotated[ProjectId, Query(description="The project whose Trash is listed.")],
        type: Annotated[str | None, Query(description="card, ai_message, or all; an unknown type lists nothing.")] = None,
    ) -> list[TrashItem]:
        """List a project's trashed cards and AI messages.

        A project that is not registered has an empty Trash.
        """


    @contract.route("DELETE", "/trash", errors=[KeyringUnavailable, KeyMissing, DataCorrupted])
    def empty_trash(
        project_id: Annotated[ProjectId, Query(description="The project whose Trash is emptied.")],
        type: Annotated[TrashFilter, Query(description="Which item kinds to remove.")] = "all",
    ) -> TrashEmptyResult:
        """Permanently remove a project's trashed items.

        An empty or unregistered project succeeds without changes. Old Git revisions of cards remain available.
        """


    @contract.route("DELETE", "/trash/{type}/{id}",
                    errors=[NotFound, NotTrashed, KeyringUnavailable, KeyMissing, DataCorrupted])
    def delete_trash_item(
        type: Annotated[TrashItemType, Path()],
        id: Annotated[str, Path()],
    ) -> TrashDeleteResult:
        """Permanently remove a trashed item by its global ID (legacy route)."""


    @contract.route("DELETE", "/projects/{project_id}/trash/{type}/{id}",
                    errors=[NotFound, NotTrashed, KeyringUnavailable, KeyMissing, DataCorrupted])
    def delete_project_trash_item(
        project_id: Annotated[ProjectId, Path()],
        type: Annotated[TrashItemType, Path()],
        id: Annotated[str, Path()],
    ) -> TrashDeleteResult:
        """Permanently remove a trashed item from the named project.

        Old Git revisions of cards remain available.
        """
