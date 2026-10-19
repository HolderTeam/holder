"""Projects and their synchronous collection queries."""

from __future__ import annotations

import os
import time
from dataclasses import dataclass, field, fields
from functools import cached_property
from typing import TYPE_CHECKING, Any, Literal, Self, cast

from ..changes import Changes
from ..collections import Collection
from ..exceptions import ProtocolError
from ..generated.api.default import (
    delete_projects_project_id,
    get_projects,
    get_projects_project_id,
    get_projects_project_id_encryption_check,
    patch_projects_project_id,
    post_projects,
    post_projects_project_id_recovery_token_export,
    post_projects_project_id_recovery_token_import,
    post_projects_project_id_repair_hierarchy,
    post_recovery_token_import,
)
from ..generated.models import (
    GetProjectsOrder,
    HierarchyRepairRequest,
    ProjectCreateRequest,
    ProjectCreateRequestPrivacyMode,
    ProjectPrivacyMode,
    ProjectRecoveryTokenExportRequest,
    ProjectRecoveryTokenImportRequest,
    ProjectSync,
    ProjectUpdateRequest,
)
from ..generated.types import UNSET, Unset
from .card import CardCollection
from .history import ProjectActivityCollection
from .milestone import ProjectMilestones
from .project_git import ProjectGit
from .resource import ResourceCollection
from .storage import StorageLocationCollection
from .tag import TagCollection
from .trash import TrashCollection

if TYPE_CHECKING:
    from ..client import Holder


class ProjectCollection(Collection["Project"]):
    def recover(self, token: str, *, pin: str) -> RecoveryResult:
        """Recover an encrypted project from a token, matching or creating it.

        Remote configuration and pull can fail after the project is recovered;
        inspect the returned result for those outcomes.
        """
        _check_recovery_inputs(token, pin)
        data = self._holder._call(
            post_recovery_token_import,
            body=ProjectRecoveryTokenImportRequest(pin=pin, recovery_token=token),
            write=True,
        )
        return RecoveryResult(
            project=self.get(data.project_id),
            project_created=data.project_created,
            remote_hint_present=data.remote_hint_present,
            remote_configured=data.remote_configured,
            remote_error=_optional(data.remote_error),
            pull_status=str(data.pull_status),
            pull_error=_optional(data.pull_error),
        )

    def create(
        self,
        *,
        name: str,
        root_path: str | os.PathLike[str] | Unset = UNSET,
        privacy_mode: Literal["encrypted_git", "plain"] | Unset = UNSET,
    ) -> Project:
        """Create a project using daemon defaults for omitted settings.

        Listing filters do not supply creation values. Cached listings remain
        unchanged until refreshed. The write is never retried; if the subsequent
        fetch fails, the project may already exist.
        """
        if not isinstance(name, str):
            raise TypeError("name must be a string")
        if not isinstance(root_path, Unset):
            root_path = os.fspath(root_path)
            if not isinstance(root_path, str):
                raise TypeError("root_path must be a string or a text path")
        mode = (
            UNSET
            if isinstance(privacy_mode, Unset)
            else ProjectCreateRequestPrivacyMode(privacy_mode)
        )
        data = self._holder._call(
            post_projects,
            body=ProjectCreateRequest(
                name=name, root_path=root_path, privacy_mode=mode
            ),
            write=True,
        )
        return self.get(data.project_id)

    def _clone(self, query: dict[str, Any]) -> Self:
        return type(self)(self._holder, query)

    def filter(
        self,
        *,
        name: str | Unset = UNSET,
        updated_after: int | Unset = UNSET,
        updated_before: int | Unset = UNSET,
    ) -> Self:
        values = {
            key: value
            for key, value in {
                "name": name,
                "updated_after": updated_after,
                "updated_before": updated_before,
            }.items()
            if not isinstance(value, Unset)
        }
        if "name" in values and not isinstance(name, str):
            raise TypeError("name must be a string")
        for key in ("updated_after", "updated_before"):
            if key in values and (
                not isinstance(values[key], int) or isinstance(values[key], bool)
            ):
                raise TypeError(f"{key} must be an integer")
        return self._clone({**self._query, **values})

    def order_by(
        self,
        field: Literal["name", "created_at", "updated_at"],
        *,
        descending: bool = False,
    ) -> Self:
        """List projects by name, creation time, or last update time."""
        if field not in ("name", "created_at", "updated_at"):
            raise ValueError("field must be name, created_at, or updated_at")
        if not isinstance(descending, bool):
            raise TypeError("descending must be a bool")
        direction = "desc" if descending else "asc"
        return self._clone(
            {**self._query, "order": GetProjectsOrder(f"{field}_{direction}")}
        )

    def alphabetical(self) -> Self:
        """List projects by name, A to Z."""
        return self.order_by("name")

    def recent(self) -> Self:
        """List the most recently updated projects first."""
        return self.order_by("updated_at", descending=True)

    def with_card_counts(self) -> Self:
        """Include each project's live card and root card counts."""
        return self._clone({**self._query, "count": True})

    def _fetch_page(self) -> tuple[list[Project], int | None]:
        offset = cast(int, self._next or 0)
        data = self._holder._call(get_projects, limit=100, offset=offset, **self._query)
        if self._query.get("count") and any(
            isinstance(item.card_count, Unset)
            or isinstance(item.root_card_count, Unset)
            for item in data
        ):
            raise ProtocolError("Project listing omitted requested card counts")
        rows = [Project._from(self._holder, item) for item in data]
        next_offset = offset + len(data) if len(data) == 100 else None
        return rows, next_offset

    def get(self, project_id: str) -> Project:
        project = Project._from(
            self._holder, self._holder._call(get_projects_project_id, project_id)
        )
        # get(id) is a direct scoped lookup, not evaluation of a filtered listing.
        return project


@dataclass(frozen=True)
class RepairedCard:
    """A hidden branch's top card and where repair moves it (None: top level)."""

    card_id: str
    parent_card_id: str | None


@dataclass(frozen=True)
class HierarchyRepair:
    """Cards hidden under missing or trashed parents, and what repair does."""

    applied: bool
    moved: list[RepairedCard]
    changed_card_count: int
    cycle_card_ids: list[str]


def _optional(value: Any) -> Any:
    return None if isinstance(value, Unset) else value


def _check_recovery_inputs(token: str, pin: str) -> None:
    if not isinstance(token, str) or not token.strip():
        raise ValueError("token must be a nonempty string")
    if not isinstance(pin, str) or not pin:
        raise ValueError("pin must be a nonempty string")


@dataclass(frozen=True)
class RecoveryResult:
    project: Project
    project_created: bool
    remote_hint_present: bool
    remote_configured: bool
    remote_error: str | None
    pull_status: str
    pull_error: str | None


@dataclass(frozen=True)
class EncryptionCheck:
    privacy_mode: str
    ok: bool
    checked_files: int
    unsafe_files: int
    unsafe_paths: tuple[str, ...]
    message: str


@dataclass(frozen=True, eq=False)
class Project:
    project_id: str
    name: str
    root_path: str
    privacy_mode: ProjectPrivacyMode
    created_at: int
    updated_at: int
    sync: ProjectSync
    git_remote_url: str | None
    git_provider: str | None
    project_key_id: str | None
    card_count: int | None
    root_card_count: int | None
    _holder: Holder = field(repr=False, compare=False)

    @property
    def id(self) -> str:
        return self.project_id

    @cached_property
    def cards(self) -> CardCollection:
        return CardCollection(self)

    @cached_property
    def milestones(self) -> ProjectMilestones:
        """Search labelled milestone dates across this project."""
        return ProjectMilestones(self)

    @cached_property
    def activity(self) -> ProjectActivityCollection:
        """Project-wide saved activity, newest first."""
        return ProjectActivityCollection(self)

    @cached_property
    def trash(self) -> TrashCollection:
        """Browse and manage this project's Trash."""
        return TrashCollection(self)

    @cached_property
    def resources(self) -> ResourceCollection:
        """Reusable resources in this project."""
        return ResourceCollection(self)

    @cached_property
    def storage_locations(self) -> StorageLocationCollection:
        """Storage declarations and this device's bindings."""
        return StorageLocationCollection(self)

    @cached_property
    def tags(self) -> TagCollection:
        return TagCollection(self)

    @cached_property
    def git(self) -> ProjectGit:
        """Live remote checks, Git status, pull, push, and sync."""
        return ProjectGit(self)

    @cached_property
    def changes(self) -> Changes:
        """Checkpoints and live change notifications scoped to this project."""
        return Changes(self._holder, self.id)

    @classmethod
    def _from(cls, holder: Holder, data: Any) -> Self:
        values: dict[str, Any] = {}
        for f in fields(cls):
            if f.name != "_holder":
                value = getattr(data, f.name, None)
                values[f.name] = None if isinstance(value, Unset) else value
        return cls(**values, _holder=holder)

    def refresh(self) -> Self:
        fresh = self._holder.projects.get(self.id)
        for f in fields(self):
            if f.name != "_holder":
                object.__setattr__(self, f.name, getattr(fresh, f.name))
        return self

    def update(self, *, name: str) -> Self:
        """Rename this project and refresh its server metadata."""
        if not isinstance(name, str):
            raise TypeError("name must be a string")
        self._holder._call(
            patch_projects_project_id,
            self.id,
            body=ProjectUpdateRequest(updated_at=int(time.time()), name=name),
            write=True,
        )
        return self.refresh()

    def repair_hierarchy(self, *, apply: bool = False) -> HierarchyRepair:
        """Find, and with apply=True fix, cards hidden under missing or trashed parents.

        Each hidden branch moves to its nearest live ancestor (or the top level)
        with its descendants, in one commit. Cards in parent cycles are only
        reported. The daemon also repairs at startup, and every pull repairs
        what it brings in. Cached listings need an explicit refresh.
        """
        if not isinstance(apply, bool):
            raise TypeError("apply must be a bool")
        data = self._holder._call(
            post_projects_project_id_repair_hierarchy,
            self.id,
            body=HierarchyRepairRequest(apply=apply),
            write=apply,
        )
        return HierarchyRepair(
            applied=data.applied,
            moved=[RepairedCard(m.card_id, m.parent_card_id) for m in data.moved],
            changed_card_count=data.changed_card_count,
            cycle_card_ids=list(data.cycle_card_ids),
        )

    def check_encryption(self) -> EncryptionCheck:
        """Inspect project blobs for plaintext before sharing an encrypted project."""
        data = self._holder._call(get_projects_project_id_encryption_check, self.id)
        if data.project_id != self.id:
            raise ProtocolError("Encryption check returned a different project")
        check = data.check
        return EncryptionCheck(
            privacy_mode=str(data.privacy_mode),
            ok=check.ok,
            checked_files=check.checked_files,
            unsafe_files=check.unsafe_files,
            unsafe_paths=tuple(check.unsafe_paths),
            message=check.message,
        )

    def export_recovery_token(self, *, pin: str) -> str:
        """Export the project's encrypted key as a serialized .hrk token.

        Keep the returned token and PIN private; the token is not cached here.
        """
        if not isinstance(pin, str) or not pin:
            raise ValueError("pin must be a nonempty string")
        data = self._holder._call(
            post_projects_project_id_recovery_token_export,
            self.id,
            body=ProjectRecoveryTokenExportRequest(pin=pin),
            write=True,
        )
        if data.project_id != self.id:
            raise ProtocolError("Recovery export returned a different project")
        return data.recovery_token

    def import_recovery_token(self, token: str, *, pin: str) -> Self:
        """Restore this existing project's encryption key from a token."""
        _check_recovery_inputs(token, pin)
        data = self._holder._call(
            post_projects_project_id_recovery_token_import,
            self.id,
            body=ProjectRecoveryTokenImportRequest(pin=pin, recovery_token=token),
            write=True,
        )
        if data.project_id != self.id:
            raise ProtocolError("Recovery import returned a different project")
        return self.refresh()

    def remove_from_device(self) -> None:
        """Unregister this project locally; files and encryption keys remain."""
        data = self._holder._call(delete_projects_project_id, self.id, write=True)
        if data.project_id != self.id:
            raise ProtocolError("Project removal returned a different project")
