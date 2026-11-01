"""Generate the JSON codec for the daemon's AI records from the Python contract."""

from __future__ import annotations

import json
from typing import Any

from authoring.routes import Contract

from .generate import Unsupported, header_lines
from .json_codec import SUPPORT


def _cpp_name(name: str) -> str:
    namespace = ("holder::api::routes" if name in {
        "CardLink", "CardLinkCreateRequest", "CardLinkCreateResponse", "CardLinkListResponse"
    } else "holder::api")
    return f"{namespace}::{name}"


def _field_type(record: str, field: str) -> str:
    return f"std::remove_cvref_t<decltype(std::declval<{record}>().{field})>"


def _unwrapped(ref: dict[str, Any]) -> dict[str, Any]:
    while ref["kind"] == "nullable":
        ref = ref["value"]
    return ref


def _validation(ref: dict[str, Any], expression: str, name: str) -> str:
    ref = _unwrapped(ref)
    kind = ref["kind"]
    if kind == "string":
        return (f'ai_json::check_text({expression}, "{name}", {ref.get("min_length", 0)}, '
                f'{ref.get("max_length", "std::numeric_limits<std::size_t>::max()")})')
    if kind == "literal":
        values = ref["values"]
        checks = " && ".join(f"result != {json.dumps(v)}" for v in values)
        return (f'[&] {{ auto result = {expression}; if ({checks}) '
                f'throw std::invalid_argument("invalid {name}"); return result; }}()')
    if kind in ("int32", "int64") and ("ge" in ref or "le" in ref):
        low = ref.get("ge", "std::numeric_limits<long long>::min()")
        high = ref.get("le", "std::numeric_limits<long long>::max()")
        return f'ai_json::check_range({expression}, "{name}", {low}, {high})'
    return expression


def generate_ai_codec(ir: dict[str, Any], contract: Contract) -> str:
    # Path-based until this generator moves to typed IR; the AI domain package owns these records.
    records = [r for r in ir["records"] if r["source"].startswith("interface/ai/")]
    names = {r["name"] for r in records}
    if len(names) != len(records):
        raise Unsupported("duplicate AI record names")
    lines = [*header_lines(ir, "The daemon AI JSON codec, generated from contract/interface/ai/models.py.", "cpp"),
             "#pragma once", '#include "api/AiRuntime.h"',
             "#include <nlohmann/json.hpp>", "#include <cmath>", "#include <cstddef>",
             "#include <cstdint>", "#include <limits>", "#include <map>", "#include <optional>",
             "#include <stdexcept>", "#include <string>", "#include <type_traits>",
             "#include <utility>", "#include <variant>", "#include <vector>",
             SUPPORT.replace("contract_json", "ai_json").rstrip(),
             "template <> struct codec<json> {",
             "    static json encode(const json& value) { return value; }",
             "    static json decode(const json& value) { return value; }",
             "};",
             "template <typename T> struct codec<std::optional<T>> {",
             "    static json encode(const std::optional<T>& value) {",
             "        return value ? codec<T>::encode(*value) : json(nullptr);",
             "    }",
             "    static std::optional<T> decode(const json& value) {",
             "        return value.is_null() ? std::nullopt : std::optional<T>(codec<T>::decode(value));",
             "    }",
             "};",
             "template <typename T> struct is_optional : std::false_type {};",
             "template <typename T> struct is_optional<std::optional<T>> : std::true_type {};",
             "template <typename T> T decode_optional_nullable(const json& value) {",
             "    if constexpr (is_optional<typename T::value_type>::value) {",
             "        if (value.is_null()) return T{std::in_place, typename T::value_type{}};",
             "    }",
             "    return codec<T>::decode(value);",
             "}",
             ""]
    # The stream events are stored as tagged C++ variants. Their variant alternatives are
    # encoded as records; the event name is supplied by the stream writer.
    lines += ["template <typename... Ts> struct codec<std::variant<Ts...>> {",
              "    static json encode(const std::variant<Ts...>& value) {",
              "        return std::visit([](const auto& item) { return codec<std::decay_t<decltype(item)>>::encode(item); }, value);",
              "    }",
              "};", ""]
    lines += ["template <> struct codec<holder::api::Error> {",
              "    static json encode(const holder::api::Error& value) {",
              "        return {{\"code\", value.code}, {\"message\", value.message}};",
              "    }",
              "};",
              "template <> struct codec<holder::api::ErrorResponse> {",
              "    static json encode(const holder::api::ErrorResponse& value) {",
              "        return {{\"ok\", value.ok}, {\"error\", codec<holder::api::Error>::encode(value.error)}};",
              "    }",
              "};", ""]
    for record in records:
        qualified = _cpp_name(record["name"])
        lines += [f"template <> struct codec<{qualified}> {{",
                  f"    static json encode(const {qualified}& value);",
                  f"    static {qualified} decode(const json& value);", "};"]
    lines.append("")
    for record in records:
        qualified = _cpp_name(record["name"])
        fields = record["fields"]
        lines += [f"inline json codec<{qualified}>::encode(const {qualified}& value) {{",
                  "    json result = json::object();"]
        for field in fields:
            name = field["name"]
            key = json.dumps(name)
            optional = field.get("omittable", False)
            if optional:
                lines += [f"    if (value.{name}) result[{key}] = codec<{_field_type(qualified, name)}>::encode(value.{name});"]
            else:
                lines += [f"    result[{key}] = codec<{_field_type(qualified, name)}>::encode(value.{name});"]
        lines += ["    return result;", "}",
                  f"inline {qualified} codec<{qualified}>::decode(const json& value) {{",
                  f'    if (!value.is_object()) throw std::invalid_argument("expected {record["name"]}");',
                  f"    {qualified} out{{}};"]
        for field in fields:
            name = field["name"]
            key = json.dumps(name)
            optional = field.get("omittable", False)
            nullable = field["type"]["kind"] == "nullable"
            field_type = _field_type(qualified, name)
            decoded = f"codec<{field_type}>::decode(value.at({key}))"
            if optional and nullable:
                decoded = f"decode_optional_nullable<{field_type}>(value.at({key}))"
            if not optional and not nullable:
                decoded = _validation(field["type"], decoded, name)
            inner = _unwrapped(field["type"])
            if inner["kind"] == "map":
                lines += [(f"    if (value.contains({key}) && !value.at({key}).is_null() && "
                           f'!value.at({key}).is_object()) throw std::invalid_argument("{name} must be an object");')]
            if optional:
                lines += [f"    if (value.contains({key})) out.{name} = {decoded};"]
                if not nullable:
                    lines += [(f"    if (value.contains({key}) && value.at({key}).is_null()) "
                               f'throw std::invalid_argument("{name} must not be null");')]
            else:
                lines += [(f"    if (!value.contains({key})) "
                           f'throw std::invalid_argument("missing {name}");'),
                          f"    out.{name} = {decoded};"]
                if not nullable:
                    lines += [(f"    if (value.at({key}).is_null()) "
                               f'throw std::invalid_argument("{name} must not be null");')]
        lines += ["    return out;", "}", ""]
    lines += ["template <typename T> json encode(const T& value) { return codec<T>::encode(value); }",
              "template <typename T> T decode(const json& value) { return codec<T>::decode(value); }",
              "} // namespace ai_json", "", "namespace ai_events {"]
    events = {(name, record.__name__) for op in contract.operations if op.sse_events
              for name, record in op.sse_events.items()}
    for name, record in sorted(events):
        if record not in names:
            continue
        qualified = _cpp_name(record)
        lines += [f"inline api_runtime::event<nlohmann::json> {name}(const {qualified}& data) {{",
                  f'    return {{"{name}", ai_json::encode(data)}};', "}"]
    lines += ["} // namespace ai_events", ""]
    return "\n".join(lines)
