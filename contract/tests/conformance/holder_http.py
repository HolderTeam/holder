"""The daemon's HTTP routes for the contract's operations, for the conformance tests.

Daemon starts a disposable holderd on temporary directories; Http calls the contract's operations
on it through their bindings in interface/http.py, taking and returning values as
holder_contract.values describes them, as holder_capi.Core does through the C API. A few routes
the contract does not have yet set up a project and trash cards.
"""

from __future__ import annotations

import http.client
import json
import os
import shutil
import subprocess
import tempfile
import time
import urllib.error
import urllib.parse
import urllib.request
from pathlib import Path
from typing import Any

from holder_capi import OperationError, Unrepresentable
from holder_contract.generate import snake

# The longest request target to send. The daemon reads at most 8 KB of request line and headers,
# and drops a connection whose request is longer (#300), so a longer argument cannot be sent over HTTP.
MAX_TARGET = 7 * 1024
# The codes contract_http::failure writes that are not error kinds.
CODES = {"invalid_argument": "InvalidArgument", "internal_error": "Runtime"}


class Daemon:
    """A holderd of its own, on directories of its own, until close."""

    def __init__(self, binary: str):
        self.directory = Path(tempfile.mkdtemp(prefix="holder-conformance-daemon-"))
        environment = {**os.environ}
        for variable, name in (("XDG_DATA_HOME", "data"), ("XDG_CONFIG_HOME", "config"),
                               ("XDG_CACHE_HOME", "cache"), ("HOLDER_PROJECTS_ROOT", "projects"),
                               ("HOLDER_TEST_KEYSTORE_DIR", "keystore")):
            environment[variable] = str(self.directory / name)
        binary_path = Path(binary).resolve()
        self.log = (self.directory / "daemon.log").open("w")
        # The binary's build directory supplies its resources in a development build.
        self.process = subprocess.Popen([str(binary_path), "--bind", "127.0.0.1", "--port", "0"],
                                        cwd=os.environ.get("HOLDER_DAEMON_CWD", str(binary_path.parent.parent)),
                                        env=environment, stdout=self.log, stderr=self.log)
        info_path = self.directory / "data/holder/server/holder.json"
        deadline = time.monotonic() + 20
        while True:
            if self.process.poll() is not None:
                raise RuntimeError(f"holderd exited during startup; see {self.directory / 'daemon.log'}")
            try:
                info = json.loads(info_path.read_text())
                self.url = f"http://127.0.0.1:{info['port']}"
                self.token = info["auth_token"]
                if self.request("GET", "/health")[0] == 200:
                    break
            except (OSError, ValueError, KeyError):
                pass
            if time.monotonic() >= deadline:
                raise RuntimeError("holderd did not become healthy")
            time.sleep(0.05)

    def request(self, method: str, target: str, body: bytes | None = None) -> tuple[int, Any]:
        request = urllib.request.Request(self.url + target, data=body, method=method,
                                         headers={"Authorization": f"Bearer {self.token}",
                                                  "Content-Type": "application/json"})
        try:
            with urllib.request.urlopen(request, timeout=30) as response:
                return response.status, json.loads(response.read() or b"null")
        except (ConnectionError, http.client.HTTPException) as error:
            self.log.flush()
            tail = (self.directory / "daemon.log").read_text(errors="replace").splitlines()[-15:]
            raise RuntimeError(f"holderd did not answer {method} {target}: {error}\n" + "\n".join(tail)) from None
        except urllib.error.HTTPError as error:
            text = error.read()
            try:
                return error.code, json.loads(text)
            except ValueError:
                return error.code, {"error": {"code": f"http_{error.code}", "message": text.decode("replace")}}

    def close(self) -> None:
        if self.process.poll() is None:
            self.process.terminate()
        try:
            self.process.wait(timeout=10)
        except subprocess.TimeoutExpired:
            self.process.kill()
            self.process.wait(timeout=5)
        self.log.close()
        shutil.rmtree(self.directory, ignore_errors=True)


def text(value: Any) -> str:
    """A path or query value, percent-encoded; bytes pass as they are, for text that is not UTF-8."""
    if isinstance(value, bool):
        return "true" if value else "false"
    if isinstance(value, (int, float)):
        return str(value)
    return urllib.parse.quote(value, safe="")


class Http:
    def __init__(self, ir: dict[str, Any], daemon: Daemon):
        self.daemon = daemon
        self.bindings = {b["operation"]: b for b in ir["http"]}
        self.kinds = {**CODES, **{snake(e["name"]): e["name"] for e in ir["errors"]}}

    def close(self) -> None:
        """The daemon outlives each run; Daemon.close stops it."""

    def expect(self, status: int, payload: Any) -> Any:
        if 200 <= status < 300 and payload.get("ok") is True:
            return payload["data"]
        code = payload.get("error", {}).get("code", f"status {status}")
        raise OperationError(self.kinds.get(code, code), payload.get("error", {}).get("message", ""))

    def create_project(self, name: str) -> str:
        body = json.dumps({"name": name, "privacy_mode": "plain"}).encode()
        return self.expect(*self.daemon.request("POST", "/projects", body))["project_id"]

    def delete_subtree(self, card_id: str) -> None:
        self.expect(*self.daemon.request("DELETE", f"/cards/{text(card_id)}?children=trash_subtree"))

    def call(self, operation: str, **arguments: Any) -> Any:
        """Call an operation; raises OperationError with the error kind's name when it fails."""
        binding = self.bindings[operation]
        path = binding["path"]
        for name in binding["path_parameters"]:
            value = arguments.get(name, ...)
            if value is ... or value is None or value in ("", b""):
                raise Unrepresentable(f"{name} has no place in the path")
            path = path.replace("{" + name + "}", text(value))
        query = [f"{name}={text(arguments[name])}" for name in binding["query"]
                 if arguments.get(name, None) is not None]
        target = path + ("?" + "&".join(query) if query else "")
        if len(target) > MAX_TARGET:
            raise Unrepresentable(f"a request target of {len(target)} bytes, longer than the daemon reads")
        body = None
        if "body" in binding:
            value = arguments.get(binding["body"], ...)
            if value is ...:
                raise Unrepresentable("a request body is required")
            try:
                body = json.dumps(value).encode()
            except TypeError as error:  # bytes, which JSON cannot carry
                raise Unrepresentable(str(error)) from None
        return self.expect(*self.daemon.request(binding["method"], target, body))
