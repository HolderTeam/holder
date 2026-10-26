"""Generate the daemon's JSON codec for the contract's C++ types (core/src/api/Contract.h).

Ported from RIG's json_codec.py (https://github.com/zeth/rig, src/rig/json_codec.py at e32ea68),
which read declarations reflected from C++; this reads the contract's IR. Each record gets a
codec<T> with encode and decode. Decoding checks what the contract says about the value: required
and omitted fields, null, text (UTF-8 with no U+0000, lengths in characters), integer ranges and
finite numbers. A record accepts fields it does not know, as the contract's records do.
"""

from __future__ import annotations

import re

from .generate import SCALARS_CPP, Instance, Model, Slot, Unsupported, header_lines

NAMESPACE = "contract_json"
INT_RANGES = {"int32": ("std::numeric_limits<std::int32_t>::min()", "std::numeric_limits<std::int32_t>::max()"),
              "int64": ("std::numeric_limits<long long>::min()", "std::numeric_limits<long long>::max()")}

SUPPORT = r"""
namespace contract_json {
using json = nlohmann::json;
template <typename T> struct codec;

// The number of characters in text, or nothing when it is not well-formed UTF-8.
inline std::optional<std::size_t> utf8_length(const std::string& text) {
    std::size_t count = 0;
    for (std::size_t i = 0; i < text.size(); ++count) {
        const auto lead = static_cast<unsigned char>(text[i]);
        std::size_t size = 1;
        char32_t point = lead;
        if (lead >= 0xC2 && lead <= 0xDF) {
            size = 2, point = lead & 0x1F;
        } else if (lead >= 0xE0 && lead <= 0xEF) {
            size = 3, point = lead & 0x0F;
        } else if (lead >= 0xF0 && lead <= 0xF4) {
            size = 4, point = lead & 0x07;
        } else if (lead >= 0x80) {
            return std::nullopt;
        }
        if (text.size() - i < size) return std::nullopt;
        for (std::size_t k = 1; k < size; ++k) {
            const auto next = static_cast<unsigned char>(text[i + k]);
            if ((next & 0xC0) != 0x80) return std::nullopt;
            point = (point << 6) | (next & 0x3F);
        }
        if ((size == 3 && (point < 0x800 || (point >= 0xD800 && point <= 0xDFFF))) ||
            (size == 4 && (point < 0x10000 || point > 0x10FFFF)))
            return std::nullopt;
        i += size;
    }
    return count;
}

// Text the contract accepts: UTF-8 without U+0000, its length in characters within bounds.
inline const std::string& check_text(const std::string& value, const char* name, std::size_t minimum,
                                     std::size_t maximum) {
    const auto length = utf8_length(value);
    if (!length) throw std::invalid_argument(std::string(name) + " must be UTF-8");
    if (value.find('\0') != std::string::npos)
        throw std::invalid_argument(std::string(name) + " must not contain U+0000");
    if (*length < minimum)
        throw std::invalid_argument(std::string(name) + (minimum == 1 ? " must not be empty" :
            " must be at least " + std::to_string(minimum) + " characters"));
    if (*length > maximum)
        throw std::invalid_argument(std::string(name) + " must be at most " + std::to_string(maximum) +
                                    " characters");
    return value;
}

template <typename T>
T check_range(T value, const char* name, T minimum, T maximum) {
    if (value < minimum || value > maximum)
        throw std::invalid_argument(std::string(name) + " must be from " + std::to_string(minimum) + " to " +
                                    std::to_string(maximum));
    return value;
}

inline double check_finite(double value, const char* name) {
    if (!std::isfinite(value)) throw std::invalid_argument(std::string(name) + " must be a finite number");
    return value;
}

// A field's value, with the field's name on any complaint about it.
template <typename T> T decode_field(const json& value, const char* name) {
    try {
        return codec<T>::decode(value);
    } catch (const std::invalid_argument& error) {
        throw std::invalid_argument(std::string(name) + " " + error.what());
    }
}

template <> struct codec<std::int32_t> {
    static json encode(std::int32_t value) { return value; }
    static std::int32_t decode(const json& value) {
        if (!value.is_number_integer()) throw std::invalid_argument("must be an integer");
        if (value.is_number_unsigned()) {
            if (value.get<std::uint64_t>() > static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max()))
                throw std::invalid_argument("is out of range");
        } else {
            const auto number = value.get<std::int64_t>();
            if (number < std::numeric_limits<std::int32_t>::min() || number > std::numeric_limits<std::int32_t>::max())
                throw std::invalid_argument("is out of range");
        }
        return value.get<std::int32_t>();
    }
};
template <> struct codec<bool> {
    static json encode(bool value) { return value; }
    static bool decode(const json& value) {
        if (!value.is_boolean()) throw std::invalid_argument("must be a boolean");
        return value.get<bool>();
    }
};
template <> struct codec<long long> {
    static json encode(long long value) { return value; }
    static long long decode(const json& value) {
        if (!value.is_number_integer()) throw std::invalid_argument("must be an integer");
        if (value.is_number_unsigned() &&
            value.get<std::uint64_t>() > static_cast<std::uint64_t>(std::numeric_limits<long long>::max()))
            throw std::invalid_argument("is out of range");
        return value.get<long long>();
    }
};
template <> struct codec<double> {
    static json encode(double value) { return value; }
    static double decode(const json& value) {
        if (!value.is_number()) throw std::invalid_argument("must be a number");
        const double number = value.get<double>();
        if (!std::isfinite(number)) throw std::invalid_argument("must be a finite number");
        return number;
    }
};
template <> struct codec<std::string> {
    static json encode(const std::string& value) { return value; }
    static std::string decode(const json& value) {
        if (!value.is_string()) throw std::invalid_argument("must be a string");
        return value.get<std::string>();
    }
};
template <typename T> struct codec<std::vector<T>> {
    static json encode(const std::vector<T>& value) {
        json result = json::array();
        for (const auto& item : value) result.push_back(codec<T>::encode(item));
        return result;
    }
    static std::vector<T> decode(const json& value) {
        if (!value.is_array()) throw std::invalid_argument("must be an array");
        std::vector<T> result;
        for (const auto& item : value) result.push_back(codec<T>::decode(item));
        return result;
    }
};
"""


def cpp_base(model: Model, ref: dict) -> str:
    """The C++ type of a value, as core/src/api/Contract.h declares it, qualified for use outside it."""
    ref = model.resolve(ref)
    if ref["kind"] == "string":
        return "std::string"
    if ref["kind"] in SCALARS_CPP:
        return SCALARS_CPP[ref["kind"]]
    if ref["kind"] == "list":
        return f"std::vector<{cpp_base(model, ref['items'])}>"
    if ref["kind"] == "named":
        return model.instance(ref, {}).qualified("holder::contract::")
    raise Unsupported(f"{ref['kind']} in JSON")


def field_decoder(model: Model, slot: Slot, label: str) -> str:
    """An expression decoding the present, non-null JSON value `item` into the slot's C++ type."""
    t = slot.type
    kind = t["kind"]
    base = cpp_base(model, t)
    # A record's own complaints name its fields.
    value = f"codec<{base}>::decode(item)" if kind == "named" else f'decode_field<{base}>(item, "{label}")'
    if kind == "string":
        maximum = t.get("max_length", "std::numeric_limits<std::size_t>::max()")
        return f'check_text({value}, "{label}", {t.get("min_length", 0)}, {maximum})'
    if kind in INT_RANGES and ("ge" in t or "le" in t):
        low, high = INT_RANGES[kind]
        cpp = SCALARS_CPP[kind]
        return f'check_range<{cpp}>({value}, "{label}", {t.get("ge", low)}, {t.get("le", high)})'
    if kind == "float64" and any(k in t for k in ("ge", "le", "gt", "lt")):
        raise Unsupported(f"{label}: bounds on a float64")
    if kind == "list":
        items = model.resolve(t["items"])
        if any(k in items for k in ("min_length", "max_length", "ge", "le", "pattern")):
            raise Unsupported(f"{label}: constraints on list items")
        if items["kind"] == "string":
            # Each item is checked as text.
            return (f'[&] {{ auto items = {value}; for (const auto& text : items) '
                    f'check_text(text, "{label}", 0, std::numeric_limits<std::size_t>::max()); return items; }}()')
    return value


def record_lines(model: Model, instance: Instance) -> list[str]:
    qualified = instance.qualified("holder::contract::")
    name = instance.record
    slots = model.slots(instance)
    lines = [f"inline json codec<{qualified}>::encode(const {qualified}& value) {{",
             "    json result = json::object();"]
    for slot in slots:
        base = cpp_base(model, slot.type)
        held = slot.nullable or (slot.optional and not slot.has_default)
        if not held:
            lines.append(f'    result["{slot.name}"] = codec<{base}>::encode(value.{slot.name});')
        elif slot.nullable:
            lines.append(f'    result["{slot.name}"] = value.{slot.name} ? '
                         f'codec<{base}>::encode(*value.{slot.name}) : json(nullptr);')
        else:
            lines.append(f'    if (value.{slot.name}) result["{slot.name}"] = codec<{base}>::encode(*value.{slot.name});')
    lines += ["    return result;", "}",
              f"inline {qualified} codec<{qualified}>::decode(const json& value) {{",
              f'    if (!value.is_object()) throw std::invalid_argument("expected a {name} object");',
              f"    {qualified} out;"]
    for slot in slots:
        label = slot.name
        decoded = field_decoder(model, slot, label)
        present = f'value.contains("{slot.name}")'
        lines.append(f'    if ({present} && !value.at("{slot.name}").is_null()) {{')
        lines.append(f'        const auto& item = value.at("{slot.name}");')
        lines.append(f"        out.{slot.name} = {decoded};")
        if slot.nullable:
            lines.append("    }")
        elif slot.optional:
            lines += [f"    }} else if ({present}) {{",
                      f'        throw std::invalid_argument("{label} must not be null");', "    }"]
        else:
            lines += ["    } else {",
                      f'        throw std::invalid_argument("{label} is required and must not be null");', "    }"]
    lines += ["    return out;", "}", ""]
    return lines


def generate_codec(model: Model, api_header: str) -> str:
    if not re.fullmatch(r"[A-Za-z_0-9/.-]+", api_header):
        raise ValueError("invalid API header path")
    seen, instances = set(), []
    for instance in [*model.outputs, *model.inputs]:
        if instance not in seen:
            seen.add(instance)
            instances.append(instance)
    lines = [*header_lines(model.ir, "The JSON codec for the contract's types, for the daemon's generated routes.", "cpp"),
             "#pragma once",
             f'#include "{api_header}"',
             "#include <nlohmann/json.hpp>",
             "#include <cmath>",
             "#include <cstddef>",
             "#include <cstdint>",
             "#include <limits>",
             "#include <optional>",
             "#include <stdexcept>",
             "#include <string>",
             "#include <vector>",
             SUPPORT.rstrip(), ""]
    for instance in instances:
        qualified = instance.qualified("holder::contract::")
        lines += [f"template <> struct codec<{qualified}> {{",
                  f"    static json encode(const {qualified}& value);",
                  f"    static {qualified} decode(const json& value);",
                  "};"]
    lines.append("")
    for instance in instances:
        lines += record_lines(model, instance)
    lines += ["template <typename T> json encode(const T& value) { return codec<T>::encode(value); }",
              "template <typename T> T decode(const json& value) { return codec<T>::decode(value); }",
              f"}}  // namespace {NAMESPACE}", ""]
    return "\n".join(lines)

