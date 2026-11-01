"""Contract.h: the C++ declarations core implements, one function per operation, in holder::contract."""

from __future__ import annotations

from compiler.ir import LiteralType, NamedType, OperationView, UnionType

from ..common import Unsupported, contract_sources, header_lines
from .abi import SCALARS_CPP
from .render import Param, c_param, comment, cpp_base, cpp_literal, cpp_type
from .types import CoreTypes, Slot, camel, unique


def cpp_param(types: CoreTypes, op: OperationView, param: Param) -> tuple[str, list[str]]:
    slot, t = param.slot, param.slot.type
    type_name = camel(op.name) + camel(slot.name)
    if isinstance(t, LiteralType):
        members = ", ".join(camel(str(v)) for v in t.values)
        held = f"std::optional<{type_name}>" if slot.nullable else type_name
        return f"{held} {slot.name}", [f"enum class {type_name} {{ {members} }};"]
    if isinstance(t, UnionType):
        lines, alternatives = [], []
        for value in [v for a in t.alternatives if isinstance(a, LiteralType) for v in a.values]:
            lines.append(f"struct {type_name}{camel(str(value))} {{}};")
            alternatives.append(f"{type_name}{camel(str(value))}")
        alternatives.append("std::string")
        lines.append(f"using {type_name} = std::variant<{', '.join(alternatives)}>;")
        return f"std::optional<{type_name}> {slot.name}", lines
    if isinstance(t, NamedType):
        return f"const {cpp_base(types, t, {})}& {slot.name}", []
    return f"{cpp_type(types, slot)} {slot.name}", []


def cpp_initializer(types: CoreTypes, slot: Slot) -> str:
    """A field's default: the contract's, or {} for a number, so a new record holds no garbage."""
    if slot.has_default and isinstance(slot.type, LiteralType):
        return f" = {slot.enum}::{camel(str(slot.default))}"
    if slot.has_default and slot.default is not None:
        return f" = {cpp_literal(slot.default)}"
    if types.kind(slot.type) in SCALARS_CPP and not (slot.nullable or slot.optional):
        return "{}"
    return ""


def cpp_string(value: str, indent: str = "    ") -> str:
    """value as a C++ string literal, one source line per line of text."""
    def escape(text: str) -> str:
        return text.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n").replace("\t", "\\t")
    lines = value.splitlines(keepends=True) or [""]
    if len(lines) == 1:
        return f'"{escape(lines[0])}"'
    return "\n" + "\n".join(f'{indent}"{escape(line)}"' for line in lines)


def generate_cpp_header(types: CoreTypes) -> str:
    out = [*header_lines(contract_sources(types.contract),
                         "The C++ declarations core implements: one function per operation. A missing "
                         "implementation fails to link.", "cpp"),
           "#pragma once", "", "#include <cstdint>", "#include <map>", "#include <optional>", "#include <stdexcept>",
           "#include <string>", "#include <string_view>", "#include <variant>", "#include <vector>", "", "struct holder_context;",
           "struct holder_error;", "", "namespace holder::contract {", "",
           "/// Base of the error kinds operations throw. Anything else thrown is an unexpected failure.",
           "struct Error : std::runtime_error {", "  using std::runtime_error::runtime_error;", "};", "",
           "/// Thrown for an argument the contract's checks cannot catch.",
           "struct InvalidArgument : Error {", "  using Error::Error;", "};", ""]
    for constant in types.contract.constants:
        out += comment(constant.doc, style="cpp")
        literal = cpp_string(constant.value)
        separator = "" if literal.startswith("\n") else " "
        out += [f"inline constexpr std::string_view {constant.name} ={separator}{literal};", ""]
    if types.changes():
        out += ["/// A field of a partial update: nullopt keeps it, an empty value clears it, and a value sets it.",
                "template <class T>", "using Change = std::optional<std::optional<T>>;", ""]
    for error in types.errors:
        out += comment(error.doc, style="cpp")
        out += [f"struct {error.name} : Error {{", "  using Error::Error;", "};", ""]
    for name, record in types.records.items():
        if not record.type_parameters:
            continue
        out += comment(record.doc, style="cpp")
        params = record.type_parameters
        out.append(f"template <{', '.join('class ' + p for p in params)}>")
        out.append(f"struct {name} {{")
        for field in record.fields:
            slot = types.slot(field.name, field.type, field.doc, omittable=field.omittable,
                              default=field.default, has_default="default" in field.model_fields_set)
            if isinstance(slot.type, LiteralType):
                raise Unsupported(f"{name}.{slot.name}: a choice of literals in a generic record")
            out.append(f"  {cpp_type(types, slot, {p: p for p in params})} {slot.name};"
                       + (f"  ///< {slot.doc}" if slot.doc else ""))
        out += ["};", ""]
    # Records as C++ structs, each after the record it extends, which C++ inherits rather than flattens.
    names: list[str] = []
    for instance in unique(types.inputs + types.outputs):
        if instance.arguments:
            continue
        chain, name = [], instance.record
        while name:
            chain.insert(0, name)
            name = types.records[name].base
        names += [n for n in chain if n not in names]
    for name in names:
        record = types.records[name]
        for slot in types.own_enums(name):
            out += comment(slot.doc, style="cpp")
            out.append(f"enum class {slot.enum} {{ {', '.join(camel(v) for v in slot.type.values)} }};")
            out.append("")
        out += comment(record.doc, style="cpp")
        out.append(f"struct {name}" + (f" : {record.base}" if record.base else "") + " {")
        for field in record.fields:
            slot = types.field_slot(name, field)
            out.append(f"  {cpp_type(types, slot)} {slot.name}{cpp_initializer(types, slot)};"
                       + (f"  ///< {slot.doc}" if slot.doc else ""))
        out += ["};", ""]
    for op in types.operations:
        params = [c_param(types, op, p) for p in op.parameters]
        declarations = []
        for param in params:
            decl, extra = cpp_param(types, op, param)
            out += extra
            declarations.append(decl)
        if any(cpp_param(types, op, p)[1] for p in params):
            out.append("")
        doc = "\n".join(x for x in [op.summary, op.description] if x)
        out += comment(doc, style="cpp")
        out += [f"/// @param {p.slot.name} {p.slot.doc}" for p in params if p.slot.doc]
        if op.errors:
            out.append(f"/// @throws {', '.join(op.errors)}")
        result = types.resolve(op.returns).type
        returns = "void" if result.kind == "none" else cpp_base(types, result, {})
        if op.context == "opens":
            out.append("/// @return A new context, which the caller owns and releases with the closing operation.")
            out.append(f"holder_context* {op.name}(" + ", ".join(declarations) + ");")
        elif op.context == "closes":
            out.append(f"void {op.name}(holder_context* context) noexcept;")
        else:
            context = ["holder_context& context"] if op.context == "required" else []
            out.append(f"{returns} {op.name}(" + ", ".join([*context, *declarations]) + ");")
        out.append("")
    out += ["namespace detail {", "",
            "/// Core supplies this: sets *out_error (when not null) to a new holder_error with message, and",
            "/// returns code.",
            "int fail(holder_error** out_error, int code, const std::string& message);", "",
            "}  // namespace detail", "", "}  // namespace holder::contract", ""]
    return "\n".join(out)
