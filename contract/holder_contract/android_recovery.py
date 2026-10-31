"""Android adapters for project recovery token export and import."""

from __future__ import annotations

from .android import _field_type, _record_jni_lines, _record_kotlin
from .android_templates import TEMPLATES
from .ir import ContractView, LiteralType, NamedType


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
    global_import = contract.operation("import_recovery_token_global")
    if [parameter.name for parameter in global_import.parameters] != ["request"] or \
            global_import.parameter("request").type != NamedType(name="RecoveryTokenImportRequest") or \
            global_import.returns != NamedType(name="RecoveryTokenGlobalImportResult"):
        raise ValueError("Android import_recovery_token_global parameters or return need a generator update")
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
    global_result = contract.record("RecoveryTokenGlobalImportResult")
    pull_status = global_result.field("pull_status").type
    if not isinstance(pull_status, LiteralType) or not all(isinstance(value, str) for value in pull_status.values):
        raise ValueError("Android global recovery pull status needs string literals")
    global_fields = [(field.name, *_field_type(field, contract, allow_literal=field.name == "pull_status"))
                     for field in contract.record("ProjectRecoveryTokenImportResult").fields + global_result.fields]
    record = (_record_kotlin("ContractProjectRecoveryTokenExportResult", fields) + "\n" +
              _record_kotlin("ContractRecoveryTokenGlobalImportResult", global_fields))
    descriptor = "(" + "".join(signature for _, _, signature, _ in fields) + ")V"
    lines, arguments, refs = _record_jni_lines(fields, "value", indent="  ")
    global_descriptor = "(" + "".join(signature for _, _, signature, _ in global_fields) + ")V"
    global_lines, global_arguments, global_refs = _record_jni_lines(
        global_fields, "value", {"pull_status": "status_name"}, indent="  ")
    jni = TEMPLATES.get_template("android_recovery.cpp.j2").render(
        descriptor=descriptor, record_lines="".join(lines), arguments=", ".join(arguments),
        release_refs="".join(f"  if ({ref}) env->DeleteLocalRef({ref});\n" for ref in refs),
        global_descriptor=global_descriptor, global_record_lines="".join(global_lines),
        global_arguments=", ".join(global_arguments),
        global_release_refs="".join(f"  if ({ref}) env->DeleteLocalRef({ref});\n" for ref in global_refs),
        status_cases="".join(
            f'    case HOLDER_RECOVERY_TOKEN_GLOBAL_IMPORT_RESULT_PULL_STATUS_{value.upper()}: '
            f'status_name = "{value}"; break;\n' for value in pull_status.values),
    )
    return record, jni
