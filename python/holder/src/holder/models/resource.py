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
    NotFoundError,
    ProtocolError,
)
from ..generated.api.default import (
    delete_cards_card_id_resources,
    delete_resources_resource_id,
    get_resources,
    get_resources_resource_id,
    get_resources_resource_id_assets_asset_id_content,
    patch_resources_resource_id,
    post_cards_card_id_resources,
    post_resources,
)
from ..generated.models import (
    DeleteCardsCardIdResourcesBody,
    PostCardsCardIdResourcesBody,
    ResourceCreateRequest,
    ResourceMetadata,
    ResourceUpdateRequest,
)
from ..generated.streams import StreamError
from ..generated.types import UNSET, Unset

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


@dataclass(frozen=True)
class ResourceReference:
    """A live card referring to a resource, including attachment links."""

    card_id: str
    title: str
    updated_at: int
    link_kinds: tuple[str, ...]


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
                    with download, tempfile.NamedTemporaryFile(
                        mode="wb",
                        dir=destination.parent,
                        prefix=f".{destination.name}.",
                        delete=False,
                    ) as output:
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
    referenced_by_cards: tuple[ResourceReference, ...]
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
                            str(p.encoding),
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
            referenced_by_cards=tuple(
                ResourceReference(
                    reference.card_id,
                    reference.title,
                    reference.updated_at,
                    tuple(reference.link_kinds),
                )
                for reference in row.referenced_by_cards
            ),
            created_at=row.created_at,
            updated_at=row.updated_at,
            _project=project,
        )

    def refresh(self) -> Self:
        fresh = self._project.resources.get(self.id)
        for item in fields(self):
            if item.name != "_project":
                object.__setattr__(self, item.name, getattr(fresh, item.name))
        return self

    def update(
        self,
        *,
        type: str | Unset = UNSET,
        label: str | Unset = UNSET,
        metadata: dict[str, list[str]] | Unset = UNSET,
    ) -> Self:
        """Patch supplied fields; empty metadata lists remove those keys."""
        if all(isinstance(x, Unset) for x in (type, label, metadata)):
            raise TypeError("Supply type, label, or metadata")
        for name, value in (("type", type), ("label", label)):
            if not isinstance(value, (str, Unset)):
                raise TypeError(f"{name} must be a string")
        values = UNSET if isinstance(metadata, Unset) else _metadata(metadata)
        self._project.resources.get(
            self.id
        )  # Verify project membership before writing.
        self._project._holder._call(
            patch_resources_resource_id,
            self.id,
            body=ResourceUpdateRequest(type_=type, label=label, metadata=values),
            write=True,
        )
        return self.refresh()

    def delete(self) -> None:
        """Remove the resource and its card links; stored file bytes may remain."""
        self._project.resources.get(self.id)
        self._project._holder._call(delete_resources_resource_id, self.id, write=True)


class ResourceCollection(Collection[Resource]):
    def __init__(self, project: Project) -> None:
        self._project = project
        super().__init__(project._holder)

    def _clone(self, query: dict[str, Any]) -> Self:
        return type(self)(self._project)

    def _fetch_page(self) -> tuple[list[Resource], None]:
        data = self._holder._call(
            get_resources, project_id=self._project.id, limit=UNSET, offset=UNSET
        )
        return [Resource._from(self._project, row) for row in data], None

    def get(self, resource_id: str) -> Resource:
        row = self._holder._call(get_resources_resource_id, resource_id)
        if row.project_id != self._project.id:
            raise NotFoundError(404, "not_found", "Resource not found in this project")
        return Resource._from(self._project, row)

    def create(
        self,
        *,
        type: str,
        label: str,
        metadata: dict[str, list[str]] | None = None,
    ) -> Resource:
        if not isinstance(type, str) or not type or not isinstance(label, str):
            raise ValueError("type and label must be nonempty strings")
        row = self._holder._call(
            post_resources,
            body=ResourceCreateRequest(
                project_id=self._project.id,
                type_=type,
                label=label,
                metadata=_metadata(metadata) if metadata is not None else UNSET,
            ),
            write=True,
        )
        return Resource._from(self._project, row)


class CardAttachmentCollection(Collection[Resource]):
    def __init__(self, card: Card) -> None:
        self._card = card
        super().__init__(card._project._holder)

    def _clone(self, query: dict[str, Any]) -> Self:
        return type(self)(self._card)

    def _fetch_page(self) -> tuple[list[Resource], int | None]:
        offset = int(self._next or 0)
        envelope = self._holder._call(
            get_resources,
            project_id=self._card.project_id,
            card_id=self._card.id,
            limit=100,
            offset=offset,
            return_envelope=True,
        )
        if envelope.card_id != self._card.id:
            raise ProtocolError("Attachment listing returned another card")
        rows = [Resource._from(self._card._project, row) for row in envelope.data]
        next_offset = envelope.next_offset
        if isinstance(next_offset, Unset) or (
            next_offset is not None and next_offset <= offset
        ):
            raise ProtocolError("Attachment pagination did not advance")
        return rows, next_offset

    def _id(self, resource: Resource | str) -> str:
        if isinstance(resource, Resource):
            if resource.project_id != self._card.project_id:
                raise ValueError("Resource belongs to another project")
            return resource.id
        if not isinstance(resource, str) or not resource:
            raise ValueError("resource must be a Resource or nonempty ID")
        return resource

    def attach(self, resource: Resource | str) -> bool:
        """Attach a project resource; return whether a link was created."""
        row = self._holder._call(
            post_cards_card_id_resources,
            self._card.id,
            body=PostCardsCardIdResourcesBody(
                project_id=self._card.project_id, resource_id=self._id(resource)
            ),
            write=True,
        )
        self.refresh()
        return row.changed

    def detach(self, resource: Resource | str) -> bool:
        """Detach a resource without deleting it; return whether a link existed."""
        row = self._holder._call(
            delete_cards_card_id_resources,
            self._card.id,
            body=DeleteCardsCardIdResourcesBody(
                project_id=self._card.project_id, resource_id=self._id(resource)
            ),
            write=True,
        )
        self.refresh()
        return row.changed
