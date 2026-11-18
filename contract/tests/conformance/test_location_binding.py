"""The migrated location-binding boundary against a real, disposable holderd.

Set HOLDER_DAEMON to a binary built from this checkout. Schema checks use the
declared records; state, preview defaults and secret non-disclosure are independent
behavioral assertions.
"""

import json
import os
from pathlib import Path

import pytest
import requests

from authoring.routes import ErrorResponse
from holder_http import Daemon
from test_support.contract import extract

CONTRACT = Path(__file__).resolve().parents[2]
IR = extract(CONTRACT, "interface")

from interface.locations.models import LocationActionResponse


@pytest.fixture(scope="module")
def location_daemon():
    binary = os.environ.get("HOLDER_DAEMON")
    if not binary:
        pytest.skip("set HOLDER_DAEMON to run the live location-binding boundary checks")
    daemon = Daemon(binary)
    try:
        status, project = daemon.request(
            "POST",
            "/projects",
            json.dumps(
                {
                    "name": "Location boundary conformance",
                    "privacy_mode": "plain",
                }
            ).encode(),
        )
        assert status == 201, project
        status, location = daemon.request(
            "POST",
            "/locations",
            json.dumps(
                {
                    "location_id": "boundary-location",
                    "project_id": project["data"]["project_id"],
                    "name": "Boundary storage",
                    "provider": "local_directory",
                }
            ).encode(),
        )
        assert status == 201, location
        yield daemon
    finally:
        daemon.close()


def exchange(daemon, target, body):
    status, media, raw = daemon.exchange("PUT", target, body)
    assert media == "application/json"
    (LocationActionResponse if status == 200 else ErrorResponse).model_validate_json(raw, strict=True)
    return status, json.loads(raw)


@pytest.mark.parametrize(
    "preview, expected", [(None, "Configured"), ("", ""), ("Safe café preview", "Safe café preview")]
)
def test_real_daemon_binding_preserves_preview_and_keeps_secrets_private(location_daemon, preview, expected):
    secret = "private-value-never-returned-453454"
    request = {"values": {"root_path": str(location_daemon.directory / "storage"), "credential": secret}}
    if preview is not None:
        request["preview"] = preview
    status, payload = exchange(location_daemon, "/locations/boundary-location/binding", json.dumps(request).encode())
    assert status == 200
    assert payload == {
        "ok": True,
        "data": {
            "location_id": "boundary-location",
            "bound": True,
            "binding_preview": expected,
        },
    }
    assert secret not in json.dumps(payload)
    status, details = location_daemon.request("GET", "/locations/boundary-location")
    assert status == 200 and details["data"]["bound"] is True
    assert details["data"]["binding_preview"] == expected
    assert secret not in json.dumps(details)


@pytest.mark.parametrize(
    "target, body",
    [
        ("/locations/boundary-location/binding", b""),
        ("/locations/boundary-location/binding", b"{}"),
        ("/locations/boundary-location/binding", b"[]"),
        ("/locations/boundary-location/binding", b'{"values":[] }'),
        ("/locations/boundary-location/binding", b'{"values":{"secret":7}}'),
        ("/locations/boundary-location/binding", b'{"values":{},"preview":null}'),
        ("/locations/boundary-location/binding", b'{"values":{},"preview":4}'),
        ("/locations/boundary-location/binding", b'{"values":{},"unexpected":true}'),
        ("/locations/boundary-location/binding", b'{"values":{"secret":"bad\\u0000text"}}'),
        ("/locations/boundary-location/binding", b'{"values":{"bad\\u0000key":"secret"}}'),
        ("/locations/boundary-location/binding", b'{"values":{"secret":"\xff"}}'),
        ("/locations/boundary-location/binding", b'{"values":'),
        ("/locations/boundary-location/binding?unexpected=true", b'{"values":{}}'),
        ("/locations/boundary-location/binding?x=1&x=2", b'{"values":{}}'),
        ("/locations/%FF/binding", b'{"values":{}}'),
        ("/locations/%00/binding", b'{"values":{}}'),
    ],
)
def test_real_daemon_rejects_invalid_requests_without_changing_the_binding(location_daemon, target, body):
    _, before = location_daemon.request("GET", "/locations/boundary-location")
    status, payload = exchange(location_daemon, target, body)
    assert status == 400
    assert payload["ok"] is False and payload["error"]["code"] == "invalid_argument"
    _, after = location_daemon.request("GET", "/locations/boundary-location")
    assert after == before


def test_real_daemon_missing_location_authentication_and_method_rules(location_daemon):
    status, payload = exchange(location_daemon, "/locations/missing/binding", b'{"values":{}}')
    assert status == 404 and payload["error"]["code"] == "not_found"
    target = location_daemon.url + "/locations/boundary-location/binding"
    unauthorized = requests.put(target, json={"values": {}}, timeout=10)
    assert unauthorized.status_code == 401
    wrong_method = requests.patch(
        target, json={"values": {}}, timeout=10, headers={"Authorization": f"Bearer {location_daemon.token}"}
    )
    assert wrong_method.status_code == 405
    assert set(wrong_method.headers["Allow"].split(", ")) == {"PUT", "DELETE"}
