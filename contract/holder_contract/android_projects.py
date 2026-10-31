"""Generate Android's typed project listing from the contract and its C ABI."""

from __future__ import annotations

from typing import Any

from .android import PROJECT_JNI_PATH, PROJECT_KOTLIN_PATH, _field_type, _record_jni_lines, _record_kotlin


def generate_projects(ir: dict[str, Any], records: dict[str, dict[str, Any]],
                      aliases: dict[str, dict[str, Any]]) -> dict[str, str]:
    operation = next(op for op in ir["operations"] if op["name"] == "list_projects")
    if [p["name"] for p in operation["parameters"]] != [
        "name", "updated_after", "updated_before", "order", "limit", "offset", "include_card_counts"
    ] or operation["parameters"][4]["type"].get("le") != 1000 or \
            "updated_at_desc" not in operation["parameters"][3]["type"]["value"]["values"]:
        raise ValueError("Android list_projects parameters need a generator update")
    if operation["returns"] != {"kind": "list", "items": {"kind": "named", "name": "ProjectListItem"}}:
        raise ValueError("Android list_projects needs list[ProjectListItem]")
    mode = aliases["PrivacyMode"]
    if mode["kind"] != "literal" or not all(isinstance(v, str) for v in mode["values"]):
        raise ValueError("Android project privacy mode needs string literals")
    fields = records["Project"]["fields"] + records["ProjectListItem"]["fields"]
    typed = [(field["name"], *_field_type(field, aliases, allow_literal=field["name"] == "privacy_mode"))
             for field in fields]
    header = "// Generated from contract/interface/projects.py and contract/interface/app.py by holder_contract.\n" \
             "// Do not edit; run make -C contract update.\n"
    kotlin = header + "package team.holder.android.generated\n\n" + \
        _record_kotlin("ContractProjectListItem", typed) + \
        "\nobject ContractProjects {\n" \
        "    external fun nativeListProjectsPage(contextHandle: Long, offset: Int): List<ContractProjectListItem>\n" \
        "}\n"
    descriptor = "(" + "".join(signature for _, _, signature, _ in typed) + ")V"
    record_lines, arguments, refs = _record_jni_lines(typed, "project", {"privacy_mode": "mode"})
    mode_lines = "".join(
        f"      case HOLDER_PROJECT_PRIVACY_MODE_{value.upper()}: return \"{value}\";\n"
        for value in mode["values"])
    jni = header + '''#include "holder/holder.h"
#include <jni.h>
#include <cstdint>
#include <string>

namespace {
struct Arena {
  holder_arena* value = holder_arena_new();
  ~Arena() { holder_arena_free(value); }
};

void fail(JNIEnv* env, const char* message) {
  jclass type = env->FindClass("java/lang/RuntimeException");
  if (type) env->ThrowNew(type, message);
}

// JNI strings use UTF-16; the C contract strings use ordinary UTF-8.
jstring string(JNIEnv* env, const char* utf8) {
  if (!utf8) return nullptr;
  std::u16string out;
  const auto* p = reinterpret_cast<const unsigned char*>(utf8);
  while (*p) {
    uint32_t cp = *p++;
    if (cp >= 0xf0) { cp = (cp & 7) << 18; cp |= (*p++ & 63) << 12; cp |= (*p++ & 63) << 6; cp |= *p++ & 63; }
    else if (cp >= 0xe0) { cp = (cp & 15) << 12; cp |= (*p++ & 63) << 6; cp |= *p++ & 63; }
    else if (cp >= 0xc0) { cp = (cp & 31) << 6; cp |= *p++ & 63; }
    if (cp <= 0xffff) out.push_back(static_cast<char16_t>(cp));
    else { cp -= 0x10000; out.push_back(static_cast<char16_t>(0xd800 + (cp >> 10))); out.push_back(static_cast<char16_t>(0xdc00 + (cp & 1023))); }
  }
  return env->NewString(reinterpret_cast<const jchar*>(out.data()), static_cast<jsize>(out.size()));
}

jobject boxed_int(JNIEnv* env, int32_t value) {
  jclass type = env->FindClass("java/lang/Integer");
  jmethodID ctor = env->GetMethodID(type, "<init>", "(I)V");
  jobject result = env->NewObject(type, ctor, static_cast<jint>(value));
  env->DeleteLocalRef(type);
  return result;
}

const char* privacy_mode(holder_project_privacy_mode value) {
  switch (value) {
''' + mode_lines + '''      default: return nullptr;
  }
}
}  // namespace

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_generated_ContractProjects_nativeListProjectsPage(
    JNIEnv* env, jobject, jlong context_handle, jint offset) {
  if (!context_handle || offset < 0) { fail(env, "context and nonnegative offset are required"); return nullptr; }
  Arena arena;
  const holder_project_list_item_list* page = nullptr;
  holder_error* error = nullptr;
  const int32_t limit = 1000;
  int status = arena.value ? holder_list_projects(
      reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      nullptr, nullptr, nullptr, HOLDER_LIST_PROJECTS_ORDER_UPDATED_AT_DESC, &limit, offset, false,
      arena.value, &page, &error) : HOLDER_ERROR_ALLOCATION;
  if (status != HOLDER_OK) {
    std::string message = error ? holder_error_message(error) : "cannot allocate project list";
    if (error) holder_error_destroy(error);
    fail(env, message.c_str()); return nullptr;
  }
  jclass item_type = env->FindClass("team/holder/android/generated/ContractProjectListItem");
''' + f'  jmethodID item_ctor = env->GetMethodID(item_type, "<init>", "{descriptor}");\n' + '''  jclass list_type = env->FindClass("java/util/ArrayList");
  jmethodID list_ctor = env->GetMethodID(list_type, "<init>", "(I)V");
  jmethodID add = env->GetMethodID(list_type, "add", "(Ljava/lang/Object;)Z");
  jobject items = env->NewObject(list_type, list_ctor, static_cast<jint>(page->count));
  if (env->ExceptionCheck()) return nullptr;
  for (size_t i = 0; i < page->count; ++i) {
    const auto& project = page->items[i];
    const char* mode = privacy_mode(project.privacy_mode);
    if (!mode) { fail(env, "unknown project privacy mode"); return nullptr; }
''' + "".join(record_lines) + \
        f"    jobject item = env->NewObject(item_type, item_ctor, {', '.join(arguments)});\n" + \
        "".join(f"    if ({ref}) env->DeleteLocalRef({ref});\n" for ref in refs) + '''    if (env->ExceptionCheck()) return nullptr;
    env->CallBooleanMethod(items, add, item);
    env->DeleteLocalRef(item);
    if (env->ExceptionCheck()) return nullptr;
  }
  return items;
}
'''
    return {PROJECT_KOTLIN_PATH: kotlin, PROJECT_JNI_PATH: jni}
