"""Generators consume a serialized compilation result without the declaration package."""

import json
import subprocess
import sys
import textwrap
from pathlib import Path

import pytest

from compiler.errors import ContractError
from compiler.ir import ContractView
from compiler.pipeline import Compiler, ContractSource
from generators.android.client import generate_android_client
from generators.common import Unsupported
from generators.core import render_core
from generators.daemon import render_daemon
from generators.daemon.ai_json import generate_ai_codec
from generators.daemon.openapi import document
from generators.mcp import generate_mcp_tools

CONTRACT = Path(__file__).resolve().parents[1]


def compile_example(tmp_path, routes):
    package = tmp_path / "boundary_example"
    package.mkdir()
    (package / "__init__.py").write_text("")
    (package / "models.py").write_text(textwrap.dedent('''
        from authoring import Record

        class Notice(Record):
            """An event payload reachable only through its named event."""

            message: str
            """The event text."""
    '''))
    (package / "app.py").write_text(textwrap.dedent('''
        from authoring import Bytes, Contract, Stream
        from .models import Notice

        contract = Contract()
    ''') + textwrap.dedent(routes))
    return Compiler().compile(ContractSource(tmp_path, package.name))


def test_every_target_generates_in_a_fresh_process_without_authoring_imports(tmp_path):
    result = Compiler().compile(ContractSource(CONTRACT, "interface"))
    serialized = tmp_path / "result.json"
    serialized.write_text(result.model_dump_json(exclude_unset=True))
    expected = {"core": render_core(result), "daemon": render_daemon(result),
                "android": generate_android_client(result), "mcp": generate_mcp_tools(result)}
    script = '''
import importlib.abc
import json
import sys

class NoDeclarations(importlib.abc.MetaPathFinder):
    def find_spec(self, fullname, path=None, target=None):
        if fullname.split(".")[0] in {"interface", "authoring"} or fullname in {
            "compiler.source", "compiler.builder", "compiler.pipeline"
        }:
            raise AssertionError(f"backend imported declaration machinery: {fullname}")

sys.meta_path.insert(0, NoDeclarations())
from compiler.ir import ContractView
from generators.android.client import generate_android_client
from generators.core import render_core
from generators.daemon import render_daemon
from generators.mcp import generate_mcp_tools

with open(sys.argv[1]) as stream:
    result = ContractView.model_validate_json(stream.read())
print(json.dumps({"core": render_core(result), "daemon": render_daemon(result),
                  "android": generate_android_client(result), "mcp": generate_mcp_tools(result)}))
'''
    generated = subprocess.run([sys.executable, "-c", script, str(serialized)], cwd=CONTRACT,
                               capture_output=True, text=True, check=True)
    assert json.loads(generated.stdout) == expected


def test_additional_responses_raw_bytes_and_event_only_models_survive_serialization(tmp_path):
    compiled = compile_example(tmp_path, '''
        @contract.route("GET", "/events", core=False, status=201, operation_id="watch",
                        sse_events={"notice": Notice}, responses={
                            204: {"description": "Finished."},
                            409: {"model": Notice, "description": "Retry.",
                                  "headers": {"Retry-After": {"schema": {"type": "integer"}}}}
                        })
        def get_ai_status() -> Stream[str]:
            """Watch notices."""

        @contract.route("GET", "/bytes", core=False, raw_media_type="image/png",
                        raw_schema={"type": "string", "contentEncoding": "base64"})
        def read_bytes() -> Bytes:
            """Read bytes."""

        @contract.route("GET", "/binary", core=False)
        def read_binary() -> Bytes:
            """Read untyped binary content."""
    ''')
    result = ContractView.model_validate_json(compiled.model_dump_json(exclude_unset=True))
    event_binding, raw_binding, binary_binding = result.http
    assert event_binding.sse_events["notice"].name == "Notice"
    assert event_binding.response_mode == "stream"
    assert raw_binding.response_mode == binary_binding.response_mode == "raw"
    assert raw_binding.response_media_type == "image/png"
    assert binary_binding.response_media_type == "application/octet-stream"
    paths = document(result)["paths"]
    events = paths["/events"]["get"]
    assert events["operationId"] == "watch"
    assert events["responses"]["204"] == {"description": "Finished."}
    retry = events["responses"]["409"]
    assert retry["description"] == "Retry."
    assert retry["headers"]["Retry-After"]["schema"] == {"type": "integer"}
    assert retry["content"]["text/event-stream"]["schema"] == {"$ref": "#/components/schemas/Notice"}
    event = events["responses"]["201"]["content"]["text/event-stream"]["itemSchema"]["oneOf"][0]
    assert event["properties"]["event"] == {"const": "notice"}
    assert event["properties"]["data"]["contentSchema"] == {"$ref": "#/components/schemas/Notice"}
    assert paths["/bytes"]["get"]["responses"]["200"]["content"]["image/png"]["schema"] == raw_binding.raw_schema
    assert paths["/binary"]["get"]["responses"]["200"]["content"]["application/octet-stream"] == {}
    codec = generate_ai_codec(result)
    assert "codec<holder::api::Notice>" in codec
    assert 'notice(const holder::api::Notice& data)' in codec
    # Normalization must not mutate the compiler's artifact or affect later runs.
    assert result.model_dump_json(exclude_unset=True) == compiled.model_dump_json(exclude_unset=True)


@pytest.mark.parametrize("options, returns, message", [
    ('sse_events={"notice": Notice}', "Notice", "named SSE events require a Stream"),
    ('sse_events={"notice": str}', "Stream[str]", "must name a declared, non-generic Record"),
    ('sse_events={"bad\\nname": Notice}', "Stream[str]", "nonempty single-line strings"),
    ('raw_schema={}', "str", "raw_schema requires a raw response"),
    ('raw_media_type="text/plain", raw_schema={"example": object()}', "str", "invalid HTTP response metadata"),
    ('sse_events={"notice": Notice}, raw_media_type="text/plain"', "Stream[str]",
     "named SSE events cannot use raw_media_type"),
    ('responses={204: {"model": Notice}}', "str", "cannot be imported"),
])
def test_unsupported_response_shapes_are_compiler_errors(tmp_path, options, returns, message):
    with pytest.raises(ContractError, match=message):
        compile_example(tmp_path, f'''
            @contract.route("GET", "/watch", core=False, {options})
            def watch() -> {returns}:
                """Watch events."""
        ''')


def test_event_names_that_cannot_name_cpp_helpers_are_backend_errors(tmp_path):
    result = compile_example(tmp_path, '''
        @contract.route("GET", "/watch", core=False, sse_events={"notice.changed": Notice})
        def get_ai_status() -> Stream[str]:
            """Watch events."""
    ''')
    assert document(result)["paths"]["/watch"]["get"]["responses"]["200"]
    with pytest.raises(Unsupported, match="cannot name a C\\+\\+ event helper"):
        generate_ai_codec(result)


def test_binary_response_can_override_schema_without_an_explicit_media_type(tmp_path):
    result = compile_example(tmp_path, '''
        @contract.route("GET", "/bytes", core=False, raw_schema={})
        def read_bytes() -> Bytes:
            """Read binary content."""
    ''')
    response = document(result)["paths"]["/bytes"]["get"]["responses"]["200"]
    assert response["content"]["application/octet-stream"] == {"schema": {}}
