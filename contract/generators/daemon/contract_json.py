"""Generate the daemon's JSON codec for the contract's C++ types (ContractJson.h, for core/src/api/Contract.h).

Ported from RIG's json_codec.py (https://github.com/zeth/rig, src/rig/json_codec.py at e32ea68),
which read declarations reflected from C++; this reads the contract's typed IR. Each record gets a
codec<T> with encode and decode. Decoding checks what the contract says about the value: required
and omitted fields, null, text (UTF-8 with no U+0000, lengths in characters), integer ranges and
finite numbers. It also rejects fields the record does not have, so that a caller who sends one,
such as a card_id for a card Holder names, learns it was not used. A client reading results
should still ignore fields it does not know, which later versions may add.
"""

from __future__ import annotations

import re

from compiler.ir import ListType, LiteralType, MapType, NamedType, TypeRef

from ..common import Unsupported, contract_sources, header_lines
from ..core.abi import SCALARS_CPP
from ..core.types import CoreTypes, Instance, Slot, camel, constrained, constraint
from .json_support import SUPPORT

NAMESPACE = "contract_json"
INT_RANGES = {"int32": ("std::numeric_limits<std::int32_t>::min()", "std::numeric_limits<std::int32_t>::max()"),
              "int64": ("std::numeric_limits<long long>::min()", "std::numeric_limits<long long>::max()")}


def cpp_base(types: CoreTypes, ref: TypeRef, enum: str | None = None) -> str:
    """The C++ type of a value, as core/src/api/Contract.h declares it, qualified for use outside it."""
    ref = types.resolve(ref).type
    if isinstance(ref, LiteralType) and enum is not None:
        return "holder::contract::" + enum
    if ref.kind == "string":
        return "std::string"
    if ref.kind in SCALARS_CPP:
        return SCALARS_CPP[ref.kind]
    if isinstance(ref, ListType):
        return f"std::vector<{cpp_base(types, ref.items)}>"
    if isinstance(ref, MapType):
        return f"std::map<std::string, {cpp_base(types, ref.values)}>"
    if isinstance(ref, NamedType):
        return types.instance(ref, {}).qualified("holder::contract::")
    raise Unsupported(f"{ref.kind} in JSON")


def field_decoder(types: CoreTypes, slot: Slot, label: str) -> str:
    """An expression decoding the present, non-null JSON value `item` into the slot's C++ type."""
    t = slot.type
    kind = t.kind
    base = cpp_base(types, t, slot.enum)
    # A record's own complaints name its fields.
    value = f"codec<{base}>::decode(item)" if kind == "named" else f'decode_field<{base}>(item, "{label}")'
    if kind == "string":
        maximum = constraint(t, "max_length")
        maximum = "std::numeric_limits<std::size_t>::max()" if maximum is None else maximum
        minimum = constraint(t, "min_length")
        return f'check_text({value}, "{label}", {0 if minimum is None else minimum}, {maximum})'
    if kind in INT_RANGES and constrained(t, "ge", "le"):
        low, high = INT_RANGES[kind]
        cpp = SCALARS_CPP[kind]
        ge, le = constraint(t, "ge"), constraint(t, "le")
        return f'check_range<{cpp}>({value}, "{label}", {low if ge is None else ge}, {high if le is None else le})'
    if kind == "float64" and constrained(t, "ge", "le", "gt", "lt"):
        raise Unsupported(f"{label}: bounds on a float64")
    if isinstance(t, MapType):
        # Each key and each value is checked as text.
        if types.kind(t.values) == "string":
            return (f'[&] {{ auto entries = {value}; for (const auto& [key, text] : entries) {{ '
                    f'check_text(key, "{label} key", 0, std::numeric_limits<std::size_t>::max()); '
                    f'check_text(text, "{label}", 0, std::numeric_limits<std::size_t>::max()); }} return entries; }}()')
        return (f'[&] {{ auto entries = {value}; for (const auto& [key, values] : entries) {{ '
                f'check_text(key, "{label} key", 0, std::numeric_limits<std::size_t>::max()); '
                f'for (const auto& text : values) '
                f'check_text(text, "{label}", 0, std::numeric_limits<std::size_t>::max()); }} return entries; }}()')
    if isinstance(t, ListType):
        items = types.resolve(t.items).type
        if constrained(items, "min_length", "max_length", "ge", "le", "pattern"):
            raise Unsupported(f"{label}: constraints on list items")
        if items.kind == "string":
            # Each item is checked as text.
            return (f'[&] {{ auto items = {value}; for (const auto& text : items) '
                    f'check_text(text, "{label}", 0, std::numeric_limits<std::size_t>::max()); return items; }}()')
    return value


def enum_lines(slot: Slot) -> list[str]:
    """The codec of a record's choice of literals: its C++ enum as the JSON string it stands for."""
    qualified = "holder::contract::" + slot.enum
    values = slot.type.values
    lines = [f"template <> struct codec<{qualified}> {{",
             f"    static json encode({qualified} value) {{",
             "        switch (value) {"]
    lines += [f'        case {qualified}::{camel(v)}: return "{v}";' for v in values]
    lines += ["        }", '        throw std::invalid_argument("is not a known value");', "    }",
              f"    static {qualified} decode(const json& value) {{",
              '        if (!value.is_string()) throw std::invalid_argument("must be a string");',
              "        const auto& text = value.get_ref<const std::string&>();"]
    lines += [f'        if (text == "{v}") return {qualified}::{camel(v)};' for v in values]
    lines += [f'        throw std::invalid_argument("must be one of {", ".join(values)}");', "    }", "};", ""]
    return lines


def record_lines(types: CoreTypes, instance: Instance) -> list[str]:
    qualified = instance.qualified("holder::contract::")
    name = instance.record
    slots = types.slots(instance)
    lines = [f"inline json codec<{qualified}>::encode(const {qualified}& value) {{",
             "    json result = json::object();"]
    for slot in slots:
        base = cpp_base(types, slot.type, slot.enum)
        held = slot.nullable or (slot.optional and not slot.has_default)
        if slot.change:
            lines.append(f'    if (value.{slot.name}) result["{slot.name}"] = *value.{slot.name} ? '
                         f'codec<{base}>::encode(**value.{slot.name}) : json(nullptr);')
        elif not held:
            lines.append(f'    result["{slot.name}"] = codec<{base}>::encode(value.{slot.name});')
        elif slot.nullable:
            lines.append(f'    result["{slot.name}"] = value.{slot.name} ? '
                         f'codec<{base}>::encode(*value.{slot.name}) : json(nullptr);')
        else:
            lines.append(f'    if (value.{slot.name}) result["{slot.name}"] = codec<{base}>::encode(*value.{slot.name});')
    lines += ["    return result;", "}",
              f"inline {qualified} codec<{qualified}>::decode(const json& value) {{",
              f'    if (!value.is_object()) throw std::invalid_argument("expected a {name} object");']
    known = " && ".join(f'it.key() != "{slot.name}"' for slot in slots) or "true"
    lines += ["    for (auto it = value.begin(); it != value.end(); ++it)",
              f'        if ({known}) throw std::invalid_argument("{name} has no field " + it.key());',
              f"    {qualified} out;"]
    for slot in slots:
        label = slot.name
        decoded = field_decoder(types, slot, label)
        present = f'value.contains("{slot.name}")'
        lines.append(f'    if ({present} && !value.at("{slot.name}").is_null()) {{')
        lines.append(f'        const auto& item = value.at("{slot.name}");')
        lines.append(f"        out.{slot.name} = {decoded};")
        if slot.change:
            # Absent keeps the field; null clears it.
            lines += [f"    }} else if ({present}) {{", f"        out.{slot.name}.emplace();", "    }"]
        elif slot.nullable:
            lines.append("    }")
        elif slot.optional:
            lines += [f"    }} else if ({present}) {{",
                      f'        throw std::invalid_argument("{label} must not be null");', "    }"]
        else:
            lines += ["    } else {",
                      f'        throw std::invalid_argument("{label} is required and must not be null");', "    }"]
    lines += ["    return out;", "}", ""]
    return lines


def generate_codec(types: CoreTypes, api_header: str) -> str:
    if not re.fullmatch(r"[A-Za-z_0-9/.-]+", api_header):
        raise ValueError("invalid API header path")
    seen, instances = set(), []
    for instance in [*types.outputs, *types.inputs]:
        if instance not in seen:
            seen.add(instance)
            instances.append(instance)
    lines = [*header_lines(contract_sources(types.contract),
                           "The JSON codec for the contract's types, for the daemon's generated routes.", "cpp"),
             "#pragma once",
             f'#include "{api_header}"',
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
             SUPPORT.rstrip(), ""]
    enums: dict[str, Slot] = {}
    for instance in instances:
        for slot in types.slots(instance):
            if isinstance(slot.type, LiteralType):
                enums.setdefault(slot.enum, slot)
    for slot in enums.values():
        lines += enum_lines(slot)
    for instance in instances:
        qualified = instance.qualified("holder::contract::")
        lines += [f"template <> struct codec<{qualified}> {{",
                  f"    static json encode(const {qualified}& value);",
                  f"    static {qualified} decode(const json& value);",
                  "};"]
    lines.append("")
    for instance in instances:
        lines += record_lines(types, instance)
    lines += ["template <typename T> json encode(const T& value) { return codec<T>::encode(value); }",
              "template <typename T> T decode(const json& value) { return codec<T>::decode(value); }",
              f"}}  // namespace {NAMESPACE}", ""]
    return "\n".join(lines)

