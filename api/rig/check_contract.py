"""Compare reflected operations with Holder's checked-in API contract."""

import json
import sys
from pathlib import Path

import yaml


def main(control_path: str, generated_path: str) -> None:
    control = yaml.safe_load(Path(control_path).read_text())
    generated = json.loads(Path(generated_path).read_text())
    for key in ("openapi", "info", "servers", "security"):
        if generated[key] != control[key]:
            raise SystemExit(f"RIG {key} differs from api/openapi.yaml")
    if generated["components"]["securitySchemes"] != control["components"]["securitySchemes"]:
        raise SystemExit("RIG security schemes differ from api/openapi.yaml")
    for path, method in (("/bye", "post"), ("/projects/{project_id}/tags", "get")):
        expected_operation = control["paths"][path][method]
        actual_operation = generated["paths"][path][method]
        if actual_operation != expected_operation:
            raise SystemExit(f"RIG {method.upper()} {path} differs from api/openapi.yaml")
    for name in ("ByeResponse", "ErrorResponse", "Error", "ProjectTag",
                 "ProjectTagListResponse"):
        expected = control["components"]["schemas"][name]
        actual = generated["components"]["schemas"][name]
        if actual != expected:
            raise SystemExit(f"RIG {name} differs from api/openapi.yaml")
    print("RIG service metadata, two operations, and schemas match api/openapi.yaml")


if __name__ == "__main__":
    main(*sys.argv[1:])
