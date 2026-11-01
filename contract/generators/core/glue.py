"""ContractCApi.cpp: the C entry points.

Each checks its arguments against the contract, calls core's C++ function, converts the result
into the caller's arena, and turns error kinds into status codes.
"""

from __future__ import annotations

from pathlib import Path

from jinja2 import Environment, FileSystemLoader, StrictUndefined

from compiler.ir import ListType, LiteralType, MapType, NamedType, OperationView, TypeRef, UnionType

from ..common import Unsupported, contract_sources, header_lines
from .abi import SCALARS_C, SCALARS_CPP, literal_member, snake
from .render import Param, c_out, c_param, c_signature, cpp_base, cpp_literal
from .types import CoreTypes, Slot, c_enum_name, camel, constrained, constraint, list_c_name, map_c_name, unique

TEMPLATES = Environment(
    loader=FileSystemLoader(Path(__file__).with_name("templates")),
    undefined=StrictUndefined,
    autoescape=False,
    trim_blocks=True,
    keep_trailing_newline=True,
    newline_sequence="\n",
)

CHANGE_SUPPORT = r"""
// A partial update's text field: kept, set to its value, or cleared.
contract::Change<std::string> changed_text(const holder_string_change& value, const char* name,
                                           std::size_t min_length, std::size_t max_length) {
  switch (value.change) {
  case HOLDER_KEEP: return std::nullopt;
  case HOLDER_SET: return text(value.value, (std::string(name) + ".value").c_str(), min_length, max_length);
  case HOLDER_CLEAR: return std::optional<std::string>{};
  default: throw ArgumentError(std::string(name) + ".change is not a holder_change value");
  }
}
"""


def limits(t: TypeRef) -> tuple[str, str]:
    low, high = constraint(t, "min_length"), constraint(t, "max_length")
    return str(0 if low is None else low), "SIZE_MAX" if high is None else str(high)


def to_c_value(types: CoreTypes, slot: Slot, source: str) -> str:
    t = slot.type
    if slot.change:
        raise Unsupported(f"{slot.name}: a partial update in a result")
    optional = slot.nullable or (slot.optional and not slot.has_default)
    if isinstance(t, LiteralType):
        return f"to_c_enum({source})"
    if t.kind == "string":
        return f"copy_text(arena, {source})" if optional else f"copy(arena, {source})"
    if t.kind in SCALARS_C:
        return f"copy_value<{SCALARS_C[t.kind]}>(arena, {source})" if optional else source
    if isinstance(t, ListType):
        convert = "copy_strings" if types.kind(t.items) == "string" else "to_c_list"
        if optional:
            name = list_c_name(types, t.items)
            return (f"copy_optional<{name}>(arena, {source}, "
                    f"[&](const auto& items) {{ return {convert}(arena, items); }})")
        return f"{convert}(arena, {source})"
    if isinstance(t, NamedType):
        if optional:
            name = types.instance(t, {}).c_name
            return (f"copy_optional<{name}>(arena, {source}, "
                    "[&](const auto& item) { return to_c(arena, item); })")
        return f"to_c(arena, {source})"
    if isinstance(t, MapType):
        if optional:
            raise Unsupported(f"{slot.name}: optional maps")
        return f"to_c_{snake(slot.alias)}(arena, {source})"
    raise Unsupported(f"{slot.name}: {t.kind}")


def from_c_value(types: CoreTypes, slot: Slot, source: str, label: str) -> str:
    t = slot.type
    optional = slot.nullable or slot.optional
    if slot.change:
        low, high = limits(t)
        return f'changed_text({source}, "{label}", {low}, {high})'
    if t.kind == "string":
        low, high = limits(t)
        if optional:
            value = f'optional_text({source}, "{label}", {low}, {high})'
            if slot.has_default and slot.default is not None:
                return f"{value}.value_or({cpp_literal(slot.default)})"
            return value
        return f'text({source}, "{label}", {low}, {high})'
    if t.kind in SCALARS_C:
        check = scalar_check(t, label)
        if optional:
            cpp = SCALARS_CPP[t.kind]
            inner = f"{check.format(f'*{source}')}"
            if slot.has_default and slot.default is not None:
                return f"({source} ? {inner} : {cpp}{{{cpp_literal(slot.default)}}})"
            return f"({source} ? std::optional<{cpp}>({inner}) : std::nullopt)"
        return check.format(source)
    if isinstance(t, MapType):
        convert = f"from_c_{snake(slot.alias)}"
        if optional:
            return f'({source} ? std::optional<{cpp_base(types, t, {})}>({convert}(*{source}, "{label}")) : std::nullopt)'
        return f'{convert}({source}, "{label}")'
    if isinstance(t, ListType):
        items = types.resolve(t.items).type
        if not isinstance(items, NamedType):
            raise Unsupported(f"{label}: lists of {items.kind} as inputs")
        if optional:
            cpp = f"std::vector<{types.instance(items, {}).qualified('contract::')}>"
            return f'({source} ? std::optional<{cpp}>(from_c_list(*{source}, "{label}")) : std::nullopt)'
        return f'from_c_list({source}, "{label}")'
    raise Unsupported(f"{label}: {t.kind} inputs")


def scalar_check(t: TypeRef, label: str) -> str:
    if t.kind in ("int32", "int64") and constrained(t, "ge", "le"):
        cpp = SCALARS_CPP[t.kind]
        low = constraint(t, "ge")
        high = constraint(t, "le")
        low = f"std::numeric_limits<{cpp}>::min()" if low is None else low
        high = f"std::numeric_limits<{cpp}>::max()" if high is None else high
        return f'in_range<{cpp}>({{}}, "{label}", {low}, {high})'
    if t.kind == "float64":
        if constrained(t, "ge", "le", "gt", "lt"):
            raise Unsupported(f"{label}: bounds on a float64")
        return f'finite({{}}, "{label}")'
    return "{}"


def map_glue(types: CoreTypes, alias: str, ref: MapType) -> list[str]:
    """A map's conversions: entries in the C++ map's order, and back, rejecting a repeated key."""
    name, cpp, alias_name = map_c_name(alias), cpp_base(types, ref, {}), snake(alias)
    values_ref = types.resolve(ref.values).type
    if values_ref.kind == "string":
        low, high = limits(values_ref)
        return [f"{name} to_c_{alias_name}(holder_arena* arena, const {cpp}& value) {{",
                f"  auto* items = make<{name}_entry>(arena, value.size());",
                "  std::size_t i = 0;",
                "  for (const auto& [key, text] : value) items[i++] = {copy(arena, key), copy(arena, text)};",
                "  return {items, value.size()};", "}", "",
                f"{cpp} from_c_{alias_name}(const {name}& value, const char* name) {{",
                '  require(value.items != nullptr || value.count == 0, std::string(name) + ".items must not be null");',
                f"  {cpp} out;",
                "  for (std::size_t i = 0; i < value.count; ++i) {",
                "    const auto& entry = value.items[i];",
                '    auto key = text(entry.key, (std::string(name) + " key").c_str(), 0, SIZE_MAX);',
                f'    auto value_text = text(entry.values, (std::string(name) + "." + key).c_str(), {low}, {high});',
                '    require(out.emplace(std::move(key), std::move(value_text)).second, std::string(name) + " repeats a key");',
                "  }", "  return out;", "}", ""]
    low, high = limits(types.resolve(values_ref.items).type)
    return [f"{name} to_c_{alias_name}(holder_arena* arena, const {cpp}& value) {{",
            f"  auto* items = make<{name}_entry>(arena, value.size());",
            "  std::size_t i = 0;",
            "  for (const auto& [key, values] : value) items[i++] = {copy(arena, key), copy_strings(arena, values)};",
            "  return {items, value.size()};", "}", "",
            f"{cpp} from_c_{alias_name}(const {name}& value, const char* name) {{",
            '  require(value.items != nullptr || value.count == 0, std::string(name) + ".items must not be null");',
            f"  {cpp} out;",
            "  for (std::size_t i = 0; i < value.count; ++i) {",
            "    const auto& entry = value.items[i];",
            '    auto key = text(entry.key, (std::string(name) + " key").c_str(), 0, SIZE_MAX);',
            "    const auto& list = entry.values;",
            '    require(list.items != nullptr || list.count == 0, std::string(name) + "." + key + ".items must not be null");',
            "    std::vector<std::string> values;",
            "    for (std::size_t k = 0; k < list.count; ++k)",
            f'      values.push_back(text(list.items[k], (std::string(name) + "." + key).c_str(), {low}, {high}));',
            '    require(out.emplace(std::move(key), std::move(values)).second, std::string(name) + " repeats a key");',
            "  }", "  return out;", "}", ""]


def glue_param(types: CoreTypes, op: OperationView, param: Param) -> list[str]:
    slot, t, name = param.slot, param.slot.type, param.slot.name
    prefix = f"holder_{op.name}_{name}".upper()
    type_name = camel(op.name) + camel(name)
    if isinstance(t, LiteralType) and slot.nullable:
        lines = [f"    std::optional<contract::{type_name}> {name}_value;", f"    switch ({name}) {{",
                 f"    case {prefix}_NONE: break;"]
        for value in t.values:
            lines.append(f"    case {prefix}_{literal_member(value)}: {name}_value = contract::{type_name}::{camel(str(value))}; break;")
        lines += [f'    default: throw ArgumentError("{name} is not a {prefix.lower()} value");', "    }"]
        return lines
    if isinstance(t, LiteralType):
        lines = [f"    contract::{type_name} {name}_value;", f"    switch ({name}) {{"]
        if slot.has_default:
            lines.append(f"    case {prefix}_DEFAULT: {name}_value = contract::{type_name}::{camel(str(slot.default))}; break;")
        for value in t.values:
            lines.append(f"    case {prefix}_{literal_member(value)}: {name}_value = contract::{type_name}::{camel(str(value))}; break;")
        lines += [f'    default: throw ArgumentError("{name} is not a {prefix.lower()} value");', "    }"]
        return lines
    if isinstance(t, UnionType):
        value = next(types.resolve(a) for a in t.alternatives if not isinstance(a, LiteralType))
        value_name = snake(value.alias or "value")
        low, high = limits(value.type)
        lines = [f"    std::optional<contract::{type_name}> {name}_value;", f"    switch ({name}.kind) {{",
                 f"    case {prefix}_NONE: break;"]
        for lit in [v for a in t.alternatives if isinstance(a, LiteralType) for v in a.values]:
            lines.append(f"    case {prefix}_{literal_member(lit)}: {name}_value = contract::{type_name}{camel(str(lit))}{{}}; break;")
        lines += [(f"    case {prefix}_{value_name.upper()}: {name}_value = "
                   f'text({name}.{value_name}, "{name}.{value_name}", {low}, {high}); break;'),
                  f'    default: throw ArgumentError("{name}.kind is not a {prefix.lower()}_kind value");', "    }"]
        return lines
    if isinstance(t, NamedType):
        return [f'    require({name} != nullptr, "{name} must not be null");',
                f'    const auto {name}_value = from_c(*{name});']
    if t.kind in SCALARS_C and "*" not in param.c_decl:
        # Passed by value: required, or a default that is the zero value.
        return [f"    const auto {name}_value = {scalar_check(t, name).format(name)};"]
    return [f"    const auto {name}_value = {from_c_value(types, slot, name, name)};"]


def glue_operation(types: CoreTypes, op: OperationView) -> list[str]:
    params = [c_param(types, op, p) for p in op.parameters]
    if op.context == "closes":
        return [c_signature(types, op) + " { contract::" + op.name + "(context); }", ""]
    result = c_out(types, op)
    if op.context == "opens":
        lines = [c_signature(types, op) + " {", "  return run_guarded(out_error, [&] {",
                 '    require(out_context != nullptr, "out_context must not be null");', "    *out_context = nullptr;"]
    else:
        lines = [c_signature(types, op) + " {", "  return guarded(arena, out_error, [&] {"]
        if op.context == "required":
            lines.append('    require(context != nullptr, "context must not be null");')
        if result:
            lines += [f'    require({result[1]} != nullptr, "{result[1]} must not be null");',
                      f"    *{result[1]} = nullptr;"]
    call_args = ["*context"] if op.context == "required" else []
    for param in params:
        lines += glue_param(types, op, param)
        call_args.append(f"{param.slot.name}_value")
    call = f"contract::{op.name}(" + ", ".join(call_args) + ")"
    if op.context == "opens":
        lines.append(f"    *out_context = {call};")
    elif result:
        returns = types.resolve(op.returns).type
        convert = "to_c"
        if isinstance(returns, ListType):
            convert = "copy_strings" if types.kind(returns.items) == "string" else "to_c_list"
        lines += [f"    auto* out = make<{result[0]}>(arena);", f"    *out = {convert}(arena, {call});",
                  f"    *{result[1]} = out;"]
    else:
        lines.append(f"    {call};")
    lines += ["  });", "}", ""]
    return lines


def generate_glue(types: CoreTypes) -> str:
    errors = [{"name": error.name, "code": snake(error.name).upper()} for error in types.errors]
    out = [*header_lines(contract_sources(types.contract),
                         "The C entry points: argument checks, conversions and error codes.", "cpp"),
           '#include "Contract.h"', '#include "holder/holder.h"', "", "#include <cmath>", "#include <cstddef>", "#include <cstdint>",
           "#include <cstring>", "#include <limits>", "#include <map>", "#include <memory>", "#include <new>",
           "#include <optional>",
           "#include <stdexcept>", "#include <string>",
           "#include <utility>", "#include <vector>",
           TEMPLATES.get_template("glue_support.cpp.j2").render(errors=errors).rstrip(), ""]
    if types.changes():
        out += [CHANGE_SUPPORT.strip("\n"), ""]
    for alias, ref in types.maps.items():
        out += map_glue(types, alias, ref)
    outputs = unique(types.outputs)
    # A result's choices of literals, from the C++ enum to the C one.
    converted: set[str] = set()
    for instance in outputs:
        for slot in types.slots(instance):
            if not isinstance(slot.type, LiteralType) or c_enum_name(slot) in converted:
                continue
            enum = c_enum_name(slot)
            converted.add(enum)
            out += [f"{enum} to_c_enum(contract::{slot.enum} value) {{", "  switch (value) {"]
            out += [f"  case contract::{slot.enum}::{camel(v)}: return {enum.upper()}_{literal_member(v)};"
                    for v in slot.type.values]
            out += ["  }", f'  throw std::logic_error("not a contract::{slot.enum} value");', "}", ""]
    for instance in outputs:
        out.append(f"{instance.c_name} to_c(holder_arena* arena, const {instance.qualified('contract::')}& value);")
        if instance in types.listed:
            out.append(f"{instance.c_name}_list to_c_list(holder_arena* arena, "
                       f"const std::vector<{instance.qualified('contract::')}>& values);")
    out.append("")
    for instance in outputs:
        out.append(f"{instance.c_name} to_c(holder_arena* arena, const {instance.qualified('contract::')}& value) {{")
        out.append(f"  {instance.c_name} out{{}};")
        for slot in types.slots(instance):
            out.append(f"  out.{slot.name} = {to_c_value(types, slot, 'value.' + slot.name)};")
        out += ["  return out;", "}", ""]
        if instance in types.listed:
            out += [(f"{instance.c_name}_list to_c_list(holder_arena* arena, "
                     f"const std::vector<{instance.qualified('contract::')}>& values) {{"),
                    f"  auto* items = make<{instance.c_name}>(arena, values.size());",
                    "  for (std::size_t i = 0; i < values.size(); ++i) items[i] = to_c(arena, values[i]);",
                    "  return {items, values.size()};", "}", ""]
    for instance in unique(types.inputs):
        out.append(f"{instance.qualified('contract::')} from_c(const {instance.c_name}& value) {{")
        out.append(f"  {instance.qualified('contract::')} out;")
        for slot in types.slots(instance):
            label = f"{snake(instance.record)}.{slot.name}"
            if isinstance(slot.type, LiteralType):
                enum = c_enum_name(slot)
                out.append(f"  switch (value.{slot.name}) {{")
                if slot.has_default:
                    out.append(f"  case {enum.upper()}_DEFAULT: break;")
                out += [f"  case {enum.upper()}_{literal_member(v)}: out.{slot.name} = "
                        f"contract::{slot.enum}::{camel(v)}; break;" for v in slot.type.values]
                out += [f'  default: throw ArgumentError("{label} is not a {enum} value");', "  }"]
                continue
            out.append(f"  out.{slot.name} = {from_c_value(types, slot, 'value.' + slot.name, label)};")
        out += ["  return out;", "}", ""]
        if instance in types.listed:
            out += [(f"std::vector<{instance.qualified('contract::')}> from_c_list("
                     f"const {instance.c_name}_list& values, const char* name) {{"),
                    '  require(values.items != nullptr || values.count == 0, std::string(name) + ".items must not be null");',
                    f"  std::vector<{instance.qualified('contract::')}> out;",
                    "  out.reserve(values.count);",
                    "  for (std::size_t i = 0; i < values.count; ++i) out.push_back(from_c(values.items[i]));",
                    "  return out;", "}", ""]
    out += ["}  // namespace", "", 'extern "C" {', "",
            "holder_arena* holder_arena_new(void) { return new (std::nothrow) holder_arena; }",
            "void holder_arena_free(holder_arena* arena) { delete arena; }", ""]
    for op in types.operations:
        out += glue_operation(types, op)
    out += ['}  // extern "C"', ""]
    return "\n".join(out)
