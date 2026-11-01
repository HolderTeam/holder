"""Public maintenance operations."""

from typing import Annotated

from fastapi import Body, Path

from authoring import Contract

from ..cards.models import ProjectId
from ..errors import DataCorrupted, KeyMissing, KeyringUnavailable, NotFound, RebuildFailed, RecoveryTokenInvalid
from ..maintenance import HierarchyRepair, HierarchyRepairRequest, ProjectEncryptionCheck
from ..rebuild import RebuildRequest, RebuildResult, ReindexResult
from ..recovery import (
    ProjectRecoveryTokenExportResult,
    ProjectRecoveryTokenImportResult,
    RecoveryTokenExportRequest,
    RecoveryTokenGlobalImportResult,
    RecoveryTokenImportRequest,
)
from .common import KEYS


def register(contract: Contract) -> None:
    @contract.route("GET", "/projects/{project_id}/encryption-check", errors=[NotFound])
    def check_project_encryption(project_id: Annotated[ProjectId, Path()]) -> ProjectEncryptionCheck:
        """Check whether an encrypted project's card files are safe to push.

        A plain project needs no check and reports a successful result with zero files.
        """


    @contract.route("POST", "/projects/{project_id}/repair-hierarchy", errors=[NotFound, *KEYS])
    def repair_project_hierarchy(
        project_id: Annotated[ProjectId, Path()], request: Annotated[HierarchyRepairRequest, Body()],
    ) -> HierarchyRepair:
        """Find hidden live card branches, and optionally move them into the reachable tree.

        An empty body plans without writing. Applying moves each hidden branch to its nearest reachable
        ancestor in one commit; a second apply changes nothing. Parent cycles are reported and left alone.
        """


    @contract.route("POST", "/projects/{project_id}/recovery-token/export",
                    errors=[NotFound, KeyringUnavailable, KeyMissing, DataCorrupted])
    def export_project_recovery_token(
        project_id: Annotated[ProjectId, Path()], request: Annotated[RecoveryTokenExportRequest, Body()],
    ) -> ProjectRecoveryTokenExportResult:
        """Wrap an encrypted project's key in a portable token protected by a PIN."""


    @contract.route("POST", "/projects/{project_id}/recovery-token/import",
                    errors=[NotFound, RecoveryTokenInvalid, KeyringUnavailable, DataCorrupted])
    def import_project_recovery_token(
        project_id: Annotated[ProjectId, Path()], request: Annotated[RecoveryTokenImportRequest, Body()],
    ) -> ProjectRecoveryTokenImportResult:
        """Restore an encrypted project's key from a token for that project."""


    @contract.route("POST", "/recovery-token/import",
                    errors=[RecoveryTokenInvalid, KeyringUnavailable, DataCorrupted],
                    created_when="project_created")
    def import_recovery_token_global(request: Annotated[RecoveryTokenImportRequest, Body()]) -> RecoveryTokenGlobalImportResult:
        """Restore a token, finding its project here or making one from the token's metadata.

        If the token names a Git remote, configure it and attempt a pull. Remote and pull failures
        are reported in the result. Returns 201 when the project was created, otherwise 200.
        """


    @contract.route("POST", "/rebuild", errors=[NotFound, RebuildFailed])
    def rebuild_project(request: Annotated[RebuildRequest, Body()]) -> RebuildResult:
        """Rebuild a project's database state from its files.

        A missing project is NotFound. Files that cannot be read back give RebuildFailed.
        """


    @contract.route("POST", "/reindex")
    def reindex_search() -> ReindexResult:
        """Rebuild all search indexes from database state."""
