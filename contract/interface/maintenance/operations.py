"""Maintenance operations."""

from typing import Annotated

from fastapi import Body, Path

from authoring import Contract

from ..errors import DataCorrupted, KeyMissing, KeyringUnavailable, NotFound
from ..projects.ids import ProjectId
from .errors import RebuildFailed
from .models import HierarchyRepair, HierarchyRepairRequest, ProjectEncryptionCheck
from .rebuild import RebuildRequest, RebuildResult, ReindexResult


def register(contract: Contract) -> None:
    @contract.route("GET", "/projects/{project_id}/encryption-check", errors=[NotFound])
    def check_project_encryption(project_id: Annotated[ProjectId, Path()]) -> ProjectEncryptionCheck:
        """Check whether an encrypted project's card files are safe to push.

        A plain project needs no check and reports a successful result with zero files.
        """


    @contract.route("POST", "/projects/{project_id}/repair-hierarchy",
                    errors=[NotFound, KeyringUnavailable, KeyMissing, DataCorrupted])
    def repair_project_hierarchy(
        project_id: Annotated[ProjectId, Path()], request: Annotated[HierarchyRepairRequest, Body()],
    ) -> HierarchyRepair:
        """Find hidden live card branches, and optionally move them into the reachable tree.

        An empty body plans without writing. Applying moves each hidden branch to its nearest reachable
        ancestor in one commit; a second apply changes nothing. Parent cycles are reported and left alone.
        """


    @contract.route("POST", "/rebuild", errors=[NotFound, RebuildFailed])
    def rebuild_project(request: Annotated[RebuildRequest, Body()]) -> RebuildResult:
        """Rebuild a project's database state from its files.

        A missing project is NotFound. Files that cannot be read back give RebuildFailed.
        """


    @contract.route("POST", "/reindex")
    def reindex_search() -> ReindexResult:
        """Rebuild all search indexes from database state."""
