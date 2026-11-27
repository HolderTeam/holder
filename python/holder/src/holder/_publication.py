"""Publish or discard a managed workspace."""

from __future__ import annotations

import re
from collections.abc import Iterator
from contextlib import contextmanager
from dataclasses import dataclass
from os.path import samestat
from pathlib import Path
from uuid import uuid4

from .workspaces import (
    ManagedWorkspace,
    _checkout_safe,
    _git,
    _Marker,
    _private_tree,
    _Publication,
    _read_marker,
    _remote,
    _remove_tree,
    _revision,
    _write_marker,
    reopen_workspace,
)


@dataclass(frozen=True)
class PushPreview:
    """The destination, commit, and working-tree state before publication."""

    remote_url: str
    branch: str
    revision: str
    new_branch: bool
    has_uncommitted_changes: bool


@dataclass(frozen=True)
class PushResult:
    """A confirmed publication to a remote branch."""

    remote_url: str
    branch: str
    revision: str


@dataclass(frozen=True)
class DiscardPreview:
    """The exact local directory and potential losses from permanent disposal."""

    path: Path
    revision: str
    has_unpublished_commits: bool
    has_uncommitted_changes: bool
    warnings: tuple[str, ...]


def _branch_ref(branch: str) -> str:
    if not isinstance(branch, str):
        raise TypeError("branch must be a string")
    if not branch or branch.startswith(("-", "refs/")) or branch == "HEAD":
        raise ValueError("An explicit branch name is required")
    ref = "refs/heads/" + branch
    try:
        _git("check-ref-format", ref)
    except RuntimeError:
        raise ValueError("Invalid branch name") from None
    return ref


def _validate_publication(record: object) -> None:
    if not isinstance(record, dict) or not all(
        isinstance(record.get(key), str) for key in ("remote_url", "branch", "revision")
    ):
        raise ValueError("Invalid managed workspace publication record")
    _remote(record["remote_url"])
    if not re.fullmatch(r"[0-9a-f]{40}", record["revision"]):
        raise ValueError("Invalid managed workspace publication revision")


def _current_workspace(
    workspace: ManagedWorkspace, *, device: int | None = None
) -> tuple[Path, _Marker]:
    root = Path(workspace.project["root_path"])
    path = workspace.path
    if path.resolve(strict=True) != path:
        raise ValueError("Managed workspace path was redirected")
    _private_tree(path, device=device)
    marker = _read_marker(path)
    if (
        marker.get("workspace_id") != workspace._workspace_id
        or marker["project_id"] != workspace.project_id
        or path / marker["project_path"] != root
    ):
        raise ValueError("Managed workspace does not match the active project")
    _checkout_safe(root)
    publications = marker.get("publications", [])
    if not isinstance(publications, list):
        raise TypeError("Invalid managed workspace publication history")
    for record in publications:
        _validate_publication(record)
    if "publication_attempt" in marker:
        _validate_publication(marker["publication_attempt"])
    return root, marker


def _on_remote(
    root: Path, destination: str, *args: str, refs: tuple[str, ...]
) -> bytes:
    # Use the same explicit URL for inspection and publication. An explicit
    # pushurl keeps Git's pushInsteadOf rules from redirecting just the push.
    name = "holder-publication-" + uuid4().hex
    return _git(
        "-C",
        str(root),
        "-c",
        f"remote.{name}.url={destination}",
        "-c",
        f"remote.{name}.pushurl={destination}",
        *args,
        "--",
        name,
        *refs,
        network=True,
    )


@dataclass(frozen=True)
class _Target:
    preview: PushPreview
    root: Path
    marker: _Marker
    recovered: bool


def _target(
    workspace: ManagedWorkspace, branch: str, remote_url: str | None
) -> _Target:
    ref = _branch_ref(branch)
    root, marker = _current_workspace(workspace)
    destination = remote_url if remote_url is not None else marker["source"]
    if destination is None:
        raise ValueError("A new workspace needs an explicit remote_url")
    destination = _remote(destination)
    revision = _revision(root)
    advertised = _on_remote(
        root, destination, "ls-remote", "--symref", refs=("HEAD", ref)
    ).decode()
    remote_revision = None
    for line in advertised.splitlines():
        value, _, name = line.partition("\t")
        if name == "HEAD" and value == "ref: " + ref:
            raise ValueError("Publication cannot update the remote default branch")
        if name == ref:
            remote_revision = value

    previous = any(
        record["remote_url"] == destination and record["branch"] == branch
        for record in marker.get("publications", [])
    )
    attempted = marker.get("publication_attempt")
    recovered = remote_revision == revision and attempted == {
        "remote_url": destination,
        "branch": branch,
        "revision": revision,
    }
    if remote_revision is not None and not recovered:
        if not previous:
            raise ValueError("First publication requires a new remote branch")
        if not re.fullmatch(r"[0-9a-f]{40}", remote_revision):
            raise ValueError("Invalid remote branch revision")
        try:
            _git(
                "-C",
                str(root),
                "merge-base",
                "--is-ancestor",
                remote_revision,
                revision,
            )
        except RuntimeError:
            raise ValueError(
                "Remote branch has diverged; publication must be fast-forward"
            ) from None

    dirty = bool(
        _git("--no-optional-locks", "-C", str(root), "status", "--porcelain", "-z")
    )
    return _Target(
        PushPreview(destination, branch, revision, remote_revision is None, dirty),
        root,
        marker,
        recovered,
    )


def preview_push(
    workspace: ManagedWorkspace, *, branch: str, remote_url: str | None
) -> PushPreview:
    return _target(workspace, branch, remote_url).preview


def push(
    workspace: ManagedWorkspace,
    *,
    branch: str,
    remote_url: str | None,
    expected_revision: str | None,
) -> PushResult:
    target = _target(workspace, branch, remote_url)
    preview, marker = target.preview, target.marker
    if expected_revision is not None and expected_revision != preview.revision:
        raise ValueError("Project changed since the reviewed revision")
    record: _Publication = {
        "remote_url": preview.remote_url,
        "branch": branch,
        "revision": preview.revision,
    }
    if not target.recovered:
        marker["publication_attempt"] = record
        _write_marker(workspace.path, marker)
        ref = "refs/heads/" + branch
        lease = ["--force-with-lease=" + ref + ":"] if preview.new_branch else []
        try:
            result = _on_remote(
                target.root,
                preview.remote_url,
                "-c",
                "push.followTags=false",
                "push",
                "--porcelain",
                "--no-verify",
                "--recurse-submodules=no",
                *lease,
                refs=(preview.revision + ":" + ref,),
            )
        except RuntimeError:
            raise RuntimeError(
                "Publication failed or its outcome is unknown. Local work is retained. "
                "Check the remote branch before retrying."
            ) from None
        if preview.new_branch and any(
            line.startswith(b"=\t") for line in result.splitlines()
        ):
            # Git can report "up to date" without checking the empty lease if
            # someone created the branch at the exact same commit in the meantime.
            del marker["publication_attempt"]
            _write_marker(workspace.path, marker)
            raise ValueError("Remote branch was created before publication")

    marker["publications"] = [
        entry
        for entry in marker.get("publications", [])
        if (entry["remote_url"], entry["branch"]) != (preview.remote_url, branch)
    ] + [record]
    marker.pop("publication_attempt", None)
    try:
        _write_marker(workspace.path, marker)
    except OSError:
        raise RuntimeError(
            "Push succeeded, but its local record could not be saved. Retry this revision."
        ) from None
    return PushResult(preview.remote_url, branch, preview.revision)


@contextmanager
def _for_discard(workspace: ManagedWorkspace) -> Iterator[ManagedWorkspace]:
    if not workspace.session._closed:
        yield workspace
        return
    # A fresh checkout of the same project can have the same project and card IDs.
    if workspace.path.resolve(strict=True) != workspace.path:
        raise ValueError("Managed workspace path was redirected")
    if _read_marker(workspace.path).get("workspace_id") != workspace._workspace_id:
        raise ValueError("Managed workspace was replaced")
    with reopen_workspace(workspace.path) as active:
        if active._workspace_id != workspace._workspace_id:
            raise ValueError("Managed workspace was replaced")
        yield active


def preview_discard(workspace: ManagedWorkspace) -> DiscardPreview:
    with _for_discard(workspace) as active:
        root, marker = _current_workspace(active)
        revision = _revision(root)
        published = set()
        for record in marker.get("publications", []):
            commit = record["revision"]
            try:
                _git("-C", str(root), "cat-file", "-e", commit + "^{commit}")
            except RuntimeError:
                continue
            published.add(commit)
        unpublished = bool(
            _git(
                "-C", str(root), "rev-list", "-1", revision, "--not", *sorted(published)
            )
        )
        dirty = bool(
            _git("--no-optional-locks", "-C", str(root), "status", "--porcelain", "-z")
        )
        warnings = ["Discard permanently removes local project data and artifacts."]
        if unpublished:
            warnings.append("Some commits have no recorded successful publication.")
        if dirty:
            warnings.append("Uncommitted or untracked project files will be lost.")
        return DiscardPreview(
            workspace.path, revision, unpublished, dirty, tuple(warnings)
        )


def discard(workspace: ManagedWorkspace, *, confirm: bool) -> None:
    if confirm is not True:
        raise ValueError("Permanent disposal requires confirm=True")
    with _for_discard(workspace) as active:
        path = active.path
        identity = path.stat(follow_symlinks=False)
        _, marker = _current_workspace(active, device=identity.st_dev)
        removed = path.with_name(".holder-discard-" + uuid4().hex)
        marker["discarding"] = True
        _write_marker(path, marker)
        try:
            # Windows requires closed database and lock handles before a rename.
            # The marker stops managed reopens once the profile lock is released.
            active.session.close()
            if not samestat(identity, path.stat(follow_symlinks=False)):
                raise ValueError("Managed workspace was replaced")
            path.rename(removed)
        except BaseException as error:
            marker.pop("discarding")
            try:
                if samestat(identity, path.stat(follow_symlinks=False)):
                    _write_marker(path, marker)
            except OSError:
                error.add_note(f"Could not restore the workspace marker at {path}")
            raise
        if not samestat(identity, removed.stat(follow_symlinks=False)):
            raise RuntimeError(
                f"Workspace was replaced during discard. Moved directory remains at {removed}"
            )
        try:
            _remove_tree(removed)
        except OSError as error:
            raise RuntimeError(
                f"Incomplete workspace cleanup remains at {removed}"
            ) from error
