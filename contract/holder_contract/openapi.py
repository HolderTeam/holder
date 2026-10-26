"""Describe the contract's HTTP operations in OpenAPI 3.2.

Ported from RIG's openapi.py (https://github.com/zeth/rig, src/rig/openapi.py at e32ea68), which
read declarations reflected from C++; this reads the contract's IR and its HTTP bindings. The
result has the operations' paths and the schemas they use, in the shapes contract_http::dispatch
(http_routes.py) sends and accepts. Authentication, servers and the API's metadata belong to the
daemon's document, which these paths are merged into.
"""

from __future__ import annotations

import json
from typing import Any

from .generate import Instance, Model, Unsupported

OPENAPI_VERSION = "3.2.0"
SCALARS = {
    "string": {"type": "string"},
    "int32": {"type": "integer", "format": "int32"},
    "int64": {"type": "integer", "format": "int64"},
    "float64": {"type": "number", "format": "double"},
    "bool": {"type": "boolean"},
}
KEYWORDS = (("min_length", "minLength"), ("max_length", "maxLength"), ("pattern", "pattern"),
            ("ge", "minimum"), ("le", "maximum"), ("gt", "exclusiveMinimum"), ("lt", "exclusiveMaximum"))
# The error body every failure has, as contract_http::failure writes it.
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


def schema_name(instance: Instance) -> str:
    """A record's schema name; a generic record's names its arguments first, as CardListItemPage."""
    return "".join([*instance.arguments, instance.record])


class Converter:
    def __init__(self, model: Model):
        self.model = model
        self.components: dict[str, Any] = {}

    def schema(self, ref: dict[str, Any]) -> dict[str, Any]:
        ref = self.model.resolve(ref)
        kind = ref["kind"]
        if kind in SCALARS:
            result = dict(SCALARS[kind])
        elif kind == "literal":
            result = {"type": "string" if isinstance(ref["values"][0], str) else "integer", "enum": list(ref["values"])}
        elif kind == "list":
            result = {"type": "array", "items": self.schema(ref["items"])}
        elif kind == "nullable":
            inner = self.schema({**self.model.resolve(ref["value"]),
                                 **{k: v for k, v in ref.items() if k not in ("kind", "value", "alias")}})
            # OpenAPI 3.1's idiom: a null type beside a typed schema, anyOf for a reference.
            if isinstance(inner.get("type"), str):
                inner["type"] = [inner["type"], "null"]
                if "enum" in inner:
                    inner["enum"].append(None)
                return inner
            return {"anyOf": [inner, {"type": "null"}]}
        elif kind == "union":
            result = {"oneOf": [self.schema(a) for a in ref["alternatives"]]}
        elif kind == "named":
            instance = self.model.instance(ref, {})
            self.record(instance)
            return {"$ref": f"#/components/schemas/{schema_name(instance)}"}
        else:
            raise Unsupported(f"{kind} in OpenAPI")
        for key, keyword in KEYWORDS:
            if key in ref:
                result[keyword] = ref[key]
        return result

    def record(self, instance: Instance) -> None:
        name = schema_name(instance)
        if name in self.components:
            return
        self.components[name] = {}  # reserved, for records that refer to themselves
        declaration = self.model.records[instance.record]
        own = {f["name"] for f in declaration["fields"]}
        properties, required = {}, []
        for slot in self.model.slots(instance):
            if slot.name not in own:
                continue
            property_schema = self.schema({"kind": "nullable", "value": slot.type} if slot.nullable else slot.type)
            if slot.doc:
                if "$ref" in property_schema:
                    property_schema = {"allOf": [property_schema]}
                property_schema["description"] = slot.doc
            if slot.has_default and slot.default is not None:
                property_schema["default"] = slot.default
            properties[slot.name] = property_schema
            if not slot.optional:
                required.append(slot.name)
        result: dict[str, Any] = {"type": "object", "properties": properties}
        if required:
            result["required"] = required
        if "base" in declaration:
            base = Instance(declaration["base"])
            self.record(base)
            result = {"allOf": [{"$ref": f"#/components/schemas/{schema_name(base)}"}, result]}
        if declaration.get("doc"):
            result["description"] = declaration["doc"]
        self.components[name] = result

    def parameter_schema(self, parameter: dict[str, Any]) -> dict[str, Any]:
        """A path or query parameter's schema: its value's, without null, which a URL cannot say."""
        ref = self.model.resolve(parameter["type"])
        if ref["kind"] == "nullable":
            ref = {**self.model.resolve(ref["value"]), **{k: v for k, v in ref.items() if k not in ("kind", "value")}}
        result = self.schema(ref)
        if parameter.get("default") is not None:
            result["default"] = parameter["default"]
        return result

    def operation(self, op: dict[str, Any], binding: dict[str, Any], statuses: dict[str, int]) -> dict[str, Any]:
        result: dict[str, Any] = {}
        for key in ("summary", "description"):
            if op.get(key):
                result[key] = op[key]
        parameters, body = [], None
        for parameter in op["parameters"]:
            name = parameter["name"]
            if name in binding["path_parameters"]:
                item = {"name": name, "in": "path", "required": True, "schema": self.parameter_schema(parameter)}
            elif name in binding["query"]:
                nullable = self.model.resolve(parameter["type"])["kind"] == "nullable"
                item = {"name": name, "in": "query", "required": not nullable and "default" not in parameter,
                        "schema": self.parameter_schema(parameter)}
            else:
                body = parameter
                continue
            if parameter.get("doc"):
                item["description"] = parameter["doc"]
            parameters.append(item)
        if parameters:
            result["parameters"] = parameters
        if body is not None:
            result["requestBody"] = {"required": True,
                                     "content": {"application/json": {"schema": self.schema(body["type"])}}}
        data = self.schema(op["returns"])
        envelope = {"type": "object",
                    "properties": {"ok": {"type": "boolean", "enum": [True]}, "data": data},
                    "required": ["ok", "data"]}
        responses = {str(binding["status"]): {"description": "Success",
                                              "content": {"application/json": {"schema": envelope}}}}
        descriptions: dict[int, list[str]] = {400: ["Invalid argument."]}
        for error in op["errors"]:
            doc = next(e for e in self.model.errors if e["name"] == error)["doc"]
            descriptions.setdefault(statuses[error], []).append(doc or error)
        descriptions.setdefault(500, []).append("Unexpected failure.")
        for status, texts in descriptions.items():
            responses[str(status)] = {"description": " ".join(texts),
                                      "content": {"application/json": {"schema": dict(ERROR_RESPONSE)}}}
        result["responses"] = responses
        return result


def convert(model: Model) -> dict[str, Any]:
    """The OpenAPI paths and schemas of the contract's HTTP operations."""
    converter = Converter(model)
    operations = {op["name"]: op for op in model.operations}
    statuses = {e["error"]: e["status"] for e in model.ir["http_errors"]}
    paths: dict[str, dict[str, Any]] = {}
    for binding in model.ir["http"]:
        if binding["operation"] not in operations:
            continue
        paths.setdefault(binding["path"], {})[binding["method"].lower()] = converter.operation(
            operations[binding["operation"]], binding, statuses)
    components = {**ERROR_SCHEMAS, **converter.components}
    return ordered({"openapi": OPENAPI_VERSION, "paths": paths, "components": {"schemas": components}})


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
    result["components"] = {"schemas": dict(sorted(document["components"]["schemas"].items()))}
    return result


def dumps(document: dict[str, Any], output_format: str = "json") -> str:
    """Serialize a document as JSON or YAML, with a final newline."""
    if output_format == "json":
        return json.dumps(document, indent=2) + "\n"
    import yaml  # Only YAML output needs PyYAML.

    # Lists and maps of plain values stay on one line, as hand-written OpenAPI usually has them.
    return yaml.safe_dump(document, sort_keys=False, allow_unicode=True, width=100, default_flow_style=None)

