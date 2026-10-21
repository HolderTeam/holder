"""Compare every reflected operation and schema with Holder's checked-in API contract."""

import json
import sys
from copy import deepcopy
from pathlib import Path

import yaml


def normalized_operation(operation: dict) -> dict:
    result = deepcopy(operation)
    for parameter in result.get("parameters", []):
        if parameter["in"] == "query":
            parameter.setdefault("required", False)
    return result


def main(control_path: str, generated_path: str) -> None:
    control = yaml.safe_load(Path(control_path).read_text())
    generated = json.loads(Path(generated_path).read_text())
    for key in ("openapi", "info", "servers", "security"):
        if generated[key] != control[key]:
            raise SystemExit(f"RIG {key} differs from api/openapi.yaml")
    if generated["components"]["securitySchemes"] != control["components"]["securitySchemes"]:
        raise SystemExit("RIG security schemes differ from api/openapi.yaml")
    differences = []
    count = 0
    for path, methods in generated["paths"].items():
        for method, operation in methods.items():
            count += 1
            expected = control["paths"].get(path, {}).get(method)
            if expected is None:
                differences.append(f"{method.upper()} {path} is not in api/openapi.yaml")
            elif normalized_operation(operation) != normalized_operation(expected):
                differences.append(f"{method.upper()} {path} differs")
    for name, schema in generated["components"]["schemas"].items():
        if control["components"]["schemas"].get(name) != schema:
            differences.append(f"schema {name} differs")
    if differences:
        raise SystemExit("RIG differs from api/openapi.yaml:\n  " + "\n  ".join(differences))
    print(f"RIG service metadata, {count} operations, and "
          f"{len(generated['components']['schemas'])} schemas match api/openapi.yaml")


if __name__ == "__main__":
    main(*sys.argv[1:])
