"""Project resources, card attachments, and downloadable assets."""

from __future__ import annotations

import os
import tempfile
from dataclasses import dataclass, field, fields
from pathlib import Path
from typing import TYPE_CHECKING, Any, Self

import httpx

from .._streams import stream_failure
from ..collections import Collection
from ..exceptions import (
    AuthenticationError,
    ConnectionError,
    ProtocolError,
)
from ..generated.api.default import (
    delete_projects_project_id_cards_card_id_resources_resource_id,
    delete_projects_project_id_resources_resource_id,
    get_projects_project_id_cards_card_id_resources,
    get_projects_project_id_resources,
    get_projects_project_id_resources_resource_id,
    get_projects_project_id_resources_resource_id_backlinks,
    get_resources_resource_id_assets_asset_id_content,
    patch_projects_project_id_resources_resource_id,
    post_projects_project_id_resources,
    put_projects_project_id_cards_card_id_resources_resource_id,
)
from ..generated.models import NewResource, ResourceMetadata, ResourceUpdate
from ..generated.streams import StreamError
from ..generated.types import UNSET, Unset
from .link import CardLink

if TYPE_CHECKING:
    from .card import Card
    from .project import Project


def _metadata(values: dict[str, list[str]]) -> ResourceMetadata:
    if not isinstance(values, dict) or any(
        not isinstance(key, str)
        or not isinstance(items, list)
        or any(not isinstance(item, str) for item in items)
        for key, items in values.items()
    ):
        raise TypeError("metadata must map strings to lists of strings")
    if "" in values:
        raise ValueError("a metadata name must not be empty")
    result = ResourceMetadata()
    result.additional_properties = {key: list(items) for key, items in values.items()}
    return result


@dataclass(frozen=True)
class AssetPlacement:
    id: str
    location_id: str
    encoding: str
    stored_byte_size: int
    created_at: int


@dataclass(frozen=True, eq=False)
class Asset:
    id: str
    resource_id: str
    filename: str
    media_type: str
    byte_size: int
    sha256: str
    placements: tuple[AssetPlacement, ...]
    created_at: int
    updated_at: int
    _project: Project = field(repr=False, compare=False)

    def download_to(
        self, path: str | os.PathLike[str], *, placement: AssetPlacement | None = None
    ) -> Path:
        """Stream verified content to a file, replacing it only on success."""
        destination = Path(path)
        if placement is not None and placement not in self.placements:
            raise ValueError("placement must belong to this asset")
        holder = self._project._holder
        temporary: str | None = None
        try:
            for attempt in range(2):
                download = get_resources_resource_id_assets_asset_id_content.stream(
                    self.resource_id,
                    self.id,
                    client=holder._active(),
                    placement_id=placement.id if placement is not None else UNSET,
                )
                try:
                    with (
                        download,
                        tempfile.NamedTemporaryFile(
                            mode="wb",
                            dir=destination.parent,
                            prefix=f".{destination.name}.",
                            delete=False,
                        ) as output,
                    ):
                        temporary = output.name
                        download.write_to(output)
                except StreamError as error:
                    if error.status_code == 401 and holder._local and attempt == 0:
                        holder._discover()
                        # Preserve the pool and custom transport while updating a restarted daemon.
                        client = holder._active().get_httpx_client()
                        client.base_url = holder._url
                        client.headers["Authorization"] = f"Bearer {holder._token}"
                        continue
                    raise stream_failure(error, "Asset download failed") from None
                os.replace(temporary, destination)
                temporary = None
                return destination
        except httpx.TransportError:
            raise ConnectionError(
                "Could not download asset from the Holder daemon"
            ) from None
        finally:
            if temporary is not None:
                os.unlink(temporary)
        raise AuthenticationError(
            401, "unauthorized", "Daemon rejected refreshed credentials"
        )


@dataclass(frozen=True, eq=False)
class Resource:
    id: str
    project_id: str
    type: str
    label: str
    metadata: dict[str, list[str]]
    assets: tuple[Asset, ...]
    created_at: int
    updated_at: int
    _project: Project = field(repr=False, compare=False)

    @classmethod
    def _from(cls, project: Project, row: Any) -> Self:
        if row.project_id != project.id:
            raise ProtocolError("Resource belongs to another project")
        return cls(
            id=row.resource_id,
            project_id=row.project_id,
            type=row.type_,
            label=row.label,
            metadata={
                k: list(v) for k, v in row.metadata.additional_properties.items()
            },
            assets=tuple(
                Asset(
                    id=asset.asset_id,
                    resource_id=row.resource_id,
                    filename=asset.original_filename,
                    media_type=asset.media_type,
                    byte_size=asset.byte_size,
                    sha256=asset.plaintext_sha256,
                    placements=tuple(
                        AssetPlacement(
                            p.placement_id,
                            p.location_id,
                            p.encoding.value,
                            p.stored_byte_size,
                            p.created_at,
                        )
                        for p in asset.placements
                    ),
                    created_at=asset.created_at,
                    updated_at=asset.updated_at,
                    _project=project,
                )
                for asset in row.assets
            ),
            created_at=row.created_at,
            updated_at=row.updated_at,
            _project=project,
        )

    @property
    def backlinks(self) -> ResourceBacklinkCollection:
        """The links to this resource, of every kind, from its project's cards and AI messages."""
        return ResourceBacklinkCollection(self)

    def refresh(self) -> Self:
        self._replace(self._project.resources.get(self.id))
        return self

    def update(
        self,
        *,
        type: str | Unset = UNSET,
        label: str | Unset = UNSET,
        metadata: dict[str, list[str]] | Unset = UNSET,
    ) -> Self:
        """Change the fields given; in metadata, each name given replaces its values, and an empty list
        removes it."""
        if all(isinstance(x, Unset) for x in (type, label, metadata)):
            raise TypeError("Supply type, label, or metadata")
        for name, value in (("type", type), ("label", label)):
            if not isinstance(value, (str, Unset)):
                raise TypeError(f"{name} must be a string")
        values = UNSET if isinstance(metadata, Unset) else _metadata(metadata)
        row = self._project._holder._call(
            patch_projects_project_id_resources_resource_id,
            self.project_id,
            self.id,
            body=ResourceUpdate(type_=type, label=label, metadata=values),
            write=True,
        )
        self._replace(Resource._from(self._project, row))
        return self

    def _replace(self, fresh: Resource) -> None:
        for item in fields(self):
            if item.name != "_project":
                object.__setattr__(self, item.name, getattr(fresh, item.name))

    def delete(self) -> None:
        """Remove the resource and its card links; stored file bytes may remain."""
        self._project._holder._call(
            delete_projects_project_id_resources_resource_id,
            self.project_id,
            self.id,
            write=True,
        )


class ResourceCollection(Collection[Resource]):
    def __init__(self, project: Project) -> None:
        self._project = project
        super().__init__(project._holder)

    def _clone(self, query: dict[str, Any]) -> Self:
        return type(self)(self._project)

    def _fetch_page(self) -> tuple[list[Resource], None]:
        data = self._holder._call(get_projects_project_id_resources, self._project.id)
        return [Resource._from(self._project, row) for row in data], None

    def get(self, resource_id: str) -> Resource:
        """Fetch a resource; one in another project is not found."""
        row = self._holder._call(
            get_projects_project_id_resources_resource_id, self._project.id, resource_id
        )
        return Resource._from(self._project, row)

    def create(
        self,
        *,
        type: str,
        label: str,
        metadata: dict[str, list[str]] | None = None,
    ) -> Resource:
        if (
            not isinstance(type, str)
            or not type
            or not isinstance(label, str)
            or not label
        ):
            raise ValueError("type and label must be nonempty strings")
        row = self._holder._call(
            post_projects_project_id_resources,
            self._project.id,
            body=NewResource(
                type_=type,
                label=label,
                metadata=_metadata(metadata) if metadata is not None else UNSET,
            ),
            write=True,
        )
        return Resource._from(self._project, row)


class ResourceBacklinkCollection(Collection["CardLink"]):
    """The links to a resource, newest first; evaluated results stay cached until refresh()."""

    def __init__(self, resource: Resource, *, include_deleted: bool = False) -> None:
        self._resource = resource
        self._include_deleted = include_deleted
        super().__init__(resource._project._holder)

    def _clone(self, query: dict[str, Any]) -> Self:
        return type(self)(
            self._resource,
            include_deleted=query.get("include_deleted", self._include_deleted),
        )

    def including_deleted(self) -> Self:
        """Return a fresh query that includes links from cards and AI messages in the trash."""
        return self._clone({"include_deleted": True})

    def _fetch_page(self) -> tuple[list[CardLink], None]:
        data = self._holder._call(
            get_projects_project_id_resources_resource_id_backlinks,
            self._resource.project_id,
            self._resource.id,
            include_deleted=self._include_deleted,
        )
        return [CardLink._from(row) for row in data], None


class CardAttachmentCollection(Collection[Resource]):
    """The resources attached to a card, by last change; cached until refresh()."""

    def __init__(self, card: Card) -> None:
        self._card = card
        super().__init__(card._project._holder)

    def _clone(self, query: dict[str, Any]) -> Self:
        return type(self)(self._card)

    def _fetch_page(self) -> tuple[list[Resource], None]:
        data = self._holder._call(
            get_projects_project_id_cards_card_id_resources,
            self._card.project_id,
            self._card.id,
        )
        return [Resource._from(self._card._project, row) for row in data], None

    def _id(self, resource: Resource | str) -> str:
        if isinstance(resource, Resource):
            if resource.project_id != self._card.project_id:
                raise ValueError("Resource belongs to another project")
            return resource.id
        if not isinstance(resource, str) or not resource:
            raise ValueError("resource must be a Resource or nonempty ID")
        return resource

    def attach(self, resource: Resource | str) -> None:
        """Attach a project resource; attaching one already attached changes nothing."""
        self._holder._call(
            put_projects_project_id_cards_card_id_resources_resource_id,
            self._card.project_id,
            self._card.id,
            self._id(resource),
            write=True,
        )
        self.refresh()

    def detach(self, resource: Resource | str) -> None:
        """Detach a resource without deleting it; detaching one not attached changes nothing."""
        self._holder._call(
            delete_projects_project_id_cards_card_id_resources_resource_id,
            self._card.project_id,
            self._card.id,
            self._id(resource),
            write=True,
        )
        self.refresh()
