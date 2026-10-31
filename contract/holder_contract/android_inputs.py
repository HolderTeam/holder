"""Generate Kotlin request records and their C ABI field conversions from contract records."""

from __future__ import annotations

from .android_models import KotlinField as InputField, map_record
from .ir import ContractView


def generate_input_record(contract: ContractView, record_name: str) -> tuple[str, list[InputField]]:
    """Return a Kotlin data class and the field metadata for its JNI request converter."""
    rec = map_record(contract.record(record_name), contract, allow_literal_fields={"privacy_mode"})
    for field in rec.fields:
        if field.kind not in {"string", "literal", "int64"}:
            raise ValueError(f"Android input conversion does not support {field.kind} for {record_name}.{field.name}")
    return rec.render(include_defaults=True), list(rec.fields)
