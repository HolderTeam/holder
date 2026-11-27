"""Recovery operations."""

from typing import Annotated

from fastapi import Body, Path

from authoring import Bytes, Contract, Field, Int32
from authoring.routes import ErrorResponse

from ..errors import DataCorrupted, KeyMissing, KeyringUnavailable, NotFound
from ..projects.ids import ProjectId
from ..projects.models import Project
from .errors import RecoveryTokenInvalid
from .models import (
    ProjectRecoveryTokenExportResult,
    ProjectRecoveryTokenImportResult,
    RecoveryTokenExportRequest,
    RecoveryTokenGlobalImportResult,
    RecoveryTokenImportRequest,
)
from .snapshot import RecoverySnapshotPage, RecoverySnapshotRestore


def register(contract: Contract) -> None:
    @contract.core_operation(errors=[NotFound])
    def read_recovery_snapshot(project_id: ProjectId, cursor: str | None = None,
                               limit: Annotated[Int32, Field(ge=1, le=500)] = 500) -> RecoverySnapshotPage:
        """Read one page of a project's recovery snapshot: its live cards, most recently updated first.

        Each card comes as a record and as a line in core's encoding; a platform keeps the lines and
        later hands them to restore_recovery_snapshot. The daemon does not serve this.
        """

    @contract.core_operation()
    def restore_recovery_snapshot(request: RecoverySnapshotRestore) -> Project:
        """Make a new project from a recovery snapshot's lines and commit the restored cards to it.

        Cards, resources, assets, placements and storage locations get new IDs; links between the restored
        cards and resources, holder://resource/ references in content and the card tree follow them. Lines
        are restored in any order. A line core cannot read fails the restore and leaves no project behind.
        The daemon does not serve this.
        """

    @contract.core_operation(errors=[DataCorrupted])
    def restore_recovery_bundle(
        request: RecoverySnapshotRestore, asset_directory: Annotated[str, Field(min_length=1)]
    ) -> Project:
        """Restore a plain project with verified local copies of its assets.

        The caller extracts each plaintext payload as a file named by its SHA-256 in
        asset_directory. Core checks the bytes, copies them into the private workspace,
        and replaces source placements with local ones. A failed restore leaves no project.
        """

    @contract.route("GET", "/projects/{project_id}/recovery-snapshot", core=False,
                    errors=[NotFound, KeyringUnavailable, KeyMissing, DataCorrupted], responses={
        413: {"model": ErrorResponse, "description": "The recovery snapshot exceeds 2 MiB."},
    })
    def download_recovery_snapshot(project_id: Annotated[ProjectId, Path()]) -> Bytes:
        """Download a consistent recovery snapshot of the project's live cards.

        Returns UTF-8 recovery snapshot lines, one versioned JSON record per card, up to 2 MiB.
        An empty project returns zero bytes. The download includes readable Markdown bodies,
        including plaintext from encrypted projects. It includes linked resource declarations
        but omits asset bytes, credentials, Git history, trash, and AI threads and messages.
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
