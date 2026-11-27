"""Generate the live daemon's MCP tool definitions from the compiled contract."""

from __future__ import annotations

import json
from dataclasses import replace
from typing import Any

from compiler.ir import ContractView

from ..clang_format import clang_format
from ..common import Unsupported, header_lines
from ..core.types import CoreTypes
from ..daemon.contract_json import field_decoder
from ..daemon.openapi import document

PATH = "daemon/src/api/generated/McpTools.json"
BINDINGS_PATH = "daemon/src/api/generated/McpBindings.h"
# The live MCP adapter starts with these reads. The registry applies runtime permissions.
TOOLS = ("get_card", "get_resource", "search_cards")
SEARCH_LIMIT = 100
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
        properties = {key: parameter["schema"] for key, parameter in parameters.items()}
        if name == "search_cards":
            limit = properties["limit"]
            limit["maximum"] = min(SEARCH_LIMIT, limit.get("maximum", SEARCH_LIMIT))
        tools.append({
            "name": name,
            "description": description,
            "inputSchema": standalone_schema({
                "type": "object",
                "properties": properties,
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


def generate_mcp_bindings(contract: ContractView) -> str:
    """Bind each allowed tool to its typed core operation and use the daemon JSON codec."""
    operations = [contract.operation(name) for name in TOOLS]
    lines = header_lines((op.source for op in operations if op.source),
                         "Live MCP read tool bindings.", "cpp")
    lines += [
        "#pragma once",
        '#include "api/generated/ContractJson.h"',
        "#include <functional>",
        "#include <string_view>",
        "#include <utility>",
        "namespace holder::api::mcp {",
        "// Authorization must throw on denial. Domain errors propagate to the protocol adapter.",
        "inline nlohmann::json call_read_tool(holder_context& context, std::string_view name,",
        "    const nlohmann::json& arguments,",
        "    const std::function<void(std::string_view)>& authorize_project) {",
        "  using namespace contract_json;",
        '  if (!arguments.is_object()) throw std::invalid_argument("tool arguments must be an object");',
    ]
    types = CoreTypes(contract)
    for operation in operations:
        slots = [types.parameter_slot(parameter) for parameter in operation.parameters]
        if (operation.daemon_only or operation.context != "required"
                or not any(slot.name == "project_id" and slot.type.kind == "string" for slot in slots)):
            raise Unsupported(f"{operation.name}: MCP requires a project-scoped Core operation")
        lines.append(f'  if (name == "{operation.name}") {{')
        unknown = " && ".join(f'it.key() != "{slot.name}"' for slot in slots)
        lines.extend([
            "    for (auto it = arguments.begin(); it != arguments.end(); ++it) {",
            f'      if ({unknown}) throw std::invalid_argument("unknown tool argument");',
            "    }",
        ])
        for slot in slots:
            if (slot.nullable or slot.type.kind not in {"string", "int32"}
                    or any(getattr(slot.type, key) is not None for key in ("pattern", "gt", "lt"))):
                raise Unsupported(f"{operation.name}.{slot.name}: unsupported MCP argument")
            if slot.has_default:
                if type(slot.default) is not int or slot.type.kind != "int32":
                    raise Unsupported(f"{operation.name}.{slot.name}: unsupported MCP default")
                source = f'arguments.value("{slot.name}", json({slot.default}))'
            else:
                lines.append(f'    if (!arguments.contains("{slot.name}")) '
                             f'throw std::invalid_argument("{slot.name} is required");')
                source = f'arguments.at("{slot.name}")'
            if operation.name == "search_cards" and slot.name == "limit":
                maximum = SEARCH_LIMIT if slot.type.le is None else min(SEARCH_LIMIT, slot.type.le)
                slot = replace(slot, type=slot.type.model_copy(update={"le": maximum}))
            decode = field_decoder(types, slot, slot.name, source)
            lines.append(f"    auto {slot.name} = {decode};")
        lines.append("    authorize_project(project_id);")
        args = ", ".join(f"std::move({slot.name})" if slot.type.kind == "string" else slot.name for slot in slots)
        lines.append(f'    return json{{{{"data", encode(holder::contract::{operation.name}(context, {args}))}}}};')
        lines.append("  }")
    lines += ['  throw std::invalid_argument("unknown MCP tool");', "}", "} // namespace holder::api::mcp", ""]
    return clang_format("\n".join(lines), BINDINGS_PATH)
