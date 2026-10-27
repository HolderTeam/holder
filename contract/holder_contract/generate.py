"""Generate core's C API from the contract's IR.

Three files come out:

- holder_api.h: the C declarations every consumer of core's C API sees.
- Contract.h: the C++ declarations core implements, one function per operation, in holder::contract.
- ContractGlue.cpp: the C entry points. Each checks its arguments against the contract, calls
  core's C++ function, converts the result into the caller's arena, and turns error kinds into
  status codes.

Only what the contract uses so far is supported; anything else stops generation with a message,
and is added when the first declaration needs it.
"""

from __future__ import annotations

import re
from dataclasses import dataclass
from typing import Any

FIRST_ERROR_CODE = 100
SCALARS_C = {"int32": "int32_t", "int64": "int64_t", "float64": "double", "bool": "bool"}
# C++ uses core's own spelling of a 64-bit integer; the glue converts to int64_t at the C boundary.
SCALARS_CPP = {"int32": "std::int32_t", "int64": "long long", "float64": "double", "bool": "bool"}


class Unsupported(Exception):
    """The contract uses something the C generator does not support yet."""


def snake(name: str) -> str:
    return re.sub(r"(?<=[a-z0-9])([A-Z])", r"_\1", name).lower()


def camel(name: str) -> str:
    return "".join(part[:1].upper() + part[1:] for part in re.split(r"[_\W]+", name) if part)


def comment(text: str | None, indent: str = "", style: str = "c") -> list[str]:
    if not text:
        return []
    lines = text.splitlines()
    if style == "cpp":
        return [f"{indent}/// {line}".rstrip() for line in lines]
    if len(lines) == 1:
        return [f"{indent}/* {lines[0]} */"]
    return [f"{indent}/* {lines[0]}", *[f"{indent} * {line}".rstrip() for line in lines[1:]], f"{indent} */"]


# ---- model -----------------------------------------------------------------------------------


@dataclass(frozen=True)
class Instance:
    """A record as used: its name and, for a generic record, its type arguments."""

    record: str
    arguments: tuple[str, ...] = ()

    @property
    def c_name(self) -> str:
        return "holder_" + "_".join([snake(a) for a in self.arguments] + [snake(self.record)])

    @property
    def cpp_name(self) -> str:
        return self.qualified("")

    def qualified(self, namespace: str) -> str:
        """The C++ name, with namespace before the record and each type argument."""
        arguments = f"<{', '.join(namespace + a for a in self.arguments)}>" if self.arguments else ""
        return namespace + self.record + arguments


@dataclass
class Slot:
    """A field or parameter after aliases are resolved."""

    name: str
    type: dict[str, Any]
    doc: str | None
    nullable: bool
    optional: bool  # omittable, or has a default
    default: Any = None
    has_default: bool = False
    change: bool = False  # nullable and omittable: a partial update keeps, sets or clears it


class Model:
    def __init__(self, ir: dict[str, Any]):
        self.ir = ir
        self.aliases = {a["name"]: a for a in ir["aliases"]}
        self.records = {r["name"]: r for r in ir["records"]}
        self.errors = ir["errors"]
        self.operations = [op for op in ir["operations"] if not op["daemon_only"]]
        if ir["enums"]:
            raise Unsupported("enums")
        if ir["interfaces"]:
            raise Unsupported("host interfaces")
        for error in self.errors:
            if error["fields"]:
                raise Unsupported(f"error kinds with fields ({error['name']})")
        self.codes = {e["name"]: FIRST_ERROR_CODE + i for i, e in enumerate(self.errors)}
        self.outputs: list[Instance] = []
        self.inputs: list[Instance] = []
        self.listed: set[Instance] = set()
        for op in self.operations:
            for parameter in op["parameters"]:
                self.collect(parameter["type"], self.inputs, {})
            self.collect(op["returns"], self.outputs, {})
        for instance in self.outputs:
            for slot in self.slots(instance):
                if slot.change:
                    raise Unsupported(f"{instance.record}.{slot.name}: a partial update in a result")

    def resolve(self, ref: dict[str, Any]) -> dict[str, Any]:
        """The type with aliases replaced by what they name, keeping any constraints on top."""
        if ref["kind"] == "named" and ref["name"] in self.aliases:
            inner = dict(self.resolve(self.aliases[ref["name"]]["type"]))
            inner.update({k: v for k, v in ref.items() if k not in ("kind", "name")})
            inner.setdefault("alias", ref["name"])
            return inner
        return ref

    def instance(self, ref: dict[str, Any], bindings: dict[str, Instance]) -> Instance:
        if ref["kind"] == "parameter":
            return bindings[ref["name"]]
        arguments = tuple(self.instance(a, bindings).record for a in ref.get("arguments", []))
        for argument in ref.get("arguments", []):
            if self.instance(argument, bindings).arguments:
                raise Unsupported("generic records as type arguments")
        return Instance(ref["name"], arguments)

    def collect(self, ref: dict[str, Any], into: list[Instance], bindings: dict[str, Instance]) -> None:
        ref = self.resolve(ref)
        kind = ref["kind"]
        if kind in ("nullable",):
            self.collect(ref["value"], into, bindings)
        elif kind == "list":
            items = self.resolve(ref["items"])
            if items["kind"] in ("named", "parameter"):
                self.listed.add(self.instance(items, bindings))
            self.collect(ref["items"], into, bindings)
        elif kind == "union":
            for alternative in ref["alternatives"]:
                self.collect(alternative, into, bindings)
        elif kind in ("named", "parameter"):
            instance = self.instance(ref, bindings)
            if instance in into:
                return
            record = self.records[instance.record]
            inner = dict(zip(record.get("type_parameters", []), [Instance(a) for a in instance.arguments]))
            for slot in self.slots(instance):
                self.collect(slot.type, into, inner)
            into.append(instance)

    def slots(self, instance: Instance) -> list[Slot]:
        """A record's fields, its base's first, with type parameters replaced by the instance's arguments."""
        record = self.records[instance.record]
        out = self.slots(Instance(record["base"])) if "base" in record else []
        bindings = dict(zip(record.get("type_parameters", []), instance.arguments))
        for field in record["fields"]:
            out.append(self.slot(field["name"], self.substitute(field["type"], bindings), field.get("doc"),
                                 omittable=field.get("omittable", False), default=field.get("default"),
                                 has_default="default" in field))
        return out

    def substitute(self, ref: dict[str, Any], bindings: dict[str, str]) -> dict[str, Any]:
        if ref["kind"] == "parameter":
            return {"kind": "named", "name": bindings[ref["name"]]}
        out = dict(ref)
        for key in ("value", "items"):
            if key in out:
                out[key] = self.substitute(out[key], bindings)
        if "alternatives" in out:
            out["alternatives"] = [self.substitute(a, bindings) for a in out["alternatives"]]
        return out

    def slot(self, name, ref, doc, omittable=False, default=None, has_default=False) -> Slot:
        ref = self.resolve(ref)
        nullable = ref["kind"] == "nullable"
        if nullable:
            ref = {**self.resolve(ref["value"]), **{k: v for k, v in ref.items() if k not in ("kind", "value")}}
        if nullable and omittable and ref["kind"] != "string":
            raise Unsupported(f"{name}: a partial update of a {ref['kind']}, which only strings have so far")
        if ref["kind"] in ("map", "json", "secret", "stream", "bytes", "none"):
            raise Unsupported(f"{name}: {ref['kind']}")
        if "pattern" in ref:
            raise Unsupported(f"{name}: patterns, which the glue would not check")
        return Slot(name, ref, doc, nullable, omittable or has_default, default, has_default, nullable and omittable)

    def changes(self) -> bool:
        """Whether an input record has a field a partial update keeps, sets or clears."""
        return any(slot.change for instance in self.inputs for slot in self.slots(instance))


# ---- shared C and C++ spellings ---------------------------------------------------------------


def list_c_name(model: Model, items: dict[str, Any], bindings=None) -> str:
    items = model.resolve(items)
    if items["kind"] == "string":
        return "holder_string_list"
    if items["kind"] in SCALARS_C:
        return f"holder_{items['kind']}_list"
    return model.instance(items, bindings or {}).c_name + "_list"


def literal_member(value: str) -> str:
    return re.sub(r"\W", "_", str(value)).upper()


# ---- C header ---------------------------------------------------------------------------------


def c_field(model: Model, slot: Slot) -> str:
    t = slot.type
    if slot.change:
        return f"holder_string_change {slot.name};"
    if t["kind"] == "string":
        return f"const char* {slot.name};"
    if t["kind"] in SCALARS_C:
        return f"const {SCALARS_C[t['kind']]}* {slot.name};" if slot.nullable or slot.optional \
            else f"{SCALARS_C[t['kind']]} {slot.name};"
    if t["kind"] == "list":
        name = list_c_name(model, t["items"])
        return f"const {name}* {slot.name};" if slot.nullable or slot.optional else f"{name} {slot.name};"
    if t["kind"] == "named":
        name = model.instance(t, {}).c_name
        return f"const {name}* {slot.name};" if slot.nullable or slot.optional else f"{name} {slot.name};"
    raise Unsupported(f"{slot.name}: {t['kind']} in a record")


def c_field_note(slot: Slot) -> str:
    notes = []
    if slot.change:
        notes.append("Zero keeps it.")
    elif slot.nullable:
        notes.append("May be NULL.")
    elif slot.optional and not slot.has_default:
        notes.append("NULL when absent.")
    elif slot.has_default:
        notes.append(f"NULL for the default, {format_default(slot.default)}.")
    return " ".join(notes)


def format_default(value: Any) -> str:
    return '""' if value == "" else ("null" if value is None else str(value).lower() if isinstance(value, bool) else str(value))


@dataclass
class Param:
    """How one operation parameter is passed in C."""

    slot: Slot
    c_decl: str
    note: str
    enum_lines: list[str]


def c_param(model: Model, op: dict[str, Any], parameter: dict[str, Any]) -> Param:
    slot = model.slot(parameter["name"], parameter["type"], parameter.get("doc"),
                      default=parameter.get("default"), has_default="default" in parameter)
    t, name, prefix = slot.type, slot.name, f"holder_{op['name']}_{slot.name}"
    upper = prefix.upper()
    if t["kind"] == "string":
        note = "May be NULL." if slot.nullable else (f"NULL for {format_default(slot.default)}." if slot.has_default else "")
        return Param(slot, f"const char* {name}", note, [])
    if t["kind"] in SCALARS_C:
        zero_default = slot.has_default and not slot.nullable and slot.default in (0, False, 0.0)
        if slot.nullable or (slot.optional and not zero_default):
            note = "May be NULL." if slot.nullable else f"NULL for {format_default(slot.default)}."
            return Param(slot, f"const {SCALARS_C[t['kind']]}* {name}", note, [])
        return Param(slot, f"{SCALARS_C[t['kind']]} {name}", "", [])
    if t["kind"] == "literal" and not slot.nullable:
        members = []
        if slot.has_default:
            members.append(f"    {upper}_DEFAULT = 0, /* {format_default(slot.default)} */")
        for i, value in enumerate(t["values"], 1):
            members.append(f"    {upper}_{literal_member(value)} = {i},")
        members.append(f"    {upper}_MAX_ENUM = 0x7FFFFFFF /* Makes every int a valid value, so bad ones can be rejected. */")
        lines = ["typedef enum " + prefix + " {", *members, "} " + prefix + ";"]
        return Param(slot, f"{prefix} {name}", "", lines)
    if t["kind"] == "union" and slot.nullable:
        literals = [a for a in t["alternatives"] if a["kind"] == "literal"]
        others = [model.resolve(a) for a in t["alternatives"] if a["kind"] != "literal"]
        if len(others) != 1 or others[0]["kind"] != "string":
            raise Unsupported(f"{name}: unions other than literals plus one string")
        value_name = snake(others[0].get("alias", "value"))
        members = [f"    {upper}_NONE = 0,"]
        for value in [v for lit in literals for v in lit["values"]]:
            members.append(f"    {upper}_{literal_member(value)},")
        members.append(f"    {upper}_{value_name.upper()},")
        members.append(f"    {upper}_MAX_ENUM = 0x7FFFFFFF /* Makes every int a valid value, so bad ones can be rejected. */")
        lines = ["typedef enum " + prefix + "_kind {", *members, "} " + prefix + "_kind;", "",
                 "typedef struct " + prefix + " {", f"    {prefix}_kind kind;",
                 f"    const char* {value_name}; /* Set when kind is {upper}_{value_name.upper()}. */",
                 "} " + prefix + ";"]
        return Param(slot, f"{prefix} {name}", "The zero value is null.", lines)
    if t["kind"] == "named" and not slot.nullable:
        return Param(slot, f"const {model.instance(t, {}).c_name}* {name}", "", [])
    raise Unsupported(f"{op['name']}({name}): {t['kind']} parameters")


def c_out(model: Model, op: dict[str, Any]) -> tuple[str, str] | None:
    ref = model.resolve(op["returns"])
    if ref["kind"] == "none":
        return None
    if ref["kind"] != "named":
        raise Unsupported(f"{op['name']}: {ref['kind']} results")
    instance = model.instance(ref, {})
    return instance.c_name, "out_" + snake(instance.record)


def header_lines(ir: dict[str, Any], what: str, style: str) -> list[str]:
    sources = sorted({d["source"] for key in ("records", "operations", "errors", "aliases") for d in ir[key]})
    text = [f"Generated from contract/{', contract/'.join(sources)} by holder_contract.",
            "Do not edit; change the contract, then run make -C contract update.", "", what]
    return [("// " if style == "cpp" else " * ") + line if line else ("//" if style == "cpp" else " *") for line in text]


def generate_c_header(model: Model) -> str:
    out = ["/*", *header_lines(model.ir, "The C declarations for the contract's operations, included by holder/holder.h, "
                                         "which declares holder_context, holder_error and the base status codes. "
                                         "Results live in the holder_arena the caller passes, until holder_arena_free.",
                               "c"), " */",
           "#ifndef HOLDER_API_H", "#define HOLDER_API_H", "", "#include <stdbool.h>", "#include <stddef.h>",
           "#include <stdint.h>", "", "#ifdef __cplusplus", 'extern "C" {', "#endif", "",
           "typedef struct holder_arena holder_arena;", ""]
    for error in model.errors:
        out += comment(error["doc"])
        out.append(f"#define HOLDER_ERROR_{snake(error['name']).upper()} {model.codes[error['name']]}")
    out += ["", "holder_arena* holder_arena_new(void);", "void holder_arena_free(holder_arena* arena);", "",
            "typedef struct holder_string_list {", "    const char* const* items;", "    size_t count;",
            "} holder_string_list;", ""]
    if model.changes():
        out += ["/* What a partial update does to a field. */", "typedef enum holder_change {",
                "    HOLDER_KEEP = 0, /* Leave it as it is. */", "    HOLDER_SET = 1, /* Set it to value. */",
                "    HOLDER_CLEAR = 2, /* Make it null. */",
                "    HOLDER_CHANGE_MAX_ENUM = 0x7FFFFFFF /* Makes every int a valid value, so bad ones can be rejected. */",
                "} holder_change;", "",
                "typedef struct holder_string_change {", "    holder_change change;",
                "    const char* value; /* Read when change is HOLDER_SET. */", "} holder_string_change;", ""]
    for instance in unique(model.inputs + model.outputs):
        record = model.records[instance.record]
        out += comment(record["doc"])
        out.append(f"typedef struct {instance.c_name} {{")
        for slot in model.slots(instance):
            note = " ".join(x for x in [slot.doc or "", c_field_note(slot)] if x)
            out.append(f"    {c_field(model, slot)}" + (f" /* {note} */" if note else ""))
        out += [f"}} {instance.c_name};", ""]
        if instance in model.listed:
            out += [f"typedef struct {instance.c_name}_list {{", f"    const {instance.c_name}* items;",
                    "    size_t count;", f"}} {instance.c_name}_list;", ""]
    for op in model.operations:
        params = [c_param(model, op, p) for p in op["parameters"]]
        for param in params:
            if param.enum_lines:
                out += [*param.enum_lines, ""]
        doc = "\n".join(x for x in [op["summary"], op["description"]] if x)
        notes = [f"{p.slot.name}: " + " ".join(x for x in [p.slot.doc, p.note] if x) for p in params if p.note or p.slot.doc]
        if op["errors"]:
            notes.append("Errors: " + ", ".join(f"HOLDER_ERROR_{snake(e).upper()}" for e in op["errors"]) + ".")
        out += comment("\n".join([doc, *notes]))
        result = c_out(model, op)
        args = ["holder_context* context", *[p.c_decl for p in params], "holder_arena* arena"]
        if result:
            args.append(f"const {result[0]}** {result[1]}")
        args.append("holder_error** out_error")
        out.append(f"int holder_{op['name']}(" + ", ".join(args) + ");")
        out.append("")
    out += ["#ifdef __cplusplus", "}", "#endif", "", "#endif", ""]
    return "\n".join(out)


def unique(items: list[Instance]) -> list[Instance]:
    seen, out = set(), []
    for item in items:
        if item not in seen:
            seen.add(item)
            out.append(item)
    return out


# ---- C++ declarations for core ----------------------------------------------------------------


def cpp_type(model: Model, slot: Slot, generic: dict[str, str] | None = None) -> str:
    base = cpp_base(model, slot.type, generic or {})
    if slot.change:
        return f"Change<{base}>"
    return f"std::optional<{base}>" if slot.nullable or (slot.optional and not slot.has_default) else base


def cpp_base(model: Model, ref: dict[str, Any], generic: dict[str, str]) -> str:
    ref = model.resolve(ref)
    if ref["kind"] == "string":
        return "std::string"
    if ref["kind"] in SCALARS_CPP:
        return SCALARS_CPP[ref["kind"]]
    if ref["kind"] == "list":
        return f"std::vector<{cpp_base(model, ref['items'], generic)}>"
    if ref["kind"] == "parameter":
        return generic[ref["name"]]
    if ref["kind"] == "named":
        return model.instance(ref, {}).cpp_name
    raise Unsupported(ref["kind"])


def cpp_param(model: Model, op: dict[str, Any], param: Param) -> tuple[str, list[str]]:
    slot, t = param.slot, param.slot.type
    type_name = camel(op["name"]) + camel(slot.name)
    if t["kind"] == "literal":
        members = ", ".join(camel(str(v)) for v in t["values"])
        return f"{type_name} {slot.name}", [f"enum class {type_name} {{ {members} }};"]
    if t["kind"] == "union":
        lines, alternatives = [], []
        for value in [v for a in t["alternatives"] if a["kind"] == "literal" for v in a["values"]]:
            lines.append(f"struct {type_name}{camel(str(value))} {{}};")
            alternatives.append(f"{type_name}{camel(str(value))}")
        alternatives.append("std::string")
        lines.append(f"using {type_name} = std::variant<{', '.join(alternatives)}>;")
        return f"std::optional<{type_name}> {slot.name}", lines
    if t["kind"] == "named":
        return f"const {cpp_base(model, t, {})}& {slot.name}", []
    return f"{cpp_type(model, slot)} {slot.name}", []


def generate_cpp_header(model: Model) -> str:
    out = [*header_lines(model.ir, "The C++ declarations core implements: one function per operation. A missing "
                                   "implementation fails to link.", "cpp"),
           "#pragma once", "", "#include <cstdint>", "#include <optional>", "#include <stdexcept>",
           "#include <string>", "#include <variant>", "#include <vector>", "", "struct holder_context;",
           "struct holder_error;", "", "namespace holder::contract {", "",
           "/// Base of the error kinds operations throw. Anything else thrown is an unexpected failure.",
           "struct Error : std::runtime_error {", "  using std::runtime_error::runtime_error;", "};", "",
           "/// Thrown for an argument the contract's checks cannot catch.",
           "struct InvalidArgument : Error {", "  using Error::Error;", "};", ""]
    if model.changes():
        out += ["/// A field of a partial update: nullopt keeps it, an empty value clears it, and a value sets it.",
                "template <class T>", "using Change = std::optional<std::optional<T>>;", ""]
    for error in model.errors:
        out += comment(error["doc"], style="cpp")
        out += [f"struct {error['name']} : Error {{", "  using Error::Error;", "};", ""]
    for name, record in model.records.items():
        if not record.get("type_parameters"):
            continue
        out += comment(record["doc"], style="cpp")
        params = record["type_parameters"]
        out.append(f"template <{', '.join('class ' + p for p in params)}>")
        out.append(f"struct {name} {{")
        for field in record["fields"]:
            slot = model.slot(field["name"], field["type"], field.get("doc"), omittable=field.get("omittable", False),
                              default=field.get("default"), has_default="default" in field)
            out.append(f"  {cpp_type(model, slot, {p: p for p in params})} {slot.name};"
                       + (f"  ///< {slot.doc}" if slot.doc else ""))
        out += ["};", ""]
    # Records as C++ structs, each after the record it extends, which C++ inherits rather than flattens.
    names: list[str] = []
    for instance in unique(model.inputs + model.outputs):
        if instance.arguments:
            continue
        chain, name = [], instance.record
        while name:
            chain.insert(0, name)
            name = model.records[name].get("base")
        names += [n for n in chain if n not in names]
    for name in names:
        record = model.records[name]
        out += comment(record["doc"], style="cpp")
        out.append(f"struct {name}" + (f" : {record['base']}" if "base" in record else "") + " {")
        for field in record["fields"]:
            slot = model.slot(field["name"], field["type"], field.get("doc"), omittable=field.get("omittable", False),
                              default=field.get("default"), has_default="default" in field)
            out.append(f"  {cpp_type(model, slot)} {slot.name}{cpp_initializer(model, slot)};"
                       + (f"  ///< {slot.doc}" if slot.doc else ""))
        out += ["};", ""]
    for op in model.operations:
        params = [c_param(model, op, p) for p in op["parameters"]]
        declarations = []
        for param in params:
            decl, extra = cpp_param(model, op, param)
            out += extra
            declarations.append(decl)
        if any(cpp_param(model, op, p)[1] for p in params):
            out.append("")
        doc = "\n".join(x for x in [op["summary"], op["description"]] if x)
        out += comment(doc, style="cpp")
        out += [f"/// @param {p.slot.name} {p.slot.doc}" for p in params if p.slot.doc]
        if op["errors"]:
            out.append(f"/// @throws {', '.join(op['errors'])}")
        result = model.resolve(op["returns"])
        returns = "void" if result["kind"] == "none" else cpp_base(model, result, {})
        out.append(f"{returns} {op['name']}(" + ", ".join(["holder_context& context", *declarations]) + ");")
        out.append("")
    out += ["namespace detail {", "",
            "/// Core supplies this: sets *out_error (when not null) to a new holder_error with message, and",
            "/// returns code.",
            "int fail(holder_error** out_error, int code, const std::string& message);", "",
            "}  // namespace detail", "", "}  // namespace holder::contract", ""]
    return "\n".join(out)


def cpp_initializer(model: Model, slot: Slot) -> str:
    """A field's default: the contract's, or {} for a number, so a new record holds no garbage."""
    if slot.has_default and slot.default is not None:
        return f" = {cpp_literal(slot.default)}"
    if model.resolve(slot.type)["kind"] in SCALARS_CPP and not (slot.nullable or slot.optional):
        return "{}"
    return ""


def cpp_literal(value: Any) -> str:
    if isinstance(value, bool):
        return "true" if value else "false"
    if isinstance(value, str):
        return '"' + value.replace("\\", "\\\\").replace('"', '\\"') + '"'
    return str(value)


# ---- glue -------------------------------------------------------------------------------------

GLUE_SUPPORT = r"""
namespace contract = holder::contract;

struct holder_arena {
  std::vector<std::unique_ptr<std::byte[]>> blocks;
};

namespace {

template <class T>
T* make(holder_arena* arena, std::size_t count = 1) {
  if (count == 0) return nullptr;
  auto block = std::make_unique<std::byte[]>(sizeof(T) * count);
  T* items = reinterpret_cast<T*>(block.get());
  std::uninitialized_value_construct_n(items, count);
  arena->blocks.push_back(std::move(block));
  return items;
}

const char* copy(holder_arena* arena, const std::string& text) {
  char* out = make<char>(arena, text.size() + 1);
  std::memcpy(out, text.data(), text.size());
  return out;
}

template <class C, class T>
const C* copy_value(holder_arena* arena, const std::optional<T>& value) {
  if (!value) return nullptr;
  C* out = make<C>(arena);
  *out = static_cast<C>(*value);
  return out;
}

const char* copy_text(holder_arena* arena, const std::optional<std::string>& value) {
  return value ? copy(arena, *value) : nullptr;
}

holder_string_list copy_strings(holder_arena* arena, const std::vector<std::string>& values) {
  auto* items = make<const char*>(arena, values.size());
  for (std::size_t i = 0; i < values.size(); ++i) items[i] = copy(arena, values[i]);
  return {items, values.size()};
}

struct ArgumentError : std::invalid_argument {
  using std::invalid_argument::invalid_argument;
};

void require(bool ok, const std::string& message) {
  if (!ok) throw ArgumentError(message);
}

// The number of characters in text, or nothing when it is not well-formed UTF-8: no overlong
// forms, surrogates or code points above U+10FFFF.
std::optional<std::size_t> utf8_length(const std::string& text) {
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

// Lengths count characters, as the contract does.
std::string text(const char* value, const char* name, std::size_t min_length, std::size_t max_length) {
  require(value != nullptr, std::string(name) + " must not be null");
  std::string out(value);
  const auto length = utf8_length(out);
  require(length.has_value(), std::string(name) + " must be UTF-8");
  require(*length >= min_length, std::string(name) + (min_length == 1 ? " must not be empty"
                                                                       : " must be at least " + std::to_string(min_length) + " characters"));
  require(*length <= max_length, std::string(name) + " must be at most " + std::to_string(max_length) + " characters");
  return out;
}

std::optional<std::string> optional_text(const char* value, const char* name, std::size_t min_length,
                                         std::size_t max_length) {
  if (value == nullptr) return std::nullopt;
  return text(value, name, min_length, max_length);
}

template <class T>
T in_range(T value, const char* name, T minimum, T maximum) {
  require(value >= minimum && value <= maximum, std::string(name) + " must be from " + std::to_string(minimum) +
                                                    " to " + std::to_string(maximum));
  return value;
}

double finite(double value, const char* name) {
  require(std::isfinite(value), std::string(name) + " must be a finite number");
  return value;
}

template <class Body>
int guarded(holder_arena* arena, holder_error** out_error, Body&& body) {
  if (out_error != nullptr) *out_error = nullptr;
  if (arena == nullptr) return contract::detail::fail(out_error, HOLDER_ERROR_INVALID_ARGUMENT, "arena must not be null");
  try {
    body();
    return HOLDER_OK;
  } catch (const ArgumentError& e) {
    return contract::detail::fail(out_error, HOLDER_ERROR_INVALID_ARGUMENT, e.what());
  } catch (const contract::InvalidArgument& e) {
    return contract::detail::fail(out_error, HOLDER_ERROR_INVALID_ARGUMENT, e.what());
@@ERROR_CATCHES@@  } catch (const std::bad_alloc&) {
    return contract::detail::fail(out_error, HOLDER_ERROR_ALLOCATION, "allocation failed");
  } catch (const std::exception& e) {
    return contract::detail::fail(out_error, HOLDER_ERROR_RUNTIME, e.what());
  } catch (...) {
    return contract::detail::fail(out_error, HOLDER_ERROR_RUNTIME, "unknown failure");
  }
}
"""


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


def limits(t: dict[str, Any]) -> tuple[str, str]:
    return str(t.get("min_length", 0)), str(t["max_length"]) if "max_length" in t else "SIZE_MAX"


def to_c_value(model: Model, slot: Slot, source: str) -> str:
    t = slot.type
    if slot.change:
        raise Unsupported(f"{slot.name}: a partial update in a result")
    optional = slot.nullable or (slot.optional and not slot.has_default)
    if t["kind"] == "string":
        return f"copy_text(arena, {source})" if optional else f"copy(arena, {source})"
    if t["kind"] in SCALARS_C:
        return f"copy_value<{SCALARS_C[t['kind']]}>(arena, {source})" if optional else source
    if t["kind"] == "list":
        items = model.resolve(t["items"])
        if optional:
            raise Unsupported(f"{slot.name}: optional lists")
        if items["kind"] == "string":
            return f"copy_strings(arena, {source})"
        return f"to_c_list(arena, {source})"
    if t["kind"] == "named":
        if optional:
            raise Unsupported(f"{slot.name}: optional records")
        return f"to_c(arena, {source})"
    raise Unsupported(f"{slot.name}: {t['kind']}")


def from_c_value(model: Model, slot: Slot, source: str, label: str) -> str:
    t = slot.type
    optional = slot.nullable or slot.optional
    if slot.change:
        low, high = limits(t)
        return f'changed_text({source}, "{label}", {low}, {high})'
    if t["kind"] == "string":
        low, high = limits(t)
        if optional:
            value = f'optional_text({source}, "{label}", {low}, {high})'
            if slot.has_default and slot.default is not None:
                return f"{value}.value_or({cpp_literal(slot.default)})"
            return value
        return f'text({source}, "{label}", {low}, {high})'
    if t["kind"] in SCALARS_C:
        check = scalar_check(t, label)
        if optional:
            cpp = SCALARS_CPP[t["kind"]]
            inner = f"{check.format(f'*{source}')}"
            if slot.has_default and slot.default is not None:
                return f"({source} ? {inner} : {cpp}{{{cpp_literal(slot.default)}}})"
            return f"({source} ? std::optional<{cpp}>({inner}) : std::nullopt)"
        return check.format(source)
    raise Unsupported(f"{label}: {t['kind']} inputs")


def scalar_check(t: dict[str, Any], label: str) -> str:
    if t["kind"] in ("int32", "int64") and ("ge" in t or "le" in t):
        cpp = SCALARS_CPP[t["kind"]]
        low = t.get("ge", f"std::numeric_limits<{cpp}>::min()")
        high = t.get("le", f"std::numeric_limits<{cpp}>::max()")
        return f'in_range<{cpp}>({{}}, "{label}", {low}, {high})'
    if t["kind"] == "float64":
        if any(k in t for k in ("ge", "le", "gt", "lt")):
            raise Unsupported(f"{label}: bounds on a float64")
        return f'finite({{}}, "{label}")'
    return "{}"


def generate_glue(model: Model) -> str:
    catches = "".join(
        f"  }} catch (const contract::{e['name']}& e) {{\n"
        f"    return contract::detail::fail(out_error, HOLDER_ERROR_{snake(e['name']).upper()}, e.what());\n"
        for e in model.errors)
    out = [*header_lines(model.ir, "The C entry points: argument checks, conversions and error codes.", "cpp"),
           '#include "Contract.h"', '#include "holder/holder.h"', "", "#include <cmath>", "#include <cstddef>", "#include <cstdint>",
           "#include <cstring>", "#include <limits>", "#include <memory>", "#include <new>", "#include <optional>", "#include <string>",
           "#include <utility>", "#include <vector>",
           GLUE_SUPPORT.replace("@@ERROR_CATCHES@@", catches).rstrip(), ""]
    if model.changes():
        out += [CHANGE_SUPPORT.strip("\n"), ""]
    outputs = unique(model.outputs)
    for instance in outputs:
        out.append(f"{instance.c_name} to_c(holder_arena* arena, const {instance.qualified('contract::')}& value);")
        if instance in model.listed:
            out.append(f"{instance.c_name}_list to_c_list(holder_arena* arena, "
                       f"const std::vector<{instance.qualified('contract::')}>& values);")
    out.append("")
    for instance in outputs:
        out.append(f"{instance.c_name} to_c(holder_arena* arena, const {instance.qualified('contract::')}& value) {{")
        out.append(f"  {instance.c_name} out{{}};")
        for slot in model.slots(instance):
            out.append(f"  out.{slot.name} = {to_c_value(model, slot, 'value.' + slot.name)};")
        out += ["  return out;", "}", ""]
        if instance in model.listed:
            out += [(f"{instance.c_name}_list to_c_list(holder_arena* arena, "
                     f"const std::vector<{instance.qualified('contract::')}>& values) {{"),
                    f"  auto* items = make<{instance.c_name}>(arena, values.size());",
                    "  for (std::size_t i = 0; i < values.size(); ++i) items[i] = to_c(arena, values[i]);",
                    "  return {items, values.size()};", "}", ""]
    for instance in unique(model.inputs):
        out.append(f"{instance.qualified('contract::')} from_c(const {instance.c_name}& value) {{")
        out.append(f"  {instance.qualified('contract::')} out;")
        for slot in model.slots(instance):
            label = f"{snake(instance.record)}.{slot.name}"
            out.append(f"  out.{slot.name} = {from_c_value(model, slot, 'value.' + slot.name, label)};")
        out += ["  return out;", "}", ""]
    out += ["}  // namespace", "", 'extern "C" {', "",
            "holder_arena* holder_arena_new(void) { return new (std::nothrow) holder_arena; }",
            "void holder_arena_free(holder_arena* arena) { delete arena; }", ""]
    for op in model.operations:
        out += glue_operation(model, op)
    out += ['}  // extern "C"', ""]
    return "\n".join(out)


def glue_operation(model: Model, op: dict[str, Any]) -> list[str]:
    params = [c_param(model, op, p) for p in op["parameters"]]
    result = c_out(model, op)
    args = ["holder_context* context", *[p.c_decl for p in params], "holder_arena* arena"]
    if result:
        args.append(f"const {result[0]}** {result[1]}")
    args.append("holder_error** out_error")
    lines = [f"int holder_{op['name']}(" + ", ".join(args) + ") {", "  return guarded(arena, out_error, [&] {",
             '    require(context != nullptr, "context must not be null");']
    if result:
        lines += [f'    require({result[1]} != nullptr, "{result[1]} must not be null");', f"    *{result[1]} = nullptr;"]
    call_args = ["*context"]
    for param in params:
        lines += glue_param(model, op, param)
        call_args.append(f"{param.slot.name}_value")
    call = f"contract::{op['name']}(" + ", ".join(call_args) + ")"
    if result:
        lines += [f"    auto* out = make<{result[0]}>(arena);", f"    *out = to_c(arena, {call});",
                  f"    *{result[1]} = out;"]
    else:
        lines.append(f"    {call};")
    lines += ["  });", "}", ""]
    return lines


def glue_param(model: Model, op: dict[str, Any], param: Param) -> list[str]:
    slot, t, name = param.slot, param.slot.type, param.slot.name
    prefix = f"holder_{op['name']}_{name}".upper()
    type_name = camel(op["name"]) + camel(name)
    if t["kind"] == "literal":
        lines = [f"    contract::{type_name} {name}_value;", f"    switch ({name}) {{"]
        if slot.has_default:
            lines.append(f"    case {prefix}_DEFAULT: {name}_value = contract::{type_name}::{camel(str(slot.default))}; break;")
        for value in t["values"]:
            lines.append(f"    case {prefix}_{literal_member(value)}: {name}_value = contract::{type_name}::{camel(str(value))}; break;")
        lines += [f'    default: throw ArgumentError("{name} is not a {prefix.lower()} value");', "    }"]
        return lines
    if t["kind"] == "union":
        value = next(model.resolve(a) for a in t["alternatives"] if a["kind"] != "literal")
        value_name = snake(value.get("alias", "value"))
        low, high = limits(value)
        lines = [f"    std::optional<contract::{type_name}> {name}_value;", f"    switch ({name}.kind) {{",
                 f"    case {prefix}_NONE: break;"]
        for lit in [v for a in t["alternatives"] if a["kind"] == "literal" for v in a["values"]]:
            lines.append(f"    case {prefix}_{literal_member(lit)}: {name}_value = contract::{type_name}{camel(str(lit))}{{}}; break;")
        lines += [(f"    case {prefix}_{value_name.upper()}: {name}_value = "
                   f'text({name}.{value_name}, "{name}.{value_name}", {low}, {high}); break;'),
                  f'    default: throw ArgumentError("{name}.kind is not a {prefix.lower()}_kind value");', "    }"]
        return lines
    if t["kind"] == "named":
        return [f'    require({name} != nullptr, "{name} must not be null");',
                f'    const auto {name}_value = from_c(*{name});']
    if t["kind"] in SCALARS_C and "*" not in param.c_decl:
        # Passed by value: required, or a default that is the zero value.
        return [f"    const auto {name}_value = {scalar_check(t, name).format(name)};"]
    return [f"    const auto {name}_value = {from_c_value(model, slot, name, name)};"]


CORE_PATHS = {"holder_api.h": "core/include/holder/holder_api.h", "Contract.h": "core/src/api/Contract.h",
              "ContractGlue.cpp": "core/src/api/ContractGlue.cpp"}


def generate(ir: dict[str, Any]) -> dict[str, str]:
    """The generated files, by name."""
    model = Model(ir)
    return {"holder_api.h": generate_c_header(model), "Contract.h": generate_cpp_header(model),
            "ContractGlue.cpp": generate_glue(model)}
