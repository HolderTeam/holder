"""Android card reference resolution through the contract C API."""

from __future__ import annotations

from .android import _field_type, _record_jni_lines, _record_kotlin
from .android_templates import TEMPLATES
from .ir import ContractView, ListType, LiteralType, NamedType, ScalarType


def generate_references(contract: ContractView) -> tuple[str, str, str]:
    operation = contract.operation("resolve_card_reference")
    if [parameter.name for parameter in operation.parameters] != ["request"] or \
            operation.parameter("request").type != NamedType(name="CardReferenceResolveRequest") or \
            operation.returns != NamedType(name="CardReferenceResolveData"):
        raise ValueError("Android resolve_card_reference needs its request and result records")
    request = contract.record("CardReferenceResolveRequest")
    if [field.name for field in request.fields] != ["project_id", "reference", "scope"] or \
            not all(_field_type(field, contract) == ("String", "Ljava/lang/String;", False)
                    for field in request.fields[:2]):
        raise ValueError("Android card reference request fields need a generator update")
    scopes = request.field("scope").type
    if not isinstance(scopes, LiteralType) or scopes.values != ["live", "trashed", "either"]:
        raise ValueError("Android card reference scopes need a generator update")
    result = contract.record("CardReferenceResolveData")
    if [field.name for field in result.fields] != ["status", "match_kind", "card", "candidates"] or \
            result.field("match_kind").type != ScalarType(kind="string") or \
            result.field("card").type != NamedType(name="CardReferenceCandidate") or \
            result.field("candidates").type != ListType(kind="list", items=NamedType(name="CardReferenceCandidate")) or \
            not all(field.omittable for field in result.fields[1:]):
        raise ValueError("Android card reference result fields need a generator update")
    statuses = result.field("status").type
    if not isinstance(statuses, LiteralType) or statuses.values != ["resolved", "ambiguous", "not_found"]:
        raise ValueError("Android card reference statuses need a generator update")
    candidate = contract.record("CardReferenceCandidate")
    fields = [(field.name, *_field_type(field, contract)) for field in candidate.fields]
    record = _record_kotlin("ContractCardReferenceCandidate", fields)
    record += ("\ndata class ContractCardReferenceResult(\n"
               "    val status: String,\n    val matchKind: String?,\n"
               "    val card: ContractCardReferenceCandidate?,\n"
               "    val candidates: List<ContractCardReferenceCandidate>?,\n)\n")
    descriptor = "(" + "".join(signature for _, _, signature, _ in fields) + ")V"
    lines, arguments, refs = _record_jni_lines(fields, "value", indent="  ")
    jni = TEMPLATES.get_template("android_references.cpp.j2").render(
        descriptor=descriptor, record_lines="".join(lines), arguments=", ".join(arguments),
        release_refs="".join(f"  if ({ref}) env->DeleteLocalRef({ref});\n" for ref in refs),
        scope_cases="".join(
            f"    case {index}: request.scope = HOLDER_CARD_REFERENCE_RESOLVE_REQUEST_SCOPE_{value.upper()}; break;\n"
            for index, value in enumerate(scopes.values)),
        status_cases="".join(
            f'    case HOLDER_CARD_REFERENCE_RESOLVE_DATA_STATUS_{value.upper()}: status_name = "{value}"; break;\n'
            for value in statuses.values),
    )
    method = ("    external fun nativeResolveCardReference(contextHandle: Long, projectId: String, "
              "reference: String, scope: Int): ContractCardReferenceResult\n")
    return record, method, jni
