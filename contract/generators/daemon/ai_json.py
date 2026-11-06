"""Generate the JSON codec for the daemon's AI records (AiJson.h) from the contract's typed IR.

Its records are the ones the AI routes take, return or send as events: every record reachable from
an operation with a daemon runtime binding (bindings.py).
"""

from __future__ import annotations

import json

from authoring.routes import Contract
from compiler.ir import (
    ContractView,
    FieldView,
    ListType,
    LiteralType,
    MapType,
    NamedType,
    NullableType,
    RecordView,
    StreamType,
    TypeRef,
    UnionType,
)

from ..common import Unsupported, contract_sources, header_lines
from .bindings import bindings as load_bindings
from .bindings import cpp_name
from .json_support import SUPPORT


def _field_type(record: str, field: str) -> str:
    return f"std::remove_cvref_t<decltype(std::declval<{record}>().{field})>"


def _unwrapped(ref: TypeRef) -> TypeRef:
    while isinstance(ref, NullableType):
        ref = ref.value
    return ref


def _validation(ref: TypeRef, expression: str, name: str) -> str:
    ref = _unwrapped(ref)
    kind = ref.kind
    if kind == "string":
        low = 0 if ref.min_length is None else ref.min_length
        high = "std::numeric_limits<std::size_t>::max()" if ref.max_length is None else ref.max_length
        return f'ai_json::check_text({expression}, "{name}", {low}, {high})'
    if isinstance(ref, LiteralType):
        checks = " && ".join(f"result != {json.dumps(v)}" for v in ref.values)
        return (f'[&] {{ auto result = {expression}; if ({checks}) '
                f'throw std::invalid_argument("invalid {name}"); return result; }}()')
    if kind in ("int32", "int64") and (ref.ge is not None or ref.le is not None):
        low = "std::numeric_limits<long long>::min()" if ref.ge is None else ref.ge
        high = "std::numeric_limits<long long>::max()" if ref.le is None else ref.le
        return f'ai_json::check_range({expression}, "{name}", {low}, {high})'
    return expression


def ai_records(contract: ContractView, authoring: Contract) -> list[RecordView]:
    """The records reachable from the operations with daemon runtime bindings, in IR order."""
    bound = load_bindings().keys()
    aliases = {alias.name: alias.type for alias in contract.aliases}
    records = {record.name: record for record in contract.records}
    reached: set[str] = set()

    def reach(ref: TypeRef) -> None:
        if isinstance(ref, NamedType):
            if ref.name in aliases:
                reach(aliases[ref.name])
            elif ref.name in records and ref.name not in reached:
                reached.add(ref.name)
                record = records[ref.name]
                if record.base:
                    reach(NamedType(name=record.base))
                for field in record.fields:
                    reach(field.type)
            for argument in ref.arguments:
                reach(argument)
        elif isinstance(ref, NullableType):
            reach(ref.value)
        elif isinstance(ref, ListType | StreamType):
            reach(ref.items)
        elif isinstance(ref, MapType):
            reach(ref.values)
        elif isinstance(ref, UnionType):
            for alternative in ref.alternatives:
                reach(alternative)

    for operation in contract.operations:
        if operation.name in bound:
            for parameter in operation.parameters:
                reach(parameter.type)
            reach(operation.returns)
    # Stream events are not in the IR yet; the authoring Contract still holds them.
    for operation in authoring.operations:
        if operation.function.__name__ in bound and operation.sse_events:
            for record in operation.sse_events.values():
                reach(NamedType(name=record.__name__))
    return [record for record in contract.records if record.name in reached]


def _decode_lines(qualified: str, field: FieldView) -> list[str]:
    name = field.name
    key = json.dumps(name)
    optional = field.omittable
    nullable = isinstance(field.type, NullableType)
    field_type = _field_type(qualified, name)
    decoded = f"codec<{field_type}>::decode(value.at({key}))"
    if optional and nullable:
        decoded = f"decode_optional_nullable<{field_type}>(value.at({key}))"
    if not optional and not nullable:
        decoded = _validation(field.type, decoded, name)
    lines = []
    if isinstance(_unwrapped(field.type), MapType):
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
    return lines


def generate_ai_codec(contract: ContractView, authoring: Contract) -> str:
    records = ai_records(contract, authoring)
    names = {record.name for record in records}
    if len(names) != len(records):
        raise Unsupported("duplicate AI record names")
    lines = [*header_lines(contract_sources(contract),
                           "The daemon AI JSON codec, generated from contract/interface/ai/models.py.", "cpp"),
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
        qualified = cpp_name(record.name)
        lines += [f"template <> struct codec<{qualified}> {{",
                  f"    static json encode(const {qualified}& value);",
                  f"    static {qualified} decode(const json& value);", "};"]
    lines.append("")
    for record in records:
        qualified = cpp_name(record.name)
        lines += [f"inline json codec<{qualified}>::encode(const {qualified}& value) {{",
                  "    json result = json::object();"]
        for field in record.fields:
            key = json.dumps(field.name)
            encoded = f"codec<{_field_type(qualified, field.name)}>::encode(value.{field.name})"
            lines += [f"    if (value.{field.name}) result[{key}] = {encoded};" if field.omittable
                      else f"    result[{key}] = {encoded};"]
        lines += ["    return result;", "}",
                  f"inline {qualified} codec<{qualified}>::decode(const json& value) {{",
                  f'    if (!value.is_object()) throw std::invalid_argument("expected {record.name}");',
                  f"    {qualified} out{{}};"]
        for field in record.fields:
            lines += _decode_lines(qualified, field)
        lines += ["    return out;", "}", ""]
    lines += ["template <typename T> json encode(const T& value) { return codec<T>::encode(value); }",
              "template <typename T> T decode(const json& value) { return codec<T>::decode(value); }",
              "} // namespace ai_json", "", "namespace ai_events {"]
    events = {(name, record.__name__) for op in authoring.operations if op.sse_events
              for name, record in op.sse_events.items()}
    for name, record in sorted(events):
        if record not in names:
            continue
        qualified = cpp_name(record)
        lines += [f"inline api_runtime::event<nlohmann::json> {name}(const {qualified}& data) {{",
                  f'    return {{"{name}", ai_json::encode(data)}};', "}"]
    lines += ["} // namespace ai_events", ""]
    return "\n".join(lines)
