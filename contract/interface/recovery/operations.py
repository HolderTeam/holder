"""Recovery operations."""

from typing import Annotated

from fastapi import Body, Path, Query

from authoring import Bytes, Contract, Field, Int32
from authoring.routes import ErrorResponse

from ..errors import DataCorrupted, KeyMissing, KeyringUnavailable, NotFound
from ..projects.ids import ProjectId
from ..projects.models import Project, ProjectName
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

    @contract.core_operation(errors=[DataCorrupted])
    def restore_recovery_snapshot_file(
        snapshot_path: Annotated[str, Field(min_length=1)], project_name: ProjectName,
        asset_directory: Annotated[str, Field(min_length=1)] | None = None,
    ) -> Project:
        """Restore a local recovery JSON Lines file into a new unencrypted project.

        Uses the same ID remapping as restore_recovery_snapshot. The caller keeps the
        file unchanged for the duration of the call and owns its cleanup. Supply an
        asset directory with SHA-256-named plaintext files to verify and restore
        local copies of the assets in a private workspace.
        """

    @contract.route("GET", "/projects/{project_id}/recovery-snapshot", core=False,
                    errors=[NotFound, KeyringUnavailable, KeyMissing, DataCorrupted], responses={
        413: {"model": ErrorResponse, "description": "The recovery snapshot exceeds 2 MiB."},
    })
    def download_recovery_snapshot(
        project_id: Annotated[ProjectId, Path()], stream: Annotated[bool, Query()] = False,
    ) -> Bytes:
        """Download a consistent recovery snapshot of the project's live cards.

        Returns UTF-8 recovery snapshot lines, one versioned JSON record per card. The default
        response is capped at 2 MiB. With stream=true, the daemon captures to a temporary file
        before transferring it, and the total size is limited by available storage.
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
