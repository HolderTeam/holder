"""Private project workspaces backed by an independent Git checkout."""

from __future__ import annotations

import json
import os
import re
import shutil
import stat
import subprocess
import sys
from _thread import LockType
from collections.abc import Callable, Iterator
from contextlib import AbstractContextManager, ExitStack, contextmanager
from dataclasses import dataclass, field
from pathlib import Path, PurePosixPath
from tempfile import mkstemp
from threading import Lock
from types import TracebackType
from typing import TYPE_CHECKING, NotRequired, TypedDict, cast
from urllib.parse import urlsplit

from ._workspace_contract import Project as ProjectRecord
from .cards import EmbeddedWorkspace, open_workspace

if TYPE_CHECKING:
    from ._publication import PushPreview, PushResult

_MARKER = ".holder-workspace.json"


class _Publication(TypedDict):
    remote_url: str
    branch: str
    revision: str


class _Marker(TypedDict):
    version: int
    project_path: str
    project_id: str
    source: str | None
    ref: str | None
    revision: str
    publications: NotRequired[list[_Publication]]
    publication_attempt: NotRequired[_Publication]


@dataclass(frozen=True)
class ManagedWorkspace:
    """A single private project and its active shared Holder session."""

    path: Path
    project_id: str
    source: str | None
    ref: str | None
    session: EmbeddedWorkspace
    _publication_lock: LockType = field(
        default_factory=Lock, init=False, repr=False, compare=False
    )

    @property
    def project(self) -> ProjectRecord:
        """Fetch the project's current detached record."""
        return self.session.get_project(self.project_id)

    @property
    def revision(self) -> str:
        """Read current Git HEAD, including local commits."""
        return _revision(Path(self.project["root_path"]))

    def preview_push(
        self, *, branch: str, remote_url: str | None = None
    ) -> PushPreview:
        """Check the destination and show which commit would be published."""
        from ._publication import preview_push

        with self._publication_lock:
            return preview_push(self, branch=branch, remote_url=remote_url)

    def push(
        self,
        *,
        branch: str,
        remote_url: str | None = None,
        expected_revision: str | None = None,
    ) -> PushResult:
        """Publish committed work to a named branch while retaining the workspace."""
        from ._publication import push

        with self._publication_lock:
            return push(
                self,
                branch=branch,
                remote_url=remote_url,
                expected_revision=expected_revision,
            )


def _destination(workspace: os.PathLike[str] | str) -> Path:
    if not os.fspath(workspace):
        raise ValueError("Supply a destination for the managed workspace")
    requested = Path(workspace).expanduser().absolute()
    if requested.is_symlink():
        raise ValueError("Workspace destination must not be a symlink")
    path = requested.parent.resolve() / requested.name
    path.parent.mkdir(parents=True, exist_ok=True)
    path.mkdir(mode=0o700)
    return path


def _remove_tree(path: Path) -> None:
    """Remove owned storage, including Git's read-only files on Windows."""

    def retry_readonly(
        function: Callable[[str], object],
        filename: str,
        error: tuple[type[BaseException], BaseException, TracebackType],
    ) -> None:
        file = Path(filename)
        if (
            sys.platform != "win32"
            or not isinstance(error[1], PermissionError)
            or file.is_symlink()
            or not file.is_file()
            or not file.resolve().is_relative_to(path.resolve())
            or file.stat().st_mode & stat.S_IWRITE
        ):
            raise error[1]
        file.chmod(stat.S_IREAD | stat.S_IWRITE)
        function(filename)

    shutil.rmtree(path, onerror=retry_readonly)


def _remote(remote_url: str) -> str:
    if not isinstance(remote_url, str):
        raise TypeError("remote_url must be a string")
    if (
        not remote_url
        or "::" in remote_url
        or any(ord(c) < 33 or ord(c) == 127 for c in remote_url)
    ):
        raise ValueError("An explicit Git remote URL is required")
    if "://" in remote_url:
        parsed = urlsplit(remote_url)
        if (
            parsed.scheme not in {"https", "ssh", "git"}
            or not parsed.hostname
            or not parsed.path
        ):
            raise ValueError(
                "Use an HTTPS, SSH or git:// remote; local paths are unsupported"
            )
        if (
            parsed.password is not None
            or parsed.query
            or parsed.fragment
            or (parsed.username is not None and parsed.scheme != "ssh")
        ):
            raise ValueError(
                "Do not embed credentials, query parameters or fragments in remote URLs"
            )
        if parsed.hostname.startswith("-") or (
            parsed.username
            and not re.fullmatch(r"[A-Za-z0-9_][A-Za-z0-9_.-]*", parsed.username)
        ):
            raise ValueError("Unsupported SSH user or host")
    elif not re.fullmatch(
        r"(?:[A-Za-z0-9_][A-Za-z0-9_.-]*@)?[A-Za-z0-9][A-Za-z0-9.-]*:[^\\]+", remote_url
    ):
        raise ValueError(
            "Use an explicit Git remote; local paths and helper transports are unsupported"
        )
    elif re.match(r"^[A-Za-z]:", remote_url):
        raise ValueError("Local paths are unsupported")
    return remote_url


def _git(*args: str, network: bool = False) -> bytes:
    env = os.environ.copy()
    for key in tuple(env):
        if key.startswith("GIT_") and key not in {
            "GIT_SSH",
            "GIT_SSH_COMMAND",
            "GIT_ASKPASS",
        }:
            env.pop(key)
    env["GIT_TERMINAL_PROMPT"] = "0"
    # Network clone can use configured credential helpers and SSH credentials.
    # Checkout ignores host filters, attributes and templates; project-supplied
    # hooks must never execute. No submodule initialization is performed.
    if not network:
        env["GIT_CONFIG_NOSYSTEM"] = "1"
        env["GIT_CONFIG_GLOBAL"] = os.devnull
    try:
        result = subprocess.run(
            [
                "git",
                "-c",
                f"core.hooksPath={os.devnull}",
                "-c",
                f"core.attributesFile={os.devnull}",
                "-c",
                "core.fsmonitor=false",
                "-c",
                "core.protectNTFS=true",
                "-c",
                "core.protectHFS=true",
                "-c",
                "protocol.allow=never",
                "-c",
                "protocol.file.allow=never",
                "-c",
                "protocol.ext.allow=never",
                "-c",
                "protocol.https.allow=always",
                "-c",
                "protocol.ssh.allow=always",
                "-c",
                "protocol.git.allow=always",
                *args,
            ],
            env=env,
            capture_output=True,
            check=False,
        )
    except OSError:
        raise RuntimeError("Git is required for managed Holder workspaces") from None
    if result.returncode:
        # Git diagnostics can contain URLs, helper output and credentials.
        raise RuntimeError(
            "Git operation failed; check the remote, ref and Git credential setup"
        )
    return result.stdout


def _private_tree(path: Path) -> None:
    def fail(error: OSError) -> None:
        raise error

    for directory, names, files in os.walk(path, onerror=fail, followlinks=False):
        for name in names + files:
            item = Path(directory) / name
            mode = item.lstat()
            if stat.S_ISLNK(mode.st_mode) or (
                getattr(mode, "st_file_attributes", 0)
                & stat.FILE_ATTRIBUTE_REPARSE_POINT
            ):
                raise ValueError("Managed workspaces do not support symlinks")
            if not (stat.S_ISDIR(mode.st_mode) or stat.S_ISREG(mode.st_mode)):
                raise ValueError(
                    "Managed workspaces require ordinary files and directories"
                )
            if stat.S_ISREG(mode.st_mode) and mode.st_nlink != 1:
                raise ValueError(
                    "Managed workspaces do not support shared hard-linked files"
                )


def _checkout_safe(root: Path) -> None:
    if not (root / ".git").is_dir():
        raise ValueError("A standalone project Git checkout is required")
    for relative in (".git/commondir", ".git/objects/info/alternates"):
        if (root / relative).exists():
            raise ValueError("Shared Git object stores and worktrees are unsupported")
    top = Path(
        os.fsdecode(_git("-C", str(root), "rev-parse", "--show-toplevel")).strip()
    )
    if top.resolve() != root:
        raise ValueError("Git working tree escapes the private checkout")
    # Do not read project secrets or attempt keyring access for encrypted copies.
    bootstrap = root / ".holder/privacy.json"
    if not bootstrap.is_file() or not (root / ".holder/project.json").is_file():
        raise ValueError("A durable Holder project manifest is required")
    privacy = json.loads(bootstrap.read_text(encoding="utf-8"))
    if not isinstance(privacy, dict) or privacy.get("mode") != "plain":
        raise ValueError("Managed workspaces currently support plain projects only")
    if (root / "server").exists():
        raise ValueError("Project checkouts must not contain daemon server state")


def _revision(root: Path) -> str:
    return _git("-C", str(root), "rev-parse", "HEAD").decode("ascii").strip()


@contextmanager
def _initialize_workspace(
    destination: os.PathLike[str] | str,
    initialize: Callable[[EmbeddedWorkspace, Path], ProjectRecord],
    *,
    source: str | None = None,
    ref: str | None = None,
) -> Iterator[ManagedWorkspace]:
    path = _destination(destination)
    with ExitStack() as stack:
        try:
            # Open the empty projection before putting durable project data on disk.
            session = stack.enter_context(open_workspace(path / "data"))
            project = initialize(session, path)
            root = Path(project["root_path"])
            if root.parent != path / "data/projects":
                raise ValueError(
                    "Managed project must stay within its private workspace"
                )
            _private_tree(path)
            _checkout_safe(root)
            marker: _Marker = {
                "version": 1,
                "project_path": root.relative_to(path).as_posix(),
                "project_id": project["project_id"],
                "source": source,
                "ref": ref,
                "revision": _revision(root),
            }
            _write_marker(path, marker)
        except BaseException as error:
            try:
                stack.close()
                _remove_tree(path)
            except OSError:
                error.add_note(
                    f"Managed workspace setup could not fully clean up {path}"
                )
            raise
        # Closing the returned session preserves a completed workspace, even on exceptions.
        yield ManagedWorkspace(path, project["project_id"], source, ref, session)


def create_managed_workspace(
    name: str, *, workspace: os.PathLike[str] | str
) -> AbstractContextManager[ManagedWorkspace]:
    """Create a plain project in a new managed workspace."""

    def initialize(session: EmbeddedWorkspace, _: Path) -> ProjectRecord:
        return session.create_plain_project(name=name)

    return _initialize_workspace(workspace, initialize)


def _clone_project(
    session: EmbeddedWorkspace, path: Path, source: str, ref: str | None
) -> ProjectRecord:
    root = path / "data/projects/project"
    root.parent.mkdir(parents=True)
    _git(
        "clone",
        "--no-checkout",
        "--no-local",
        "--no-hardlinks",
        "--template=",
        "--",
        source,
        str(root),
        network=True,
    )
    try:
        selected = (
            _git(
                "-C",
                str(root),
                "rev-parse",
                "--verify",
                "--end-of-options",
                (ref or "HEAD") + "^{commit}",
            )
            .decode("ascii")
            .strip()
        )
    except RuntimeError:
        if ref is None:
            raise
        selected = (
            _git(
                "-C",
                str(root),
                "rev-parse",
                "--verify",
                "--end-of-options",
                "refs/remotes/origin/" + ref + "^{commit}",
            )
            .decode("ascii")
            .strip()
        )
    tree = _git("-C", str(root), "ls-tree", "-r", "-z", selected)
    for entry in tree.split(b"\0"):
        if not entry:
            continue
        info, name = entry.split(b"\t", 1)
        if info.split(b" ", 1)[0] not in {b"100644", b"100755"}:
            raise ValueError("Clone does not support symlinks or submodules")
        parts = PurePosixPath(os.fsdecode(name)).parts
        if any(
            part.lower()
            in {".git", "server", "holder.db", "holder.db-wal", "holder.db-shm"}
            or "\\" in part
            or ":" in part
            for part in parts
        ):
            raise ValueError("Clone contains unsupported storage paths or server state")
    _git("-C", str(root), "checkout", "--detach", selected)
    _private_tree(root)
    _checkout_safe(root)
    project = cast("ProjectRecord", session._call("import_project", str(root)))
    if Path(project["root_path"]) != root:
        raise ValueError("Clone must reconstruct exactly one private project")
    return project


def clone_workspace(
    remote_url: str,
    *,
    workspace: os.PathLike[str] | str,
    ref: str | None = None,
) -> AbstractContextManager[ManagedWorkspace]:
    """Clone a plain Holder Git project into a new private workspace.

    Select a branch, tag or commit with ref, or use the remote's default HEAD.
    The cloned checkout is detached. Core reconstructs the local projection.
    """
    source = _remote(remote_url)
    if ref is not None and (
        not isinstance(ref, str)
        or not ref
        or ref.startswith("-")
        or any(ord(character) < 33 or ord(character) == 127 for character in ref)
    ):
        raise ValueError("ref must be a nonempty branch, tag or commit")

    def initialize(session: EmbeddedWorkspace, path: Path) -> ProjectRecord:
        return _clone_project(session, path, source, ref)

    return _initialize_workspace(workspace, initialize, source=source, ref=ref)


def _read_marker(path: Path) -> _Marker:
    marker_path = path / _MARKER
    if not marker_path.is_file():
        raise ValueError("Not a completed managed Holder workspace")
    marker = json.loads(marker_path.read_text(encoding="utf-8"))
    if not isinstance(marker, dict) or marker.get("version") != 1:
        raise ValueError("Unsupported or incomplete managed Holder workspace")
    relative = marker.get("project_path")
    parts = PurePosixPath(relative).parts if isinstance(relative, str) else ()
    if (
        len(parts) != 3
        or parts[:2] != ("data", "projects")
        or parts[2] in {".", ".."}
        or "/".join(parts) != relative
        or "\\" in relative
        or ":" in relative
    ):
        raise ValueError("Invalid managed workspace project path")
    project_id = marker.get("project_id")
    source, ref, revision = (
        marker.get("source"),
        marker.get("ref"),
        marker.get("revision"),
    )
    if source is not None:
        _remote(source)
    if (
        not isinstance(project_id, str)
        or not project_id
        or (ref is not None and not isinstance(ref, str))
        or not isinstance(revision, str)
        or not re.fullmatch(r"[0-9a-f]{40}", revision)
    ):
        raise ValueError("Invalid managed workspace identity or revision")
    return cast("_Marker", marker)


def _write_marker(path: Path, marker: _Marker) -> None:
    descriptor, filename = mkstemp(dir=path, prefix=_MARKER + "-")
    temporary = Path(filename)
    try:
        with os.fdopen(descriptor, "w", encoding="utf-8") as output:
            json.dump(marker, output, indent=2)
            output.write("\n")
        temporary.replace(path / _MARKER)
    finally:
        temporary.unlink(missing_ok=True)


@contextmanager
def reopen_workspace(workspace: os.PathLike[str] | str) -> Iterator[ManagedWorkspace]:
    """Open a completed managed workspace and retain any local edits."""
    if not os.fspath(workspace):
        raise ValueError("Supply a managed workspace directory")
    requested = Path(workspace).expanduser()
    if requested.is_symlink():
        raise ValueError("Workspace must not be a symlink")
    path = requested.resolve(strict=True)
    if not path.is_dir():
        raise ValueError("Supply a managed workspace directory")
    _private_tree(path)
    marker = _read_marker(path)
    root = path / marker["project_path"]
    if not root.is_dir() or list(root.parent.iterdir()) != [root]:
        raise ValueError("Workspace must contain exactly its recorded project")
    _checkout_safe(root)
    with open_workspace(path / "data") as session:
        projects = session.list_projects()
        if (
            len(projects) != 1
            or projects[0]["project_id"] != marker["project_id"]
            or Path(projects[0]["root_path"]) != root
        ):
            raise ValueError("Workspace database does not match its private project")
        yield ManagedWorkspace(
            path, marker["project_id"], marker["source"], marker["ref"], session
        )
