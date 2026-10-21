"""Compare reflected operations with Holder's checked-in API contract."""

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
    for path, method in (("/bye", "post"), ("/rebuild", "post"),
                         ("/ai/providers/settings", "put"),
                         ("/ai/providers/settings", "get"),
                         ("/ai/providers/settings/{provider}", "delete"),
                         ("/ai/providers/credentials", "get"),
                         ("/ai/providers/credentials", "put"),
                         ("/projects/{project_id}/tags", "get"),
                         ("/ai/runner/pull/{job_id}", "get"),
                         ("/cards/{card_id}/links", "get"),
                         ("/card-references/resolve", "post")):
        expected_operation = control["paths"][path][method]
        actual_operation = generated["paths"][path][method]
        if normalized_operation(actual_operation) != normalized_operation(expected_operation):
            raise SystemExit(f"RIG {method.upper()} {path} differs from api/openapi.yaml")
    for name in ("ByeResponse", "RebuildRequest", "RebuildData", "RebuildResponse",
                 "AiProviderSetting", "AiProviderSettingUpdateRequest",
                 "AiProviderSettingResponse", "AiProviderSettingsResponse",
                 "AiProviderSettingDeleteResponse", "AiProviderCredential",
                 "AiProviderCredentialsListResponse", "AiProviderCredentialUpsertRequest",
                 "AiProviderCredentialUpsertResponse",
                 "ErrorResponse", "Error", "ProjectTag",
                 "ProjectTagListResponse", "RunnerPullProgress", "RunnerPullJob",
                 "RunnerPullStatusResponse", "CardLink", "CardLinkListResponse",
                 "CardReferenceResolveRequest", "CardReferenceCandidate",
                 "CardReferenceResolveData", "CardReferenceResolveResponse"):
        expected = control["components"]["schemas"][name]
        actual = generated["components"]["schemas"][name]
        if actual != expected:
            raise SystemExit(f"RIG {name} differs from api/openapi.yaml")
    print("RIG service metadata, eleven operations, and schemas match api/openapi.yaml")


if __name__ == "__main__":
    main(*sys.argv[1:])
