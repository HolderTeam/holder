"""holder_api.h: the C declarations every consumer of core's C API sees."""

from __future__ import annotations

from compiler.builder import BASE_ERROR_CODES
from compiler.ir import ListType, LiteralType, MapType, NamedType, NullableType

from ..common import Unsupported, contract_sources, header_lines
from .abi import SCALARS_C, format_default, literal_member, snake
from .render import c_param, c_signature, comment, host_c_params
from .types import CoreTypes, Slot, c_enum_name, list_c_name, map_c_name, unique


def c_field(types: CoreTypes, slot: Slot) -> str:
    t = slot.type
    if slot.change:
        return f"holder_string_change {slot.name};"
    if isinstance(t, LiteralType):
        return f"{c_enum_name(slot)} {slot.name};"
    if t.kind == "string":
        return f"const char* {slot.name};"
    if t.kind in SCALARS_C:
        return f"const {SCALARS_C[t.kind]}* {slot.name};" if slot.nullable or slot.optional \
            else f"{SCALARS_C[t.kind]} {slot.name};"
    if isinstance(t, ListType):
        name = list_c_name(types, t.items)
        return f"const {name}* {slot.name};" if slot.nullable or slot.optional else f"{name} {slot.name};"
    if isinstance(t, MapType):
        name = map_c_name(slot.alias)
        return f"const {name}* {slot.name};" if slot.nullable or slot.optional else f"{name} {slot.name};"
    if isinstance(t, NamedType):
        name = types.instance(t, {}).c_name
        return f"const {name}* {slot.name};" if slot.nullable or slot.optional else f"{name} {slot.name};"
    raise Unsupported(f"{slot.name}: {t.kind} in a record")


def c_field_note(slot: Slot) -> str:
    notes = []
    if slot.change:
        notes.append("Zero keeps it.")
    elif slot.nullable:
        notes.append("May be NULL.")
    elif slot.optional and not slot.has_default:
        notes.append("NULL when absent.")
    elif slot.has_default and isinstance(slot.type, LiteralType):
        notes.append(f"Zero for the default, {format_default(slot.default)}.")
    elif slot.has_default:
        notes.append(f"NULL for the default, {format_default(slot.default)}.")
    return " ".join(notes)


def host_callback(types: CoreTypes, interface, method) -> str:
    args = ["void* user_data"]
    for param in method.parameters:
        ref = types.resolve(param.type).type
        kind = types.resolve(ref.value).type.kind if isinstance(ref, NullableType) else ref.kind
        if kind == "bytes":
            args.extend([f"const unsigned char* {param.name}", f"size_t {param.name}_len"])
        else:
            args.append(f"{ {'string': 'const char*', 'int32': 'int32_t', 'int64': 'int64_t'}[kind] } {param.name}")
    result = types.resolve(method.returns).type
    if isinstance(result, NullableType):
        if types.resolve(result.value).type.kind == "int64":
            args += ["int64_t* out_value", "int* out_present"]
        else:
            args.append("char** out_value")
    elif result.kind == "string":
        args.append("char** out_value")
    elif result.kind == "bytes":
        args.extend(["unsigned char** out_value", "size_t* out_value_len"])
    elif result.kind == "bool":
        args.append("int* out_value")
    if interface.error_codes:
        args.append("int* out_error_code")
    args.append("char** out_error")
    return f"    int (*{method.name})({', '.join(args)});"


def generate_c_header(types: CoreTypes) -> str:
    out = ["/*", *header_lines(contract_sources(types.contract),
                               "The standalone C declarations for the contract's operations. "
                               "Results live in the holder_arena the caller passes, until holder_arena_free.",
                               "c"), " */",
           "#ifndef HOLDER_API_H", "#define HOLDER_API_H", "", "#include <stdbool.h>", "#include <stddef.h>",
           "#include <stdint.h>", "", "#ifdef __cplusplus", 'extern "C" {', "#endif", "",
           "typedef struct holder_context holder_context;", "typedef struct holder_error holder_error;",
           "typedef struct holder_arena holder_arena;", "",
           "/* An error code is the HTTP status it echoes plus one digit, so it reads as that",
           " * status: an invalid argument is 4000, an unexpected failure is 5000.",
           " * Contract error kinds use the same scheme. */", "#define HOLDER_OK 0"]
    out += [f"#define HOLDER_ERROR_{base.name} {code}" for code, base in sorted(BASE_ERROR_CODES.items())]
    out.append("")
    for error in sorted(types.errors, key=lambda e: e.code):
        out += comment(error.doc)
        out.append(f"#define HOLDER_ERROR_{snake(error.name).upper()} {types.codes[error.name]}")
    out += ["", "holder_arena* holder_arena_new(void);", "void holder_arena_free(holder_arena* arena);", "",
            "typedef struct holder_string_list {", "    const char* const* items;", "    size_t count;",
            "} holder_string_list;", ""]
    if types.host_operations:
        out += ["/* Called once to release callback user data, including on rejected registration. */",
                "typedef void (*holder_destroy_fn)(void* user_data);", ""]
        for op in types.host_operations:
            interface = types.contract.interface(op.host)
            name = f"holder_{snake(interface.name)}_callbacks"
            if interface.error_codes:
                enum = f"holder_{snake(interface.name.removesuffix('Provider'))}_error_code"
                out += [f"typedef enum {enum} {{",
                        *[f"    {enum.upper().removesuffix('_CODE')}_{code.upper()} = {index},"
                          for index, code in enumerate(interface.error_codes)],
                        f"}} {enum};", ""]
            out += comment(interface.doc)
            out += [f"typedef struct {name} {{"]
            for method in interface.methods:
                out += comment(method.summary, indent="    ")
                out.append(host_callback(types, interface, method))
            out += [f"}} {name};", "",
                    "/* Each callback returns zero on success; nonzero may set malloc-owned *out_error.",
                    " * String and byte results must be malloc-allocated; core frees them after copying.",
                    " * Null strings mean absence only for nullable results.",
                    " * A null numeric result sets *out_present=0. */", ""]
    for alias, ref in types.maps.items():
        name = map_c_name(alias)
        value_type = "const char*" if types.kind(ref.values) == "string" else "holder_string_list"
        out += comment(types.aliases[alias].doc)
        out += [f"typedef struct {name}_entry {{", "    const char* key;", f"    {value_type} values;",
                f"}} {name}_entry;", "", f"typedef struct {name} {{", f"    const {name}_entry* items;",
                "    size_t count;", f"}} {name};", ""]
    if types.changes():
        out += ["/* What a partial update does to a field. */", "typedef enum holder_change {",
                "    HOLDER_KEEP = 0, /* Leave it as it is. */", "    HOLDER_SET = 1, /* Set it to value. */",
                "    HOLDER_CLEAR = 2, /* Make it null. */",
                "    HOLDER_CHANGE_MAX_ENUM = 0x7FFFFFFF /* Makes every int a valid value, so bad ones can be rejected. */",
                "} holder_change;", "",
                "typedef struct holder_string_change {", "    holder_change change;",
                "    const char* value; /* Read when change is HOLDER_SET. */", "} holder_string_change;", ""]
    declared: set[str] = set()
    for instance in unique(types.inputs + types.outputs):
        record = types.records[instance.record]
        for slot in types.slots(instance):
            if not isinstance(slot.type, LiteralType) or c_enum_name(slot) in declared:
                continue
            name = c_enum_name(slot)
            declared.add(name)
            out += comment(slot.doc)
            out.append(f"typedef enum {name} {{")
            if slot.has_default:
                out.append(f"    {name.upper()}_DEFAULT = 0, /* {format_default(slot.default)} */")
            out += [f"    {name.upper()}_{literal_member(v)} = {i}," for i, v in enumerate(slot.type.values, 1)]
            out += [f"    {name.upper()}_MAX_ENUM = 0x7FFFFFFF /* Makes every int a valid value, so bad ones can be rejected. */",
                    f"}} {name};", ""]
        out += comment(record.doc)
        out.append(f"typedef struct {instance.c_name} {{")
        for slot in types.slots(instance):
            note = " ".join(x for x in [slot.doc or "", c_field_note(slot)] if x)
            out.append(f"    {c_field(types, slot)}" + (f" /* {note} */" if note else ""))
        out += [f"}} {instance.c_name};", ""]
        if instance in types.listed:
            out += [f"typedef struct {instance.c_name}_list {{", f"    const {instance.c_name}* items;",
                    "    size_t count;", f"}} {instance.c_name}_list;", ""]
    for op in types.operations:
        params = host_c_params(types, op) if op.host else [c_param(types, op, p) for p in op.parameters]
        for param in params:
            if param.enum_lines:
                out += [*param.enum_lines, ""]
        doc = "\n".join(x for x in [op.summary, op.description] if x)
        notes = [f"{p.slot.name}: " + " ".join(x for x in [p.slot.doc, p.note] if x) for p in params if p.note or p.slot.doc]
        if op.errors:
            notes.append("Errors: " + ", ".join(f"HOLDER_ERROR_{snake(e).upper()}" for e in op.errors) + ".")
        out += comment("\n".join([doc, *notes]))
        out.append(c_signature(types, op) + ";")
        out.append("")
    out += ["#ifdef __cplusplus", "}", "#endif", "", "#endif", ""]
    return "\n".join(out)
