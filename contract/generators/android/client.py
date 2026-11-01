"""Lower every core operation into a typed Kotlin/JNI client over the generated C API.

The plan reads the same lowering as core's C glue (generators/core/types.py) and the same C
signatures (generators/core/abi.py), so a Kotlin value crosses JNI exactly as the C API takes or
returns it. Records become Kotlin data classes, a record's choice of literals an enum class, error
kinds exceptions, and each contract domain one Kotlin file with an object holding its operations.
It knows type shapes, not operation or record names; templates write the syntax.
"""

from __future__ import annotations

import json
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

from jinja2 import Environment, FileSystemLoader, StrictUndefined

from compiler.builder import BASE_ERROR_CODES
from compiler.ir import (
    ConstantView,
    ContractView,
    FieldView,
    ListType,
    LiteralType,
    MapType,
    NamedType,
    OperationView,
    ParameterType,
    UnionType,
)

from ..common import Unsupported
from ..core.abi import SCALARS_C, CAbiError, literal_member
from ..core.types import CoreTypes, Instance, Slot, c_enum_name, list_c_name, map_c_name, unique

TEMPLATES = Environment(
    loader=FileSystemLoader(Path(__file__).with_name("templates")),
    undefined=StrictUndefined,
    autoescape=False,
    trim_blocks=True,
    lstrip_blocks=True,
    keep_trailing_newline=True,
    newline_sequence="\n",
)
# A Kotlin string literal, and a C one.
TEMPLATES.filters["kstring"] = lambda value: json.dumps(value, ensure_ascii=False).replace("$", "\\$")
TEMPLATES.filters["cstring"] = json.dumps

PACKAGE = "team.holder.android.contract"
CLASS_PATH = PACKAGE.replace(".", "/")
KOTLIN_DIR = "apps/mobile/android/app/src/main/java/team/holder/android/contract"
JNI_PATH = "apps/mobile/android/app/src/main/cpp/generated/contract_client.cpp"

# The C API's base status codes (holder.h), as Kotlin exceptions.
BASE_EXCEPTIONS = {4000: "InvalidArgumentException", 5000: "UnexpectedFailureException",
                   5001: "AllocationFailureException"}
if BASE_EXCEPTIONS.keys() != BASE_ERROR_CODES.keys():
    raise RuntimeError("Android's base exceptions differ from the C API's base codes")

KOTLIN_KEYWORDS = {"as", "break", "class", "continue", "do", "else", "false", "for", "fun", "if", "in", "interface",
                   "is", "null", "object", "package", "return", "super", "this", "throw", "true", "try", "typealias",
                   "typeof", "val", "var", "when", "while"}
SCALAR_KOTLIN = {"string": "String", "int32": "Int", "int64": "Long", "float64": "Double", "bool": "Boolean"}
PRIMITIVE = {"int32": ("I", "jint", "i"), "int64": ("J", "jlong", "j"), "float64": ("D", "jdouble", "d"),
             "bool": ("Z", "jboolean", "z")}
BOXED = {"int32": "java/lang/Integer", "int64": "java/lang/Long", "float64": "java/lang/Double",
         "bool": "java/lang/Boolean"}


def camel(name: str) -> str:
    first, *rest = name.split("_")
    return first + "".join(part[:1].upper() + part[1:] for part in rest)


def pascal(name: str) -> str:
    return "".join(part[:1].upper() + part[1:] for part in name.split("_"))


def kotlin_identifier(name: str) -> str:
    return f"`{name}`" if name in KOTLIN_KEYWORDS else name


def domain(source: str | None) -> str:
    """The contract domain a declaration belongs to: interface/cards/models.py is cards."""
    parts = (source or "").split("/")
    return parts[1] if len(parts) > 2 else parts[-1].removesuffix(".py")


def kdoc(text: str | None, indent: str = "") -> list[str]:
    if not text:
        return []
    lines = text.replace("*/", "* /").splitlines()
    return [f"{indent}/** {lines[0]} */"] if len(lines) == 1 else \
        [f"{indent}/**", *[f"{indent} * {line}".rstrip() for line in lines], f"{indent} */"]


@dataclass(frozen=True)
class Value:
    """How one value crosses JNI: its Kotlin type and how its C form is converted."""

    kind: str  # string, int32, int64, float64, bool, enum, record, list, map, change
    kotlin: str  # the Kotlin type, with ? when it may be null
    descriptor: str  # its JVM descriptor
    nullable: bool  # Kotlin may hold null; the C form is NULL then
    c_pointer: bool = False  # the C form is a pointer to the value, NULL when absent
    c_type: str = ""  # the C type of a record, list, map or enum
    java_class: str = ""  # the JNI class of a record or enum
    item: Value | None = None  # a list's items
    list_values: bool = False  # a map whose values are lists of strings

    @property
    def jni_type(self) -> str:
        if self.kind in PRIMITIVE and not self.nullable:
            return PRIMITIVE[self.kind][1]
        return "jstring" if self.kind == "string" else "jobject"

    @property
    def jvalue(self) -> str:
        """The jvalue member that holds it."""
        return PRIMITIVE[self.kind][2] if self.kind in PRIMITIVE and not self.nullable else "l"

    @property
    def is_object(self) -> bool:
        return self.jvalue == "l"


@dataclass(frozen=True)
class FieldPlan:
    name: str  # the C and contract name
    kotlin_name: str  # the Kotlin property, also its JVM field
    value: Value
    default: str | None  # its Kotlin default
    doc: str | None


@dataclass(frozen=True)
class EnumPlan:
    kotlin_name: str
    c_type: str
    doc: str | None
    entries: tuple[tuple[str, str], ...]  # (Kotlin entry, contract value), in C's order from 1


@dataclass(frozen=True)
class UnionPlan:
    """A literal-or-text parameter as a Kotlin sealed interface: an object per literal, a class for the text."""

    kotlin_name: str
    choices: tuple[tuple[str, str], ...]  # (Kotlin object, contract value)
    value_class: str  # the class that holds the text
    doc: str | None


@dataclass(frozen=True)
class RecordPlan:
    """A record's Kotlin declaration."""

    kotlin_name: str
    type_parameters: tuple[str, ...]
    doc: str | None
    fields: tuple[FieldPlan, ...]


@dataclass(frozen=True)
class ConversionPlan:
    """A record instance's conversions between its C struct and its Kotlin object."""

    c_type: str
    java_class: str
    constructor: str  # the Kotlin primary constructor's JVM descriptor
    fields: tuple[FieldPlan, ...]
    to_java: bool
    from_java: bool


@dataclass(frozen=True)
class ParameterPlan:
    name: str
    kotlin_name: str
    value: Value
    default: str | None
    shape: str  # string, scalar, scalar_pointer, enum, union, record
    c_type: str = ""
    choices: tuple[tuple[str, str], ...] = ()  # a union's (JNI class of the choice, C kind symbol)
    union_value_class: str | None = None  # the JNI class of a union's text
    union_value_field: str | None = None
    union_value_kind: str | None = None
    none_symbol: str | None = None  # a nullable enum's or union's zero


@dataclass(frozen=True)
class OperationPlan:
    name: str
    kotlin_name: str
    jni_name: str
    doc: str | None
    parameters: tuple[ParameterPlan, ...]
    result: Value | None
    result_c_type: str | None
    errors: tuple[str, ...]
    # How it uses a context, as in the IR: "required", "none", "opens" or "closes".
    context: str = "required"


@dataclass(frozen=True)
class ErrorPlan:
    kotlin_name: str
    code: int
    doc: str | None


@dataclass
class DomainPlan:
    name: str
    kotlin_name: str
    constants: list[ConstantView] = field(default_factory=list)
    enums: list[EnumPlan] = field(default_factory=list)
    unions: list[UnionPlan] = field(default_factory=list)
    records: list[RecordPlan] = field(default_factory=list)
    errors: list[ErrorPlan] = field(default_factory=list)
    operations: list[OperationPlan] = field(default_factory=list)


def _kotlin_default(value: Value, default: Any) -> str:
    if default is None:
        return "null"
    if value.kind == "enum":
        return f"{value.kotlin.removesuffix('?')}.{literal_member(default)}"
    if value.kind == "string":
        return json.dumps(default, ensure_ascii=False).replace("$", "\\$")
    if value.kind == "bool":
        return "true" if default else "false"
    if value.kind == "float64":
        return repr(float(default))
    if value.kind == "int64":
        return f"{int(default)}L"
    if value.kind == "int32":
        return str(int(default))
    raise Unsupported(f"a default for a {value.kind}")


class AndroidClient:
    def __init__(self, contract: ContractView):
        self.contract = contract
        self.types = CoreTypes(contract)
        self.enums: dict[str, EnumPlan] = {}
        self.enum_domains: dict[str, str] = {}
        self.unions: dict[str, tuple[str, UnionPlan]] = {}

    # ---- values --------------------------------------------------------------------------------

    def record_class(self, instance: Instance) -> tuple[str, str]:
        """An instance's Kotlin type and its JNI class."""
        arguments = f"<{', '.join(instance.arguments)}>" if instance.arguments else ""
        return instance.record + arguments, f"{CLASS_PATH}/{instance.record}"

    def item_value(self, ref: Any, where: str) -> Value:
        """A list item: never null, so numbers are boxed."""
        ref = self.types.resolve(ref).type
        if ref.kind == "string":
            return Value("string", "String", "Ljava/lang/String;", False)
        if ref.kind in BOXED:
            return Value(ref.kind, SCALAR_KOTLIN[ref.kind], f"L{BOXED[ref.kind]};", True)
        if isinstance(ref, NamedType):
            instance = self.types.instance(ref, {})
            kotlin, java_class = self.record_class(instance)
            return Value("record", kotlin, f"L{java_class};", False, c_type=instance.c_name, java_class=java_class)
        raise Unsupported(f"{where}: lists of {ref.kind} in the Android client")

    def slot_value(self, slot: Slot, where: str) -> Value:
        """A record field's value."""
        t = slot.type
        if slot.change:
            return Value("change", "Change<String>", f"L{CLASS_PATH}/Change;", False)
        held = slot.nullable or (slot.optional and not slot.has_default)
        pointer = slot.nullable or slot.optional  # the C struct's pointer fields
        mark = "?" if held else ""
        if isinstance(t, LiteralType):
            enum = self.field_enum(slot)
            return Value("enum", enum.kotlin_name + mark, f"L{CLASS_PATH}/{enum.kotlin_name};", held,
                         c_type=enum.c_type, java_class=f"{CLASS_PATH}/{enum.kotlin_name}")
        if t.kind == "string":
            return Value("string", "String" + mark, "Ljava/lang/String;", held)
        if t.kind in PRIMITIVE:
            if held:
                return Value(t.kind, SCALAR_KOTLIN[t.kind] + "?", f"L{BOXED[t.kind]};", True, c_pointer=True)
            # A defaulted number: Kotlin holds the default, which C takes by pointer.
            return Value(t.kind, SCALAR_KOTLIN[t.kind], PRIMITIVE[t.kind][0], False, c_pointer=pointer)
        if isinstance(t, ListType):
            item = self.item_value(t.items, where)
            return Value("list", f"List<{item.kotlin}>" + mark, "Ljava/util/List;", held, c_pointer=pointer,
                         c_type=list_c_name(self.types, t.items), item=item)
        if isinstance(t, MapType):
            list_values = self.types.kind(t.values) == "list"
            kotlin = "Map<String, List<String>>" if list_values else "Map<String, String>"
            return Value("map", kotlin + mark, "Ljava/util/Map;", held, c_pointer=pointer,
                         c_type=map_c_name(slot.alias), list_values=list_values)
        if isinstance(t, NamedType):
            instance = self.types.instance(t, {})
            kotlin, java_class = self.record_class(instance)
            return Value("record", kotlin + mark, f"L{java_class};", held, c_pointer=pointer,
                         c_type=instance.c_name, java_class=java_class)
        raise Unsupported(f"{where}: {t.kind} in the Android client")

    def field_enum(self, slot: Slot) -> EnumPlan:
        if slot.enum not in self.enums:
            self.enums[slot.enum] = EnumPlan(slot.enum, c_enum_name(slot), slot.doc,
                                             tuple((literal_member(v), v) for v in slot.type.values))
        return self.enums[slot.enum]

    # ---- records -------------------------------------------------------------------------------

    def field_plan(self, slot: Slot, record: str) -> FieldPlan:
        value = self.slot_value(slot, f"{record}.{slot.name}")
        default = None
        if value.kind == "change":
            default = "Change.Keep"
        elif slot.has_default:
            default = _kotlin_default(value, slot.default)
        elif value.nullable:
            default = "null"
        return FieldPlan(slot.name, kotlin_identifier(camel(slot.name)), value, default, slot.doc)

    def record_plan(self, name: str) -> RecordPlan:
        """A record's Kotlin data class: its own fields after its base's, as C flattens them."""
        record = self.types.records[name]
        parameters = tuple(record.type_parameters)
        slots = self.types.slots(Instance(name, parameters))
        fields = []
        declared = {f.name: f for r in self._chain(name) for f in r.fields}
        for slot in slots:
            if isinstance(declared[slot.name].type, ParameterType):
                fields.append(FieldPlan(slot.name, kotlin_identifier(camel(slot.name)),
                                        Value("record", slot.type.name, "Ljava/lang/Object;", False),
                                        None, slot.doc))
            elif parameters and self._generic_list(declared[slot.name]):
                parameter = declared[slot.name].type.items.name
                fields.append(FieldPlan(slot.name, kotlin_identifier(camel(slot.name)),
                                        Value("list", f"List<{parameter}>", "Ljava/util/List;", False),
                                        None, slot.doc))
            else:
                fields.append(self.field_plan(slot, name))
        return RecordPlan(name, parameters, record.doc, tuple(fields))

    def _chain(self, name: str) -> list:
        chain, current = [], self.types.records[name]
        while current:
            chain.insert(0, current)
            current = self.types.records[current.base] if current.base else None
        return chain

    @staticmethod
    def _generic_list(declared: FieldView) -> bool:
        return isinstance(declared.type, ListType) and isinstance(declared.type.items, ParameterType)

    def conversion_plan(self, instance: Instance, to_java: bool, from_java: bool) -> ConversionPlan:
        record = self.types.records[instance.record]
        declared = {f.name: f for r in self._chain(instance.record) for f in r.fields}
        fields = []
        for slot in self.types.slots(instance):
            plan = self.field_plan(slot, instance.record)
            if record.type_parameters and isinstance(declared[slot.name].type, ParameterType):
                plan = FieldPlan(plan.name, plan.kotlin_name,
                                 Value(plan.value.kind, plan.value.kotlin, "Ljava/lang/Object;", plan.value.nullable,
                                       plan.value.c_pointer, plan.value.c_type, plan.value.java_class),
                                 plan.default, plan.doc)
            fields.append(plan)
        _, java_class = self.record_class(instance)
        constructor = "(" + "".join(f.value.descriptor for f in fields) + ")V"
        return ConversionPlan(instance.c_name, java_class, constructor, tuple(fields), to_java, from_java)

    # ---- operations ----------------------------------------------------------------------------

    def parameter_plan(self, op: OperationView, name: str) -> ParameterPlan:
        try:
            planned = self.types.abi.operation(op.name).parameter(name)
        except CAbiError as error:
            raise Unsupported(str(error)) from error
        parameter = op.parameter(name)
        slot = self.types.parameter_slot(parameter)
        where = f"{op.name}({name})"
        kotlin_name = kotlin_identifier(camel(name))
        has_default = slot.has_default
        ref = planned.ref
        if ref.kind == "string":
            value = Value("string", "String" + ("?" if planned.nullable else ""), "Ljava/lang/String;",
                          planned.nullable)
            default = _kotlin_default(value, slot.default) if has_default else ("null" if planned.nullable else None)
            return ParameterPlan(name, kotlin_name, value, default, "string")
        if ref.kind in PRIMITIVE:
            if planned.nullable:
                value = Value(ref.kind, SCALAR_KOTLIN[ref.kind] + "?", f"L{BOXED[ref.kind]};", True, c_pointer=True)
                return ParameterPlan(name, kotlin_name, value, "null", "scalar_pointer", SCALARS_C[ref.kind])
            value = Value(ref.kind, SCALAR_KOTLIN[ref.kind], PRIMITIVE[ref.kind][0], False)
            default = _kotlin_default(value, slot.default) if has_default else None
            return ParameterPlan(name, kotlin_name, value, default,
                                 "scalar_pointer" if planned.pointer else "scalar", SCALARS_C[ref.kind])
        if isinstance(ref, LiteralType):
            enum_name = pascal(op.name) + pascal(name)
            c_type = planned.c_type
            if enum_name not in self.enums:
                self.enums[enum_name] = EnumPlan(enum_name, c_type, None,
                                                 tuple((literal_member(c.value), str(c.value)) for c in planned.choices))
                self.enum_domains[enum_name] = domain(op.source)
            value = Value("enum", enum_name + ("?" if planned.nullable else ""), f"L{CLASS_PATH}/{enum_name};",
                          planned.nullable, c_type=c_type, java_class=f"{CLASS_PATH}/{enum_name}")
            default = "null" if planned.nullable else (_kotlin_default(value, slot.default) if has_default else None)
            return ParameterPlan(name, kotlin_name, value, default, "enum", c_type,
                                 none_symbol=f"{c_type.upper()}_NONE" if planned.nullable else None)
        if isinstance(ref, UnionType):
            if not all(isinstance(c.value, str) for c in planned.choices):
                raise Unsupported(f"{where}: a union of integer literals")
            union_name = pascal(op.name) + pascal(name)
            value_class = pascal(planned.union_value_field)
            union = UnionPlan(union_name, tuple((pascal(str(c.value)), str(c.value)) for c in planned.choices),
                              value_class, slot.doc)
            self.unions[union_name] = (domain(op.source), union)
            value = Value("union", union_name + "?", f"L{CLASS_PATH}/{union_name};", True)
            choices = tuple((f"{CLASS_PATH}/{union_name}${pascal(str(c.value))}", c.symbol) for c in planned.choices)
            return ParameterPlan(name, kotlin_name, value, "null", "union", planned.c_type, choices,
                                 union_value_class=f"{CLASS_PATH}/{union_name}${value_class}",
                                 union_value_field=planned.union_value_field,
                                 union_value_kind=planned.union_value_kind,
                                 none_symbol=f"{planned.c_type.upper()}_NONE")
        if isinstance(ref, NamedType):
            instance = self.types.instance(ref, {})
            kotlin, java_class = self.record_class(instance)
            value = Value("record", kotlin, f"L{java_class};", False, c_type=instance.c_name, java_class=java_class)
            return ParameterPlan(name, kotlin_name, value, None, "record", instance.c_name)
        raise Unsupported(f"{where}: {ref.kind} parameters in the Android client")

    def operation_plan(self, op: OperationView) -> OperationPlan:
        object_name = pascal(domain(op.source))
        kotlin_name = camel(op.name)
        parameters = tuple(self.parameter_plan(op, p.name) for p in op.parameters)
        try:
            result = self.types.abi.operation(op.name).result
        except CAbiError as error:
            raise Unsupported(str(error)) from error
        value = None
        if result is not None:
            returns = self.types.resolve(op.returns).type
            if isinstance(returns, ListType):
                item = self.item_value(returns.items, op.name)
                value = Value("list", f"List<{item.kotlin}>", "Ljava/util/List;", False,
                              c_type=result.type_name, item=item)
            elif isinstance(returns, NamedType):
                instance = self.types.instance(returns, {})
                kotlin, java_class = self.record_class(instance)
                value = Value("record", kotlin, f"L{java_class};", False, c_type=instance.c_name,
                              java_class=java_class)
            else:
                raise Unsupported(f"{op.name}: {returns.kind} results in the Android client")
        doc = "\n".join(x for x in [op.summary, op.description] if x) or None
        return OperationPlan(op.name, kotlin_name, f"Java_{CLASS_PATH.replace('/', '_')}_{object_name}_{kotlin_name}",
                             doc, parameters, value, result.type_name if result else None, tuple(op.errors),
                             op.context)

    # ---- the client ----------------------------------------------------------------------------

    def plan(self) -> tuple[dict[str, DomainPlan], list[ConversionPlan], list[EnumPlan], list[OperationPlan]]:
        domains: dict[str, DomainPlan] = {}

        def of(name: str) -> DomainPlan:
            return domains.setdefault(name, DomainPlan(name, pascal(name)))

        operations = []
        for op in self.types.operations:
            plan = self.operation_plan(op)
            operations.append(plan)
            of(domain(op.source)).operations.append(plan)
        outputs, inputs = set(self.types.outputs), set(self.types.inputs)
        conversions = [self.conversion_plan(i, i in outputs, i in inputs)
                       for i in unique(self.types.outputs + self.types.inputs)]
        for instance in unique(self.types.outputs + self.types.inputs):
            for slot in self.types.slots(instance):
                if isinstance(slot.type, LiteralType):
                    self.enum_domains.setdefault(slot.enum, domain(self.types.records[instance.record].source))
        # A base record's fields are flattened into the records that extend it, as in C, so only
        # records the operations use get a Kotlin class.
        for name in dict.fromkeys(instance.record for instance in unique(self.types.outputs + self.types.inputs)):
            of(domain(self.types.records[name].source)).records.append(self.record_plan(name))
        for constant in self.contract.constants:
            of(domain(constant.source)).constants.append(constant)
        for error in self.contract.errors:
            of(domain(error.source)).errors.append(ErrorPlan(error.name + "Exception", error.code, error.doc))
        for enum in self.enums.values():
            of(self.enum_domains[enum.kotlin_name]).enums.append(enum)
        for name, union in self.unions.values():
            of(name).unions.append(union)
        return domains, conversions, list(self.enums.values()), operations


def generate_android_client(contract: ContractView) -> dict[str, str]:
    client = AndroidClient(contract)
    domains, conversions, enums, operations = client.plan()
    header = "// Generated from contract/interface by holder_contract. Do not edit; run make -C contract update.\n"
    files = {}
    for plan in domains.values():
        files[f"{KOTLIN_DIR}/{plan.kotlin_name}.kt"] = header + TEMPLATES.get_template("client_domain.kt.j2").render(
            package=PACKAGE, domain=plan, kdoc=kdoc)
    files[f"{KOTLIN_DIR}/Client.kt"] = header + TEMPLATES.get_template("client_support.kt.j2").render(
        package=PACKAGE, base_exceptions=BASE_EXCEPTIONS)
    errors = [(error.code, error.name + "Exception") for error in contract.errors]
    files[JNI_PATH] = header + TEMPLATES.get_template("client.cpp.j2").render(
        class_path=CLASS_PATH, conversions=conversions, enums=enums, operations=operations,
        errors=sorted([*errors, *BASE_EXCEPTIONS.items()]), has_changes=client.types.changes(),
        output_enums={f.value.c_type for c in conversions if c.to_java for f in c.fields if f.value.kind == "enum"})
    return files
