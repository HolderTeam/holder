"""Compare the reflected /bye slice with Holder's checked-in API contract."""

import json
import sys
from pathlib import Path

import yaml


def main(control_path: str, generated_path: str) -> None:
    control = yaml.safe_load(Path(control_path).read_text())
    generated = json.loads(Path(generated_path).read_text())
    expected_operation = control["paths"]["/bye"]["post"]
    actual_operation = generated["paths"]["/bye"]["post"]
    if actual_operation != expected_operation:
        raise SystemExit("RIG /bye operation differs from api/openapi.yaml")
    for name in ("ByeResponse", "ErrorResponse", "Error"):
        expected = control["components"]["schemas"][name]
        actual = generated["components"]["schemas"][name]
        if actual != expected:
            raise SystemExit(f"RIG {name} differs from api/openapi.yaml")
    print("RIG /bye operation and schemas match api/openapi.yaml")


if __name__ == "__main__":
    main(*sys.argv[1:])
