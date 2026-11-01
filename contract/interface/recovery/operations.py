"""Recovery operations."""

from typing import Annotated

from fastapi import Body, Path

from authoring import Contract

from ..errors import DataCorrupted, KeyMissing, KeyringUnavailable, NotFound
from ..projects.ids import ProjectId
from .errors import RecoveryTokenInvalid
from .models import (
    ProjectRecoveryTokenExportResult,
    ProjectRecoveryTokenImportResult,
    RecoveryTokenExportRequest,
    RecoveryTokenGlobalImportResult,
    RecoveryTokenImportRequest,
)


def register(contract: Contract) -> None:
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
