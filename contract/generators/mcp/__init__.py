"""Generate the live daemon's MCP tool definitions from the compiled contract."""

from __future__ import annotations

import json
from dataclasses import replace
from typing import Any

from compiler.ir import ContractView

from ..clang_format import clang_format
from ..common import Unsupported, header_lines
from ..core.types import CoreTypes, Slot, camel
from ..daemon.contract_json import cpp_base, enum_lines, field_decoder
from ..daemon.openapi import document

PATH = "daemon/src/api/generated/McpTools.json"
BINDINGS_PATH = "daemon/src/api/generated/McpBindings.h"
# Select reads whose arguments and results stay inside the authorized project.
TOOLS = (
    "get_card", "get_card_breadcrumbs", "get_resource", "list_card_backlinks",
    "list_card_links", "list_card_milestones", "list_card_resources", "list_cards", "list_project_tags", "search_cards",
)
PAGE_LIMITS = {"list_cards": 100, "search_cards": 100}
PAGE_DEFAULTS = {"list_cards": 50}
FIXED_ARGUMENTS = {
    "list_cards": {"include_content": False},
    "list_card_links": {"include_deleted": False},
    "list_card_backlinks": {"include_deleted": False, "scope": "local"},
}
LOCAL_LINK_TOOLS = {"list_card_links", "list_card_backlinks"}
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
        fixed = FIXED_ARGUMENTS.get(name, {})
        properties = {
            key: dict(parameter["schema"]) for key, parameter in parameters.items()
            if key not in fixed
        }
        properties["project_id"]["description"] = "Omit to use the project granted to this MCP token."
        if name in PAGE_LIMITS:
            limit = properties["limit"]
            maximum = PAGE_LIMITS[name]
            limit["maximum"] = min(maximum, limit.get("maximum", maximum))
        if name in PAGE_DEFAULTS:
            properties["limit"]["default"] = PAGE_DEFAULTS[name]
        if "include_content" in fixed:
            description += "\n\nThis tool lists metadata. Use get_card to read one card body."
        if name in LOCAL_LINK_TOOLS:
            description += "\n\nBoth ends must belong to the authorized project. Entries in Trash are excluded."
        tools.append({
            "name": name,
            "description": description,
            "inputSchema": standalone_schema({
                "type": "object",
                "properties": properties,
                "required": [key for key, parameter in parameters.items()
                             if key in properties and key != "project_id" and parameter["required"]],
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


def optional_string_choice(types: CoreTypes, operation: str, slot: Slot) -> list[str]:
    """Decode a nullable string-or-literal parameter such as the card parent filter."""
    name = slot.name
    choices = [types.resolve(alternative).type for alternative in slot.type.alternatives]
    literals = [literal for item in choices if item.kind == "literal" for literal in item.values]
    strings = [item for item in choices if item.kind == "string"]
    if (not slot.nullable or not slot.has_default or slot.default is not None
            or len(strings) != 1 or not literals
            or any(item.kind not in {"literal", "string"} for item in choices)
            or any(not isinstance(literal, str) for literal in literals)):
        raise Unsupported(f"{operation}.{name}: unsupported MCP union")
    variant = "holder::contract::" + camel(operation) + camel(name)
    decoded = field_decoder(types, replace(slot, type=strings[0]), name, "*value")
    lines = [
        f"    std::optional<{variant}> {name};",
        f'    if (const auto value = arguments.find("{name}"); value != arguments.end() && !value->is_null()) {{',
        f'      auto text = decode_field<std::string>(*value, "{name}");',
    ]
    for index, literal in enumerate(literals):
        condition = "if" if index == 0 else "else if"
        lines.append(f"      {condition} (text == {json.dumps(literal)}) {name} = {variant}{{{variant}{camel(literal)}{{}}}};")
    lines += [f"      else {name} = {variant}{{{decoded}}};", "    }"]
    return lines


def generate_mcp_bindings(contract: ContractView) -> str:
    """Bind each allowed tool to its typed core operation and use the daemon JSON codec."""
    operations = [contract.operation(name) for name in TOOLS]
    lines = header_lines((op.source for op in operations if op.source),
                         "Live MCP read tool bindings.", "cpp")
    lines += [
        "#pragma once",
        '#include "api/generated/ContractJson.h"',
        "#include <functional>",
        "#include <vector>",
        "#include <string_view>",
        "#include <utility>",
    ]
    types = CoreTypes(contract)
    slots_by_operation = {}
    lines.append("namespace contract_json {")
    for operation in operations:
        slots = [types.parameter_slot(parameter) for parameter in operation.parameters]
        for slot in slots:
            if slot.type.kind == "literal":
                slot.enum = camel(operation.name) + camel(slot.name)
                if slot.name not in FIXED_ARGUMENTS.get(operation.name, {}):
                    lines.extend(enum_lines(slot))
        slots_by_operation[operation.name] = slots
    lines += [
        "} // namespace contract_json",
        "namespace holder::api::mcp {",
        "// Authorization must throw on denial. Domain errors propagate to the protocol adapter.",
        "inline nlohmann::json call_read_tool(holder_context& context, std::string_view name,",
        "    const nlohmann::json& arguments,",
        "    std::string_view default_project,",
        "    const std::function<void(std::string_view)>& authorize_project) {",
        "  using namespace contract_json;",
        '  if (!arguments.is_object()) throw std::invalid_argument("tool arguments must be an object");',
    ]
    for operation in operations:
        slots = slots_by_operation[operation.name]
        fixed = FIXED_ARGUMENTS.get(operation.name, {})
        if (operation.daemon_only or operation.context != "required"
                or not any(slot.name == "project_id" and slot.type.kind == "string" for slot in slots)):
            raise Unsupported(f"{operation.name}: MCP requires a project-scoped Core operation")
        lines.append(f'  if (name == "{operation.name}") {{')
        unknown = " && ".join(f'it.key() != "{slot.name}"' for slot in slots if slot.name not in fixed)
        lines.extend([
            "    for (auto it = arguments.begin(); it != arguments.end(); ++it) {",
            f'      if ({unknown}) throw std::invalid_argument("unknown tool argument");',
            "    }",
        ])
        args = []
        for slot in slots:
            if slot.type.kind == "union":
                lines.extend(optional_string_choice(types, operation.name, slot))
                args.append(f"std::move({slot.name})")
                continue
            if slot.type.kind not in {"string", "int32", "bool", "literal"} or any(
                getattr(slot.type, key) is not None for key in ("pattern", "gt", "lt")
            ):
                raise Unsupported(f"{operation.name}.{slot.name}: unsupported MCP argument")
            if slot.name in fixed:
                value = fixed[slot.name]
                if slot.type.kind == "literal":
                    if value not in slot.type.values:
                        raise Unsupported(f"{operation.name}.{slot.name}: unknown fixed MCP value")
                    args.append(f"holder::contract::{slot.enum}::{camel(value)}")
                else:
                    args.append(json.dumps(value))
                continue
            arg = slot.name
            keep_project = arg == "project_id" and operation.name in LOCAL_LINK_TOOLS
            args.append(f"std::move({arg})" if slot.type.kind == "string" and not keep_project else arg)
            if arg == "limit" and operation.name in PAGE_LIMITS:
                cap = PAGE_LIMITS[operation.name]
                maximum = cap if slot.type.le is None else min(cap, slot.type.le)
                slot = replace(slot, type=slot.type.model_copy(update={"le": maximum}))
            if slot.nullable:
                if not slot.has_default or slot.default is not None:
                    raise Unsupported(f"{operation.name}.{arg}: MCP needs a null default")
                base = cpp_base(types, slot.type, slot.enum)
                decoded = field_decoder(types, slot, arg, "*value")
                lines += [
                    f"    std::optional<{base}> {arg};",
                    f'    if (const auto value = arguments.find("{arg}"); value != arguments.end() && !value->is_null())',
                    f"      {arg} = {decoded};",
                ]
                continue
            if arg == "project_id":
                source = 'arguments.value("project_id", json(default_project))'
            elif slot.has_default:
                default = PAGE_DEFAULTS.get(operation.name, slot.default) if arg == "limit" else slot.default
                if type(default) not in {int, str, bool}:
                    raise Unsupported(f"{operation.name}.{arg}: unsupported MCP default")
                source = f'arguments.value("{arg}", json({json.dumps(default)}))'
            else:
                lines.append(f'    if (!arguments.contains("{arg}")) '
                             f'throw std::invalid_argument("{arg} is required");')
                source = f'arguments.at("{arg}")'
            decode = field_decoder(types, slot, arg, source)
            lines.append(f"    auto {arg} = {decode};")
        lines.append("    authorize_project(project_id);")
        args = ", ".join(args)
        call = f"holder::contract::{operation.name}(context, {args})"
        if operation.name in LOCAL_LINK_TOOLS:
            lines += [
                f"    auto links = {call};",
                "    std::erase_if(links, [&project_id](const auto& link) {",
                "      return link.source_project_id != project_id || link.target_project_id != project_id;",
                "    });",
            ]
            call = "links"
        lines.append(f'    return json{{{{"data", encode({call})}}}};')
        lines.append("  }")
    lines += ['  throw std::invalid_argument("unknown MCP tool");', "}", "} // namespace holder::api::mcp", ""]
    return clang_format("\n".join(lines), BINDINGS_PATH)
