import json
import shlex
import shutil
import subprocess
import sys
from importlib.util import find_spec
from pathlib import Path

import pytest

from holder import clone_workspace, create_managed_workspace, reopen_workspace
from holder.exceptions import APIError, ClosedError


@pytest.fixture
def embedded():
    if not (find_spec("holder._embedded._native") or find_spec("holderkit")):
        pytest.skip("Requires an embedded Holder build")
    if shutil.which("git") is None:
        pytest.skip("Requires Git")


def git(path, *args):
    return (
        subprocess.run(
            ["git", "-C", str(path), *args], check=True, capture_output=True, timeout=20
        )
        .stdout.decode()
        .strip()
    )


@pytest.fixture
def remote(embedded, tmp_path, monkeypatch):
    with create_managed_workspace("Research", workspace=tmp_path / "origin") as source:
        card = source.session.card_operations.create_card(
            source.project_id,
            {"title": "Evidence", "content": "Captured body #research"},
        )
        before = source.revision
        root = Path(source.project["root_path"])
        git(root, "branch", "review", before)
        source.session.card_operations.update_card(
            source.project_id, card["card_id"], {"content": "New upstream body"}
        )
        project_id = source.project_id
        latest = source.revision

    bare = tmp_path / "research.git"
    git(tmp_path, "clone", "--bare", str(root), str(bare))
    # Exercise Git's SSH transport through a local upload-pack without credentials.
    ssh = tmp_path / "fixture-ssh.py"
    ssh.write_text(
        "import shlex, subprocess, sys\n"
        "command = shlex.split(sys.argv[-1])\n"
        "if command != ['git-upload-pack', '/research.git']:\n"
        "    sys.exit(1)\n"
        f"sys.exit(subprocess.call(['git', 'upload-pack', {str(bare)!r}]))\n",
        encoding="utf-8",
    )
    monkeypatch.setenv(
        "GIT_SSH_COMMAND",
        f"{shlex.quote(Path(sys.executable).as_posix())} {shlex.quote(ssh.as_posix())}",
    )
    return (
        "ssh://fixture@localhost/research.git",
        project_id,
        card["card_id"],
        before,
        latest,
        bare,
    )


def test_create_reopen_and_keep_edits(embedded, tmp_path):
    destination = tmp_path / "workspace"
    with (
        pytest.raises(RuntimeError, match="script stopped"),
        create_managed_workspace("Notebook", workspace=destination) as work,
    ):
        assert work.project["name"] == "Notebook"
        assert work.path == destination.resolve()
        assert work.source is None
        card = work.session.card_operations.create_card(
            work.project_id, {"title": "Notes", "content": "Keep this"}
        )
        revision = work.revision
        with (
            pytest.raises(APIError, match="workspace is in use"),
            reopen_workspace(destination),
        ):
            pass
        raise RuntimeError("script stopped")
    with pytest.raises(ClosedError):
        assert work.project

    with reopen_workspace(destination) as reopened:
        assert reopened.revision == revision
        assert reopened.project_id == work.project_id
        assert (
            reopened.session.card_operations.get_card(work.project_id, card["card_id"])
            == card
        )


def test_clone_selected_branch_and_keep_private_edits(remote, tmp_path):
    url, project_id, card_id, before, latest, bare = remote
    destination = tmp_path / "clone"
    with clone_workspace(url, workspace=destination, ref="review") as work:
        assert work.project_id == project_id
        assert work.source == url
        assert work.ref == "review"
        assert work.revision == before
        assert (
            git(Path(work.project["root_path"]), "rev-parse", "--abbrev-ref", "HEAD")
            == "HEAD"
        )
        cards = work.session.card_operations
        assert (
            cards.get_card(project_id, card_id)["content"] == "Captured body #research"
        )
        cards.update_card(project_id, card_id, {"content": "Private edit"})
    assert git(bare, "rev-parse", "HEAD") == latest
    assert git(bare, "rev-parse", "review") == before

    with reopen_workspace(destination) as work:
        assert (
            work.session.card_operations.get_card(project_id, card_id)["content"]
            == "Private edit"
        )


def test_clone_failure_leaves_existing_paths_alone(remote, tmp_path):
    url, *_ = remote
    destination = tmp_path / "failed"
    with (
        pytest.raises(RuntimeError, match="Git operation failed"),
        clone_workspace(url, workspace=destination, ref="missing"),
    ):
        pytest.fail("Opened a missing ref")
    assert not destination.exists()

    destination.mkdir()
    sentinel = destination / "keep"
    sentinel.write_text("user data")
    with pytest.raises(FileExistsError), clone_workspace(url, workspace=destination):
        pass
    assert sentinel.read_text() == "user data"


def test_clone_checks_the_manifest_before_import(remote, tmp_path):
    url, *_, bare = remote
    scratch = tmp_path / "edit-remote"
    git(tmp_path, "clone", str(bare), str(scratch))
    git(scratch, "rm", ".holder/project.json")
    git(
        scratch,
        "-c",
        "user.name=Test",
        "-c",
        "user.email=test@example.org",
        "commit",
        "-m",
        "Remove the Holder manifest",
    )
    git(scratch, "push", str(bare), "HEAD:refs/heads/broken")
    destination = tmp_path / "clone"
    with (
        pytest.raises(ValueError, match="durable Holder project manifest"),
        clone_workspace(url, workspace=destination, ref="broken"),
    ):
        pass
    assert not destination.exists()


@pytest.mark.parametrize(
    "source",
    [
        "/tmp/project",
        "file:///tmp/project",
        "ext::helper",
        "https://user:password@example.org/project.git",
        "https://example.org/project.git?token=secret",
    ],
)
def test_clone_refuses_local_paths_and_embedded_credentials(tmp_path, source):
    path = tmp_path / "workspace"
    with pytest.raises(ValueError), clone_workspace(source, workspace=path):
        pass
    assert not path.exists()


def test_reopen_refuses_workspace_with_redirected_project(embedded, tmp_path):
    with create_managed_workspace("Notebook", workspace=tmp_path / "workspace") as work:
        path = work.path
    marker = path / ".holder-workspace.json"
    data = json.loads(marker.read_text(encoding="utf-8"))
    data["project_path"] = "data/projects/../outside"
    marker.write_text(json.dumps(data), encoding="utf-8")
    with pytest.raises(ValueError, match="project path"), reopen_workspace(path):
        pass
