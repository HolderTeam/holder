"""The daemon's generators: the HTTP dispatcher, its JSON codec and the OpenAPI paths.

The route rules are ported with RIG's generators, as are their tests. tests/http/ compiles the
dispatcher against the stand-in core in tests/c/ and sends it requests.
"""

import json
import re
import shutil
import subprocess
from pathlib import Path

import pytest

from holder_contract.daemon import generate_daemon
from holder_contract.extract import extract, load_contract
from holder_contract.generate import Model, Unsupported, generate
from holder_contract.http_routes import generate_routes, route_pattern, validate_routes
from holder_contract.openapi import ERROR_SCHEMAS, document, dumps, exact_bounds, merge, named_by_title

CONTRACT = Path(__file__).resolve().parents[1]
TESTS = CONTRACT / "tests"
STRICT = ["-Wall", "-Wextra", "-Wpedantic", "-Werror"]
SANITIZE = ["-fsanitize=address,undefined", "-fno-sanitize-recover=all"]
NLOHMANN = [Path(p) for p in ("/usr/include/nlohmann/json.hpp", "/usr/local/include/nlohmann/json.hpp")]


def holder_ir():
    return extract(CONTRACT, "interface")


def holder_contract():
    holder_ir()  # checks the contract first
    return load_contract(CONTRACT, "interface")


def binding(method, path):
    return {"operation": path, "method": method, "path": path}


# ---- routes ------------------------------------------------------------------------------------


def test_route_pattern_captures_each_parameter():
    assert route_pattern("/projects/{project_id}/cards/{card_id}") == (
        "^/projects/([^/]+)/cards/([^/]+)$", {"project_id": 1, "card_id": 2})


@pytest.mark.parametrize("path", ["cards", "/cards/", "//cards", "/cards/{id}/{id}", "/cards/a b"])
def test_unsupported_route_paths_are_rejected(path):
    with pytest.raises(ValueError):
        route_pattern(path)


def test_equivalent_parameterized_paths_are_rejected():
    with pytest.raises(ValueError, match="ambiguous routes"):
        validate_routes([binding("GET", "/items/{id}"), binding("GET", "/items/{slug}")])


def test_crossing_route_specificity_is_rejected():
    with pytest.raises(ValueError, match="ambiguous routes"):
        validate_routes([binding("GET", "/{group}/fixed"), binding("GET", "/known/{item}")])


def test_disjoint_routes_and_other_methods_are_accepted():
    validate_routes([binding("GET", "/items/{id}"), binding("GET", "/items/new"),
                     binding("POST", "/items/{slug}"), binding("GET", "/items/{id}/parts")])


def test_a_literal_segment_is_tried_before_a_parameter():
    ir = holder_ir()
    get_card = next(b for b in ir["http"] if b["operation"] == "get_card")
    ir["http"].insert(0, dict(get_card))
    get_card["path"] = "/projects/{project_id}/cards/latest"
    get_card["path_parameters"] = ["project_id"]
    get_card["query"] = ["card_id"]
    routes = generate_routes(Model(ir), "codec.h")
    assert (routes.index('regex route_1("^/projects/([^/]+)/cards/latest$")')
            < routes.index('regex route_0("^/projects/([^/]+)/cards/([^/]+)$")'))


# ---- generated files ---------------------------------------------------------------------------


def test_every_generated_file_names_its_source():
    for name, text in generate_daemon(holder_ir(), holder_contract()).items():
        if name.endswith(".h"):
            header = text.splitlines()[0]
            assert "Generated from contract/interface/app.py," in header, name
            assert "contract/interface/cards.py" in header, name


def test_unsupported_parts_of_the_contract_stop_generation():
    with pytest.raises(Unsupported):
        generate_daemon(extract(TESTS / "fixtures", "sample"), load_contract(TESTS / "fixtures", "sample"))


def test_a_body_that_is_not_a_record_stops_generation():
    ir = holder_ir()
    create = next(b for b in ir["http"] if b["operation"] == "create_card")
    create_op = next(op for op in ir["operations"] if op["name"] == "create_card")
    next(p for p in create_op["parameters"] if p["name"] == "card")["type"] = {"kind": "string"}
    with pytest.raises(Unsupported, match="a request body that is not a record"):
        generate_routes(Model(ir), "codec.h")
    assert create["body"] == "card"


# ---- OpenAPI -----------------------------------------------------------------------------------


def openapi():
    return document(holder_contract())


def test_fastapis_validation_responses_and_operation_ids_are_left_out():
    result = openapi()
    for methods in result["paths"].values():
        for operation in methods.values():
            assert "422" not in operation["responses"]
            assert "operationId" not in operation
    assert not {"HTTPValidationError", "ValidationError"} & set(result["components"]["schemas"])


def test_parameters_come_from_fastapis_analysis_of_the_routes():
    operation = openapi()["paths"]["/projects/{project_id}/cards"]["get"]
    parameters = {p["name"]: p for p in operation["parameters"]}
    assert parameters["project_id"] == {"name": "project_id", "in": "path", "required": True,
                                        "schema": {"$ref": "#/components/schemas/ProjectId"}}
    assert parameters["limit"]["required"] is False
    limit = parameters["limit"]["schema"]
    assert (limit["format"], limit["minimum"], limit["maximum"], limit["default"]) == ("int32", 1, 5000, 200)
    assert parameters["order"]["schema"]["enum"] == ["card_id_asc", "updated_desc", "sort_key_asc"]
    assert {"const": "roots", "type": "string"} in parameters["parent"]["schema"]["anyOf"]
    assert operation["summary"] == "Page through a project's live cards."


def test_trash_delete_paths_only_advertise_supported_item_types():
    result = openapi()
    for path in ("/trash/{type}/{id}", "/projects/{project_id}/trash/{type}/{id}"):
        parameters = result["paths"][path]["delete"]["parameters"]
        item_type = next(parameter for parameter in parameters if parameter["name"] == "type")
        schema_name = item_type["schema"]["$ref"].split("/")[-1]
        assert result["components"]["schemas"][schema_name]["enum"] == ["card", "ai_message"]


def test_success_is_enveloped_with_the_routes_status_and_failures_have_the_daemons_body():
    result = openapi()
    responses = result["paths"]["/projects/{project_id}/cards"]["post"]["responses"]
    assert list(responses) == ["201", "400", "404", "409", "500", "503"]
    assert responses["201"]["content"]["application/json"]["schema"] == {"$ref": "#/components/schemas/CardResponse"}
    envelope = result["components"]["schemas"]["CardResponse"]
    assert envelope["properties"]["data"] == {"$ref": "#/components/schemas/Card"}
    assert envelope["required"] == ["ok", "data"]
    assert responses["404"]["description"] == "The thing asked for does not exist, or is in the trash."
    # Error kinds that share a status share its response.
    assert responses["409"]["description"] == (
        "The request conflicts with what exists: what would be created already exists, such as a card with the "
        "same ID or file, or the thing to change is not in the state the request needs, such as restoring a card "
        "that is not in the trash, or removing a tag that a card would still carry in its prose. "
        "An encrypted project's key is not on this device; the project's recovery token restores it.")
    assert responses["404"]["content"]["application/json"]["schema"] == {"$ref": "#/components/schemas/ErrorResponse"}
    # The same error schemas as RIG's document, so that the two merge.
    assert {k: result["components"]["schemas"][k] for k in ERROR_SCHEMAS} == ERROR_SCHEMAS


def test_records_are_closed_and_list_their_bases_fields():
    schemas = openapi()["components"]["schemas"]
    item = schemas["CardListItem"]
    assert "allOf" not in item and item["additionalProperties"] is False
    assert list(item["properties"])[:2] == ["card_id", "project_id"]
    assert "child_count" not in item["required"] and "card_id" in item["required"]
    assert {"type": "null"} in item["properties"]["parent_card_id"]["anyOf"]
    assert schemas["NewCard"]["required"] == ["title"]
    assert schemas["NewCard"]["properties"]["content"]["default"] == ""


def test_a_list_results_envelope_is_named_for_its_items():
    result = openapi()
    responses = result["paths"]["/projects/{project_id}/tags"]["get"]["responses"]
    assert responses["200"]["content"]["application/json"]["schema"] == {
        "$ref": "#/components/schemas/TagCountListResponse"}
    data = result["components"]["schemas"]["TagCountListResponse"]["properties"]["data"]
    assert (data["type"], data["items"]) == ("array", {"$ref": "#/components/schemas/TagCount"})


def test_a_generic_records_use_is_named_for_its_arguments():
    schemas = openapi()["components"]["schemas"]
    assert {"CardListItemPage", "CardListItemPageResponse", "CardResponse"} <= set(schemas)
    assert not any("_" in name for name in schemas)
    assert schemas["CardListItemPage"]["properties"]["items"]["items"] == {"$ref": "#/components/schemas/CardListItem"}


def test_integer_bounds_are_exact():
    created_at = openapi()["components"]["schemas"]["Card"]["properties"]["created_at"]
    assert (created_at["format"], created_at["minimum"], created_at["maximum"]) == ("int64", -(2**63), 2**63 - 1)
    assert type(created_at["minimum"]) is int and type(created_at["maximum"]) is int
    assert exact_bounds({"type": "integer", "format": "int32", "minimum": float(-(2**31)), "maximum": 5000.0}) == {
        "type": "integer", "format": "int32", "minimum": -(2**31), "maximum": 5000}
    assert exact_bounds({"type": "number", "maximum": 0.5}) == {"type": "number", "maximum": 0.5}
    with pytest.raises(ValueError, match="maximum of 1e\\+17 is not exact"):
        exact_bounds({"type": "integer", "format": "int64", "maximum": 1e17})


def test_schemas_that_would_share_a_name_are_refused():
    clash = {"paths": {}, "components": {"schemas": {"A_B_": {"title": "AB"}, "AB": {"title": "AB"}}}}
    with pytest.raises(ValueError, match="schemas would share the names AB"):
        named_by_title(clash)


def test_the_document_is_the_same_on_every_run():
    assert json.dumps(openapi()) == json.dumps(openapi())


def test_yaml_and_json_describe_the_same_document():
    yaml = pytest.importorskip("yaml")
    document = openapi()
    assert yaml.safe_load(dumps(document, "yaml")) == json.loads(dumps(document))


# ---- compiled ----------------------------------------------------------------------------------


@pytest.mark.skipif(not shutil.which("g++") or not any(p.exists() for p in NLOHMANN),
                    reason="needs g++ and nlohmann/json")
def test_generated_routes_compile_and_answer_requests(tmp_path):
    # Laid out as the daemon includes them, with core's C++ declarations from generate.py.
    (tmp_path / "api" / "generated").mkdir(parents=True)
    (tmp_path / "api" / "Contract.h").write_text(generate(holder_ir())["Contract.h"], encoding="utf-8")
    for name, text in generate_daemon(holder_ir(), holder_contract()).items():
        (tmp_path / "api" / "generated" / name).write_text(text, encoding="utf-8")

    def run(*command):
        subprocess.run(command, check=True, cwd=tmp_path, capture_output=True, text=True)

    flags = ["-std=c++20", *STRICT, *SANITIZE, "-I.", "-Iapi"]
    run("g++", *flags, "-c", str(TESTS / "c" / "stub_core.cpp"), "-o", "stub.o")
    run("g++", *flags, "-c", str(TESTS / "http" / "exercise.cpp"), "-o", "exercise.o")
    run("g++", *SANITIZE, "stub.o", "exercise.o", "-o", "exercise")
    result = subprocess.run([str(tmp_path / "exercise")], capture_output=True, text=True, check=True)
    assert result.stdout == (TESTS / "http" / "exercise.expected").read_text(encoding="utf-8")


def test_generate_command_writes_core_and_daemon_files(tmp_path):
    from holder_contract.__main__ import main

    assert main(["generate", "--output", str(tmp_path)]) == 0
    assert sorted(p.name for p in tmp_path.iterdir()) == [
        "Contract.h", "ContractCApi.cpp", "ContractJson.h", "ContractRoutes.h", "holder_api.h", "openapi.json"]


def daemon_document(paths):
    return {"openapi": "3.2.0", "info": {"title": "API", "version": "1"}, "security": [{"bearer": []}],
            "paths": paths, "components": {"securitySchemes": {"bearer": {"type": "http"}}, "schemas": {}}}


def test_merge_adds_the_contracts_routes_beside_the_daemons():
    card = "/projects/{project_id}/cards/{card_id}"
    merged = merge(daemon_document({card: {"put": {"responses": {"200": {}}}},
                                    "/projects/{project_id}/repair-hierarchy": {
                                        "post": {"responses": {"200": {}}}}}), openapi())
    assert list(merged["paths"][card]) == ["get", "put", "patch", "delete"]
    assert merged["paths"][card]["get"]["responses"]["401"]["description"] == "Unauthorized"
    assert merged["components"]["securitySchemes"] == {"bearer": {"type": "http"}}
    assert "CardListItemPage" in merged["components"]["schemas"]


@pytest.mark.parametrize("paths, message", [
    ({"/projects/{project_id}/cards": {"post": {"responses": {}}}},
     "POST /projects/{project_id}/cards is both a daemon route and a contract operation"),
    ({"/projects/{project_id}/{collection}/{id}": {"get": {"responses": {}}}},
     ("the daemon's GET /projects/{project_id}/{collection}/{id} would take requests for the contract's "
      "GET /projects/{project_id}/cards/{card_id}")),
])
def test_merge_rejects_routes_that_clash(paths, message):
    with pytest.raises(ValueError, match=re.escape(message)):
        merge(daemon_document(paths), openapi())


def test_merge_rejects_a_schema_with_another_shape():
    document = daemon_document({})
    document["components"]["schemas"]["Card"] = {"type": "object"}
    with pytest.raises(ValueError, match="the schema Card is declared by the daemon and by the contract"):
        merge(document, openapi())
