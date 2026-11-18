"""C++ types and codecs for the complete daemon-only boundaries."""

from __future__ import annotations

import json

from compiler.ir import FieldView, MapType, NamedType, NullableType, RecordView, TypeRef

from ..common import Unsupported, contract_sources, header_lines
from .boundary import SERVICE_TYPES, BoundaryPlan
from .json_support import SUPPORT


def cpp_type(plan: BoundaryPlan, ref: TypeRef) -> str:
    ref = plan.resolve(ref)
    if isinstance(ref, NullableType):
        return f"std::optional<{cpp_type(plan, ref.value)}>"
    if isinstance(ref, NamedType):
        return f"holder::api::{ref.name}"
    if isinstance(ref, MapType):
        return f"std::map<std::string, {cpp_type(plan, ref.values)}>"
    if ref.kind == "string":
        return "std::string"
    if ref.kind == "bool":
        return "bool"
    raise Unsupported(f"{ref.kind}: unsupported daemon boundary C++ type")


def cpp_default(field: FieldView) -> str:
    if "default" not in field.model_fields_set or field.default is None:
        return ""
    if type(field.default) is bool:
        return "true" if field.default else "false"
    # Fixed-width octal escapes cannot swallow a following hexadecimal digit.
    return (
        '"'
        + "".join(
            ("\\" + char)
            if char in {'"', "\\"}
            else (f"\\{ord(char):03o}" if ord(char) < 32 or ord(char) == 127 else char)
            for char in field.default
        )
        + '"'
    )


def generate_types(plan: BoundaryPlan) -> str:
    lines = [
        *header_lines(
            contract_sources(plan.contract), "Complete daemon boundary records and handler signatures.", "cpp"
        ),
        "#pragma once",
        "#include <map>",
        "#include <optional>",
        "#include <string>",
        "",
        "namespace holder::platform { class Db; }",
        "namespace holder::privacy { class SecretStore; }",
        "",
        "namespace holder::api {",
    ]
    for record in plan.records:
        lines.append(f"struct {record.name} {{")
        for field in record.fields:
            type_name = cpp_type(plan, field.type)
            if field.omittable:
                type_name = f"std::optional<{type_name}>"
            lines.append(f"    {type_name} {field.name}{{{cpp_default(field)}}};")
        lines += ["};", ""]
    for route in plan.routes:
        if not route.runtime.handler.startswith("holder::api::"):
            raise Unsupported("complete daemon handlers must be in holder::api")
        arguments = [f"{SERVICE_TYPES[service]} {service}" for service in route.runtime.services]
        arguments += [
            f"{cpp_type(plan, route.operation.parameter(name).type)} {name}" for name in route.runtime.arguments
        ]
        lines += [
            f"{cpp_type(plan, route.operation.returns)} {route.runtime.handler.removeprefix('holder::api::')}",
            f"    ({', '.join(arguments)});",
            "",
        ]
    lines += ["} // namespace holder::api", ""]
    return "\n".join(lines)


MAP_SUPPORT = r"""
inline std::map<std::string, std::string> decode_text_map(
    const json& value, const char* name, const char* value_error, std::size_t minimum, std::size_t maximum) {
    if (!value.is_object()) throw std::invalid_argument(std::string(name) + " must be an object");
    std::map<std::string, std::string> result;
    for (auto it = value.begin(); it != value.end(); ++it) {
        if (!it.value().is_string()) throw std::invalid_argument(value_error);
        const auto key_name = std::string(name) + " key";
        const auto key = check_text(it.key(), key_name.c_str(), 0, std::numeric_limits<std::size_t>::max());
        result.emplace(key, check_text(it.value().get_ref<const std::string&>(), name, minimum, maximum));
    }
    return result;
}
"""


def decode_expression(plan: BoundaryPlan, ref: TypeRef, expression: str, label: str, map_error: str) -> str:
    ref = plan.resolve(ref)
    decoded = f"decode_field<{cpp_type(plan, ref)}>({expression}, {json.dumps(label)})"
    if ref.kind == "string":
        low = 0 if ref.min_length is None else ref.min_length
        high = "std::numeric_limits<std::size_t>::max()" if ref.max_length is None else ref.max_length
        return f"check_text({decoded}, {json.dumps(label)}, {low}, {high})"
    if isinstance(ref, MapType):
        values = plan.resolve(ref.values)
        low = 0 if values.min_length is None else values.min_length
        high = "std::numeric_limits<std::size_t>::max()" if values.max_length is None else values.max_length
        return f"decode_text_map({expression}, {json.dumps(label)}, {json.dumps(map_error)}, {low}, {high})"
    return decoded


def record_codec(plan: BoundaryPlan, record: RecordView, map_error: str) -> list[str]:
    qualified = f"holder::api::{record.name}"
    lines = [
        f"inline json codec<{qualified}>::encode(const {qualified}& value) {{",
        "    json result = json::object();",
    ]
    if not record.fields:
        lines.append("    (void)value;")
    for field in record.fields:
        key = json.dumps(field.name)
        expression = f"value.{field.name}"
        ref = plan.resolve(field.type)
        nullable = isinstance(ref, NullableType)
        if field.omittable:
            lines.append(f"    if ({expression}) {{")
            expression = f"*{expression}"
        if nullable:
            encoded = f"{expression} ? codec<{cpp_type(plan, ref.value)}>::encode(*{expression}) : json(nullptr)"
        else:
            encoded = f"codec<{cpp_type(plan, ref)}>::encode({expression})"
        if (record.name, field.name) in plan.error_constants:
            constant = json.dumps(plan.error_constants[(record.name, field.name)])
            lines += [f'    if ({expression} != {constant}) throw std::logic_error("invalid error response constant");']
        lines.append(f"    {'    ' if field.omittable else ''}result[{key}] = {encoded};")
        if field.omittable:
            lines.append("    }")
    lines += [
        "    return result;",
        "}",
        f"inline {qualified} codec<{qualified}>::decode(const json& value) {{",
        '    if (!value.is_object()) throw std::invalid_argument("the request body must be an object");',
    ]
    known = " && ".join(f"it.key() != {json.dumps(field.name)}" for field in record.fields) or "true"
    lines += [
        "    for (auto it = value.begin(); it != value.end(); ++it)",
        f'        if ({known}) throw std::invalid_argument("there is no field " + it.key());',
        f"    {qualified} result{{}};",
    ]
    for field in record.fields:
        key = json.dumps(field.name)
        ref = plan.resolve(field.type)
        nullable = isinstance(ref, NullableType)
        inner = ref.value if nullable else ref
        error = map_error.format(field=field.name)
        expression = decode_expression(plan, inner, f"value.at({key})", field.name, error)
        if not field.omittable and "default" not in field.model_fields_set:
            kind = (
                "an object"
                if isinstance(inner, MapType | NamedType)
                else ("a string" if inner.kind == "string" else "a boolean")
            )
            lines += [
                f"    if (!value.contains({key})) throw std::invalid_argument({json.dumps(field.name + ' must be ' + kind)});"
            ]
        lines.append(f"    if (value.contains({key})) {{")
        if nullable:
            lines += [f"        if (value.at({key}).is_null()) {{"]
            if field.omittable:
                lines.append(f"            result.{field.name}.emplace();")
            else:
                lines.append(f"            result.{field.name}.reset();")
            lines += ["        } else {", f"            result.{field.name} = {expression};", "        }"]
        else:
            lines.append(f"        result.{field.name} = {expression};")
        lines.append("    }")
    lines += ["    return result;", "}", ""]
    return lines


def generate_json(plan: BoundaryPlan) -> str:
    lines = [
        *header_lines(contract_sources(plan.contract), "Complete daemon boundary JSON codecs.", "cpp"),
        "#pragma once",
        '#include "api/generated/DaemonTypes.h"',
        '#include "api/HttpError.h"',
        "#include <nlohmann/json.hpp>",
        "#include <cmath>",
        "#include <cstddef>",
        "#include <cstdint>",
        "#include <limits>",
        "#include <map>",
        "#include <optional>",
        "#include <stdexcept>",
        "#include <string>",
        "#include <vector>",
        SUPPORT.replace("contract_json", "daemon_json").rstrip(),
        MAP_SUPPORT.rstrip(),
        "",
    ]
    records = [*plan.errors, *plan.records]
    for record in records:
        qualified = f"holder::api::{record.name}"
        lines += [
            f"template <> struct codec<{qualified}> {{",
            f"    static json encode(const {qualified}& value);",
            f"    static {qualified} decode(const json& value);",
            "};",
        ]
    for record in records:
        lines += record_codec(plan, record, plan.routes[0].runtime.map_value_error)
    lines += [
        "template <typename T> json encode(const T& value) { return codec<T>::encode(value); }",
        "template <typename T> T decode(const json& value) { return codec<T>::decode(value); }",
        "} // namespace daemon_json",
        "",
    ]
    return "\n".join(lines)
