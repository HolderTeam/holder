"""Tests for mock_github.py. Run: python3 -m unittest test_mock_github (from this directory)."""

import base64
import hashlib
import http.client
import json
import threading
import unittest
import urllib.parse

import mock_github


def challenge_for(verifier: str) -> str:
    return base64.urlsafe_b64encode(hashlib.sha256(verifier.encode()).digest()).rstrip(b"=").decode()


class MockGitHubTest(unittest.TestCase):
    def setUp(self):
        self.state = mock_github.MockState()
        self.server = mock_github.make_server(self.state, port=0)
        self.port = self.server.server_address[1]
        threading.Thread(target=self.server.serve_forever, daemon=True).start()

    def tearDown(self):
        self.server.shutdown()
        self.server.server_close()

    def request(self, method, path, body=None, headers=None, raw=None):
        connection = http.client.HTTPConnection("127.0.0.1", self.port, timeout=5)
        payload = raw if raw is not None else (json.dumps(body).encode() if body is not None else None)
        connection.request(method, path, body=payload, headers=headers or {})
        response = connection.getresponse()
        data = response.read()
        connection.close()
        return response.status, response.getheader("Location"), data

    def relay(self, endpoint, body):
        status, _, data = self.request("POST", f"/v1/github/{endpoint}", body,
                                       {"Content-Type": "application/json; charset=UTF-8"})
        return status, data

    def api(self, method, path, token, body=None):
        status, _, data = self.request(method, "/api" + path, body, {"Authorization": f"Bearer {token}"})
        return status, json.loads(data)

    def sign_in(self, verifier="v" * 43):
        query = urllib.parse.urlencode({
            "client_id": "Iv23-test", "redirect_uri": mock_github.DEFAULT_CALLBACK, "state": "s1",
            "code_challenge": challenge_for(verifier), "code_challenge_method": "S256",
        })
        status, _, page = self.request("GET", f"/login/oauth/authorize?{query}")
        self.assertEqual(200, status)
        self.assertIn(b"Authorize", page)
        form = urllib.parse.urlencode({"state": "s1", "code_challenge": challenge_for(verifier), "decision": "approve"})
        status, location, _ = self.request("POST", "/login/oauth/authorize/decide", raw=form.encode(),
                                           headers={"Content-Type": "application/x-www-form-urlencoded"})
        self.assertEqual(302, status)
        callback = urllib.parse.urlsplit(location)
        self.assertEqual(mock_github.DEFAULT_CALLBACK, f"{callback.scheme}://{callback.netloc}{callback.path}")
        params = urllib.parse.parse_qs(callback.query)
        self.assertEqual(["s1"], params["state"])
        status, data = self.relay("exchange", {"code": params["code"][0], "code_verifier": verifier})
        self.assertEqual(200, status, data)
        return params["code"][0], json.loads(data)

    def test_sign_in_returns_exactly_the_relay_token_fields(self):
        _, tokens = self.sign_in()
        self.assertEqual({"access_token", "expires_in", "refresh_token", "refresh_cap", "refresh_token_expires_in"},
                         set(tokens))
        self.assertEqual(mock_github.DEFAULT_ACCESS_TTL, tokens["expires_in"])

    def test_code_is_single_use_and_bound_to_its_pkce_challenge(self):
        code, _ = self.sign_in()
        status, data = self.relay("exchange", {"code": code, "code_verifier": "v" * 43})
        self.assertEqual((400, {"error": "authorization_required", "reason": "reconnect"}), (status, json.loads(data)))
        wrong = self.state.issue_code(challenge_for("right"))
        status, _ = self.relay("exchange", {"code": wrong, "code_verifier": "wrong"})
        self.assertEqual(400, status)

    def test_deny_returns_access_denied_to_the_callback(self):
        form = urllib.parse.urlencode({"state": "s1", "code_challenge": "c", "decision": "deny"})
        _, location, _ = self.request("POST", "/login/oauth/authorize/decide", raw=form.encode(),
                                      headers={"Content-Type": "application/x-www-form-urlencoded"})
        self.assertEqual({"error": ["access_denied"], "state": ["s1"]},
                         urllib.parse.parse_qs(urllib.parse.urlsplit(location).query))

    def test_wrong_redirect_uri_is_refused(self):
        query = urllib.parse.urlencode({"client_id": "x", "redirect_uri": "https://evil.example/cb", "state": "s",
                                        "code_challenge": "c", "code_challenge_method": "S256"})
        status, _, _ = self.request("GET", f"/login/oauth/authorize?{query}")
        self.assertEqual(400, status)

    def test_refresh_rotates_and_a_replay_is_counted(self):
        _, tokens = self.sign_in()
        old = {"refresh_token": tokens["refresh_token"], "refresh_cap": tokens["refresh_cap"]}
        status, data = self.relay("refresh", old)
        self.assertEqual(200, status)
        self.assertNotEqual(tokens["refresh_token"], json.loads(data)["refresh_token"])
        status, data = self.relay("refresh", old)
        self.assertEqual((400, "reconnect"), (status, json.loads(data)["reason"]))
        self.assertEqual(1, self.state.replays)

    def test_refresh_with_a_wrong_cap_is_rejected_without_spending(self):
        _, tokens = self.sign_in()
        status, _ = self.relay("refresh", {"refresh_token": tokens["refresh_token"], "refresh_cap": "forged"})
        self.assertEqual(400, status)
        status, _ = self.relay("refresh", {"refresh_token": tokens["refresh_token"], "refresh_cap": tokens["refresh_cap"]})
        self.assertEqual(200, status)

    def test_relay_rejects_extra_fields_and_non_json_content_type(self):
        status, data = self.relay("refresh", {"refresh_token": "a", "refresh_cap": "b", "extra": "c"})
        self.assertEqual((400, {"error": "invalid_request"}), (status, json.loads(data)))
        status, _, _ = self.request("POST", "/v1/github/refresh", {"refresh_token": "a", "refresh_cap": "b"},
                                    {"Content-Type": "text/plain"})
        self.assertEqual(415, status)

    def test_after_faults_spend_the_refresh_token(self):
        for kind, expected_status in [("garbled", 200), ("partial", 200), ("outcome_unknown", 503),
                                      ("html", 502), ("oversize", 200)]:
            with self.subTest(kind=kind):
                _, tokens = self.sign_in()
                body = {"refresh_token": tokens["refresh_token"], "refresh_cap": tokens["refresh_cap"]}
                self.state.add_fault("refresh", kind)
                status, data = self.relay("refresh", body)
                self.assertEqual(expected_status, status)
                if kind == "garbled":
                    self.assertEqual(b"{broken", data)
                if kind == "partial":
                    self.assertNotIn("refresh_cap", json.loads(data))
                # The token was spent, so sending it again is a replay.
                replays = self.state.replays
                self.relay("refresh", body)
                self.assertEqual(replays + 1, self.state.replays)

    def test_before_faults_leave_the_refresh_token_usable(self):
        for kind, expected in [("unexpected", (500, {"error": "unexpected"})),
                               ("rate_limited", (429, {"error": "rate_limited", "retry_after_seconds": 30}))]:
            with self.subTest(kind=kind):
                _, tokens = self.sign_in()
                body = {"refresh_token": tokens["refresh_token"], "refresh_cap": tokens["refresh_cap"]}
                self.state.add_fault("refresh", kind)
                status, data = self.relay("refresh", body)
                self.assertEqual(expected, (status, json.loads(data)))
                self.assertEqual(200, self.relay("refresh", body)[0])

    def test_drop_closes_the_connection_without_a_reply(self):
        _, tokens = self.sign_in()
        self.state.add_fault("refresh", "drop")
        with self.assertRaises((http.client.RemoteDisconnected, ConnectionResetError)):
            self.relay("refresh", {"refresh_token": tokens["refresh_token"], "refresh_cap": tokens["refresh_cap"]})

    def test_api_needs_a_live_access_token(self):
        _, tokens = self.sign_in()
        access = tokens["access_token"]
        self.assertEqual((200, {"id": 4242, "login": "mock-user"}), self.api("GET", "/user", access))
        self.state.expire_access_tokens()
        self.assertEqual(401, self.api("GET", "/user", access)[0])
        _, tokens = self.sign_in()
        self.state.revoke_all()
        self.assertEqual(401, self.api("GET", "/user", tokens["access_token"])[0])
        refresh = {"refresh_token": tokens["refresh_token"], "refresh_cap": tokens["refresh_cap"]}
        self.assertEqual(400, self.relay("refresh", refresh)[0])

    def test_installation_and_repository_provisioning(self):
        _, tokens = self.sign_in()
        access = tokens["access_token"]
        status, value = self.api("GET", "/user/installations", access)
        self.assertEqual("User", value["installations"][0]["account"]["type"])
        status, repo = self.api("POST", "/user/repos", access, {"name": "holder-x", "private": True, "description": "X"})
        self.assertEqual((201, "mock-user", "holder-x"), (status, repo["owner"]["login"], repo["name"]))
        status, value = self.api("POST", "/user/repos", access, {"name": "holder-x"})
        self.assertIn("name already exists", json.dumps(value))
        key = {"title": "phone", "key": "ssh-ed25519 AAAAkey holder@phone", "read_only": False}
        self.assertEqual(201, self.api("POST", "/repos/mock-user/holder-x/keys", access, key)[0])
        status, value = self.api("POST", "/repos/mock-user/holder-x/keys", access, key)
        self.assertIn("key is already in use", json.dumps(value))
        status, keys = self.api("GET", "/repos/mock-user/holder-x/keys", access)
        self.assertEqual("ssh-ed25519 AAAAkey", keys[0]["key"])
        self.assertEqual(404, self.api("GET", "/repos/mock-user/missing", access)[0])
        self.state.update_settings({"installed": "false"})
        self.assertEqual([], self.api("GET", "/user/installations", access)[1]["installations"])

    def test_control_api_and_page(self):
        status, _, _ = self.request("POST", "/_mock/fault", {"endpoint": "refresh", "kind": "garbled"},
                                    {"Content-Type": "application/json"})
        self.assertEqual(200, status)
        status, _, _ = self.request("POST", "/_mock/fault", {"endpoint": "refresh", "kind": "nonsense"},
                                    {"Content-Type": "application/json"})
        self.assertEqual(400, status)
        status, _, data = self.request("GET", "/_mock/state")
        self.assertEqual("garbled", json.loads(data)["faults"]["refresh"][0]["kind"])
        status, _, page = self.request("GET", "/")
        self.assertEqual(200, status)
        self.assertIn(b"Mock GitHub", page)

    def test_tokens_never_appear_whole_in_the_state(self):
        _, tokens = self.sign_in()
        _, _, data = self.request("GET", "/_mock/state")
        self.assertNotIn(tokens["access_token"].encode(), data)
        self.assertNotIn(tokens["refresh_token"].encode(), data)


if __name__ == "__main__":
    unittest.main()
