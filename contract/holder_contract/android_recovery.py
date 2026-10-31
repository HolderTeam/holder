"""Android adapters for project recovery token export and import."""

from __future__ import annotations

from .android import _field_type, _record_jni_lines, _record_kotlin
from .android_ir import ContractView, NamedType
from .android_templates import TEMPLATES


def generate_recovery(contract: ContractView) -> tuple[str, str]:
    for name, request, result in (
        ("export_project_recovery_token", "RecoveryTokenExportRequest", "ProjectRecoveryTokenExportResult"),
        ("import_project_recovery_token", "RecoveryTokenImportRequest", "ProjectRecoveryTokenImportResult"),
    ):
        operation = contract.operation(name)
        if [parameter.name for parameter in operation.parameters] != ["project_id", "request"] or \
                operation.parameter("request").type != NamedType(name=request) or \
                operation.returns != NamedType(name=result):
            raise ValueError(f"Android {name} parameters or return need a generator update")
    export_request = contract.record("RecoveryTokenExportRequest")
    import_request = contract.record("RecoveryTokenImportRequest")
    if [field.name for field in export_request.fields] != ["pin"] or \
            [field.name for field in import_request.fields] != ["recovery_token"]:
        raise ValueError("Android recovery token request fields need a generator update")
    for field in (*export_request.fields, *import_request.fields):
        if _field_type(field, contract) != ("String", "Ljava/lang/String;", False):
            raise ValueError("Android recovery token requests need required strings")
    result = contract.record("ProjectRecoveryTokenExportResult")
    fields = [(field.name, *_field_type(field, contract)) for field in result.fields]
    record = _record_kotlin("ContractProjectRecoveryTokenExportResult", fields)
    descriptor = "(" + "".join(signature for _, _, signature, _ in fields) + ")V"
    lines, arguments, refs = _record_jni_lines(fields, "value", indent="  ")
    jni = TEMPLATES.get_template("android_recovery.cpp.j2").render(
        descriptor=descriptor, record_lines="".join(lines), arguments=", ".join(arguments),
        release_refs="".join(f"  if ({ref}) env->DeleteLocalRef({ref});\n" for ref in refs),
    )
    return record, jni
