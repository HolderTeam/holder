"""Verify that daemon startup leaves an incomplete recovery ready to retry."""

import json
import os
import shutil
from pathlib import Path

import pytest

from holder_http import Daemon


def test_recovery_can_pull_after_daemon_restart():
    binary = os.environ.get("HOLDER_DAEMON")
    if not binary:
        pytest.skip("set HOLDER_DAEMON to test recovery across daemon restart")
    daemon = Daemon(binary)

    def call(method, path, status=200, **data):
        code, body = daemon.request(method, path, json.dumps(data).encode() if data else None)
        assert code == status, body
        return body["data"]

    try:
        remote = daemon.directory / "remote"
        original = call("POST", "/projects", 201, name="Retry recovery", privacy_mode="encrypted_git",
                        git_remote_url=str(remote))
        project = f"/projects/{original['project_id']}"
        card = call("POST", f"{project}/cards", 201, title="Recovered", content="Remote body")
        token = call("POST", f"{project}/recovery-token/export", pin="1234")["recovery_token"]
        call("DELETE", project)

        imported = call("POST", "/recovery-token/import", 201, pin="1234", recovery_token=token)
        assert imported["pull_status"] == "failed"
        pending = Path(call("GET", project)["root_path"])
        assert not (pending / ".holder/project.json").exists()

        daemon.restart()
        assert call("GET", project)["root_path"] == str(pending)
        assert not (pending / ".holder/project.json").exists()

        shutil.copytree(original["root_path"], remote)
        pull = call("POST", f"{project}/git/pull")["pull"]
        assert pull["status"] == "succeeded", pull
        assert (pending / ".holder/project.json").is_file()
        assert call("GET", f"{project}/cards/{card['card_id']}")["content"] == "Remote body"
    finally:
        daemon.close()
