"""Exercise generated transport and serialization without a live daemon."""

import ast
import asyncio
import hashlib
import importlib
import json
import pkgutil
import re
from pathlib import Path

import httpx
import pytest
from ruamel.yaml import YAML

from holder import generated
from holder.generated import AuthenticatedClient, Client
from holder.generated.api.default import (
    get_ai_runner_pull_job_id_events,
    get_ai_runs_run_id_events,
    get_cards,
    get_event_cursor,
    get_health,
    get_openapi_yaml,
    get_ping,
    get_resources_resource_id_assets_asset_id_content,
    patch_projects_project_id_cards_card_id,
    post_bye,
    stream_changes,
)
from holder.generated.errors import UnexpectedStatus
from holder.generated.models import (
    AiLocalModelConfigData,
    AiStreamResyncData,
    ByeResponse,
    CardUpdate,
    ErrorResponse,
    EventCheckpointResponse,
    EventResyncData,
    HealthResponse,
    ProjectGitSyncOperationResponseDataPush,
    ProjectUpdate,
    RunnerCapabilitiesData,
    RunnerPullJob,
    StorageLocationProvider,
    StorageLocationRequestProvider,
)
from holder.generated.streams import StreamError

ROOT = Path(__file__).resolve().parents[1]
SCHEMA = ROOT.parents[1] / "api/openapi.yaml"


def test_every_contract_operation_is_generated():
    schema = YAML(typ="safe").load(SCHEMA.read_text())
    expected = {
        (method, re.sub(r"\{[^}]+\}", "{}", path))
        for path, operations in schema["paths"].items()
        for method in operations
        if method
        in {"get", "post", "put", "patch", "delete", "head", "options", "trace"}
    }
    actual = set()
    for file in (ROOT / "src/holder/generated/api").rglob("*.py"):
        tree = ast.parse(file.read_text())
        for node in ast.walk(tree):
            if isinstance(node, ast.Dict):
                values = {
                    key.value: value
                    for key, value in zip(node.keys, node.values)
                    if isinstance(key, ast.Constant)
                }
                if "method" not in values or "url" not in values:
                    continue
                url = values["url"]
                if isinstance(url, ast.Call):
                    url = url.func.value
                # Python keywords may acquire a suffix (e.g. type -> type_).
                actual.add(
                    (values["method"].value, re.sub(r"\{[^}]+\}", "{}", url.value))
                )
    assert actual == expected


def test_owning_contract_matches_provenance():
    provenance = json.loads((ROOT / "openapi/provenance.json").read_text())
    assert hashlib.sha256(SCHEMA.read_bytes()).hexdigest() == provenance["sha256"]
    assert provenance["source"] == "api/openapi.yaml"
    assert provenance["generator"] == {
        "openapi-python-client": (
            "https://github.com/zeth/openapi-python-client"
            "@e0abac2ad9c0df67197ef7401504cb21aab7c1af"
        ),
        "ruff": "0.16.10",
    }


def test_all_generated_modules_import():
    for module in pkgutil.walk_packages(generated.__path__, generated.__name__ + "."):
        importlib.import_module(module.name)


def test_event_checkpoint_preserves_opaque_cursor_and_nullable_revisions():
    payload = {
        "ok": True,
        "data": {
            "cursor": "process:opaque/7+tail",
            "git_revisions": {"project & one": None, "other": "a" * 40},
            "history_urls": {"project & one": "/projects/project/history"},
        },
    }

    def respond(request):
        assert request.method == "GET"
        assert request.url.path == "/events/cursor"
        assert request.url.params["project_id"] == "project & one"
        assert request.headers["Authorization"] == "Bearer test-token"
        return httpx.Response(200, json=payload)

    with AuthenticatedClient(
        base_url="http://holder.test",
        token="test-token",
        httpx_args={"transport": httpx.MockTransport(respond)},
    ) as client:
        result = get_event_cursor.sync(client=client, project_id="project & one")
    assert isinstance(result, EventCheckpointResponse)
    assert result.to_dict() == payload


@pytest.mark.parametrize(
    "endpoint,args,path,query,payload,expected",
    [
        (
            stream_changes,
            (),
            "/events",
            {"project_id": "project & one", "last_revision": "a" * 40},
            (
                b"id: opaque:8\nevent: resync_required\ndata: "
                b'{"cursor":"opaque:8","git_revisions":{},"history_urls":{},'
                b'"reason":"history_unavailable","last_revision":null}\n\n'
            ),
            EventResyncData,
        ),
        (
            get_ai_runs_run_id_events,
            ("run-1",),
            "/ai/runs/run-1/events",
            {},
            b'id: opaque:8\nevent: resync_required\ndata: {"reason":"history_unavailable"}\n\n',
            AiStreamResyncData,
        ),
        (
            get_ai_runner_pull_job_id_events,
            ("job-1",),
            "/ai/runner/pull/job-1/events",
            {},
            (
                b"id: opaque:8\nevent: progress\ndata: "
                b'{"job_id":"job-1","model":"m","status":"downloading","active":true,'
                b'"updated_at":1,"progress":'
                b'{"completed":1,"total":2,"percent":50.0,"stage":"d"}}\n\n'
            ),
            RunnerPullJob,
        ),
    ],
)
@pytest.mark.parametrize("status", [200, 400, 503])
def test_replay_headers_queries_and_documented_stream_errors(
    endpoint,
    args,
    path,
    query,
    payload,
    expected,
    status,
):
    def respond(request):
        assert request.method == "GET"
        assert request.url.path == path
        assert dict(request.url.params) == query
        assert request.headers["Last-Event-ID"] == "opaque:7"
        assert request.headers["Authorization"] == "Bearer test-token"
        if status == 200:
            return httpx.Response(
                status,
                content=payload,
                headers={"Content-Type": "text/event-stream"},
            )
        return httpx.Response(
            status,
            json={
                "ok": False,
                "error": {"code": "stream_error", "message": "Unavailable"},
            },
        )

    with AuthenticatedClient(
        base_url="http://holder.test",
        token="test-token",
        httpx_args={"transport": httpx.MockTransport(respond)},
    ) as client:
        stream = endpoint.stream(
            *args,
            client=client,
            last_event_id="opaque:7",
            reconnect=False,
            **query,
        )
        if status == 200:
            with stream as events:
                received = list(events)
            assert [type(event.data) for event in received] == [expected]
            assert events.last_event_id == "opaque:8"
        else:
            with pytest.raises(StreamError) as raised, stream:
                pass
            assert raised.value.status_code == status
            assert isinstance(raised.value.parsed, ErrorResponse)
            assert raised.value.parsed.error.code == "stream_error"


def test_change_feed_omits_unspecified_replay_and_scope():
    def respond(request):
        assert not request.url.params
        assert "Last-Event-ID" not in request.headers
        return httpx.Response(200, text="event: ready\ndata: {}\n\n")

    with Client(
        base_url="http://holder.test",
        httpx_args={"transport": httpx.MockTransport(respond)},
    ) as client:
        stream_changes.sync(client=client)


def test_goodbye_is_an_explicit_authenticated_bodyless_request():
    requests = []

    def respond(request):
        requests.append(request)
        assert request.method == "POST"
        assert request.url.path == "/bye"
        assert request.content == b""
        assert request.headers["Authorization"] == "Bearer test-token"
        return httpx.Response(200, json={"ok": True})

    with AuthenticatedClient(
        base_url="http://holder.test",
        token="test-token",
        httpx_args={"transport": httpx.MockTransport(respond)},
    ) as client:
        response = post_bye.sync_detailed(client=client)
    assert isinstance(response.parsed, ByeResponse)
    assert response.parsed.ok is True
    assert len(requests) == 1


def test_authenticated_health_response():
    def respond(request):
        assert request.headers["Authorization"] == "Bearer test-token"
        assert request.url.path == "/health"
        return httpx.Response(
            200,
            json={
                "ok": True,
                "data": {
                    "db_ok": True,
                    "uptime_ms": 123,
                    "api_version": "0.1.0",
                    "server_version": "test",
                    "pid": 42,
                    "privacy": {
                        "backend": "libsodium_xchacha20poly1305_ietf",
                        "project_mode_supported": True,
                    },
                },
            },
        )

    with AuthenticatedClient(
        base_url="http://127.0.0.1:11499",
        token="test-token",
        httpx_args={"transport": httpx.MockTransport(respond)},
    ) as client:
        response = get_health.sync_detailed(client=client)
    assert response.status_code == 200
    assert isinstance(response.parsed, HealthResponse)
    assert response.parsed.data.pid == 42
    assert response.parsed.data.privacy.backend == "libsodium_xchacha20poly1305_ietf"
    assert response.parsed.data.privacy.project_mode_supported is True


def test_card_query_and_error_envelope():
    def respond(request):
        assert request.url.params["project_id"] == "project & one"
        assert request.url.params["tag"] == "hello world"
        assert "parent_card_id" not in request.url.params
        return httpx.Response(
            401,
            json={
                "ok": False,
                "error": {
                    "code": "unauthorized",
                    "message": "Token required",
                },
            },
        )

    with Client(
        base_url="http://holder.test",
        httpx_args={
            "transport": httpx.MockTransport(respond),
        },
    ) as client:
        result = get_cards.sync(
            client=client, project_id="project & one", tag="hello world"
        )
    assert isinstance(result, ErrorResponse)
    assert result.error.code == "unauthorized"


def test_patch_preserves_omitted_and_explicit_null_fields():
    def respond(request):
        assert request.method == "PATCH"
        assert request.url.raw_path == b"/projects/project%2Fone/cards/card%2Fone"
        assert json.loads(request.content) == {"updated_at": 123, "parent_card_id": None}
        return httpx.Response(
            401, json={"ok": False, "error": {"code": "auth", "message": "auth"}}
        )

    body = CardUpdate(updated_at=123, parent_card_id=None)
    assert CardUpdate.from_dict(body.to_dict()).to_dict() == body.to_dict()
    with Client(
        base_url="http://holder.test",
        httpx_args={
            "transport": httpx.MockTransport(respond),
        },
    ) as client:
        patch_projects_project_id_cards_card_id.sync(
            "project/one", "card/one", client=client, body=body
        )


@pytest.mark.parametrize(
    "endpoint,args,content_type,payload",
    [
        (get_openapi_yaml, (), "application/yaml", b"openapi: 3.2.0\n"),
    ],
)
def test_text_responses_are_buffered(endpoint, args, content_type, payload):
    with Client(
        base_url="http://holder.test",
        httpx_args={
            "transport": httpx.MockTransport(
                lambda request: httpx.Response(
                    200,
                    content=payload,
                    headers={"Content-Type": content_type},
                )
            ),
        },
    ) as client:
        result = endpoint.sync_detailed(*args, client=client)
    assert result.content == payload
    assert result.parsed == payload.decode()


def test_binary_asset_preserves_bytes_and_headers():
    payload = b"\x00\xff\x80asset"
    transport = httpx.MockTransport(
        lambda request: httpx.Response(
            200,
            content=payload,
            headers={"Content-Disposition": 'attachment; filename="asset.bin"'},
        )
    )
    with (
        Client(
            base_url="http://holder.test", httpx_args={"transport": transport}
        ) as client,
        get_resources_resource_id_assets_asset_id_content.stream(
            "resource",
            "asset",
            client=client,
        ) as body,
    ):
        assert b"".join(body.iter_bytes()) == payload
        assert body.response.headers["Content-Disposition"] == (
            'attachment; filename="asset.bin"'
        )


def test_runner_capabilities_preserve_null_caste():
    model = RunnerCapabilitiesData(
        local_model_config=AiLocalModelConfigData(None, None, None, None),
        runners=[],
        recommended_models=[],
        recommended_install=[],
        caste=None,
    )
    assert RunnerCapabilitiesData.from_dict(model.to_dict()).caste is None


def test_nullable_enums_preserve_null():
    push = ProjectGitSyncOperationResponseDataPush(
        attempted=False,
        status=None,
        ahead_count=0,
        behind_count=0,
        local_head_commit=None,
        error_message=None,
    )
    assert (
        ProjectGitSyncOperationResponseDataPush.from_dict(push.to_dict()).status is None
    )
    # A partial update's null clears a field; it is not the same as leaving it out.
    update = ProjectUpdate(git_remote_url=None)
    assert update.to_dict() == {"git_remote_url": None}
    assert ProjectUpdate.from_dict(update.to_dict()).git_remote_url is None


def test_unexpected_status_can_raise():
    with (
        Client(
            base_url="http://holder.test",
            raise_on_unexpected_status=True,
            httpx_args={
                "transport": httpx.MockTransport(
                    lambda request: httpx.Response(418, content=b"unexpected")
                ),
            },
        ) as client,
        pytest.raises(UnexpectedStatus),
    ):
        get_ping.sync(client=client)


def test_async_public_ping():
    async def run():
        def respond(request):
            assert "Authorization" not in request.headers
            return httpx.Response(200, text="pong")

        async with Client(
            base_url="http://holder.test",
            httpx_args={
                "transport": httpx.MockTransport(respond),
            },
        ) as client:
            result = await get_ping.asyncio(client=client)
            assert result.value == "pong"

    asyncio.run(run())


def test_storage_locations_accept_every_daemon_provider():
    # The daemon supports Google Drive locations; the enums once left them out.
    for provider in ("local_directory", "s3_compatible", "google-drive"):
        assert StorageLocationProvider(provider) == provider
        assert StorageLocationRequestProvider(provider) == provider
