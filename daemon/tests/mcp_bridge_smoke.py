"""Exercise the stdio bridge as a subprocess against a real daemon."""

import contextlib
import json
import os
from pathlib import Path
import secrets
import socket
import subprocess
import sys
import tempfile
import time
from urllib.request import Request, urlopen

DAEMON, BRIDGE = (Path(value).resolve() for value in sys.argv[1:])
PROJECT = "019786ae-cc34-7e4a-944b-dc318abc8298"
VERSION = "2026-07-28"
LEGACY = "2025-11-25"


def send(process, method, request_id=None, **params):
    message = {"jsonrpc": "2.0", "method": method, "params": params}
    if request_id is not None:
        message["id"] = request_id
    process.stdin.write(json.dumps(message).encode("utf-8") + b"\n")
    process.stdin.flush()


def receive(process):
    reply = json.loads(process.stdout.readline())
    assert reply["jsonrpc"] == "2.0", reply
    return reply


def result(process, request_id):
    reply = receive(process)
    assert reply["id"] == request_id and "result" in reply, reply
    return reply["result"]


def shutdown(process):
    process.stdin.close()
    assert process.wait(timeout=3) == 0
    assert not process.stdout.read()
    assert not process.stderr.read()


@contextlib.contextmanager
def bridge(port, env):
    with subprocess.Popen(
        [str(BRIDGE), "--port", str(port)],
        env=env, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
    ) as process:
        try:
            yield process
        finally:
            if process.poll() is None:
                process.kill()


def test_available_daemon(port, env, card):
    metadata = {
        "io.modelcontextprotocol/protocolVersion": VERSION,
        "io.modelcontextprotocol/clientCapabilities": {},
    }
    with bridge(port, env) as process:
        send(process, "server/discover", "discover", _meta=metadata)
        assert result(process, "discover")["supportedVersions"] == [VERSION]
        send(process, "tools/list", 2, _meta=metadata)
        assert {tool["name"] for tool in result(process, 2)["tools"]} == {
            "get_card", "get_card_breadcrumbs", "get_resource", "list_card_milestones",
            "list_cards", "list_project_tags", "search_cards",
        }
        send(process, "tools/call", 3, _meta=metadata, name="get_card",
             arguments={"project_id": PROJECT, "card_id": card["card_id"]})
        send(process, "tools/call", 4, _meta=metadata, name="get_card",
             arguments={"project_id": "other", "card_id": card["card_id"]})
        replies = {reply["id"]: reply["result"] for reply in (receive(process), receive(process))}
        assert replies[3]["structuredContent"]["data"] == card
        assert replies[4]["isError"] is True
        shutdown(process)

    with bridge(port, env) as process:
        send(process, "initialize", 1, protocolVersion=LEGACY, capabilities={},
             clientInfo={"name": "holder-bridge-test", "version": "1"})
        assert result(process, 1)["protocolVersion"] == LEGACY
        send(process, "notifications/initialized")
        send(process, "tools/list", 2)
        discovered = result(process, 2)
        assert len(discovered["tools"]) == 7 and "resultType" not in discovered
        send(process, "tools/call", 3, name="get_card",
             arguments={"project_id": PROJECT, "card_id": card["card_id"]})
        assert result(process, 3)["structuredContent"]["data"] == card
        send(process, "tools/call", "list", name="list_cards",
             arguments={"project_id": PROJECT, "parent": "roots"})
        page = result(process, "list")["structuredContent"]["data"]
        assert [item["card_id"] for item in page["items"]] == [card["card_id"]]
        assert "content" not in page["items"][0]
        send(process, "tools/call", 4, name="unknown", arguments={})
        assert receive(process)["error"]["code"] == -32602
        shutdown(process)

    with bridge(port, dict(env, HOLDER_MCP_TOKEN=secrets.token_urlsafe(32))) as process:
        send(process, "server/discover", 1, _meta=metadata)
        assert receive(process)["error"]["message"].startswith("Holder MCP access was rejected")
        shutdown(process)


def test_cancellation(env):
    metadata = {
        "io.modelcontextprotocol/protocolVersion": VERSION,
        "io.modelcontextprotocol/clientCapabilities": {},
    }
    with socket.socket() as listener:
        listener.settimeout(3)
        listener.bind(("127.0.0.1", 0))
        listener.listen()
        for cancel_with_notification in (True, False):
            with bridge(listener.getsockname()[1], env) as process:
                send(process, "tools/list", "cancelled", _meta=metadata)
                stream, _ = listener.accept()
                with stream, stream.makefile("rb") as reader:
                    stream.settimeout(3)
                    headers = {}
                    reader.readline()
                    while (line := reader.readline()).strip():
                        key, value = line.split(b":", 1)
                        headers[key.lower()] = value.strip()
                    reader.read(int(headers[b"content-length"]))
                    if cancel_with_notification:
                        send(process, "notifications/cancelled", requestId="cancelled")
                    else:
                        process.stdin.close()
                    assert reader.read(1) == b""
                if cancel_with_notification:
                    send(process, "ping", "after-cancel")
                    assert result(process, "after-cancel") == {}
                shutdown(process)


with tempfile.TemporaryDirectory(prefix="holder-mcp-test-") as temp:
    root = Path(temp)
    env = dict(os.environ, XDG_DATA_HOME=str(root / "data"), XDG_CONFIG_HOME=str(root / "config"),
               XDG_CACHE_HOME=str(root / "cache"), HOLDER_PROJECTS_ROOT=str(root / "projects"),
               HOLDER_MCP_PROJECT=PROJECT, HOLDER_MCP_TOKEN=secrets.token_urlsafe(32))
    with subprocess.Popen(
        [str(DAEMON), "--port", "0"], cwd=DAEMON.parent, env=env,
        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
    ) as daemon:
        try:
            info_path = root / "data/holder/server/holder.json"
            for _ in range(200):
                if info_path.is_file():
                    break
                assert daemon.poll() is None, "holderd exited during startup"
                time.sleep(0.05)
            info = json.loads(info_path.read_text(encoding="utf-8"))
            port = info["port"]

            def post(path, body):
                request = Request(f"http://127.0.0.1:{port}{path}", json.dumps(body).encode(), headers={
                    "Authorization": f"Bearer {info['auth_token']}", "Content-Type": "application/json",
                })
                with urlopen(request, timeout=5) as response:
                    return json.load(response)["data"]

            post("/projects", {"name": "MCP test", "project_id": PROJECT, "privacy_mode": "plain"})
            test_available_daemon(port, env, post(f"/projects/{PROJECT}/cards", {"title": "Bridge test"}))
        finally:
            daemon.terminate()
            daemon.wait(timeout=10)
    with bridge(port, env) as process:
        send(process, "server/discover", 1, _meta={
            "io.modelcontextprotocol/protocolVersion": VERSION,
            "io.modelcontextprotocol/clientCapabilities": {},
        })
        assert receive(process)["error"]["code"] == -32000
        shutdown(process)
    test_cancellation(env)
