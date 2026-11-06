"""Daemon-local file imports and their transient jobs."""

from __future__ import annotations

import os
import time
from dataclasses import dataclass, field, fields
from typing import TYPE_CHECKING, Any, Self

from ..generated.api.default import get_imports_job_id, post_imports
from ..generated.models import AssetImportRequest

if TYPE_CHECKING:
    from .card import Card
    from .project import Project
    from .resource import Resource
    from .storage import StorageLocation


@dataclass(frozen=True, eq=False)
class ImportJob:
    id: str
    status: str
    resource_id: str | None
    asset_id: str | None
    duplicate_reused: bool
    link_created: bool
    error: str | None
    _project: Project = field(repr=False, compare=False)

    @classmethod
    def _from(cls, project: Project, row: Any) -> Self:
        return cls(
            id=row.job_id,
            status=str(row.status),
            resource_id=row.resource_id,
            asset_id=row.asset_id,
            duplicate_reused=row.duplicate_reused,
            link_created=row.link_created,
            error=row.error,
            _project=project,
        )

    def refresh(self) -> ImportJob:
        """Fetch current state; daemon restarts can make a job ID unavailable."""
        row = self._project._holder._call(get_imports_job_id, self.id)
        fresh = ImportJob._from(self._project, row)
        for item in fields(self):
            if item.name != "_project":
                object.__setattr__(self, item.name, getattr(fresh, item.name))
        return self

    def wait(self, *, timeout: float = 300, interval: float = 0.25) -> ImportJob:
        """Poll until completed or failed; raise TimeoutError at the deadline."""
        if timeout < 0 or interval <= 0:
            raise ValueError("timeout must be nonnegative and interval positive")
        deadline = time.monotonic() + timeout
        current = self
        while current.status not in ("completed", "failed"):
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                raise TimeoutError(f"Import job {self.id} did not finish in time")
            time.sleep(min(interval, remaining))
            current = current.refresh()
        return current

    @property
    def resource(self) -> Resource | None:
        if self.status != "completed" or self.resource_id is None:
            return None
        return self._project.resources.get(self.resource_id)


def start_import(
    card: Card,
    source: str | os.PathLike[str],
    location: StorageLocation | None,
) -> ImportJob:
    """Start importing a file already readable by the daemon host."""
    if location is None:
        preferred_id = card._project.storage_locations.preferred_id
        if preferred_id is None:
            raise ValueError("Choose a storage location or set a preferred location")
        location = card._project.storage_locations.get(preferred_id)
    from .storage import StorageLocation

    if not isinstance(location, StorageLocation):
        raise TypeError("location must be a StorageLocation")
    if location.project_id != card.project_id:
        raise ValueError("Storage location belongs to another project")
    path = os.fspath(source)
    if not isinstance(path, str) or not path:
        raise ValueError("source must be a nonempty text path")
    row = card._project._holder._call(
        post_imports,
        body=AssetImportRequest(
            project_id=card.project_id,
            card_id=card.id,
            location_id=location.id,
            source_path=path,
        ),
        write=True,
    )
    return ImportJob._from(card._project, row)
