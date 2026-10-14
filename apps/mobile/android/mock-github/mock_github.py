#!/usr/bin/env python3
"""A local stand-in for GitHub and Holder's sign-in relay, for testing the Android app.

It serves, on one loopback port:

* GitHub's authorization page (/login/oauth/authorize), with Authorize and Deny buttons,
  which sends the browser back to the app's real OAuth callback App Link;
* the relay's /v1/github/exchange and /v1/github/refresh, in the format README.md describes;
* the few GitHub REST calls the app makes, under /api;
* a control page at / and a JSON control API under /_mock/, for injecting faults
  (garbled replies, server errors, dropped connections, delays), expiring or revoking
  tokens, and watching every request, including any replay of a spent refresh token.

Standard library only. Run `mock_github.py --help` for the commands.
"""

from __future__ import annotations

import argparse
import base64
import collections
import hashlib
import hmac
import html
import json
import secrets
import socket
import sys
import threading
import time
import urllib.error
import urllib.parse
import urllib.request
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

DEFAULT_PORT = 8765
DEFAULT_CALLBACK = "https://auth-dev.holder.ws/android/oauth-callback"

# GitHub's own lifetimes for GitHub App user tokens.
DEFAULT_ACCESS_TTL = 8 * 60 * 60
DEFAULT_REFRESH_TTL = 184 * 24 * 60 * 60
CODE_TTL = 10 * 60
MAX_REQUEST_BODY_BYTES = 4096

# Faults the relay endpoints accept. "Before" faults leave the credential unspent, as the
# relay's contract promises for those replies. "After" faults spend it first (the code is
# used, or the refresh token rotated) and then lose or garble the reply, which is what the
# app has to treat as ambiguous.
RELAY_FAULTS_BEFORE = {
    "unexpected": "500 unexpected: the relay failed before spending anything",
    "rate_limited": "429 rate_limited, retry after 30 seconds",
    "invalid_request": "400 invalid_request",
    "reconnect": "400 authorization_required/reconnect: GitHub rejected the credential",
    "verify_email": "400 authorization_required/verify_email",
}
RELAY_FAULTS_AFTER = {
    "garbled": "spend, then reply 200 with a body that is not JSON",
    "partial": "spend, then reply 200 with tokens but no refresh_cap",
    "outcome_unknown": "spend, then reply 503 outcome_unknown",
    "html": "spend, then reply 502 with an HTML error page",
    "oversize": "spend, then reply 200 with a 20 KiB body",
    "drop": "spend, then close the connection without replying",
}
API_FAULTS = {
    "unauthorized": "401 Bad credentials",
    "server_error": "500 from GitHub",
    "rate_limited": "429 with retry_after 30",
    "drop": "close the connection without replying",
}
# Every endpoint also takes "delay": wait, then behave normally.
FAULTS = {
    "exchange": {**RELAY_FAULTS_BEFORE, **RELAY_FAULTS_AFTER},
    "refresh": {**RELAY_FAULTS_BEFORE, **RELAY_FAULTS_AFTER},
    "api": API_FAULTS,
}
DEFAULT_DELAY_SECONDS = 20


def _b64url(data: bytes) -> str:
    return base64.urlsafe_b64encode(data).rstrip(b"=").decode("ascii")


def _token(prefix: str, length: int) -> str:
    alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"
    return prefix + "".join(secrets.choice(alphabet) for _ in range(length))


def _short(token: str) -> str:
    """Enough of a token to tell them apart on the control page, never the whole thing."""
    return token[:8] + "…" + token[-4:]


class _Dropped(Exception):
    """Raised to close the connection without sending a reply."""


class MockState:
    """Everything the mock remembers. Every method takes the lock."""

    def __init__(self, callback: str = DEFAULT_CALLBACK, auto_approve: bool = False,
                 access_ttl: int = DEFAULT_ACCESS_TTL, refresh_ttl: int = DEFAULT_REFRESH_TTL,
                 clock=time.time):
        self.lock = threading.RLock()
        self.clock = clock
        self.callback = callback
        self.cap_key = secrets.token_bytes(32)
        self.initial_settings = {
            "access_ttl": access_ttl,
            "refresh_ttl": refresh_ttl,
            "installed": True,
            "auto_approve": auto_approve,
        }
        self.settings = dict(self.initial_settings)
        self.user = {"id": 4242, "login": "mock-user"}
        self.installation_id = 777
        self.codes: dict[str, dict] = {}
        self.access: dict[str, dict] = {}
        self.refresh: dict[str, dict] = {}
        self.repos: dict[tuple[str, str], dict] = {}
        self.faults: dict[str, collections.deque] = {name: collections.deque() for name in FAULTS}
        self.events: collections.deque = collections.deque(maxlen=300)
        self.replays = 0

    # Events -----------------------------------------------------------------------------

    def log(self, endpoint: str, outcome: str, warn: bool = False) -> None:
        with self.lock:
            self.events.append({
                "time": time.strftime("%H:%M:%S", time.localtime(self.clock())),
                "endpoint": endpoint,
                "outcome": outcome,
                "warn": warn,
            })
        print(f"[mock-github] {'!! ' if warn else ''}{endpoint}: {outcome}", flush=True)

    # Faults -------------------------------------------------------------------------------

    def add_fault(self, endpoint: str, kind: str, seconds: int | None = None) -> None:
        if endpoint not in FAULTS:
            raise ValueError(f"unknown endpoint {endpoint!r}; one of {', '.join(FAULTS)}")
        if kind != "delay" and kind not in FAULTS[endpoint]:
            raise ValueError(f"unknown fault {kind!r} for {endpoint}; one of delay, {', '.join(FAULTS[endpoint])}")
        with self.lock:
            self.faults[endpoint].append({"kind": kind, "seconds": seconds or DEFAULT_DELAY_SECONDS})

    def take_fault(self, endpoint: str) -> dict | None:
        with self.lock:
            return self.faults[endpoint].popleft() if self.faults[endpoint] else None

    # Settings and bulk token actions ----------------------------------------------------

    def update_settings(self, changes: dict) -> None:
        with self.lock:
            for key, value in changes.items():
                if key in ("access_ttl", "refresh_ttl"):
                    value = int(value)
                    if value <= 0:
                        raise ValueError(f"{key} must be positive")
                elif key in ("installed", "auto_approve"):
                    if isinstance(value, str):
                        value = value.lower() in ("1", "true", "yes", "on")
                    value = bool(value)
                else:
                    raise ValueError(f"unknown setting {key!r}")
                self.settings[key] = value

    def expire_access_tokens(self) -> int:
        """As if eight hours had passed: the next request needs a refresh."""
        with self.lock:
            now = self.clock()
            live = [t for t in self.access.values() if t["expires"] > now and not t["revoked"]]
            for record in live:
                record["expires"] = now
            return len(live)

    def reset(self) -> None:
        """Back to the state the mock started in: no codes, tokens, repositories or faults."""
        with self.lock:
            self.settings = dict(self.initial_settings)
            for collection in (self.codes, self.access, self.refresh, self.repos, self.events):
                collection.clear()
            for queue in self.faults.values():
                queue.clear()
            self.replays = 0

    def revoke_all(self) -> None:
        """As if the user revoked the app on github.com."""
        with self.lock:
            for record in self.access.values():
                record["revoked"] = True
            for record in self.refresh.values():
                if record["state"] == "live":
                    record["state"] = "revoked"

    # Credentials ------------------------------------------------------------------------

    def refresh_cap(self, refresh_token: str) -> str:
        return _b64url(hmac.new(self.cap_key, refresh_token.encode(), hashlib.sha256).digest())

    def issue_code(self, challenge: str) -> str:
        code = secrets.token_hex(10)
        with self.lock:
            self.codes[code] = {"challenge": challenge, "expires": self.clock() + CODE_TTL, "used": False}
        return code

    def issue_tokens(self) -> dict:
        with self.lock:
            now = self.clock()
            access = _token("ghu_", 36)
            refresh = _token("ghr_", 76)
            self.access[access] = {"expires": now + self.settings["access_ttl"], "revoked": False}
            self.refresh[refresh] = {"expires": now + self.settings["refresh_ttl"], "state": "live"}
            return {
                "access_token": access,
                "expires_in": self.settings["access_ttl"],
                "refresh_token": refresh,
                "refresh_cap": self.refresh_cap(refresh),
                "refresh_token_expires_in": self.settings["refresh_ttl"],
            }

    def spend_code(self, code: str, verifier: str) -> str | None:
        """Returns None if the code is good (and now used), else why not."""
        with self.lock:
            record = self.codes.get(code)
            if record is None:
                return "unknown code"
            if record["used"]:
                return "code already used"
            record["used"] = True
            if record["expires"] < self.clock():
                return "code expired"
            if _b64url(hashlib.sha256(verifier.encode()).digest()) != record["challenge"]:
                return "PKCE verifier does not match the challenge"
            return None

    def spend_refresh_token(self, token: str) -> str | None:
        """Returns None if the refresh token is good (and now spent), else why not."""
        with self.lock:
            record = self.refresh.get(token)
            if record is None:
                return "unknown refresh token"
            if record["state"] == "spent":
                self.replays += 1
                return "REPLAY: this refresh token was already spent"
            if record["state"] == "revoked":
                return "refresh token revoked"
            if record["expires"] < self.clock():
                return "refresh token expired"
            record["state"] = "spent"
            return None

    def reject_refresh_token(self, token: str) -> None:
        with self.lock:
            record = self.refresh.get(token)
            if record is not None and record["state"] == "live":
                record["state"] = "revoked"

    def check_access_token(self, token: str) -> str | None:
        with self.lock:
            record = self.access.get(token)
            if record is None:
                return "unknown access token"
            if record["revoked"]:
                return "access token revoked"
            if record["expires"] <= self.clock():
                return "access token expired"
            return None

    def snapshot(self) -> dict:
        with self.lock:
            now = self.clock()
            return {
                "settings": dict(self.settings),
                "callback": self.callback,
                "user": dict(self.user),
                "faults": {name: list(queue) for name, queue in self.faults.items()},
                "replays": self.replays,
                "access_tokens": [
                    {"token": _short(t), "expires_in": max(0, int(r["expires"] - now)), "revoked": r["revoked"]}
                    for t, r in self.access.items()
                ],
                "refresh_tokens": [
                    {"token": _short(t), "state": r["state"], "expires_in": max(0, int(r["expires"] - now))}
                    for t, r in self.refresh.items()
                ],
                "repos": [
                    {"name": f"{owner}/{name}", "keys": len(repo["keys"])}
                    for (owner, name), repo in self.repos.items()
                ],
                "events": list(self.events),
            }


def _relay_error(error: str, **fields) -> tuple[int, dict]:
    status = {"invalid_request": 400, "authorization_required": 400, "rate_limited": 429,
              "outcome_unknown": 503, "unexpected": 500}[error]
    return status, {"error": error, **fields}


class Handler(BaseHTTPRequestHandler):
    server_version = "mock-github"
    protocol_version = "HTTP/1.1"
    state: MockState  # set on the subclass made by make_server

    # Plumbing -------------------------------------------------------------------------------

    def log_message(self, format, *args):  # noqa: A002 - BaseHTTPRequestHandler's name
        pass  # MockState.log reports what matters

    def _body(self) -> bytes:
        length = int(self.headers.get("Content-Length") or 0)
        return self.rfile.read(length) if length else b""

    def _send(self, status: int, body: bytes, content_type: str, headers: dict | None = None) -> None:
        self.send_response(status)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        for name, value in (headers or {}).items():
            self.send_header(name, value)
        self.end_headers()
        self.wfile.write(body)

    def _json(self, status: int, value, headers: dict | None = None) -> None:
        self._send(status, json.dumps(value).encode(), "application/json; charset=utf-8", headers)

    def _html(self, status: int, page: str) -> None:
        self._send(status, page.encode(), "text/html; charset=utf-8")

    def _redirect(self, location: str, status: int = 302) -> None:
        self._send(status, b"", "text/plain", {"Location": location})

    def _drop(self) -> None:
        self.close_connection = True
        try:
            self.connection.shutdown(socket.SHUT_RDWR)
        except OSError:
            pass
        raise _Dropped()

    def _route(self, method: str) -> None:
        url = urllib.parse.urlsplit(self.path)
        try:
            if url.path == "/" and method == "GET":
                return self._html(200, control_page(self.state))
            if url.path.startswith("/_mock/"):
                return self._control(method, url.path[len("/_mock/"):], url.query)
            if url.path == "/login/oauth/authorize" and method == "GET":
                return self._authorize(urllib.parse.parse_qs(url.query))
            if url.path == "/login/oauth/authorize/decide" and method == "POST":
                form = urllib.parse.parse_qs(self._body().decode())
                return self._decide(form)
            if url.path in ("/v1/github/exchange", "/v1/github/refresh") and method == "POST":
                return self._relay(url.path.rsplit("/", 1)[1])
            if url.path.startswith("/api/"):
                return self._api(method, url.path[len("/api"):])
            self._json(404, {"message": "Not Found"})
        except _Dropped:
            pass

    def do_GET(self):  # noqa: N802 - BaseHTTPRequestHandler's name
        self._route("GET")

    def do_POST(self):  # noqa: N802
        self._route("POST")

    # GitHub's authorization page ------------------------------------------------------------

    def _authorize(self, query: dict) -> None:
        params = {key: values[0] for key, values in query.items() if len(values) == 1}
        problems = []
        if not params.get("client_id"):
            problems.append("client_id is missing")
        if params.get("redirect_uri") != self.state.callback:
            problems.append(f"redirect_uri is {params.get('redirect_uri')!r}, expected {self.state.callback!r}")
        if not params.get("state"):
            problems.append("state is missing")
        if not params.get("code_challenge") or params.get("code_challenge_method") != "S256":
            problems.append("an S256 code_challenge is required")
        if problems:
            self.state.log("authorize", "refused: " + "; ".join(problems), warn=True)
            return self._html(400, page("Bad authorization request", "".join(
                f"<p>{html.escape(p)}</p>" for p in problems)))
        self.state.log("authorize", "sign-in page shown")
        if self.state.settings["auto_approve"]:
            return self._finish_authorization(params["state"], params["code_challenge"], approve=True)
        hidden = "".join(
            f'<input type="hidden" name="{name}" value="{html.escape(params[name])}">'
            for name in ("state", "code_challenge"))
        self._html(200, page("Authorize Holder (mock)", f"""
            <p><b>Mock GitHub.</b> Signing in as <b>{html.escape(self.state.user['login'])}</b>.
               Nothing here talks to the real GitHub.</p>
            <form method="post" action="/login/oauth/authorize/decide">{hidden}
              <button name="decision" value="approve" class="primary">Authorize</button>
              <button name="decision" value="deny">Deny</button>
            </form>"""))

    def _decide(self, form: dict) -> None:
        state = (form.get("state") or [""])[0]
        challenge = (form.get("code_challenge") or [""])[0]
        approve = (form.get("decision") or [""])[0] == "approve"
        self._finish_authorization(state, challenge, approve)

    def _finish_authorization(self, state: str, challenge: str, approve: bool) -> None:
        if approve:
            code = self.state.issue_code(challenge)
            self.state.log("authorize", "approved, returning a code to the app")
            query = {"code": code, "state": state}
        else:
            self.state.log("authorize", "denied, returning access_denied to the app")
            query = {"error": "access_denied", "state": state}
        self._redirect(self.state.callback + "?" + urllib.parse.urlencode(query))

    # The relay ----------------------------------------------------------------------------

    def _relay(self, endpoint: str) -> None:
        raw = self._body()
        fault = self.state.take_fault(endpoint)
        if fault and fault["kind"] == "delay":
            self.state.log(endpoint, f"delaying {fault['seconds']}s")
            time.sleep(fault["seconds"])
            fault = None

        content_type = (self.headers.get("Content-Type") or "").split(";")[0].strip().lower()
        if content_type != "application/json":
            self.state.log(endpoint, f"415: Content-Type {self.headers.get('Content-Type')!r}", warn=True)
            return self._send(415, b"", "text/plain")
        fields = ("code", "code_verifier") if endpoint == "exchange" else ("refresh_token", "refresh_cap")
        try:
            body = json.loads(raw) if len(raw) <= MAX_REQUEST_BODY_BYTES else None
        except ValueError:
            body = None
        if (not isinstance(body, dict) or sorted(body) != sorted(fields)
                or not all(isinstance(body[f], str) and body[f] for f in fields)):
            self.state.log(endpoint, "400 invalid_request: body is not exactly " + ", ".join(fields), warn=True)
            return self._json(*_relay_error("invalid_request"))

        if fault and fault["kind"] in RELAY_FAULTS_BEFORE:
            return self._relay_fault_before(endpoint, fault["kind"], body)

        if endpoint == "exchange":
            problem = self.state.spend_code(body["code"], body["code_verifier"])
        elif not hmac.compare_digest(self.state.refresh_cap(body["refresh_token"]), body["refresh_cap"]):
            self.state.log(endpoint, "400 reconnect: refresh_cap does not match the refresh token", warn=True)
            return self._json(*_relay_error("authorization_required", reason="reconnect"))
        else:
            problem = self.state.spend_refresh_token(body["refresh_token"])
        if problem:
            self.state.log(endpoint, f"400 reconnect: {problem}", warn=problem.startswith("REPLAY"))
            return self._json(*_relay_error("authorization_required", reason="reconnect"))

        tokens = self.state.issue_tokens()
        if fault:
            return self._relay_fault_after(endpoint, fault["kind"], tokens)
        self.state.log(endpoint, f"200: issued {_short(tokens['access_token'])}, refresh {_short(tokens['refresh_token'])}")
        self._json(200, tokens)

    def _relay_fault_before(self, endpoint: str, kind: str, body: dict) -> None:
        self.state.log(endpoint, f"fault {kind}: {RELAY_FAULTS_BEFORE[kind]}", warn=True)
        if kind == "unexpected":
            return self._json(*_relay_error("unexpected"))
        if kind == "rate_limited":
            status, error = _relay_error("rate_limited", retry_after_seconds=30)
            return self._json(status, error, {"Retry-After": "30"})
        if kind == "invalid_request":
            return self._json(*_relay_error("invalid_request"))
        if kind == "reconnect" and endpoint == "refresh":
            self.state.reject_refresh_token(body["refresh_token"])
        reason = "verify_email" if kind == "verify_email" else "reconnect"
        self._json(*_relay_error("authorization_required", reason=reason))

    def _relay_fault_after(self, endpoint: str, kind: str, tokens: dict) -> None:
        self.state.log(endpoint, f"fault {kind}: {RELAY_FAULTS_AFTER[kind]}; the new tokens are lost", warn=True)
        if kind == "garbled":
            return self._send(200, b"{broken", "application/json")
        if kind == "partial":
            partial = {k: v for k, v in tokens.items() if k != "refresh_cap"}
            return self._json(200, partial)
        if kind == "outcome_unknown":
            return self._json(*_relay_error("outcome_unknown"))
        if kind == "html":
            return self._html(502, "<html><body><h1>502 Bad Gateway</h1></body></html>")
        if kind == "oversize":
            return self._json(200, {**tokens, "padding": "x" * 20 * 1024})
        self._drop()

    # GitHub's REST API --------------------------------------------------------------------

    def _api(self, method: str, path: str) -> None:
        raw = self._body()
        where = f"{method} {path}"
        fault = self.state.take_fault("api")
        if fault and fault["kind"] == "delay":
            self.state.log("api", f"{where}: delaying {fault['seconds']}s")
            time.sleep(fault["seconds"])
            fault = None
        if fault:
            self.state.log("api", f"{where}: fault {fault['kind']}: {API_FAULTS[fault['kind']]}", warn=True)
            if fault["kind"] == "drop":
                self._drop()
            status = {"unauthorized": 401, "server_error": 500, "rate_limited": 429}[fault["kind"]]
            return self._json(status, {"message": API_FAULTS[fault["kind"]], "retry_after": 30})

        auth = self.headers.get("Authorization") or ""
        problem = (self.state.check_access_token(auth[len("Bearer "):]) if auth.startswith("Bearer ")
                   else "no bearer token")
        if problem:
            self.state.log("api", f"{where}: 401 ({problem})", warn=True)
            return self._json(401, {"message": "Bad credentials", "documentation_url": "https://docs.github.com/rest"})

        status, value = self._api_route(method, path.strip("/").split("/"), raw)
        self.state.log("api", f"{where}: {status}")
        self._json(status, value)

    def _api_route(self, method: str, parts: list[str], raw: bytes) -> tuple[int, object]:
        state = self.state
        login = state.user["login"]
        not_found = (404, {"message": "Not Found"})
        if method == "GET" and parts == ["user"]:
            return 200, dict(state.user)
        if method == "GET" and parts == ["user", "installations"]:
            installations = [{
                "id": state.installation_id,
                "account": {"id": state.user["id"], "login": login, "type": "User"},
            }] if state.settings["installed"] else []
            return 200, {"total_count": len(installations), "installations": installations}
        if method == "POST" and parts == ["user", "repos"]:
            if not state.settings["installed"]:
                return 403, {"message": "Resource not accessible by integration"}
            request = _json_object(raw)
            name = request.get("name", "")
            with state.lock:
                if (login, name) in state.repos:
                    return 422, {"message": "Repository creation failed.",
                                 "errors": [{"message": "name already exists on this account"}]}
                state.repos[(login, name)] = {"description": request.get("description", ""), "keys": []}
            return 201, _repo_json(login, name)
        if len(parts) >= 3 and parts[0] == "repos":
            owner, name, rest = parts[1], parts[2], parts[3:]
            with state.lock:
                repo = state.repos.get((owner, name))
                if repo is None or not state.settings["installed"]:
                    return not_found
                if method == "GET" and rest == []:
                    return 200, _repo_json(owner, name)
                if method == "GET" and rest == ["keys"]:
                    return 200, [{"id": i + 1, "key": key, "title": title, "read_only": False}
                                 for i, (key, title) in enumerate(repo["keys"])]
                if method == "POST" and rest == ["keys"]:
                    request = _json_object(raw)
                    key = " ".join(str(request.get("key", "")).split()[:2])
                    in_use = any(key == k for r in state.repos.values() for k, _ in r["keys"])
                    if in_use:
                        return 422, {"message": "Validation Failed", "errors": [{"message": "key is already in use"}]}
                    repo["keys"].append((key, request.get("title", "")))
                    return 201, {"id": len(repo["keys"]), "key": key, "read_only": False}
        return not_found

    # Control ------------------------------------------------------------------------------

    def _control(self, method: str, action: str, query: str) -> None:
        if method == "GET" and action == "state":
            return self._json(200, self.state.snapshot())
        if method != "POST":
            return self._json(405, {"error": "use POST"})
        raw = self._body()
        from_form = (self.headers.get("Content-Type") or "").startswith("application/x-www-form-urlencoded")
        if from_form:
            args = {k: v[0] for k, v in urllib.parse.parse_qs(raw.decode()).items()}
        else:
            args = json.loads(raw or b"{}")
        try:
            if action == "fault":
                seconds = int(args["seconds"]) if args.get("seconds") else None
                self.state.add_fault(args.get("endpoint", ""), args.get("kind", ""), seconds)
                self.state.log("control", f"queued fault {args['kind']} for the next {args['endpoint']} request")
            elif action == "clear-faults":
                with self.state.lock:
                    for queue in self.state.faults.values():
                        queue.clear()
                self.state.log("control", "cleared queued faults")
            elif action == "settings":
                self.state.update_settings(args)
                self.state.log("control", f"settings {args}")
            elif action == "expire":
                count = self.state.expire_access_tokens()
                self.state.log("control", f"expired {count} access token(s); the next request must refresh")
            elif action == "reset":
                self.state.reset()
                self.state.log("control", "reset to the starting state")
            elif action == "revoke":
                self.state.revoke_all()
                self.state.log("control", "revoked every token, as if the app was revoked on github.com")
            else:
                return self._json(404, {"error": f"unknown action {action!r}"})
        except (KeyError, ValueError) as error:
            if from_form:
                return self._html(400, page("Not done", f"<p>{html.escape(str(error))}</p><p><a href='/'>Back</a></p>"))
            return self._json(400, {"error": str(error)})
        if from_form:
            return self._redirect("/", 303)
        self._json(200, {"ok": True})


def _json_object(raw: bytes) -> dict:
    try:
        value = json.loads(raw or b"{}")
    except ValueError:
        return {}
    return value if isinstance(value, dict) else {}


def _repo_json(owner: str, name: str) -> dict:
    return {"name": name, "owner": {"login": owner}, "private": True,
            "ssh_url": f"git@mock-github.invalid:{owner}/{name}.git"}


# Pages ------------------------------------------------------------------------------------

STYLE = """
body { font: 15px/1.4 system-ui, sans-serif; margin: 16px; max-width: 980px; color: #1f2328; background: #fff; }
h1 { font-size: 20px; } h2 { font-size: 16px; margin-top: 24px; }
button { font: inherit; padding: 6px 12px; margin: 2px; border: 1px solid #d0d7de; border-radius: 6px; background: #f6f8fa; cursor: pointer; }
button.primary { background: #1f883d; color: #fff; border-color: #1a7f37; }
table { border-collapse: collapse; width: 100%; } td, th { text-align: left; padding: 3px 8px; border-bottom: 1px solid #eee; font-size: 13px; vertical-align: top; }
tr.warn td { background: #fff1e5; } code { font-size: 13px; } .muted { color: #59636e; }
form.inline { display: inline; }
"""


def page(title: str, body: str) -> str:
    return (f"<!doctype html><html><head><meta charset='utf-8'>"
            f"<meta name='viewport' content='width=device-width, initial-scale=1'>"
            f"<title>{html.escape(title)}</title><style>{STYLE}</style></head>"
            f"<body><h1>{html.escape(title)}</h1>{body}</body></html>")


def _fault_buttons(endpoint: str) -> str:
    buttons = "".join(
        f"<button name='kind' value='{kind}' title='{html.escape(description)}'>{kind}</button>"
        for kind, description in [("delay", f"wait {DEFAULT_DELAY_SECONDS}s, then reply normally"),
                                  *FAULTS[endpoint].items()])
    return (f"<form method='post' action='/_mock/fault'><input type='hidden' name='endpoint' value='{endpoint}'>"
            f"<b>{endpoint}</b> {buttons}</form>")


def control_page(state: MockState) -> str:
    snap = state.snapshot()
    settings = snap["settings"]
    queued = ", ".join(f"{name}: {', '.join(f['kind'] for f in queue)}"
                       for name, queue in snap["faults"].items() if queue) or "none"
    rows = "".join(
        f"<tr class='{'warn' if e['warn'] else ''}'><td>{e['time']}</td><td>{e['endpoint']}</td>"
        f"<td>{html.escape(e['outcome'])}</td></tr>"
        for e in reversed(snap["events"]))
    refresh_rows = "".join(
        f"<tr><td><code>{html.escape(t['token'])}</code></td><td>{t['state']}</td><td>{t['expires_in']}s</td></tr>"
        for t in reversed(snap["refresh_tokens"][-8:]))
    replay = (f"<p style='color:#cf222e'><b>{snap['replays']} replay(s) of a spent refresh token.</b> "
              f"The app sent a refresh token again after it had been used.</p>" if snap["replays"] else "")
    return page("Mock GitHub", f"""
<p class="muted">Signed-in user <b>{html.escape(snap['user']['login'])}</b>. Callback
<code>{html.escape(snap['callback'])}</code>. This page reloads every 3 seconds.</p>
{replay}
<h2>Next request misbehaves</h2>
<p class="muted">Each click queues one fault for the next request to that endpoint. Hover a button for what it does.
Queued: <b>{html.escape(queued)}</b></p>
{_fault_buttons('refresh')}{_fault_buttons('exchange')}{_fault_buttons('api')}
<form method="post" action="/_mock/clear-faults"><button>Clear queued faults</button></form>
<h2>Tokens</h2>
<form class="inline" method="post" action="/_mock/expire"><button>Expire access tokens now</button></form>
<form class="inline" method="post" action="/_mock/revoke"><button>Revoke the app (like github.com)</button></form>
<form class="inline" method="post" action="/_mock/reset"><button>Reset everything</button></form>
<form method="post" action="/_mock/settings">
  Access token lifetime <input name="access_ttl" size="7" value="{settings['access_ttl']}">s
  <button>Set</button> <span class="muted">(the app refreshes 60s early, so under ~90 means a refresh on every request)</span>
</form>
<form method="post" action="/_mock/settings">
  <input type="hidden" name="installed" value="{'false' if settings['installed'] else 'true'}">
  App installed on the account: <b>{'yes' if settings['installed'] else 'no'}</b> <button>Toggle</button>
</form>
<form method="post" action="/_mock/settings">
  <input type="hidden" name="auto_approve" value="{'false' if settings['auto_approve'] else 'true'}">
  Skip the Authorize page: <b>{'yes' if settings['auto_approve'] else 'no'}</b> <button>Toggle</button>
  <span class="muted">(like GitHub for an app already authorized; only works in browsers with Auth Tab,
  because Chrome's Custom Tabs hand the callback to the app only after a tap)</span>
</form>
<table><tr><th>Recent refresh tokens</th><th>state</th><th>expires</th></tr>{refresh_rows}</table>
<h2>Requests</h2>
<table><tr><th>time</th><th>endpoint</th><th>outcome</th></tr>{rows}</table>
<script>setTimeout(() => location.reload(), 3000);</script>
""")


# Running ----------------------------------------------------------------------------------

def make_server(state: MockState, port: int = DEFAULT_PORT) -> ThreadingHTTPServer:
    handler = type("BoundHandler", (Handler,), {"state": state})
    server = ThreadingHTTPServer(("127.0.0.1", port), handler)
    server.daemon_threads = True
    return server


def _control_request(port: int, action: str, args: dict | None = None) -> dict:
    url = f"http://127.0.0.1:{port}/_mock/{action}"
    data = None if args is None else json.dumps(args).encode()
    request = urllib.request.Request(url, data=data, headers={"Content-Type": "application/json"})
    try:
        with urllib.request.urlopen(request, timeout=10) as response:
            return json.loads(response.read())
    except urllib.error.HTTPError as error:
        sys.exit(f"mock-github: {json.loads(error.read()).get('error', error)}")
    except urllib.error.URLError:
        sys.exit(f"mock-github: nothing is listening on port {port}; start it with `mock_github.py serve`")


def main(argv: list[str] | None = None) -> None:
    port = argparse.ArgumentParser(add_help=False)
    port.add_argument("--port", type=int, default=DEFAULT_PORT, help=f"default {DEFAULT_PORT}")
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0], parents=[port])
    commands = parser.add_subparsers(dest="command")

    serve = commands.add_parser("serve", parents=[port], help="run the mock (the default)")
    serve.add_argument("--callback", default=DEFAULT_CALLBACK, help="the app's OAuth callback URL")
    serve.add_argument("--auto-approve", action="store_true",
                       help="skip the Authorize page (needs a browser with Auth Tab)")
    serve.add_argument("--access-ttl", type=int, default=DEFAULT_ACCESS_TTL, help="access token lifetime, seconds")

    fault = commands.add_parser("fault", parents=[port], help="make the next request to an endpoint misbehave")
    fault.add_argument("endpoint", choices=list(FAULTS))
    fault.add_argument("kind", help="delay, or one of: " + "; ".join(
        f"{endpoint}: {', '.join(kinds)}" for endpoint, kinds in FAULTS.items()))
    fault.add_argument("--seconds", type=int, help="for delay")

    commands.add_parser("expire", parents=[port], help="expire every access token, so the next request refreshes")
    commands.add_parser("revoke", parents=[port], help="revoke every token, as if the app was revoked on github.com")
    commands.add_parser("reset", parents=[port], help="forget every code, token, repository and fault")
    setting = commands.add_parser("set", parents=[port], help="change settings, e.g. access_ttl=120 installed=false")
    setting.add_argument("pairs", nargs="+", metavar="key=value")
    commands.add_parser("status", parents=[port], help="print the mock's state as JSON")

    args = parser.parse_args(argv)
    if args.command in (None, "serve"):
        state = MockState(callback=getattr(args, "callback", DEFAULT_CALLBACK),
                          auto_approve=getattr(args, "auto_approve", False),
                          access_ttl=getattr(args, "access_ttl", DEFAULT_ACCESS_TTL))
        server = make_server(state, args.port)
        print(f"[mock-github] listening on http://127.0.0.1:{args.port}/ (control page there)", flush=True)
        try:
            server.serve_forever()
        except KeyboardInterrupt:
            pass
        return
    if args.command == "fault":
        _control_request(args.port, "fault", {"endpoint": args.endpoint, "kind": args.kind, "seconds": args.seconds})
    elif args.command in ("expire", "revoke", "reset"):
        _control_request(args.port, args.command, {})
    elif args.command == "set":
        changes = dict(pair.split("=", 1) for pair in args.pairs)
        _control_request(args.port, "settings", changes)
    elif args.command == "status":
        print(json.dumps(_control_request(args.port, "state"), indent=2))
        return
    print("ok")


if __name__ == "__main__":
    main()
