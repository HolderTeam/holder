"""Typed Android adapters for simple lists of contract records."""

from __future__ import annotations

from .android import _field_type, _record_jni_lines, _record_kotlin
from .android_ir import ContractView, ListType, NamedType, OperationView, RecordView
from .android_templates import TEMPLATES
from .generate import snake


def _list_adapter(operation: OperationView, record: RecordView, contract: ContractView, *,
                  method: str, parameters: list[str], kotlin_parameters: str, extra_parameters: str,
                  required_parameters: str = "", extra_variables: str = "", decode_parameters: str = "",
                  pre_call: str = "", c_arguments: str = "") -> tuple[str, str, str]:
    name = operation.name
    record_name = record.name
    if [parameter.name for parameter in operation.parameters] != parameters or \
            operation.returns != ListType(kind="list", items=NamedType(name=record_name)):
        raise ValueError(f"Android {name} parameters or return need a generator update")
    typed = [(field.name, *_field_type(field, contract)) for field in record.fields]
    kotlin_record = f"Contract{record_name}"
    record_kotlin = _record_kotlin(kotlin_record, typed) + "\n"
    method_kotlin = (f"    external fun {method}(contextHandle: Long, projectId: String{kotlin_parameters}): "
                     f"List<{kotlin_record}>\n")
    descriptor = "(" + "".join(signature for _, _, signature, _ in typed) + ")V"
    record_lines, arguments, refs = _record_jni_lines(typed, "value")
    c_record = f"holder_{snake(record_name)}"
    context = 'reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle))'
    c_call = (f"holder_{name}({context}, project.c_str(), {c_arguments}arena.value, &page, &error)")
    jni = TEMPLATES.get_template("android_record_list.cpp.j2").render(
        method=method, operation=name, extra_parameters=extra_parameters,
        required_parameters=required_parameters, extra_variables=extra_variables,
        decode_parameters=decode_parameters, pre_call=pre_call, c_record=c_record, c_call=c_call,
        kotlin_record=kotlin_record, descriptor=descriptor, record_lines="".join(record_lines),
        arguments=", ".join(arguments),
        release_refs="".join(f"    if ({ref}) env->DeleteLocalRef({ref});\n" for ref in refs),
    )
    return record_kotlin, method_kotlin, jni


def generate_record_lists(contract: ContractView) -> tuple[str, str, str]:
    adapters = [
        _list_adapter(contract.operation("list_project_tags"), contract.record("TagCount"), contract,
                      method="nativeListProjectTags", parameters=["project_id"],
                      kotlin_parameters="", extra_parameters=""),
        _list_adapter(contract.operation("search_cards"), contract.record("SearchCard"), contract,
                      method="nativeSearchCards", parameters=["project_id", "q", "limit", "offset"],
                      kotlin_parameters=", query: String, limit: Int", extra_parameters=", jstring q, jint limit",
                      required_parameters=" || !q", extra_variables=", query",
                      decode_parameters=" || !utf8(env, q, query)",
                      pre_call="  const int32_t count = static_cast<int32_t>(limit);\n",
                      c_arguments="query.c_str(), &count, 0, "),
    ]
    return tuple("".join(part[index] for part in adapters) for index in range(3))
