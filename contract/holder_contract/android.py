"""The first Android contract slice: a typed, paged card listing over the C ABI."""

# ruff: noqa: ISC004

from __future__ import annotations

from typing import Any

KOTLIN_PATH = "apps/mobile/android/app/src/main/java/team/holder/android/generated/ContractCards.kt"
JNI_PATH = "apps/mobile/android/app/src/main/cpp/generated/contract_cards.cpp"


def _field_type(field: dict[str, Any], aliases: dict[str, dict[str, Any]]) -> tuple[str, str, bool]:
    value = field["type"]
    optional = field.get("omittable", False) or value["kind"] == "nullable"
    if value["kind"] == "nullable":
        value = value["value"]
    while value["kind"] == "named":
        value = aliases[value["name"]]
    kind = value["kind"]
    if kind not in {"string", "int32", "int64", "float64"}:
        raise ValueError(f"Android card listing does not support {kind} for {field['name']}")
    return ({"string": "String", "int32": "Int", "int64": "Long", "float64": "Double"}[kind],
            {"string": "Ljava/lang/String;", "int32": "Ljava/lang/Integer;", "int64": "Ljava/lang/Long;",
             "float64": "Ljava/lang/Double;"}[kind] if optional else
            {"string": "Ljava/lang/String;", "int32": "I", "int64": "J", "float64": "D"}[kind],
            optional)


def _camel(name: str) -> str:
    first, *rest = name.split("_")
    return first + "".join(word.title() for word in rest)


def generate_android(ir: dict[str, Any]) -> dict[str, str]:
    records = {record["name"]: record for record in ir["records"]}
    aliases = {alias["name"]: alias["type"] for alias in ir["aliases"]}
    operation = next(op for op in ir["operations"] if op["name"] == "list_cards")
    if [parameter["name"] for parameter in operation["parameters"]] != [
        "project_id", "parent", "tag", "order", "limit", "cursor", "include_content", "include_child_count"
    ] or operation["parameters"][4]["type"].get("le") != 5000:
        raise ValueError("Android list_cards parameters need a generator update")
    if operation["returns"] != {"kind": "named", "name": "Page", "arguments":
                                [{"kind": "named", "name": "CardListItem"}]}:
        raise ValueError("Android list_cards needs Page[CardListItem]")
    fields = records["CardMetadata"]["fields"] + records["CardListItem"]["fields"]
    typed = [(field["name"], *_field_type(field, aliases)) for field in fields]
    header = "// Generated from contract/interface/cards.py and contract/interface/app.py by holder_contract.\n" \
             "// Do not edit; run make -C contract update.\n"
    kotlin = header + "package team.holder.android.generated\n\n" + \
        "data class ContractCardListItem(\n" + "".join(
            f"    val {_camel(name)}: {kind}{'?' if optional else ''},\n"
            for name, kind, _, optional in typed) + \
        ")\n\ndata class ContractCardPage(\n    val items: List<ContractCardListItem>,\n" \
        "    val nextCursor: String?,\n)\n\n" \
        "object ContractCards {\n" \
        "    external fun nativeListCardsPage(contextHandle: Long, projectId: String, cursor: String?): ContractCardPage\n" \
        "}\n"
    descriptor = "(" + "".join(signature for _, _, signature, _ in typed) + ")V"
    lines = [header, '#include "holder/holder.h"\n#include <jni.h>\n#include <cstdint>\n'
             '#include <string>\n#include <vector>\n\nnamespace {\n',
             'struct Arena {\n  holder_arena* value = holder_arena_new();\n'
             '  ~Arena() { holder_arena_free(value); }\n};\n\n',
             'void fail(JNIEnv* env, const char* message) {\n'
             '  jclass type = env->FindClass("java/lang/RuntimeException");\n'
             '  if (type) env->ThrowNew(type, message);\n}\n\n',
             '// NewStringUTF expects modified UTF-8; the contract uses ordinary UTF-8.\n'
             'jstring string(JNIEnv* env, const char* utf8) {\n'
             '  if (!utf8) return nullptr;\n'
             '  std::u16string out;\n'
             '  const auto* p = reinterpret_cast<const unsigned char*>(utf8);\n'
             '  while (*p) {\n'
             '    uint32_t cp = *p++;\n'
             '    if (cp >= 0xf0) { cp = (cp & 7) << 18; cp |= (*p++ & 63) << 12;'
             ' cp |= (*p++ & 63) << 6; cp |= *p++ & 63; }\n'
             '    else if (cp >= 0xe0) { cp = (cp & 15) << 12; cp |= (*p++ & 63) << 6; cp |= *p++ & 63; }\n'
             '    else if (cp >= 0xc0) { cp = (cp & 31) << 6; cp |= *p++ & 63; }\n'
             '    if (cp <= 0xffff) out.push_back(static_cast<char16_t>(cp));\n'
             '    else { cp -= 0x10000; out.push_back(static_cast<char16_t>(0xd800 + (cp >> 10)));'
             ' out.push_back(static_cast<char16_t>(0xdc00 + (cp & 1023))); }\n'
             '  }\n'
             '  return env->NewString(reinterpret_cast<const jchar*>(out.data()), static_cast<jsize>(out.size()));\n'
             '}\n\n',
             'jobject boxed_long(JNIEnv* env, int64_t value) {\n'
             '  jclass type = env->FindClass("java/lang/Long");\n'
             '  jmethodID ctor = env->GetMethodID(type, "<init>", "(J)V");\n'
             '  jobject result = env->NewObject(type, ctor, static_cast<jlong>(value));\n'
             '  env->DeleteLocalRef(type);\n  return result;\n}\n\n',
             'jobject boxed_int(JNIEnv* env, int32_t value) {\n'
             '  jclass type = env->FindClass("java/lang/Integer");\n'
             '  jmethodID ctor = env->GetMethodID(type, "<init>", "(I)V");\n'
             '  jobject result = env->NewObject(type, ctor, static_cast<jint>(value));\n'
             '  env->DeleteLocalRef(type);\n  return result;\n}\n\n',
             ('jobject boxed_double(JNIEnv* env, double value) {\n'
             '  jclass type = env->FindClass("java/lang/Double");\n'
             '  jmethodID ctor = env->GetMethodID(type, "<init>", "(D)V");\n'
             '  jobject result = env->NewObject(type, ctor, static_cast<jdouble>(value));\n'
             '  env->DeleteLocalRef(type);\n  return result;\n}\n\n'
             if any(kind == "Double" and optional for _, kind, _, optional in typed) else ''),
             '}  // namespace\n\n',
             'extern "C" JNIEXPORT jobject JNICALL\n'
             'Java_team_holder_android_generated_ContractCards_nativeListCardsPage(\n'
             '    JNIEnv* env, jobject, jlong context_handle, jstring project_id, jstring cursor) {\n'
             '  if (!context_handle || !project_id) { fail(env, "context and project ID are required"); return nullptr; }\n'
             '  const char* project = env->GetStringUTFChars(project_id, nullptr);\n'
             '  const char* position = cursor ? env->GetStringUTFChars(cursor, nullptr) : nullptr;\n'
             '  if (!project || (cursor && !position)) {\n'
             '    if (project) env->ReleaseStringUTFChars(project_id, project);\n'
             '    return nullptr;\n  }\n'
             '  Arena arena;\n  const holder_card_list_item_page* page = nullptr;\n'
             '  holder_error* error = nullptr;\n  const int32_t limit = 5000;\n'
             '  int status = arena.value ? holder_list_cards(\n'
             '      reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)), project,\n'
             '      {}, nullptr, HOLDER_LIST_CARDS_ORDER_SORT_KEY_ASC, &limit, position, false, false,\n'
             '      arena.value, &page, &error) : HOLDER_ERROR_ALLOCATION;\n'
             '  if (position) env->ReleaseStringUTFChars(cursor, position);\n'
             '  env->ReleaseStringUTFChars(project_id, project);\n'
             '  if (status != HOLDER_OK) {\n'
             '    std::string message = error ? holder_error_message(error) : "cannot allocate card page";\n'
             '    if (error) holder_error_destroy(error);\n'
             '    fail(env, message.c_str()); return nullptr;\n  }\n'
             '  jclass item_type = env->FindClass("team/holder/android/generated/ContractCardListItem");\n'
             f'  jmethodID item_ctor = env->GetMethodID(item_type, "<init>", "{descriptor}");\n'
             '  jclass list_type = env->FindClass("java/util/ArrayList");\n'
             '  jmethodID list_ctor = env->GetMethodID(list_type, "<init>", "(I)V");\n'
             '  jmethodID add = env->GetMethodID(list_type, "add", "(Ljava/lang/Object;)Z");\n'
             '  jobject items = env->NewObject(list_type, list_ctor, static_cast<jint>(page->items.count));\n'
             '  if (env->ExceptionCheck()) return nullptr;\n'
             '  for (size_t i = 0; i < page->items.count; ++i) {\n'
             '    const auto& card = page->items.items[i];\n']
    arguments = []
    refs = []
    for name, kind, _, optional in typed:
        var = f"v_{name}"
        if kind == "String":
            lines.append(f"    jstring {var} = string(env, card.{name});\n")
            refs.append(var)
        elif optional:
            box = {"Long": "boxed_long", "Int": "boxed_int", "Double": "boxed_double"}[kind]
            lines.append(f"    jobject {var} = card.{name} ? {box}(env, *card.{name}) : nullptr;\n")
            refs.append(var)
        else:
            cast = {"Int": "jint", "Long": "jlong", "Double": "jdouble"}[kind]
            lines.append(f"    {cast} {var} = static_cast<{cast}>(card.{name});\n")
        arguments.append(var)
    lines += [f"    jobject item = env->NewObject(item_type, item_ctor, {', '.join(arguments)});\n",
              *[f"    if ({var}) env->DeleteLocalRef({var});\n" for var in refs],
              '    if (env->ExceptionCheck()) return nullptr;\n'
              '    env->CallBooleanMethod(items, add, item);\n'
              '    env->DeleteLocalRef(item);\n'
              '    if (env->ExceptionCheck()) return nullptr;\n'
              '  }\n'
              '  jclass page_type = env->FindClass("team/holder/android/generated/ContractCardPage");\n'
              '  jmethodID page_ctor = env->GetMethodID(page_type, "<init>", "(Ljava/util/List;Ljava/lang/String;)V");\n'
              '  jstring next = string(env, page->next_cursor);\n'
              '  return env->NewObject(page_type, page_ctor, items, next);\n}\n']
    return {KOTLIN_PATH: kotlin, JNI_PATH: "".join(lines)}
