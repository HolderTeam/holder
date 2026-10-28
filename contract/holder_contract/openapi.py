"""The contract's HTTP operations in OpenAPI 3.2: FastAPI's document of the contract's app.

FastAPI writes the document from the routes in app.py (holder_contract.routes) and the Pydantic
models they use. What is added here is only what makes it the daemon's API: FastAPI's own 422
validation responses go, since the daemon answers an invalid argument with 400 and the error
envelope (the routes declare those themselves); operation IDs go, as in the rest of the daemon's
document; integer bounds are exact again, after FastAPI's models made them floats; and the error
envelope's schemas are the ones RIG's document has, so that the two merge.
Authentication, servers and the API's metadata come from the daemon's document, which these paths
are merged into until every route is in the contract.

ordered() and merge() were ported from RIG's openapi.py (https://github.com/zeth/rig,
src/rig/openapi.py at e32ea68).
"""

from __future__ import annotations

import copy
import json
from typing import Any

from .http_routes import _more_specific, _segments_overlap, route_segments
from .routes import Contract

OPENAPI_VERSION = "3.2.0"
ERROR_SCHEMAS = {
    "Error": {
        "type": "object",
        "properties": {"code": {"type": "string"}, "message": {"type": "string"}},
        "required": ["code", "message"],
    },
    "ErrorResponse": {
        "type": "object",
        "properties": {"ok": {"type": "boolean", "enum": [False]}, "error": {"$ref": "#/components/schemas/Error"}},
        "required": ["ok", "error"],
    },
}
ERROR_RESPONSE = {"$ref": "#/components/schemas/ErrorResponse"}
# What a document with bearer security answers without a valid token, as the daemon's other routes do.
UNAUTHORIZED = {"description": "Unauthorized", "content": {"application/json": {"schema": ERROR_RESPONSE}}}


# FastAPI's own response to a request it cannot validate; the daemon answers 400 instead.
FASTAPI_ONLY = ("HTTPValidationError", "ValidationError")
BOUNDS = ("minimum", "maximum", "exclusiveMinimum", "exclusiveMaximum")
# Each integer format's range, which its native type's schema states as bounds (vocabulary.Native).
FORMAT_RANGES = {"int32": (-(2**31), 2**31 - 1), "int64": (-(2**63), 2**63 - 1)}


def document(contract: Contract) -> dict[str, Any]:
    """The OpenAPI paths and schemas of the contract's routes."""
    result = copy.deepcopy(contract.app.openapi())
    for methods in result["paths"].values():
        for operation in methods.values():
            operation["responses"].pop("422", None)
            operation.pop("operationId", None)
    schemas = result["components"]["schemas"]
    for methods in result["paths"].values():
        for operation in methods.values():
            optional_body(operation, schemas)
    for name in FASTAPI_ONLY:
        schemas.pop(name, None)
    schemas.update(copy.deepcopy(ERROR_SCHEMAS))
    return ordered(named_by_title(exact_bounds({"openapi": OPENAPI_VERSION, "paths": result["paths"],
                                                "components": {"schemas": schemas}})))


def optional_body(operation: dict[str, Any], schemas: dict[str, Any]) -> None:
    """Mark a body whose record has no required field as one that may be left out, as the daemon
    reads a missing one as {}."""
    body = operation.get("requestBody")
    if body is None:
        return
    ref = body["content"]["application/json"]["schema"].get("$ref", "")
    record = schemas.get(ref.rpartition("/")[2], {})
    if record.get("type") == "object" and not record.get("required"):
        body["required"] = False


def exact_bounds(value: Any) -> Any:
    """The document with each integer schema's bounds as integers.

    FastAPI's OpenAPI models hold bounds as floats, so int64's range comes back rounded:
    2**63 - 1 becomes 2.0**63, a value past it. A bound equal to its format's limit is that limit;
    any other must be a whole number a float holds exactly, or it is refused.
    """
    if isinstance(value, list):
        return [exact_bounds(item) for item in value]
    if not isinstance(value, dict):
        return value
    result = {key: exact_bounds(item) for key, item in value.items()}
    if result.get("type") == "integer":
        limits = FORMAT_RANGES.get(result.get("format"), ())
        for key in BOUNDS:
            bound = result.get(key)
            if not isinstance(bound, float):
                continue
            exact = next((limit for limit in limits if float(limit) == bound), None)
            if exact is None and bound.is_integer() and abs(bound) <= 2**53:
                exact = int(bound)
            if exact is None:
                raise ValueError(f"an integer schema's {key} of {bound!r} is not exact")
            result[key] = exact
    return result


def named_by_title(document: dict[str, Any]) -> dict[str, Any]:
    """The document with each schema named by its title.

    Pydantic names a generic record's use after its type (Page_CardListItem_) but titles it with
    model_parametrized_name (CardListItemPage); clients are generated from the names, so the
    readable one is used.
    """
    schemas = document["components"]["schemas"]
    renames = {name: schema["title"] for name, schema in schemas.items()
               if schema.get("title") and schema["title"] != name}
    names = [renames.get(name, name) for name in schemas]
    clashes = sorted({name for name in names if names.count(name) > 1})
    if clashes:
        raise ValueError(f"schemas would share the names {', '.join(clashes)}")
    text = json.dumps(document)
    for old, new in renames.items():
        text = text.replace(f'"#/components/schemas/{old}"', f'"#/components/schemas/{new}"')
    result = json.loads(text)
    result["components"]["schemas"] = {renames.get(n, n): s for n, s in result["components"]["schemas"].items()}
    return result


METHOD_ORDER = ("get", "put", "post", "patch", "delete")
OPERATION_ORDER = ("operationId", "summary", "description", "security", "parameters", "requestBody", "responses")


def _in_order(mapping: dict[str, Any], order: tuple[str, ...]) -> dict[str, Any]:
    """The mapping with known keys in the given order, then any others."""
    keys = [key for key in order if key in mapping] + [key for key in mapping if key not in order]
    return {key: mapping[key] for key in keys}


def ordered(document: dict[str, Any]) -> dict[str, Any]:
    """Order a document so it does not depend on where declarations live.

    Paths and schemas are sorted by name, methods and operation keys follow OpenAPI's usual
    order, and responses are sorted by status.
    """
    paths = {}
    for path in sorted(document["paths"]):
        paths[path] = {}
        for method, operation in _in_order(document["paths"][path], METHOD_ORDER).items():
            operation = _in_order(operation, OPERATION_ORDER)
            operation["responses"] = dict(sorted(operation["responses"].items()))
            paths[path][method] = operation
    result = _in_order(document, ("openapi", "info", "servers", "security", "paths", "components"))
    result["paths"] = paths
    components = document["components"]
    result["components"] = {**({"securitySchemes": components["securitySchemes"]} if "securitySchemes" in components
                               else {}),
                            "schemas": dict(sorted(components["schemas"].items()))}
    return result


def merge(daemon: dict[str, Any], contract: dict[str, Any]) -> dict[str, Any]:
    """The daemon's document with the contract's paths and schemas added.

    The daemon's document is RIG's, for the routes not yet moved to the contract, and has the
    API's metadata and security; the contract's operations get the same 401 as the daemon's.
    A route in both, or a schema of the same name with another shape, is an error. The daemon tries
    RIG's routes first, so a RIG route that would take a request meant for one of the contract's,
    without being more specific than it, is an error too.
    """
    for path, methods in contract["paths"].items():
        for rig_path, rig_methods in daemon["paths"].items():
            if rig_path == path:
                continue
            shared = set(methods) & set(rig_methods)
            ours, theirs = route_segments(path), route_segments(rig_path)
            if shared and _segments_overlap(ours, theirs) and not _more_specific(theirs, ours):
                method = min(shared).upper()
                raise ValueError(f"the daemon's {method} {rig_path} would take requests for the contract's "
                                 f"{method} {path}")
    out = copy.deepcopy(daemon)
    for path, methods in contract["paths"].items():
        target = out["paths"].setdefault(path, {})
        for method, operation in methods.items():
            if method in target:
                raise ValueError(f"{method.upper()} {path} is both a daemon route and a contract operation")
            operation = copy.deepcopy(operation)
            if out.get("security"):
                operation["responses"]["401"] = copy.deepcopy(UNAUTHORIZED)
            target[method] = operation
    schemas = out["components"].setdefault("schemas", {})
    for name, schema in contract["components"]["schemas"].items():
        if name in schemas and schemas[name] != schema:
            raise ValueError(f"the schema {name} is declared by the daemon and by the contract, with different shapes")
        schemas[name] = copy.deepcopy(schema)
    return ordered(out)


def dumps(document: dict[str, Any], output_format: str = "json") -> str:
    """Serialize a document as JSON or YAML, with a final newline."""
    if output_format == "json":
        return json.dumps(document, indent=2) + "\n"
    import yaml  # Only YAML output needs PyYAML.

    # Lists and maps of plain values stay on one line, as hand-written OpenAPI usually has them.
    return yaml.safe_dump(document, sort_keys=False, allow_unicode=True, width=100, default_flow_style=None)

