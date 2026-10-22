"""High-level project Git, recovery, encryption, and local removal."""

import json

import httpx
import pytest
from test_library import failure, holder_for, ok, project_payload

from holder import EncryptionCheck, RecoveryResult, RemoteCheck, SyncResult
from holder.exceptions import APIError, ProtocolError


def sync_data(*, succeeded=True):
    return {
        "project_id": "p-0",
        "branch": "main",
        "remote_url": "https://example.test/repo.git",
        "status": "succeeded" if succeeded else "failed",
        "error_code": None if succeeded else "pull_failed",
        "error_message": None if succeeded else "Cannot pull",
        "pull": {
            "attempted": True,
            "status": "succeeded" if succeeded else "failed",
            "conflicts_resolved": 2 if succeeded else 0,
            "error_message": None if succeeded else "Cannot pull",
        },
        "push": {
            "attempted": succeeded,
            "status": "up_to_date" if succeeded else None,
            "ahead_count": 0,
            "behind_count": 0,
            "local_head_commit": "abc" if succeeded else None,
            "error_message": None,
        },
    }


def test_git_operations_use_scoped_routes_and_preserve_phase_failures():
    requests = []

    def respond(request):
        if request.url.path == "/projects/p-0":
            return ok(project_payload())
        requests.append(request)
        path = request.url.path
        if path.endswith("/git/test-remote"):
            return ok(
                {
                    "project_id": "p-0",
                    "branch": "main",
                    "status": "reachable",
                    "remote_has_head": True,
                    "remote_url": "https://example.test/repo.git",
                }
            )
        if path.endswith("/git/sync-status"):
            return ok({"project_id": "p-0", "sync": project_payload()["sync"]})
        if path.endswith("/git/push"):
            return ok(
                {
                    "project_id": "p-0",
                    "branch": "main",
                    "status": "non_fast_forward",
                    "ahead_count": 1,
                    "behind_count": 2,
                    "error_code": "non_fast_forward",
                    "error_message": "Pull first",
                    "next_action": "pull",
                }
            )
        if path.endswith("/git/pull"):
            data = sync_data()
            data["push"]["attempted"] = False
            data["push"]["status"] = None
            return ok(data)
        if path.endswith("/git/sync"):
            return ok(sync_data(succeeded=False))
        raise AssertionError(path)

    with holder_for(respond) as holder:
        project = holder.projects.get("p-0")
        assert project.git is project.git
        assert project.git.status().unpushed_commits_count == 0
        probe = project.git.test_remote(
            url="https://example.test/repo.git", branch="main"
        )
        assert isinstance(probe, RemoteCheck) and probe.reachable
        push = project.git.push(branch="main", set_upstream=False)
        assert not push.succeeded
        assert (push.status, push.behind_count, push.next_action) == (
            "non_fast_forward",
            2,
            "pull",
        )
        pulled = project.git.pull()
        assert pulled.succeeded and pulled.pull.conflicts_resolved == 2
        assert not pulled.push.attempted
        synced = project.git.sync()
        assert isinstance(synced, SyncResult) and not synced.succeeded
        assert synced.pull.status == "failed" and not synced.push.attempted
    assert [(r.method, r.url.path) for r in requests] == [
        ("GET", "/projects/p-0/git/sync-status"),
        ("POST", "/projects/p-0/git/test-remote"),
        ("POST", "/projects/p-0/git/push"),
        ("POST", "/projects/p-0/git/pull"),
        ("POST", "/projects/p-0/git/sync"),
    ]
    assert json.loads(requests[1].content) == {
        "remote_url": "https://example.test/repo.git",
        "branch": "main",
    }
    assert json.loads(requests[2].content) == {"branch": "main", "set_upstream": False}


def test_remote_configuration_refreshes_and_allows_disconnect():
    requests = []
    stored = project_payload()
    stored["git_remote_url"] = "https://example.test/repo.git"

    def respond(request):
        if request.method == "GET":
            return ok(stored)
        requests.append(request)
        body = json.loads(request.content)
        assert set(body) == {"updated_at", "git_remote_url"}
        changed = stored["git_remote_url"] != body["git_remote_url"]
        stored["git_remote_url"] = body["git_remote_url"]
        return ok({"project_id": "p-0", "git_remote_changed": changed})

    with holder_for(respond) as holder:
        project = holder.projects.get("p-0")
        assert project.git.set_remote(None) is True
        assert project.git_remote_url is None
        assert project.git.set_remote(None) is False
    assert len(requests) == 2


def test_encryption_recovery_and_removal():
    requests = []
    stored = project_payload()
    stored["privacy_mode"] = "encrypted_git"

    def respond(request):
        if request.method == "GET" and request.url.path == "/projects/p-0":
            return ok(stored)
        requests.append(request)
        path = request.url.path
        if path.endswith("/encryption-check"):
            return ok(
                {
                    "project_id": "p-0",
                    "privacy_mode": "encrypted_git",
                    "check": {
                        "ok": False,
                        "checked_files": 2,
                        "unsafe_files": 1,
                        "unsafe_paths": ["cards/unsafe.md"],
                        "message": "Unsafe",
                    },
                }
            )
        if path.endswith("/recovery-token/export"):
            assert json.loads(request.content) == {"pin": "1234"}
            return ok(
                {"project_id": "p-0", "key_id": "key", "recovery_token": "SECRET"}
            )
        if path.endswith("/recovery-token/import"):
            assert json.loads(request.content) == {
                "pin": "1234",
                "recovery_token": "SECRET",
            }
            return ok({"project_id": "p-0"})
        if request.method == "DELETE":
            return ok({"project_id": "p-0"})
        raise AssertionError(path)

    with holder_for(respond) as holder:
        project = holder.projects.get("p-0")
        check = project.check_encryption()
        assert isinstance(check, EncryptionCheck) and not check.ok
        assert check.unsafe_paths == ("cards/unsafe.md",)
        token = project.export_recovery_token(pin="1234")
        assert token == "SECRET"
        assert "SECRET" not in repr(project)
        assert project.import_recovery_token(token, pin="1234") is project
        project.remove_from_device()
    assert [(r.method, r.url.path) for r in requests] == [
        ("GET", "/projects/p-0/encryption-check"),
        ("POST", "/projects/p-0/recovery-token/export"),
        ("POST", "/projects/p-0/recovery-token/import"),
        ("DELETE", "/projects/p-0"),
    ]


def test_global_recovery_keeps_partial_remote_and_pull_outcomes():
    requests = []

    def respond(request):
        if request.method == "GET":
            return ok(project_payload())
        requests.append(request)
        return httpx.Response(
            200,
            json={
                "ok": True,
                "data": {
                    "project_id": "p-0",
                    "project_created": True,
                    "remote_hint_present": True,
                    "remote_configured": True,
                    "pull_status": "failed",
                    "pull_error": "Offline",
                    "remote_error": None,
                },
            },
        )

    with holder_for(respond) as holder:
        result = holder.projects.recover("SECRET", pin="1234")
        assert isinstance(result, RecoveryResult)
        assert result.project.id == "p-0"
        assert result.project_created and result.remote_configured
        assert (result.pull_status, result.pull_error) == ("failed", "Offline")
        assert "SECRET" not in repr(result)
    assert requests[0].url.path == "/recovery-token/import"
    assert json.loads(requests[0].content) == {
        "pin": "1234",
        "recovery_token": "SECRET",
    }


def test_project_operations_reject_cross_project_responses_and_write_failure():
    def wrong(request):
        if request.method == "GET" and request.url.path == "/projects/p-0":
            return ok(project_payload())
        return ok({"project_id": "p-1", "sync": project_payload()["sync"]})

    with holder_for(wrong) as holder, pytest.raises(ProtocolError):
        holder.projects.get("p-0").git.status()

    writes = []

    def fail(request):
        if request.method == "GET":
            return ok(project_payload())
        writes.append(request)
        return failure(409)

    with holder_for(fail) as holder, pytest.raises(APIError):
        holder.projects.get("p-0").remove_from_device()
    assert len(writes) == 1
