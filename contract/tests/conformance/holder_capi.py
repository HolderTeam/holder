"""Core's generated C API, called through cffi, for the conformance tests.

Operations take and return values as holder_contract.values describes them, converted to and from C
by the conventions in C_API.md, using the generator's own reading of the IR. A few hand-written
functions from holder.h set up a data directory and a project.
"""

from __future__ import annotations

import re
from pathlib import Path
from typing import Any

import cffi

from holder_contract.generate import SCALARS_C, Instance, Model, c_out, c_param, generate, literal_member, snake

CONTRACT = Path(__file__).resolve().parents[2]
SCHEMA = CONTRACT.parent / "core" / "schema" / "schema.sql"
# The status codes from holder.h that are not error kinds.
STATUS = {1: "InvalidArgument", 2: "Runtime", 3: "Allocation"}
# An enum value outside every enum, which the glue must reject.
BAD_ENUM = 1000

HAND_WRITTEN = """
typedef struct holder_context holder_context;
typedef struct holder_error holder_error;
int holder_context_open(const char* data_dir, const char* schema_sql, holder_context** out_context,
                        holder_error** out_error);
void holder_context_destroy(holder_context* context);
int holder_project_create(holder_context* context, const char* name, const char* root_path,
                          const char* privacy_mode, char** out_json, holder_error** out_error);
int holder_card_delete_subtree(holder_context* context, const char* card_id, holder_error** out_error);
void holder_string_free(char* value);
const char* holder_error_message(const holder_error* error);
void holder_error_destroy(holder_error* error);
"""


class OperationError(Exception):
    def __init__(self, kind: str, message: str):
        super().__init__(f"{kind}: {message}")
        self.kind = kind
        self.message = message


class Unrepresentable(Exception):
    """A value C cannot pass, such as null for a number passed by value."""


def declarations(header: str) -> str:
    """The generated header as cffi reads it: no preprocessor lines apart from integer constants."""
    out, skipping = [], False
    for line in header.splitlines():
        if line.startswith("#ifdef __cplusplus"):
            skipping = True
        elif skipping and line.startswith("#endif"):
            skipping = False
        elif not skipping and (not line.startswith("#") or re.match(r"#define \w+ \d+$", line)):
            out.append(line)
    return "\n".join(out)


class Core:
    """One context on a data directory, opened from the shared library at library_path."""

    def __init__(self, ir: dict[str, Any], library_path: str, data_dir: Path):
        self.model = Model(ir)
        self.ffi = cffi.FFI()
        self.ffi.cdef(HAND_WRITTEN + declarations(generate(ir)["holder_api.h"]))
        self.lib = self.ffi.dlopen(library_path)
        self.kinds = {**STATUS, **{code: name for name, code in self.model.codes.items()}}
        context = self.ffi.new("holder_context**")
        self.check(self.lib.holder_context_open(str(data_dir).encode(), SCHEMA.read_bytes(), context,
                                                self.error_slot()))
        self.context = context[0]

    def close(self) -> None:
        self.lib.holder_context_destroy(self.context)

    def error_slot(self):
        self.error = self.ffi.new("holder_error**")
        return self.error

    def check(self, status: int) -> None:
        if status == 0:
            return
        message = self.ffi.string(self.lib.holder_error_message(self.error[0])).decode("utf-8", "replace")
        self.lib.holder_error_destroy(self.error[0])
        raise OperationError(self.kinds.get(status, f"status {status}"), message)

    def create_project(self, name: str) -> str:
        out = self.ffi.new("char**")
        self.check(self.lib.holder_project_create(self.context, name.encode(), self.ffi.NULL, self.ffi.NULL, out,
                                                  self.error_slot()))
        text = self.ffi.string(out[0]).decode()
        self.lib.holder_string_free(out[0])
        return re.search(r'"project_id"\s*:\s*"([^"]+)"', text).group(1)

    def delete_subtree(self, card_id: str) -> None:
        self.check(self.lib.holder_card_delete_subtree(self.context, card_id.encode(), self.error_slot()))

    # ---- contract operations ------------------------------------------------------------------

    def call(self, operation: str, **arguments: Any) -> Any:
        """Call an operation; raises OperationError with the error kind's name when it fails."""
        op = next(o for o in self.model.operations if o["name"] == operation)
        keep: list[Any] = []  # cffi frees what it allocated when the last reference goes
        c_arguments = [self.context]
        for parameter in op["parameters"]:
            param = c_param(self.model, op, parameter)
            c_arguments.append(self.argument(op, param, arguments.get(parameter["name"], ...), keep))
        arena = self.lib.holder_arena_new()
        try:
            out = c_out(self.model, op)
            result = self.ffi.new(f"const {out[0]}**") if out else None
            status = getattr(self.lib, f"holder_{operation}")(*c_arguments, arena, *([result] if out else []),
                                                              self.error_slot())
            self.check(status)
            if not out:
                return None
            return self.from_c(self.model.instance(self.model.resolve(op["returns"]), {}), result[0][0])
        finally:
            self.lib.holder_arena_free(arena)

    def text(self, value: Any, keep: list[Any]):
        """A C string; bytes pass as they are, so that a test can send text that is not UTF-8."""
        if value is ... or value is None:
            return self.ffi.NULL
        keep.append(self.ffi.new("char[]", value if isinstance(value, bytes) else value.encode()))
        return keep[-1]

    def argument(self, op, param, value, keep):
        slot, t = param.slot, param.slot.type
        upper = f"holder_{op['name']}_{slot.name}".upper()
        kind = t["kind"]
        if kind == "string":
            return self.text(value, keep)
        if kind in SCALARS_C:
            if "*" in param.c_decl:
                if value is ... or value is None:
                    return self.ffi.NULL
                keep.append(self.ffi.new(f"{SCALARS_C[kind]}*", value))
                return keep[-1]
            if value is ...:
                return slot.default if slot.has_default else 0
            if value is None:
                raise Unrepresentable(f"null for {slot.name}")
            return value
        if kind == "literal":
            if value is ...:
                return getattr(self.lib, f"{upper}_DEFAULT")
            if value in t["values"]:
                return getattr(self.lib, f"{upper}_{literal_member(value)}")
            if value is None:
                raise Unrepresentable(f"null for {slot.name}")
            return BAD_ENUM
        if kind == "union":
            choice = self.ffi.new(f"holder_{op['name']}_{slot.name}*")
            literals = [v for a in t["alternatives"] if a["kind"] == "literal" for v in a["values"]]
            other = next(self.model.resolve(a) for a in t["alternatives"] if a["kind"] != "literal")
            member = snake(other.get("alias", "value"))
            if value is ... or value is None:
                choice.kind = getattr(self.lib, f"{upper}_NONE")
            elif value in literals:
                choice.kind = getattr(self.lib, f"{upper}_{literal_member(value)}")
            else:
                choice.kind = getattr(self.lib, f"{upper}_{member.upper()}")
                setattr(choice, member, self.text(value, keep))
            keep.append(choice)
            return choice[0]
        if kind == "named":
            if value is ... or value is None:
                return self.ffi.NULL
            instance = self.model.instance(t, {})
            record = self.ffi.new(f"{instance.c_name}*")
            for field in self.model.slots(instance):
                setattr(record, field.name, self.field(field, value.get(field.name, ...), keep))
            keep.append(record)
            return record
        raise Unrepresentable(f"{slot.name}: {kind}")

    def field(self, slot, value, keep):
        kind = slot.type["kind"]
        if slot.change:
            change = self.ffi.new("holder_string_change*")
            if value is ...:
                change.change = self.lib.HOLDER_KEEP
            elif value is None:
                change.change = self.lib.HOLDER_CLEAR
            else:
                change.change = self.lib.HOLDER_SET
                change.value = self.text(value, keep)
            keep.append(change)
            return change[0]
        if kind == "string":
            return self.text(value, keep)
        if kind == "literal":
            if value is ... or value is None:
                raise Unrepresentable(f"no value for {slot.name}")
            if value not in slot.type["values"]:
                return BAD_ENUM
            return getattr(self.lib, f"holder_{snake(slot.type['enum'])}_{literal_member(value)}".upper())
        if kind in SCALARS_C and (slot.nullable or slot.optional):
            if value is ... or value is None:
                return self.ffi.NULL
            keep.append(self.ffi.new(f"{SCALARS_C[kind]}*", value))
            return keep[-1]
        if kind in SCALARS_C:
            if value is ... or value is None:
                raise Unrepresentable(f"no value for {slot.name}")
            return value
        raise Unrepresentable(f"{slot.name}: {kind} in an input record")

    def from_c(self, instance: Instance, struct) -> dict[str, Any]:
        out = {}
        for slot in self.model.slots(instance):
            value = getattr(struct, slot.name)
            kind = slot.type["kind"]
            if kind == "string" or (kind in SCALARS_C and (slot.nullable or slot.optional)):
                if value == self.ffi.NULL:
                    if slot.nullable:
                        out[slot.name] = None
                    continue
                value = self.ffi.string(value).decode() if kind == "string" else value[0]
            elif kind == "list":
                items = self.model.resolve(slot.type["items"])
                if items["kind"] == "string":
                    value = [self.ffi.string(value.items[i]).decode() for i in range(value.count)]
                elif items["kind"] in SCALARS_C:
                    value = [value.items[i] for i in range(value.count)]
                else:
                    item = self.model.instance(items, {})
                    value = [self.from_c(item, value.items[i]) for i in range(value.count)]
            elif kind == "named":
                value = self.from_c(self.model.instance(slot.type, {}), value)
            out[slot.name] = bool(value) if kind == "bool" else value
        return out
