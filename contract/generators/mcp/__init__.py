"""Generate the live daemon's MCP tool definitions from the compiled contract."""

from __future__ import annotations

import json
from typing import Any

from compiler.ir import ContractView

from ..daemon.openapi import document

PATH = "daemon/src/api/generated/McpTools.json"
# The live MCP adapter starts with these reads. The registry applies runtime permissions.
TOOLS = ("get_card", "get_resource", "search_cards")
COMPONENT_REF = "#/components/schemas/"


def standalone_schema(schema: dict[str, Any], components: dict[str, Any]) -> dict[str, Any]:
    """Give a tool schema its own definitions, including transitive and recursive references."""
    definitions: dict[str, Any] = {}

    def rewrite(value: Any) -> Any:
        if isinstance(value, list):
            return [rewrite(item) for item in value]
        if not isinstance(value, dict):
            return value
        result = {}
        for key, item in value.items():
            if key == "$ref":
                if not item.startswith(COMPONENT_REF):
                    raise ValueError(f"Unsupported MCP schema reference: {item}")
                name = item.removeprefix(COMPONENT_REF)
                if name not in definitions:
                    definitions[name] = {}
                    definitions[name] = rewrite(components[name])
                result[key] = f"#/$defs/{name}"
            elif key == "discriminator":
                # OpenAPI's discriminator is a hint. The JSON Schema branches validate the tag.
                continue
            else:
                result[key] = rewrite(item)
        return result

    result = rewrite(schema)
    if definitions:
        result["$defs"] = definitions
    return result


def generate_mcp_tools(contract: ContractView) -> str:
    """Use the compiler's schema projection for the selected operations' JSON types."""
    schemas = document(contract)
    components = schemas["components"]["schemas"]
    tools = []
    for name in TOOLS:
        operation = contract.operation(name)
        bindings = [binding for binding in contract.http if binding.operation == name]
        if len(bindings) != 1:
            raise ValueError(f"MCP tool {name} requires one daemon binding")
        binding = bindings[0]
        if binding.method.upper() != "GET" or binding.response_mode != "envelope" or binding.body or binding.headers:
            raise ValueError(f"MCP tool {name} requires a JSON read with path or query parameters")

        route = schemas["paths"][binding.path]["get"]
        parameters = {parameter["name"]: parameter for parameter in route.get("parameters", [])}
        if parameters.keys() != {parameter.name for parameter in operation.parameters}:
            raise ValueError(f"MCP tool {name} does not describe every operation argument")
        response = route["responses"][str(binding.status)]["content"]["application/json"]["schema"]
        envelope = components[response["$ref"].removeprefix(COMPONENT_REF)]
        description = "\n\n".join(dict.fromkeys(filter(None, (operation.summary, operation.description))))
        tools.append({
            "name": name,
            "description": description,
            "inputSchema": standalone_schema({
                "type": "object",
                "properties": {name: parameter["schema"] for name, parameter in parameters.items()},
                "required": [name for name, parameter in parameters.items() if parameter["required"]],
                "additionalProperties": False,
            }, components),
            "outputSchema": standalone_schema({
                "type": "object",
                "properties": {"data": envelope["properties"]["data"]},
                "required": ["data"],
                "additionalProperties": False,
            }, components),
        })
    return json.dumps({"tools": tools}, ensure_ascii=False, indent=2) + "\n"
