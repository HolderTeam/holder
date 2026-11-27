"""Complete daemon boundaries use the compiler result and reject unsupported shapes."""

import shutil
import subprocess
from copy import deepcopy
from pathlib import Path

import pytest
from pydantic import ValidationError

from compiler.ir import ContractView
from compiler.pipeline import Compiler, ContractSource
from generators.common import Unsupported
from generators.core import render_core
from generators.core.types import CoreTypes
from generators.daemon.boundary import BoundaryBinding, BoundaryPlan, bindings
from generators.daemon.contract_json import generate_codec
from generators.daemon.contract_routes import generate_routes
from generators.daemon.daemon_json import generate_json, generate_types
from generators.daemon.daemon_routes import generate_routes as generate_boundary_routes
from generators.daemon.inventory import MANUAL, generate_inventory

ROOT = Path(__file__).resolve().parents[2]
FIXTURES = Path(__file__).parent / "daemon_boundary"


def contract():
    return Compiler().compile(ContractSource(ROOT / "contract", "interface"))


def test_bindings_are_validated_and_cannot_disappear():
    result = contract()
    with pytest.raises(Unsupported, match="missing=.*put_location_binding"):
        BoundaryPlan(result, {})
    raw = bindings()["put_location_binding"].model_dump()
    raw["services"] = ["unknown"]
    with pytest.raises(ValidationError):
        BoundaryBinding.model_validate(raw)
    raw["services"] = ["db", "db"]
    with pytest.raises(Unsupported, match="duplicate runtime service"):
        BoundaryPlan(result, {"put_location_binding": BoundaryBinding.model_validate(raw)})
    raw["services"] = ["db", "secret_store"]
    raw["arguments"] = ["request"]
    with pytest.raises(Unsupported, match="argument inventory"):
        BoundaryPlan(result, {"put_location_binding": BoundaryBinding.model_validate(raw)})
    raw["arguments"] = ["location_id", "request"]
    raw["exceptions"]["holder::api::Unexpected"] = 409
    with pytest.raises(Unsupported, match="status 409 is not declared"):
        BoundaryPlan(result, {"put_location_binding": BoundaryBinding.model_validate(raw)})


@pytest.mark.parametrize(
    "mutation, message",
    [
        (
            lambda raw: raw["operations"].remove(
                next(op for op in raw["operations"] if op["name"] == "put_location_binding")
            ),
            "requires one daemon-only HTTP operation",
        ),
        (
            lambda raw: next(b for b in raw["http"] if b["operation"] == "put_location_binding").update(
                response_mode="envelope"
            ),
            "requires a raw JSON response",
        ),
        (
            lambda raw: next(b for b in raw["http"] if b["operation"] == "put_location_binding").update(
                query=["extra"]
            ),
            "unsupported daemon boundary HTTP metadata",
        ),
        (
            lambda raw: next(b for b in raw["http"] if b["operation"] == "put_location_binding").update(raw_schema={}),
            "unsupported daemon boundary HTTP metadata",
        ),
        (
            lambda raw: next(r for r in raw["records"] if r["name"] == "LocationBindingRequest").update(open=True),
            "inherited, open or generic",
        ),
        (
            lambda raw: next(r for r in raw["records"] if r["name"] == "LocationBindingRequest")["fields"][0].update(
                type={"kind": "map", "values": {"kind": "int32"}}
            ),
            "maps require string values",
        ),
        (
            lambda raw: next(r for r in raw["records"] if r["name"] == "LocationBindingRequest")["fields"][1].update(
                type={"kind": "list", "items": {"kind": "string"}}
            ),
            "unsupported daemon boundary type",
        ),
        (
            lambda raw: next(r for r in raw["records"] if r["name"] == "LocationBindingRequest")["fields"][1].update(
                type={"kind": "string", "pattern": "^safe$"}
            ),
            "unsupported daemon boundary constraints",
        ),
        (
            lambda raw: raw["openapi"]["paths"]["/locations/{location_id}/binding"]["put"]["responses"]["200"][
                "content"
            ]["application/json"].update(schema={"type": "string"}),
            "success schema differs",
        ),
        (
            lambda raw: raw["openapi"]["paths"]["/locations/{location_id}/binding"]["put"]["responses"].update(
                {"409": {"description": "new failure"}}
            ),
            "status 409 has no runtime binding",
        ),
    ],
)
def test_unsupported_shapes_fail_generation(mutation, message):
    raw = contract().model_dump(exclude_unset=True)
    mutation(raw)
    with pytest.raises(Unsupported, match=message):
        BoundaryPlan(ContractView.model_validate(raw))


def test_inventory_requires_a_disposition_for_every_daemon_operation():
    result = contract()
    inventory = generate_inventory(result)
    assert "1 complete, 43 partially generated AI boundaries, 24 remaining handwritten" in inventory
    assert "| `download_recovery_snapshot` | `GET /projects/{project_id}/recovery-snapshot` |" in inventory
    for operation in result.operations:
        if operation.daemon_only:
            assert sum(line.startswith(f"| `{operation.name}` |") for line in inventory.splitlines()) == 1
    raw = result.model_dump(exclude_unset=True)
    added = deepcopy(next(op for op in raw["operations"] if op["name"] == "get_alive"))
    added["name"] = "new_daemon_operation"
    raw["operations"].append(added)
    with pytest.raises(Unsupported, match="missing=.*new_daemon_operation"):
        generate_inventory(ContractView.model_validate(raw))
    raw["operations"] = [op for op in raw["operations"] if op["name"] != "get_alive"]
    with pytest.raises(Unsupported, match="stale=.*get_alive"):
        generate_inventory(ContractView.model_validate(raw))
    for handler, _ in MANUAL.values():
        filename, name = handler.split(":")
        assert name + "(" in (ROOT / "daemon/src/api" / filename).read_text()


@pytest.mark.skipif(
    not shutil.which("g++") or not Path("/usr/include/nlohmann/json.hpp").exists(), reason="needs g++ and nlohmann/json"
)
def test_generated_boundary_compiles_and_carries_new_fields_without_handwritten_codecs(tmp_path):
    raw = contract().model_dump(exclude_unset=True)
    request = next(record for record in raw["records"] if record["name"] == "LocationBindingRequest")
    request["fields"].append(
        {"name": "proof", "type": {"kind": "nullable", "value": {"kind": "bool"}}, "default": True}
    )
    response = next(record for record in raw["records"] if record["name"] == "LocationAction")
    response["fields"].append({"name": "proof", "type": {"kind": "bool"}, "omittable": True})
    result = ContractView.model_validate(raw)
    plan = BoundaryPlan(result)
    generated = tmp_path / "api/generated"
    generated.mkdir(parents=True)
    (tmp_path / "api/Contract.h").write_text(render_core(result)["Contract.h"])
    for name, text in {
        "ContractJson.h": generate_codec(CoreTypes(result), "api/Contract.h"),
        "ContractRoutes.h": generate_routes(result, "api/generated/ContractJson.h"),
        "DaemonTypes.h": generate_types(plan),
        "DaemonJson.h": generate_json(plan),
        "DaemonRoutes.h": generate_boundary_routes(plan),
    }.items():
        (generated / name).write_text(text)
    for name in ("LocationWrite.h", "HttpError.h"):
        shutil.copyfile(FIXTURES / name, tmp_path / "api" / name)
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            "-Werror",
            "-I",
            str(tmp_path),
            str(FIXTURES / "exercise.cpp"),
            "-o",
            str(tmp_path / "exercise"),
        ],
        check=True,
        capture_output=True,
    )
    subprocess.run([str(tmp_path / "exercise")], check=True, capture_output=True)
