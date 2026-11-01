"""Resources operations."""

from typing import Annotated

from fastapi import Body, Path, Query

from authoring import Bytes, Contract
from authoring.routes import ErrorResponse

from ..cards.models import CardId
from ..errors import Conflict, DataCorrupted, KeyMissing, KeyringUnavailable, NotFound
from ..links.models import Link
from ..projects.ids import ProjectId
from .models import AssetImportJob, AssetImportRequest, NewResource, Resource, ResourceId, ResourceUpdate


def register(contract: Contract) -> None:
    @contract.route("GET", "/projects/{project_id}/resources", errors=[NotFound])
    def list_resources(project_id: Annotated[ProjectId, Path()]) -> list[Resource]:
        """List a project's resources.

        By when they last changed, latest first, then by ID.
        """


    @contract.route("GET", "/projects/{project_id}/resources/{resource_id}", errors=[NotFound])
    def get_resource(project_id: Annotated[ProjectId, Path()], resource_id: Annotated[ResourceId, Path()]) -> Resource:
        """Fetch a resource.

        A resource in another project is not found.
        """


    @contract.route("POST", "/projects/{project_id}/resources", status=201,
                    errors=[NotFound, Conflict, KeyringUnavailable, KeyMissing, DataCorrupted])
    def create_resource(project_id: Annotated[ProjectId, Path()], resource: Annotated[NewResource, Body()]) -> Resource:
        """Make a resource.

        A project that does not exist is not found. Resource IDs are unique on this device, so an ID already in use,
        in this project or another, is a Conflict.
        """


    @contract.route("PATCH", "/projects/{project_id}/resources/{resource_id}",
                    errors=[NotFound, KeyringUnavailable, KeyMissing, DataCorrupted])
    def update_resource(
        project_id: Annotated[ProjectId, Path()],
        resource_id: Annotated[ResourceId, Path()],
        update: Annotated[ResourceUpdate, Body()],
    ) -> Resource:
        """Change a resource.

        Each field left out keeps its value. A resource in another project is not found.
        """


    @contract.route("DELETE", "/projects/{project_id}/resources/{resource_id}",
                    errors=[NotFound, KeyringUnavailable, KeyMissing, DataCorrupted])
    def delete_resource(project_id: Annotated[ProjectId, Path()], resource_id: Annotated[ResourceId, Path()]) -> Resource:
        """Delete a resource.

        The resource goes, and so do the links to it from cards. Its files' stored copies are left where they are.
        Returns the resource as it was. A resource in another project is not found.
        """


    @contract.route("GET", "/projects/{project_id}/resources/{resource_id}/backlinks", errors=[NotFound])
    def list_resource_backlinks(
        project_id: Annotated[ProjectId, Path()],
        resource_id: Annotated[ResourceId, Path()],
        include_deleted: Annotated[bool, Query(description="Include links from cards and AI messages in the trash.")]
        = False,
    ) -> list[Link]:
        """List the links to a resource.

        Links of every kind, attachments among them, from cards and AI messages in the resource's project, newest
        first. A resource in another project is not found.
        """


    @contract.route("GET", "/projects/{project_id}/cards/{card_id}/resources", errors=[NotFound])
    def list_card_resources(project_id: Annotated[ProjectId, Path()], card_id: Annotated[CardId, Path()]) -> list[Resource]:
        """List the resources attached to a card.

        By when they last changed, latest first, then by ID. A card in the trash or in another project is not found.
        """


    @contract.route("PUT", "/projects/{project_id}/cards/{card_id}/resources/{resource_id}",
                    errors=[NotFound, KeyringUnavailable, KeyMissing, DataCorrupted])
    def attach_card_resource(
        project_id: Annotated[ProjectId, Path()],
        card_id: Annotated[CardId, Path()],
        resource_id: Annotated[ResourceId, Path()],
    ) -> list[Resource]:
        """Attach a resource to a card.

        An attachment is the card's link of kind attachment to the resource. A card that has it already is left as it
        is. Returns the card's attached resources, as list_card_resources lists them. A card in the trash or in another
        project is not found, and neither is a resource that is not in the card's project.
        """


    @contract.route("DELETE", "/projects/{project_id}/cards/{card_id}/resources/{resource_id}",
                    errors=[NotFound, KeyringUnavailable, KeyMissing, DataCorrupted])
    def detach_card_resource(
        project_id: Annotated[ProjectId, Path()],
        card_id: Annotated[CardId, Path()],
        resource_id: Annotated[ResourceId, Path()],
    ) -> list[Resource]:
        """Detach a resource from a card.

        The resource itself is kept. A card without the attachment is left as it is, and a resource that no longer
        exists can still be detached. Returns the card's attached resources, as list_card_resources lists them. A card
        in the trash or in another project is not found.
        """


    @contract.route("POST", "/imports", status=202, core=False, responses={
        404: {"model": ErrorResponse, "description": "Storage location not found in the project."},
        409: {"model": ErrorResponse, "description": "Storage binding required on this device."},
        501: {"model": ErrorResponse, "description": "The daemon cannot import assets."},
        503: {"model": ErrorResponse, "description": "Secret store unavailable."},
    })
    def start_asset_import(request: Annotated[AssetImportRequest, Body()]) -> AssetImportJob:
        """Start a local-file asset import.

        The location and its credentials are checked before the job is queued. Poll the job for later failures.
        """


    @contract.route("GET", "/imports/{job_id}", core=False, responses={
        404: {"model": ErrorResponse, "description": "Job not found; jobs are held in daemon memory."},
    })
    def get_asset_import_job(job_id: Annotated[str, Path()]) -> AssetImportJob:
        """Poll an asset import job.

        A job queued before the daemon restarted is not found.
        """


    @contract.route("GET", "/resources/{resource_id}/assets/{asset_id}/content", core=False, responses={
        200: {"headers": {
            "Content-Type": {"description": "Stored asset media type.", "schema": {"type": "string"}},
            "Content-Disposition": {"description": "Attachment with sanitized filename.",
                                    "schema": {"type": "string"}},
        }},
        404: {"model": ErrorResponse, "description": "Resource, asset, or placement not found."},
        409: {"model": ErrorResponse, "description": "Storage binding required or storage conflict."},
        422: {"model": ErrorResponse, "description": "Asset integrity verification failed."},
        502: {"model": ErrorResponse, "description": "Storage authentication or permission failure."},
        503: {"model": ErrorResponse, "description": "Storage unavailable or transient failure."},
        507: {"model": ErrorResponse, "description": "Storage capacity exceeded."},
    })
    def get_resource_asset_content(
        resource_id: Annotated[str, Path()], asset_id: Annotated[str, Path()],
        placement_id: Annotated[str | None, Query()] = None,
    ) -> Bytes:
        """Stream verified asset content.

        This authenticated download returns raw bytes. Content-Type preserves the asset media type and
        Content-Disposition contains a sanitized original filename. Without placement_id the daemon selects a
        stored placement.
        """
