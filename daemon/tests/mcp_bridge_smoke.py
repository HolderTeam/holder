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
            "get_calendar", "get_card", "get_card_breadcrumbs", "get_resource", "list_card_backlinks",
            "list_card_links", "list_card_milestones", "list_card_resources", "list_cards",
            "list_project_tags", "list_trash", "search_cards",
        }
        send(process, "tools/call", 3, _meta=metadata, name="get_card",
             arguments={"project_id": PROJECT, "card_id": card["card_id"]})
        send(process, "tools/call", 4, _meta=metadata, name="get_card",
             arguments={"project_id": "other", "card_id": card["card_id"]})
        replies = {reply["id"]: reply["result"] for reply in (receive(process), receive(process))}
        assert replies[3]["structuredContent"]["data"] == card
        assert replies[4]["isError"] is True
        send(process, "tools/call", "write-disabled", _meta=metadata, name="create_card",
             arguments={"card": {"title": "Must stay read-only"}})
        denied = receive(process)
        assert denied["id"] == "write-disabled" and denied["error"]["code"] == -32602
        shutdown(process)

    with bridge(port, env) as process:
        send(process, "initialize", 1, protocolVersion=LEGACY, capabilities={},
             clientInfo={"name": "holder-bridge-test", "version": "1"})
        assert result(process, 1)["protocolVersion"] == LEGACY
        send(process, "notifications/initialized")
        send(process, "tools/list", 2)
        discovered = result(process, 2)
        assert len(discovered["tools"]) == 12 and "resultType" not in discovered
        send(process, "tools/call", 3, name="get_card",
             arguments={"project_id": PROJECT, "card_id": card["card_id"]})
        assert result(process, 3)["structuredContent"]["data"] == card
        send(process, "tools/call", "list", name="list_cards",
             arguments={"parent": "roots"})
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


def test_write_tools(port, env, rest):
    metadata = {
        "io.modelcontextprotocol/protocolVersion": VERSION,
        "io.modelcontextprotocol/clientCapabilities": {},
    }
    with bridge(port, env) as process:
        send(process, "tools/list", 1, _meta=metadata)
        tools = {tool["name"]: tool for tool in result(process, 1)["tools"]}
        for name in ("create_card", "add_card_milestone"):
            assert tools[name]["annotations"]["readOnlyHint"] is False
            assert tools[name]["annotations"]["idempotentHint"] is False

        send(process, "tools/call", 2, _meta=metadata, name="create_card",
             arguments={"project_id": "other", "card": {"title": "Denied"}})
        denied = result(process, 2)
        assert denied["isError"] is True
        assert denied["content"][0]["text"] == "Access to this project was not granted."

        send(process, "tools/call", 3, _meta=metadata, name="create_card",
             arguments={"card": {"title": "Write through MCP"}})
        card = result(process, 3)["structuredContent"]["data"]
        assert card["project_id"] == PROJECT and card["title"] == "Write through MCP"
        path = f"/projects/{PROJECT}/cards/{card['card_id']}"

        send(process, "tools/call", 4, _meta=metadata, name="add_card_milestone",
             arguments={"card_id": card["card_id"], "milestone": {"dates": [{"role": "due", "date": "2026-12-04"}]}})
        milestone = result(process, 4)["structuredContent"]["data"]
        assert milestone["card_id"] == card["card_id"]
        assert milestone["dates"] == [{"role": "due", "date": "2026-12-04"}]
        persisted_card = rest(path)
        assert persisted_card["card_id"] == card["card_id"] and persisted_card["title"] == card["title"]
        assert rest(f"{path}/milestones") == [milestone]
        send(process, "tools/call", 5, _meta=metadata, name="update_card",
             arguments={"card_id": card["card_id"], "update": {"title": "Renamed", "content": "Release notes"}})
        updated = result(process, 5)["structuredContent"]["data"]
        assert updated["title"] == "Renamed" and updated["content"] == "Release notes"

        send(process, "tools/call", 6, _meta=metadata, name="update_card_milestone",
             arguments={"card_id": card["card_id"], "milestone_id": milestone["milestone_id"],
                        "update": {"dates": [{"role": "due", "date": "2026-12-11"}]}})
        changed = result(process, 6)["structuredContent"]["data"]
        assert changed["milestone_id"] == milestone["milestone_id"]
        assert changed["dates"] == [{"role": "due", "date": "2026-12-11"}]
        assert rest(f"{path}/milestones") == [changed]

        send(process, "tools/call", 7, _meta=metadata, name="add_card_tag",
             arguments={"card_id": card["card_id"], "tag": "release"})
        assert result(process, 7)["structuredContent"]["data"]["tags"] == ["release"]
        send(process, "tools/call", 8, _meta=metadata, name="remove_card_tag",
             arguments={"card_id": card["card_id"], "tag": "release"})
        assert result(process, 8)["structuredContent"]["data"]["tags"] == []

        parent = rest(f"/projects/{PROJECT}/cards", {"title": "Parent"})
        send(process, "tools/call", 9, _meta=metadata, name="move_card",
             arguments={"card_id": card["card_id"],
                        "move": {"intent": "into", "target_card_id": parent["card_id"]}})
        moved = result(process, 9)["structuredContent"]["data"]
        assert moved["parent_card_id"] == parent["card_id"]
        assert rest(path) == moved
        link = {"target_type": "card", "target_id": parent["card_id"]}
        foreign = {**link, "target_project_id": "other"}
        rest(f"{path}/links", foreign)
        for request_id, name, arguments in (
            (10, "add_card_link", {"link": foreign}),
            (11, "remove_card_link", foreign),
        ):
            send(process, "tools/call", request_id, _meta=metadata, name=name,
                 arguments={"card_id": card["card_id"], **arguments})
            denied = result(process, request_id)
            assert denied["isError"] is True
            assert denied["content"][0]["text"] == "MCP links must stay inside the authorized project"

        send(process, "tools/call", 12, _meta=metadata, name="add_card_link",
             arguments={"card_id": card["card_id"], "link": link})
        added = result(process, 12)["structuredContent"]["data"]
        assert added["target_project_id"] == PROJECT and added["target_id"] == parent["card_id"]
        send(process, "tools/call", 13, _meta=metadata, name="remove_card_link",
             arguments={"card_id": card["card_id"], **link})
        assert result(process, 13)["structuredContent"]["data"] == []
        assert [item["target_project_id"] for item in rest(f"{path}/links")] == ["other"]

        send(process, "tools/call", 14, _meta=metadata, name="remove_card_milestone",
             arguments={"card_id": card["card_id"], "milestone_id": milestone["milestone_id"]})
        assert result(process, 14)["structuredContent"]["data"] == []
        assert rest(f"{path}/milestones") == []
        send(process, "tools/call", "subtree", _meta=metadata, name="trash_card",
             arguments={"card_id": parent["card_id"], "children": "trash_subtree"})
        assert result(process, "subtree")["isError"] is True
        assert rest(path)["parent_card_id"] == parent["card_id"]

        send(process, "tools/call", 15, _meta=metadata, name="trash_card",
             arguments={"card_id": parent["card_id"]})
        trashed = result(process, 15)["structuredContent"]["data"]
        assert trashed["card_id"] == parent["card_id"] and trashed["deleted_at"] is not None
        assert rest(path)["parent_card_id"] is None

        send(process, "tools/call", 16, _meta=metadata, name="list_trash", arguments={})
        items = result(process, 16)["structuredContent"]["data"]
        assert [(item["type"], item["card_id"]) for item in items] == [("card", parent["card_id"])]

        send(process, "tools/call", 17, _meta=metadata, name="restore_card",
             arguments={"card_id": parent["card_id"]})
        restored = result(process, 17)["structuredContent"]["data"]
        assert restored["deleted_at"] is None
        assert rest(f"/projects/{PROJECT}/cards/{parent['card_id']}") == restored
        assert rest(path)["parent_card_id"] is None
        assert rest(f"/trash?project_id={PROJECT}&type=card") == []
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
                    # Closing the cancelled connection can yield EOF or a TCP reset.
                    with contextlib.suppress(ConnectionResetError):
                        assert reader.read(1) == b""
                if cancel_with_notification:
                    send(process, "ping", "after-cancel")
                    assert result(process, "after-cancel") == {}
                shutdown(process)


def test_daemon(root, *, allow_writes):
    root.mkdir()
    env = dict(os.environ, XDG_DATA_HOME=str(root / "data"), XDG_CONFIG_HOME=str(root / "config"),
               XDG_CACHE_HOME=str(root / "cache"), HOLDER_PROJECTS_ROOT=str(root / "projects"),
               HOLDER_TEST_KEYSTORE_DIR=str(root / "keystore"),
               HOLDER_MCP_PROJECT=PROJECT, HOLDER_MCP_TOKEN=secrets.token_urlsafe(32))
    env.pop("HOLDER_MCP_ALLOW_WRITES", None)
    if allow_writes:
        env["HOLDER_MCP_ALLOW_WRITES"] = "1"
    log_path = root / "daemon.stderr.log"
    with log_path.open("wb") as log, subprocess.Popen(
        [str(DAEMON), "--port", "0"], cwd=DAEMON.parent, env=env,
        stdout=subprocess.DEVNULL, stderr=log,
    ) as daemon:
        try:
            info_path = root / "data/holder/server/holder.json"
            for _ in range(200):
                if (exit_code := daemon.poll()) is not None:
                    raise AssertionError(
                        f"holderd exited during startup with status {exit_code}:\n"
                        + log_path.read_text(encoding="utf-8", errors="replace")
                    )
                if info_path.is_file():
                    break
                time.sleep(0.05)
            else:
                raise AssertionError(
                    "holderd did not publish its server info within 10 seconds:\n"
                    + log_path.read_text(encoding="utf-8", errors="replace")
                )
            info = json.loads(info_path.read_text(encoding="utf-8"))
            port = info["port"]

            def rest(path, body=None):
                payload = None if body is None else json.dumps(body).encode()
                request = Request(f"http://127.0.0.1:{port}{path}", payload, headers={
                    "Authorization": f"Bearer {info['auth_token']}", "Content-Type": "application/json",
                })
                with urlopen(request, timeout=5) as response:
                    return json.load(response)["data"]

            rest("/projects", {"name": "MCP test", "project_id": PROJECT, "privacy_mode": "plain"})
            if allow_writes:
                test_write_tools(port, env, rest)
            else:
                test_available_daemon(port, env, rest(f"/projects/{PROJECT}/cards", {"title": "Bridge test"}))
        finally:
            if daemon.poll() is None:
                daemon.terminate()
            daemon.wait(timeout=10)
    with bridge(port, env) as process:
        send(process, "server/discover", 1, _meta={
            "io.modelcontextprotocol/protocolVersion": VERSION,
            "io.modelcontextprotocol/clientCapabilities": {},
        })
        assert receive(process)["error"]["code"] == -32000
        shutdown(process)
    if not allow_writes:
        test_cancellation(env)


with tempfile.TemporaryDirectory(prefix="holder-mcp-test-") as temp:
    test_daemon(Path(temp) / "read", allow_writes=False)
    test_daemon(Path(temp) / "write", allow_writes=True)
