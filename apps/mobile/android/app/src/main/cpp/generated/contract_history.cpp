// Generated from contract/interface/history/models.py and contract/interface/history/operations.py by holder_contract.
// Do not edit; run make -C contract update.
#include "holder/holder.h"
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

// JNI strings are UTF-16; contract strings are UTF-8.
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

bool utf8(JNIEnv* env, jstring input, std::string& out) {
  const jchar* chars = env->GetStringChars(input, nullptr);
  if (!chars) return false;
  const jsize count = env->GetStringLength(input);
  for (jsize i = 0; i < count; ++i) {
    uint32_t cp = chars[i];
    if (cp >= 0xd800 && cp <= 0xdbff && i + 1 < count &&
        chars[i + 1] >= 0xdc00 && chars[i + 1] <= 0xdfff) {
      cp = 0x10000 + ((cp - 0xd800) << 10) + (chars[++i] - 0xdc00);
    } else if (cp >= 0xd800 && cp <= 0xdfff) {
      env->ReleaseStringChars(input, chars);
      fail(env, "unpaired UTF-16 surrogate"); return false;
    }
    if (cp == 0) {
      env->ReleaseStringChars(input, chars);
      fail(env, "NUL is not allowed in C strings"); return false;
    }
    if (cp < 0x80) out.push_back(static_cast<char>(cp));
    else if (cp < 0x800) {
      out.push_back(static_cast<char>(0xc0 | (cp >> 6)));
      out.push_back(static_cast<char>(0x80 | (cp & 63)));
    } else if (cp < 0x10000) {
      out.push_back(static_cast<char>(0xe0 | (cp >> 12)));
      out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 63)));
      out.push_back(static_cast<char>(0x80 | (cp & 63)));
    } else {
      out.push_back(static_cast<char>(0xf0 | (cp >> 18)));
      out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 63)));
      out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 63)));
      out.push_back(static_cast<char>(0x80 | (cp & 63)));
    }
  }
  env->ReleaseStringChars(input, chars);
  return true;
}

struct JavaTypes {
  JNIEnv* env;
  jclass list = env->FindClass("java/util/ArrayList");
  jmethodID list_ctor = env->GetMethodID(list, "<init>", "(I)V");
  jmethodID add = env->GetMethodID(list, "add", "(Ljava/lang/Object;)Z");
  jclass save = env->FindClass("team/holder/android/generated/ContractCardHistorySave");
  jmethodID save_ctor = env->GetMethodID(save, "<init>", "(Ljava/lang/String;Ljava/util/List;JJLjava/lang/String;)V");
  jclass author = env->FindClass("team/holder/android/generated/ContractHistoryAuthor");
  jmethodID author_ctor = env->GetMethodID(author, "<init>", "(Ljava/lang/String;Ljava/lang/String;)V");
  jclass entry = env->FindClass("team/holder/android/generated/ContractCardHistoryEntry");
  jmethodID entry_ctor = env->GetMethodID(entry, "<init>",
      "(Ljava/lang/String;Ljava/lang/String;Ljava/util/List;Ljava/util/List;Lteam/holder/android/generated/ContractHistoryAuthor;JJLjava/lang/String;Ljava/lang/String;IZLjava/util/List;)V");
  jclass page = env->FindClass("team/holder/android/generated/ContractCardHistoryPage");
  jmethodID page_ctor = env->GetMethodID(page, "<init>", "(Ljava/lang/String;Ljava/util/List;Ljava/lang/String;Z)V");
  jclass version = env->FindClass("team/holder/android/generated/ContractCardHistoryVersion");
  jmethodID version_ctor = env->GetMethodID(version, "<init>", "(ZLjava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
  jclass line = env->FindClass("team/holder/android/generated/ContractCardHistoryDiffLine");
  jmethodID line_ctor = env->GetMethodID(line, "<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/Long;Ljava/lang/Long;)V");
  jclass comparison = env->FindClass("team/holder/android/generated/ContractCardHistoryComparison");
  jmethodID comparison_ctor = env->GetMethodID(comparison, "<init>",
      "(Lteam/holder/android/generated/ContractCardHistoryVersion;Lteam/holder/android/generated/ContractCardHistoryVersion;Ljava/lang/String;Ljava/util/List;Z)V");
  jclass restore = env->FindClass("team/holder/android/generated/ContractCardHistoryRestoreResult");
  jmethodID restore_ctor = env->GetMethodID(restore, "<init>",
      "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;JLjava/lang/Long;)V");
  jclass boxed_long = env->FindClass("java/lang/Long");
  jmethodID boxed_long_ctor = env->GetMethodID(boxed_long, "<init>", "(J)V");
  ~JavaTypes() {
    env->DeleteLocalRef(list);
    env->DeleteLocalRef(save);
    env->DeleteLocalRef(author);
    env->DeleteLocalRef(entry);
    env->DeleteLocalRef(page);
    env->DeleteLocalRef(version);
    env->DeleteLocalRef(line);
    env->DeleteLocalRef(comparison);
    env->DeleteLocalRef(restore);
    env->DeleteLocalRef(boxed_long);
  }
  jobject array(size_t count) { return env->NewObject(list, list_ctor, static_cast<jint>(count)); }
};

jobject strings(JavaTypes& types, const holder_string_list& values) {
  jobject out = types.array(values.count);
  for (size_t i = 0; i < values.count && !types.env->ExceptionCheck(); ++i) {
    jstring value = string(types.env, values.items[i]);
    types.env->CallBooleanMethod(out, types.add, value);
    types.env->DeleteLocalRef(value);
  }
  return out;
}

jobject save(JavaTypes& types, const holder_card_history_save& value) {
  JNIEnv* env = types.env;
  jstring oid = string(env, value.oid);
  jobject parents = strings(types, value.parent_oids);
  jstring message = string(env, value.message);
  jobject out = env->NewObject(types.save, types.save_ctor, oid, parents,
      static_cast<jlong>(value.authored_at), static_cast<jlong>(value.committed_at), message);
  env->DeleteLocalRef(oid);
  env->DeleteLocalRef(parents);
  env->DeleteLocalRef(message);
  return out;
}

jobject entry(JavaTypes& types, const holder_card_history_entry& value) {
  JNIEnv* env = types.env;
  jstring first = string(env, value.first_oid);
  jstring last = string(env, value.last_oid);
  jobject parents = strings(types, value.parent_oids);
  jobject visible = strings(types, value.visible_parent_oids);
  jstring name = string(env, value.author.name);
  jstring email = string(env, value.author.email);
  jobject author = env->NewObject(types.author, types.author_ctor, name, email);
  jstring kind = string(env, value.kind);
  jstring summary = string(env, value.summary);
  jobject saves = types.array(value.saves.count);
  for (size_t i = 0; i < value.saves.count && !env->ExceptionCheck(); ++i) {
    jobject item = save(types, value.saves.items[i]);
    env->CallBooleanMethod(saves, types.add, item);
    env->DeleteLocalRef(item);
  }
  jobject out = env->ExceptionCheck() ? nullptr : env->NewObject(types.entry, types.entry_ctor,
      first, last, parents, visible, author, static_cast<jlong>(value.started_at),
      static_cast<jlong>(value.ended_at), kind, summary, static_cast<jint>(value.commit_count),
      static_cast<jboolean>(value.is_merge), saves);
  env->DeleteLocalRef(first);
  env->DeleteLocalRef(last);
  env->DeleteLocalRef(parents);
  env->DeleteLocalRef(visible);
  env->DeleteLocalRef(name);
  env->DeleteLocalRef(email);
  env->DeleteLocalRef(author);
  env->DeleteLocalRef(kind);
  env->DeleteLocalRef(summary);
  env->DeleteLocalRef(saves);
  return out;
}

jobject version(JavaTypes& types, const holder_card_history_version& value) {
  JNIEnv* env = types.env;
  jstring oid = string(env, value.oid);
  jstring title = string(env, value.title);
  jstring body = string(env, value.body);
  jobject out = env->ExceptionCheck() ? nullptr : env->NewObject(types.version, types.version_ctor,
      static_cast<jboolean>(value.exists), oid, title, body);
  env->DeleteLocalRef(oid);
  env->DeleteLocalRef(title);
  env->DeleteLocalRef(body);
  return out;
}

jobject diff_line(JavaTypes& types, const holder_card_history_diff_line& value) {
  JNIEnv* env = types.env;
  jstring origin = string(env, value.origin);
  jstring text = string(env, value.text);
  jobject old_line = value.old_line ? env->NewObject(types.boxed_long, types.boxed_long_ctor,
      static_cast<jlong>(*value.old_line)) : nullptr;
  jobject new_line = value.new_line ? env->NewObject(types.boxed_long, types.boxed_long_ctor,
      static_cast<jlong>(*value.new_line)) : nullptr;
  jobject out = env->ExceptionCheck() ? nullptr : env->NewObject(types.line, types.line_ctor,
      origin, text, old_line, new_line);
  env->DeleteLocalRef(origin);
  env->DeleteLocalRef(text);
  if (old_line) env->DeleteLocalRef(old_line);
  if (new_line) env->DeleteLocalRef(new_line);
  return out;
}
}  // namespace

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_generated_ContractHistory_nativeListCardHistory(
    JNIEnv* env, jobject, jlong context_handle, jstring project_id, jstring card_id,
    jint limit, jstring cursor) {
  if (!context_handle || !project_id || !card_id || limit < 1 || limit > 200) {
    fail(env, "context, project ID, card ID and limit 1..200 are required"); return nullptr;
  }
  std::string project, card, position;
  if (!utf8(env, project_id, project) || !utf8(env, card_id, card) ||
      (cursor && !utf8(env, cursor, position))) return nullptr;
  Arena arena;
  const holder_card_history_page* page = nullptr;
  holder_error* error = nullptr;
  const int32_t page_limit = static_cast<int32_t>(limit);
  const int status = arena.value ? holder_list_card_history(
      reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      project.c_str(), card.c_str(), &page_limit, cursor ? position.c_str() : nullptr,
      arena.value, &page, &error) : HOLDER_ERROR_ALLOCATION;
  if (status != HOLDER_OK) {
    std::string message = error ? holder_error_message(error) : "cannot allocate history page";
    if (error) holder_error_destroy(error);
    fail(env, message.c_str()); return nullptr;
  }
  JavaTypes types{env};
  if (env->ExceptionCheck()) return nullptr;
  jobject entries = types.array(page->entries.count);
  for (size_t i = 0; i < page->entries.count && !env->ExceptionCheck(); ++i) {
    jobject item = entry(types, page->entries.items[i]);
    if (!env->ExceptionCheck()) env->CallBooleanMethod(entries, types.add, item);
    if (item) env->DeleteLocalRef(item);
  }
  jstring head = string(env, page->head_oid);
  jstring next = string(env, page->next_cursor);
  jobject result = env->ExceptionCheck() ? nullptr : env->NewObject(types.page, types.page_ctor,
      head, entries, next, static_cast<jboolean>(page->scan_limited));
  env->DeleteLocalRef(head);
  env->DeleteLocalRef(entries);
  env->DeleteLocalRef(next);
  return result;
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_generated_ContractHistory_nativeCompareCardHistory(
    JNIEnv* env, jobject, jlong context_handle, jstring project_id, jstring card_id,
    jstring from_oid, jstring to_oid) {
  if (!context_handle || !project_id || !card_id || !to_oid) {
    fail(env, "context, project ID, card ID and ending revision are required"); return nullptr;
  }
  std::string project, card, from, to;
  if (!utf8(env, project_id, project) || !utf8(env, card_id, card) ||
      (from_oid && !utf8(env, from_oid, from)) || !utf8(env, to_oid, to)) return nullptr;
  Arena arena;
  const holder_card_history_comparison* comparison = nullptr;
  holder_error* error = nullptr;
  const auto mode = from_oid ? HOLDER_COMPARE_CARD_HISTORY_MODE_SINCE
                             : HOLDER_COMPARE_CARD_HISTORY_MODE_CHANGE;
  const int status = arena.value ? holder_compare_card_history(
      reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      project.c_str(), card.c_str(), to.c_str(), from_oid ? from.c_str() : nullptr,
      mode, arena.value, &comparison, &error) : HOLDER_ERROR_ALLOCATION;
  if (status != HOLDER_OK) {
    std::string message = error ? holder_error_message(error) : "cannot allocate history comparison";
    if (error) holder_error_destroy(error);
    fail(env, message.c_str()); return nullptr;
  }
  JavaTypes types{env};
  if (env->ExceptionCheck()) return nullptr;
  jobject before = version(types, comparison->before);
  jobject after = version(types, comparison->after);
  jstring summary = string(env, comparison->summary);
  jobject lines = types.array(comparison->lines.count);
  for (size_t i = 0; i < comparison->lines.count && !env->ExceptionCheck(); ++i) {
    jobject item = diff_line(types, comparison->lines.items[i]);
    if (!env->ExceptionCheck()) env->CallBooleanMethod(lines, types.add, item);
    if (item) env->DeleteLocalRef(item);
  }
  jobject result = env->ExceptionCheck() ? nullptr : env->NewObject(types.comparison,
      types.comparison_ctor, before, after, summary, lines,
      static_cast<jboolean>(comparison->truncated));
  if (before) env->DeleteLocalRef(before);
  if (after) env->DeleteLocalRef(after);
  if (summary) env->DeleteLocalRef(summary);
  if (lines) env->DeleteLocalRef(lines);
  return result;
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_generated_ContractHistory_nativeRestoreCardHistory(
    JNIEnv* env, jobject, jlong context_handle, jstring project_id, jstring card_id,
    jstring oid) {
  if (!context_handle || !project_id || !card_id || !oid) {
    fail(env, "context, project ID, card ID and revision are required"); return nullptr;
  }
  std::string project, card, revision;
  if (!utf8(env, project_id, project) || !utf8(env, card_id, card) ||
      !utf8(env, oid, revision)) return nullptr;
  Arena arena;
  const holder_card_history_restore_result* restored = nullptr;
  holder_error* error = nullptr;
  const int status = arena.value ? holder_restore_card_history_snapshot(
      reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      project.c_str(), card.c_str(), revision.c_str(), arena.value, &restored, &error)
      : HOLDER_ERROR_ALLOCATION;
  if (status != HOLDER_OK) {
    std::string message = error ? holder_error_message(error) : "cannot allocate history restore result";
    if (error) holder_error_destroy(error);
    fail(env, message.c_str()); return nullptr;
  }
  JavaTypes types{env};
  if (env->ExceptionCheck()) return nullptr;
  jstring result_card_id = string(env, restored->card_id);
  jstring from = string(env, restored->restored_from_oid);
  jstring result_oid = string(env, restored->result_oid);
  jstring title = string(env, restored->title);
  jobject deleted = restored->deleted_at ? env->NewObject(types.boxed_long, types.boxed_long_ctor,
      static_cast<jlong>(*restored->deleted_at)) : nullptr;
  jobject result = env->ExceptionCheck() ? nullptr : env->NewObject(types.restore, types.restore_ctor,
      result_card_id, from, result_oid, title, static_cast<jlong>(restored->updated_at), deleted);
  env->DeleteLocalRef(result_card_id);
  env->DeleteLocalRef(from);
  env->DeleteLocalRef(result_oid);
  env->DeleteLocalRef(title);
  if (deleted) env->DeleteLocalRef(deleted);
  return result;
}
