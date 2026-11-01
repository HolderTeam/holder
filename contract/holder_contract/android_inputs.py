"""Generate Kotlin request records and their C ABI field conversions from contract records."""

from __future__ import annotations

import json
from dataclasses import dataclass

from generators.core.abi import snake

from .android import _camel, _field_type
from .android_ir import ContractView, LiteralType
from .android_templates import TEMPLATES


@dataclass(frozen=True)
class InputField:
    name: str
    kotlin_name: str
    kotlin_type: str
    signature: str
    optional: bool
    default: str | None
    kind: str
    enum_cases: tuple[tuple[str, str], ...] = ()


def generate_input_record(contract: ContractView, record_name: str) -> tuple[str, list[InputField]]:
    """Return a Kotlin data class and the field metadata for its JNI request converter."""
    fields = []
    for field in contract.record(record_name).fields:
        resolved, _ = contract.resolve_type(field.type)
        kotlin_type, signature, optional = _field_type(field, contract, allow_literal=True)
        if resolved.kind not in {"string", "literal", "int64"}:
            raise ValueError(f"Android input conversion does not support {resolved.kind} for {record_name}.{field.name}")
        enum_cases: tuple[tuple[str, str], ...] = ()
        if isinstance(resolved, LiteralType):
            if not all(isinstance(value, str) for value in resolved.values):
                raise ValueError(f"Android input enum needs string values for {record_name}.{field.name}")
            prefix = f"HOLDER_{snake(record_name).upper()}_{field.name.upper()}"
            enum_cases = tuple((value, f"{prefix}_{value.upper()}") for value in resolved.values)
        if "default" in field.model_fields_set:
            default = json.dumps(field.default)
        elif field.omittable:
            default = "null"
        else:
            default = None
        fields.append(InputField(field.name, _camel(field.name), kotlin_type, signature,
                                 optional, default, resolved.kind, enum_cases))
    kotlin = TEMPLATES.get_template("android_input_record.kt.j2").render(
        name=f"Contract{record_name}", fields=fields)
    return kotlin, fields
