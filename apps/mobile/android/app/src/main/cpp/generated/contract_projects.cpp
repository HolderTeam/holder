// Generated from contract/interface/projects/ids.py, projects/models.py, projects/operations.py, maintenance.py and git.py by holder_contract.
// Do not edit; run make -C contract update.
#include "holder/holder.h"
#include <jni.h>
#include <cstdint>
#include <optional>
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

bool read_string_field(JNIEnv* env, jobject input, jclass type, const char* name, bool required,
                       std::optional<std::string>& out) {
  jfieldID field = env->GetFieldID(type, name, "Ljava/lang/String;");
  if (!field) return false;
  jstring value = static_cast<jstring>(env->GetObjectField(input, field));
  if (env->ExceptionCheck()) return false;
  if (!value) {
    if (required) { fail(env, "required project field is null"); return false; }
    return true;
  }
  out.emplace();
  const bool decoded = utf8(env, value, *out);
  env->DeleteLocalRef(value);
  return decoded;
}

bool read_long_field(JNIEnv* env, jobject input, jclass type, const char* name, std::optional<int64_t>& out) {
  jfieldID field = env->GetFieldID(type, name, "Ljava/lang/Long;");
  if (!field) return false;
  jobject value = env->GetObjectField(input, field);
  if (env->ExceptionCheck()) return false;
  if (!value) return true;
  jclass boxed_type = env->GetObjectClass(value);
  if (!boxed_type) return false;
  jmethodID long_value = env->GetMethodID(boxed_type, "longValue", "()J");
  if (long_value) out = static_cast<int64_t>(env->CallLongMethod(value, long_value));
  env->DeleteLocalRef(boxed_type);
  env->DeleteLocalRef(value);
  return long_value && !env->ExceptionCheck();
}

jobject boxed_int(JNIEnv* env, int32_t value) {
  jclass type = env->FindClass("java/lang/Integer");
  jmethodID ctor = env->GetMethodID(type, "<init>", "(I)V");
  jobject result = env->NewObject(type, ctor, static_cast<jint>(value));
  env->DeleteLocalRef(type);
  return result;
}

jobject boxed_long(JNIEnv* env, int64_t value) {
  jclass type = env->FindClass("java/lang/Long");
  jmethodID ctor = env->GetMethodID(type, "<init>", "(J)V");
  jobject result = env->NewObject(type, ctor, static_cast<jlong>(value));
  env->DeleteLocalRef(type);
  return result;
}

const char* privacy_mode(holder_project_privacy_mode value) {
  switch (value) {
      case HOLDER_PROJECT_PRIVACY_MODE_ENCRYPTED_GIT: return "encrypted_git";
      case HOLDER_PROJECT_PRIVACY_MODE_PLAIN: return "plain";
      default: return nullptr;
  }
}

const char* privacy_mode(holder_project_encryption_check_privacy_mode value) {
  switch (value) {
      case HOLDER_PROJECT_ENCRYPTION_CHECK_PRIVACY_MODE_ENCRYPTED_GIT: return "encrypted_git";
      case HOLDER_PROJECT_ENCRYPTION_CHECK_PRIVACY_MODE_PLAIN: return "plain";
      default: return nullptr;
  }
}

const char* pull_status(holder_pull_phase_status value) {
  switch (value) {
      case HOLDER_PULL_PHASE_STATUS_NOT_ATTEMPTED: return "not_attempted";
      case HOLDER_PULL_PHASE_STATUS_SUCCEEDED: return "succeeded";
      case HOLDER_PULL_PHASE_STATUS_REMOTE_UNSET: return "remote_unset";
      case HOLDER_PULL_PHASE_STATUS_FAILED: return "failed";
      default: return nullptr;
  }
}

const char* push_status(holder_push_phase_status value) {
  switch (value) {
      case HOLDER_PUSH_PHASE_STATUS_NOT_ATTEMPTED: return "not_attempted";
      case HOLDER_PUSH_PHASE_STATUS_PUSHED: return "pushed";
      case HOLDER_PUSH_PHASE_STATUS_UP_TO_DATE: return "up_to_date";
      case HOLDER_PUSH_PHASE_STATUS_AUTH_FAILED: return "auth_failed";
      case HOLDER_PUSH_PHASE_STATUS_NOT_FOUND: return "not_found";
      case HOLDER_PUSH_PHASE_STATUS_NETWORK_ERROR: return "network_error";
      case HOLDER_PUSH_PHASE_STATUS_NON_FAST_FORWARD: return "non_fast_forward";
      case HOLDER_PUSH_PHASE_STATUS_REMOTE_UNSET: return "remote_unset";
      case HOLDER_PUSH_PHASE_STATUS_UNKNOWN_ERROR: return "unknown_error";
      default: return nullptr;
  }
}

const char* remote_test_status(holder_remote_test_result_status value) {
  switch (value) {
      case HOLDER_REMOTE_TEST_RESULT_STATUS_REACHABLE: return "reachable";
      case HOLDER_REMOTE_TEST_RESULT_STATUS_AUTH_FAILED: return "auth_failed";
      case HOLDER_REMOTE_TEST_RESULT_STATUS_NOT_FOUND: return "not_found";
      case HOLDER_REMOTE_TEST_RESULT_STATUS_NETWORK_ERROR: return "network_error";
      case HOLDER_REMOTE_TEST_RESULT_STATUS_INVALID_REMOTE_URL: return "invalid_remote_url";
      case HOLDER_REMOTE_TEST_RESULT_STATUS_REMOTE_UNSET: return "remote_unset";
      case HOLDER_REMOTE_TEST_RESULT_STATUS_UNKNOWN_ERROR: return "unknown_error";
      default: return nullptr;
  }
}

jobject project_result(JNIEnv* env, const holder_project& project) {
  const char* mode = privacy_mode(project.privacy_mode);
  if (!mode) { fail(env, "unknown project privacy mode"); return nullptr; }
  jclass item_type = env->FindClass("team/holder/android/generated/ContractProject");
  if (!item_type) return nullptr;
  jmethodID item_ctor = env->GetMethodID(item_type, "<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;JJ)V");
  jstring v_project_id = string(env, project.project_id);
  jstring v_name = string(env, project.name);
  jstring v_root_path = string(env, project.root_path);
  jstring v_git_remote_url = string(env, project.git_remote_url);
  jstring v_git_provider = string(env, project.git_provider);
  jstring v_privacy_mode = string(env, mode);
  jstring v_project_key_id = string(env, project.project_key_id);
  jlong v_created_at = static_cast<jlong>(project.created_at);
  jlong v_updated_at = static_cast<jlong>(project.updated_at);
  jobject item = env->NewObject(item_type, item_ctor, v_project_id, v_name, v_root_path, v_git_remote_url, v_git_provider, v_privacy_mode, v_project_key_id, v_created_at, v_updated_at);
  if (v_project_id) env->DeleteLocalRef(v_project_id);
  if (v_name) env->DeleteLocalRef(v_name);
  if (v_root_path) env->DeleteLocalRef(v_root_path);
  if (v_git_remote_url) env->DeleteLocalRef(v_git_remote_url);
  if (v_git_provider) env->DeleteLocalRef(v_git_provider);
  if (v_privacy_mode) env->DeleteLocalRef(v_privacy_mode);
  if (v_project_key_id) env->DeleteLocalRef(v_project_key_id);
  env->DeleteLocalRef(item_type);
  return item;
}

jobject encryption_check_result(JNIEnv* env, const holder_project_encryption_check& value) {
  const char* mode = privacy_mode(value.privacy_mode);
  if (!mode) { fail(env, "unknown project privacy mode"); return nullptr; }
  jclass list_type = env->FindClass("java/util/ArrayList");
  if (!list_type) return nullptr;
  jmethodID list_ctor = env->GetMethodID(list_type, "<init>", "(I)V");
  jmethodID add = env->GetMethodID(list_type, "add", "(Ljava/lang/Object;)Z");
  jobject paths = env->NewObject(list_type, list_ctor, static_cast<jint>(value.check.unsafe_paths.count));
  if (env->ExceptionCheck()) return nullptr;
  for (size_t i = 0; i < value.check.unsafe_paths.count; ++i) {
    jstring path = string(env, value.check.unsafe_paths.items[i]);
    if (env->ExceptionCheck()) return nullptr;
    env->CallBooleanMethod(paths, add, path);
    env->DeleteLocalRef(path);
    if (env->ExceptionCheck()) return nullptr;
  }
  jclass item_type = env->FindClass("team/holder/android/generated/ContractEncryptionCheck");
  if (!item_type) return nullptr;
  jmethodID ctor = env->GetMethodID(item_type, "<init>",
      "(Ljava/lang/String;Ljava/lang/String;ZIILjava/util/List;Ljava/lang/String;)V");
  jstring project_id = string(env, value.project_id);
  jstring privacy = string(env, mode);
  jstring message = string(env, value.check.message);
  jobject result = env->NewObject(item_type, ctor, project_id, privacy,
      static_cast<jboolean>(value.check.ok), static_cast<jint>(value.check.checked_files),
      static_cast<jint>(value.check.unsafe_files), paths, message);
  if (project_id) env->DeleteLocalRef(project_id);
  if (privacy) env->DeleteLocalRef(privacy);
  if (message) env->DeleteLocalRef(message);
  env->DeleteLocalRef(paths);
  env->DeleteLocalRef(list_type);
  env->DeleteLocalRef(item_type);
  return result;
}

jobject sync_state_result(JNIEnv* env, const holder_sync_state& value) {
  jclass item_type = env->FindClass("team/holder/android/generated/ContractSyncState");
  if (!item_type) return nullptr;
  jmethodID item_ctor = env->GetMethodID(item_type, "<init>", "(Ljava/lang/String;Ljava/lang/Long;Ljava/lang/Long;Ljava/lang/Long;IILjava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/Long;ILjava/lang/Long;ILjava/lang/Long;Ljava/lang/Long;)V");
  jstring v_project_id = string(env, value.project_id);
  jobject v_last_commit_at = value.last_commit_at ? boxed_long(env, *value.last_commit_at) : nullptr;
  jobject v_last_push_at = value.last_push_at ? boxed_long(env, *value.last_push_at) : nullptr;
  jobject v_last_pull_at = value.last_pull_at ? boxed_long(env, *value.last_pull_at) : nullptr;
  jint v_uncommitted_changes_count = static_cast<jint>(value.uncommitted_changes_count);
  jint v_unpushed_commits_count = static_cast<jint>(value.unpushed_commits_count);
  jstring v_last_push_status = string(env, value.last_push_status);
  jstring v_last_pull_status = string(env, value.last_pull_status);
  jstring v_last_sync_error = string(env, value.last_sync_error);
  jobject v_last_sync_error_at = value.last_sync_error_at ? boxed_long(env, *value.last_sync_error_at) : nullptr;
  jint v_retry_count = static_cast<jint>(value.retry_count);
  jobject v_next_retry_at = value.next_retry_at ? boxed_long(env, *value.next_retry_at) : nullptr;
  jint v_pull_retry_count = static_cast<jint>(value.pull_retry_count);
  jobject v_next_pull_retry_at = value.next_pull_retry_at ? boxed_long(env, *value.next_pull_retry_at) : nullptr;
  jobject v_updated_at = value.updated_at ? boxed_long(env, *value.updated_at) : nullptr;
  jobject item = env->NewObject(item_type, item_ctor, v_project_id, v_last_commit_at, v_last_push_at, v_last_pull_at, v_uncommitted_changes_count, v_unpushed_commits_count, v_last_push_status, v_last_pull_status, v_last_sync_error, v_last_sync_error_at, v_retry_count, v_next_retry_at, v_pull_retry_count, v_next_pull_retry_at, v_updated_at);
  if (v_project_id) env->DeleteLocalRef(v_project_id);
  if (v_last_commit_at) env->DeleteLocalRef(v_last_commit_at);
  if (v_last_push_at) env->DeleteLocalRef(v_last_push_at);
  if (v_last_pull_at) env->DeleteLocalRef(v_last_pull_at);
  if (v_last_push_status) env->DeleteLocalRef(v_last_push_status);
  if (v_last_pull_status) env->DeleteLocalRef(v_last_pull_status);
  if (v_last_sync_error) env->DeleteLocalRef(v_last_sync_error);
  if (v_last_sync_error_at) env->DeleteLocalRef(v_last_sync_error_at);
  if (v_next_retry_at) env->DeleteLocalRef(v_next_retry_at);
  if (v_next_pull_retry_at) env->DeleteLocalRef(v_next_pull_retry_at);
  if (v_updated_at) env->DeleteLocalRef(v_updated_at);
  env->DeleteLocalRef(item_type);
  return item;
}

jobject pull_phase_result(JNIEnv* env, const holder_pull_phase& value) {
  const char* status = pull_status(value.status);
  if (!status) { fail(env, "unknown project pull status"); return nullptr; }
  jclass item_type = env->FindClass("team/holder/android/generated/ContractPullPhase");
  if (!item_type) return nullptr;
  jmethodID ctor = env->GetMethodID(item_type, "<init>", "(ZLjava/lang/String;IILjava/lang/String;)V");
  jstring state = string(env, status);
  jstring error_message = string(env, value.error_message);
  jobject result = env->NewObject(item_type, ctor, static_cast<jboolean>(value.attempted), state,
      static_cast<jint>(value.conflicts_resolved), static_cast<jint>(value.hidden_cards_repaired),
      error_message);
  if (state) env->DeleteLocalRef(state);
  if (error_message) env->DeleteLocalRef(error_message);
  env->DeleteLocalRef(item_type);
  return result;
}

jobject push_phase_result(JNIEnv* env, const holder_push_phase& value) {
  const char* status = push_status(value.status);
  if (!status) { fail(env, "unknown project push status"); return nullptr; }
  jclass type = env->FindClass("team/holder/android/generated/ContractPushPhase");
  if (!type) return nullptr;
  jmethodID ctor = env->GetMethodID(type, "<init>", "(Ljava/lang/String;IILjava/lang/String;)V");
  jstring state = string(env, status);
  jstring message = string(env, value.error_message);
  jobject result = env->NewObject(type, ctor, state, static_cast<jint>(value.ahead_count),
      static_cast<jint>(value.behind_count), message);
  if (state) env->DeleteLocalRef(state);
  if (message) env->DeleteLocalRef(message);
  env->DeleteLocalRef(type);
  return result;
}

jobject remote_test_result(JNIEnv* env, const holder_remote_test_result& value) {
  const char* status = remote_test_status(value.status);
  if (!status) { fail(env, "unknown remote test status"); return nullptr; }
  jclass type = env->FindClass("team/holder/android/generated/ContractRemoteTestResult");
  if (!type) return nullptr;
  jmethodID ctor = env->GetMethodID(type, "<init>", "(Ljava/lang/String;ZLjava/lang/String;)V");
  jstring state = string(env, status);
  jstring message = string(env, value.error_message);
  jobject result = env->NewObject(type, ctor, state, static_cast<jboolean>(value.remote_has_head), message);
  if (state) env->DeleteLocalRef(state);
  if (message) env->DeleteLocalRef(message);
  env->DeleteLocalRef(type);
  return result;
}

template <typename Result>
jobject sync_now_result(JNIEnv* env, const Result& value) {
  const char* pull_state = value.pull.attempted ? pull_status(value.pull.status) : nullptr;
  const char* push_state = value.push.attempted ? push_status(value.push.status) : nullptr;
  if ((value.pull.attempted && !pull_state) || (value.push.attempted && !push_state)) {
    fail(env, "unknown project sync status"); return nullptr;
  }
  jclass type = env->FindClass("team/holder/android/generated/ContractSyncNowResult");
  if (!type) return nullptr;
  jmethodID ctor = env->GetMethodID(type, "<init>",
      "(ZLjava/lang/String;Ljava/lang/String;IZLjava/lang/String;Ljava/lang/String;IILjava/lang/String;)V");
  if (!ctor) { env->DeleteLocalRef(type); return nullptr; }
  jstring pull_status_value = string(env, pull_state);
  jstring pull_error = string(env, value.pull.error_message);
  jstring push_status_value = string(env, push_state);
  jstring push_error = string(env, value.push.error_message);
  jstring head = string(env, value.push.local_head_commit);
  jobject result = env->NewObject(type, ctor, static_cast<jboolean>(value.pull.attempted),
      pull_status_value, pull_error, static_cast<jint>(value.pull.conflicts_resolved),
      static_cast<jboolean>(value.push.attempted), push_status_value, push_error,
      static_cast<jint>(value.push.ahead_count), static_cast<jint>(value.push.behind_count), head);
  if (pull_status_value) env->DeleteLocalRef(pull_status_value);
  if (pull_error) env->DeleteLocalRef(pull_error);
  if (push_status_value) env->DeleteLocalRef(push_status_value);
  if (push_error) env->DeleteLocalRef(push_error);
  if (head) env->DeleteLocalRef(head);
  env->DeleteLocalRef(type);
  return result;
}

void project_error(JNIEnv* env, holder_error* error, const char* fallback) {
  std::string message = error ? holder_error_message(error) : fallback;
  if (error) holder_error_destroy(error);
  fail(env, message.c_str());
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
  jmethodID item_ctor = env->GetMethodID(item_type, "<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;JJLjava/lang/Integer;Ljava/lang/Integer;)V");
  jclass list_type = env->FindClass("java/util/ArrayList");
  jmethodID list_ctor = env->GetMethodID(list_type, "<init>", "(I)V");
  jmethodID add = env->GetMethodID(list_type, "add", "(Ljava/lang/Object;)Z");
  jobject items = env->NewObject(list_type, list_ctor, static_cast<jint>(page->count));
  if (env->ExceptionCheck()) return nullptr;
  for (size_t i = 0; i < page->count; ++i) {
    const auto& project = page->items[i];
    const char* mode = privacy_mode(project.privacy_mode);
    if (!mode) { fail(env, "unknown project privacy mode"); return nullptr; }
    jstring v_project_id = string(env, project.project_id);
    jstring v_name = string(env, project.name);
    jstring v_root_path = string(env, project.root_path);
    jstring v_git_remote_url = string(env, project.git_remote_url);
    jstring v_git_provider = string(env, project.git_provider);
    jstring v_privacy_mode = string(env, mode);
    jstring v_project_key_id = string(env, project.project_key_id);
    jlong v_created_at = static_cast<jlong>(project.created_at);
    jlong v_updated_at = static_cast<jlong>(project.updated_at);
    jobject v_card_count = project.card_count ? boxed_int(env, *project.card_count) : nullptr;
    jobject v_root_card_count = project.root_card_count ? boxed_int(env, *project.root_card_count) : nullptr;
    jobject item = env->NewObject(item_type, item_ctor, v_project_id, v_name, v_root_path, v_git_remote_url, v_git_provider, v_privacy_mode, v_project_key_id, v_created_at, v_updated_at, v_card_count, v_root_card_count);
    if (v_project_id) env->DeleteLocalRef(v_project_id);
    if (v_name) env->DeleteLocalRef(v_name);
    if (v_root_path) env->DeleteLocalRef(v_root_path);
    if (v_git_remote_url) env->DeleteLocalRef(v_git_remote_url);
    if (v_git_provider) env->DeleteLocalRef(v_git_provider);
    if (v_privacy_mode) env->DeleteLocalRef(v_privacy_mode);
    if (v_project_key_id) env->DeleteLocalRef(v_project_key_id);
    if (v_card_count) env->DeleteLocalRef(v_card_count);
    if (v_root_card_count) env->DeleteLocalRef(v_root_card_count);
    if (env->ExceptionCheck()) return nullptr;
    env->CallBooleanMethod(items, add, item);
    env->DeleteLocalRef(item);
    if (env->ExceptionCheck()) return nullptr;
  }
  return items;
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_generated_ContractProjects_nativeGetProject(
    JNIEnv* env, jobject, jlong context_handle, jstring project_id) {
  if (!context_handle || !project_id) { fail(env, "context and project ID are required"); return nullptr; }
  std::string id;
  if (!utf8(env, project_id, id)) return nullptr;
  Arena arena;
  const holder_project* value = nullptr;
  holder_error* error = nullptr;
  int status = arena.value ? holder_get_project(
      reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      id.c_str(), arena.value, &value, &error) : HOLDER_ERROR_ALLOCATION;
  if (status != HOLDER_OK) { project_error(env, error, "cannot get project"); return nullptr; }
  return project_result(env, *value);
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_generated_ContractProjects_nativeCreateProject(
    JNIEnv* env, jobject, jlong context_handle, jobject input) {
  if (!context_handle || !input) { fail(env, "context and project are required"); return nullptr; }
  jclass input_type = env->GetObjectClass(input);
  if (!input_type) return nullptr;
  holder_new_project request{};
  std::optional<std::string> v_project_id;
  if (!read_string_field(env, input, input_type, "projectId", false, v_project_id)) return nullptr;
  request.project_id = v_project_id ? v_project_id->c_str() : nullptr;
  std::optional<std::string> v_name;
  if (!read_string_field(env, input, input_type, "name", true, v_name)) return nullptr;
  request.name = v_name ? v_name->c_str() : nullptr;
  std::optional<std::string> v_root_path;
  if (!read_string_field(env, input, input_type, "rootPath", false, v_root_path)) return nullptr;
  request.root_path = v_root_path ? v_root_path->c_str() : nullptr;
  std::optional<std::string> v_git_remote_url;
  if (!read_string_field(env, input, input_type, "gitRemoteUrl", false, v_git_remote_url)) return nullptr;
  request.git_remote_url = v_git_remote_url ? v_git_remote_url->c_str() : nullptr;
  std::optional<std::string> v_git_provider;
  if (!read_string_field(env, input, input_type, "gitProvider", false, v_git_provider)) return nullptr;
  request.git_provider = v_git_provider ? v_git_provider->c_str() : nullptr;
  std::optional<std::string> v_privacy_mode;
  if (!read_string_field(env, input, input_type, "privacyMode", true, v_privacy_mode)) return nullptr;
  if (v_privacy_mode) {
    if (*v_privacy_mode == "encrypted_git") request.privacy_mode = HOLDER_NEW_PROJECT_PRIVACY_MODE_ENCRYPTED_GIT;
    else if (*v_privacy_mode == "plain") request.privacy_mode = HOLDER_NEW_PROJECT_PRIVACY_MODE_PLAIN;
    else { fail(env, "unknown privacy_mode"); return nullptr; }
  }
  std::optional<int64_t> v_created_at;
  if (!read_long_field(env, input, input_type, "createdAt", v_created_at)) return nullptr;
  request.created_at = v_created_at ? &*v_created_at : nullptr;
  std::optional<int64_t> v_updated_at;
  if (!read_long_field(env, input, input_type, "updatedAt", v_updated_at)) return nullptr;
  request.updated_at = v_updated_at ? &*v_updated_at : nullptr;
  env->DeleteLocalRef(input_type);
  Arena arena;
  const holder_project* value = nullptr;
  holder_error* error = nullptr;
  int status = arena.value ? holder_create_project(
      reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      &request, arena.value, &value, &error) : HOLDER_ERROR_ALLOCATION;
  if (status != HOLDER_OK) { project_error(env, error, "cannot create project"); return nullptr; }
  return project_result(env, *value);
}


extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_generated_ContractProjects_nativeRenameProject(
    JNIEnv* env, jobject, jlong context_handle, jstring project_id, jstring name) {
  if (!context_handle || !project_id || !name) {
    fail(env, "context, project ID and name are required"); return nullptr;
  }
  std::string id, project_name;
  if (!utf8(env, project_id, id) || !utf8(env, name, project_name)) return nullptr;
  holder_project_update request{};
  request.name = project_name.c_str();
  Arena arena;
  const holder_project* value = nullptr;
  holder_error* error = nullptr;
  int status = arena.value ? holder_update_project(
      reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      id.c_str(), &request, arena.value, &value, &error) : HOLDER_ERROR_ALLOCATION;
  if (status != HOLDER_OK) { project_error(env, error, "cannot rename project"); return nullptr; }
  return project_result(env, *value);
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_generated_ContractProjects_nativeUpdateProjectGitRemote(
    JNIEnv* env, jobject, jlong context_handle, jstring project_id, jstring remote_url) {
  if (!context_handle || !project_id) { fail(env, "context and project ID are required"); return nullptr; }
  std::string id, url;
  if (!utf8(env, project_id, id) || (remote_url && !utf8(env, remote_url, url))) return nullptr;
  holder_project_update request{};
  // The old Android call also cleared an empty URL.
  request.git_remote_url.change = !remote_url || url.empty() ? HOLDER_CLEAR : HOLDER_SET;
  request.git_remote_url.value = request.git_remote_url.change == HOLDER_SET ? url.c_str() : nullptr;
  Arena arena;
  const holder_project* value = nullptr;
  holder_error* error = nullptr;
  int status = arena.value ? holder_update_project(
      reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      id.c_str(), &request, arena.value, &value, &error) : HOLDER_ERROR_ALLOCATION;
  if (status != HOLDER_OK) { project_error(env, error, "cannot update project Git remote"); return nullptr; }
  return project_result(env, *value);
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_generated_ContractProjects_nativeCheckProjectEncryption(
    JNIEnv* env, jobject, jlong context_handle, jstring project_id) {
  if (!context_handle || !project_id) { fail(env, "context and project ID are required"); return nullptr; }
  std::string id;
  if (!utf8(env, project_id, id)) return nullptr;
  Arena arena;
  const holder_project_encryption_check* value = nullptr;
  holder_error* error = nullptr;
  int status = arena.value ? holder_check_project_encryption(
      reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      id.c_str(), arena.value, &value, &error) : HOLDER_ERROR_ALLOCATION;
  if (status != HOLDER_OK) { project_error(env, error, "cannot check project encryption"); return nullptr; }
  return encryption_check_result(env, *value);
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_generated_ContractProjects_nativeGetProjectSyncState(
    JNIEnv* env, jobject, jlong context_handle, jstring project_id) {
  if (!context_handle || !project_id) { fail(env, "context and project ID are required"); return nullptr; }
  std::string id;
  if (!utf8(env, project_id, id)) return nullptr;
  Arena arena;
  const holder_sync_state* value = nullptr;
  holder_error* error = nullptr;
  int status = arena.value ? holder_get_project_sync_state(
      reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      id.c_str(), arena.value, &value, &error) : HOLDER_ERROR_ALLOCATION;
  if (status != HOLDER_OK) { project_error(env, error, "cannot read project sync state"); return nullptr; }
  return sync_state_result(env, *value);
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_generated_ContractProjects_nativePullProject(
    JNIEnv* env, jobject, jlong context_handle, jstring project_id) {
  if (!context_handle || !project_id) { fail(env, "context and project ID are required"); return nullptr; }
  std::string id;
  if (!utf8(env, project_id, id)) return nullptr;
  Arena arena;
  const holder_sync_result* value = nullptr;
  holder_error* error = nullptr;
  int status = arena.value ? holder_pull_project(
      reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      id.c_str(), arena.value, &value, &error) : HOLDER_ERROR_ALLOCATION;
  if (status != HOLDER_OK) { project_error(env, error, "cannot pull project"); return nullptr; }
  return pull_phase_result(env, value->pull);
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_generated_ContractProjects_nativeSyncProject(
    JNIEnv* env, jobject, jlong context_handle, jstring project_id, jstring branch,
    jboolean push, jboolean set_upstream) {
  if (!context_handle || !project_id) { fail(env, "context and project ID are required"); return nullptr; }
  std::string id, branch_name;
  if (!utf8(env, project_id, id) || (branch && !utf8(env, branch, branch_name))) return nullptr;
  const bool upstream = set_upstream;
  holder_push_options options{};
  options.branch = branch ? branch_name.c_str() : nullptr;
  options.set_upstream = &upstream;
  Arena arena;
  const holder_sync_result* value = nullptr;
  holder_error* error = nullptr;
  holder_context* context = reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle));
  int status = arena.value ? (push
      ? holder_sync_project(context, id.c_str(), &options, arena.value, &value, &error)
      : holder_pull_project(context, id.c_str(), arena.value, &value, &error)) : HOLDER_ERROR_ALLOCATION;
  if (status != HOLDER_OK) { project_error(env, error, "cannot sync project"); return nullptr; }
  return sync_now_result(env, *value);
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_generated_ContractProjects_nativeSyncProjectIfDue(
    JNIEnv* env, jobject, jlong context_handle, jstring project_id,
    jint push_interval_seconds, jint pull_interval_seconds) {
  if (!context_handle || !project_id) { fail(env, "context and project ID are required"); return nullptr; }
  std::string id;
  if (!utf8(env, project_id, id)) return nullptr;
  Arena arena;
  const holder_due_sync_result* value = nullptr;
  holder_error* error = nullptr;
  int status = arena.value ? holder_sync_project_if_due(
      reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      id.c_str(), push_interval_seconds > 0 ? push_interval_seconds : 0,
      pull_interval_seconds > 0 ? pull_interval_seconds : 0, arena.value, &value, &error)
      : HOLDER_ERROR_ALLOCATION;
  if (status != HOLDER_OK) { project_error(env, error, "cannot sync project if due"); return nullptr; }
  return sync_now_result(env, *value);
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_generated_ContractProjects_nativePushProject(
    JNIEnv* env, jobject, jlong context_handle, jstring project_id, jstring branch, jboolean set_upstream) {
  if (!context_handle || !project_id) { fail(env, "context and project ID are required"); return nullptr; }
  std::string id, branch_name;
  if (!utf8(env, project_id, id) || (branch && !utf8(env, branch, branch_name))) return nullptr;
  const bool upstream = set_upstream;
  holder_push_options options{};
  options.branch = branch ? branch_name.c_str() : nullptr;
  options.set_upstream = &upstream;
  Arena arena;
  const holder_sync_result* value = nullptr;
  holder_error* error = nullptr;
  int status = arena.value ? holder_push_project(
      reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      id.c_str(), &options, arena.value, &value, &error) : HOLDER_ERROR_ALLOCATION;
  if (status != HOLDER_OK) { project_error(env, error, "cannot push project"); return nullptr; }
  return push_phase_result(env, value->push);
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_generated_ContractProjects_nativeTestProjectRemote(
    JNIEnv* env, jobject, jlong context_handle, jstring project_id, jstring remote_url) {
  if (!context_handle || !project_id) { fail(env, "context and project ID are required"); return nullptr; }
  std::string id, url;
  if (!utf8(env, project_id, id) || (remote_url && !utf8(env, remote_url, url))) return nullptr;
  holder_remote_test request{};
  request.remote_url = remote_url ? url.c_str() : nullptr;
  Arena arena;
  const holder_remote_test_result* value = nullptr;
  holder_error* error = nullptr;
  int status = arena.value ? holder_test_project_remote(
      reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      id.c_str(), &request, arena.value, &value, &error) : HOLDER_ERROR_ALLOCATION;
  if (status != HOLDER_OK) { project_error(env, error, "cannot test project remote"); return nullptr; }
  return remote_test_result(env, *value);
}

extern "C" JNIEXPORT void JNICALL
Java_team_holder_android_generated_ContractProjects_nativeDeleteProject(
    JNIEnv* env, jobject, jlong context_handle, jstring project_id) {
  if (!context_handle || !project_id) { fail(env, "context and project ID are required"); return; }
  std::string id;
  if (!utf8(env, project_id, id)) return;
  Arena arena;
  const holder_project* value = nullptr;
  holder_error* error = nullptr;
  int status = arena.value ? holder_delete_project(
      reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      id.c_str(), arena.value, &value, &error) : HOLDER_ERROR_ALLOCATION;
  if (status != HOLDER_OK) project_error(env, error, "cannot delete project");
}
extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_generated_ContractProjects_nativeExportRecoveryToken(
    JNIEnv* env, jobject, jlong context_handle, jstring project_id, jstring pin) {
  if (!context_handle || !project_id || !pin) {
    fail(env, "context, project ID and PIN are required"); return nullptr;
  }
  std::string project, secret;
  if (!utf8(env, project_id, project) || !utf8(env, pin, secret)) return nullptr;
  holder_recovery_token_export_request request{secret.c_str()};
  Arena arena;
  const holder_project_recovery_token_export_result* result = nullptr;
  holder_error* error = nullptr;
  int status = arena.value ? holder_export_project_recovery_token(
      reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      project.c_str(), &request, arena.value, &result, &error) : HOLDER_ERROR_ALLOCATION;
  if (status != HOLDER_OK) {
    std::string message = error ? holder_error_message(error) : "cannot export recovery token";
    if (error) holder_error_destroy(error);
    fail(env, message.c_str()); return nullptr;
  }
  const auto& value = *result;
  jclass item_type = env->FindClass("team/holder/android/generated/ContractProjectRecoveryTokenExportResult");
  if (!item_type) return nullptr;
  jmethodID item_ctor = env->GetMethodID(item_type, "<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
  if (!item_ctor) return nullptr;
  jstring v_project_id = string(env, value.project_id);
  jstring v_key_id = string(env, value.key_id);
  jstring v_recovery_token = string(env, value.recovery_token);
  jobject item = env->NewObject(item_type, item_ctor, v_project_id, v_key_id, v_recovery_token);
  if (v_project_id) env->DeleteLocalRef(v_project_id);
  if (v_key_id) env->DeleteLocalRef(v_key_id);
  if (v_recovery_token) env->DeleteLocalRef(v_recovery_token);
  env->DeleteLocalRef(item_type);
  return item;
}

extern "C" JNIEXPORT void JNICALL
Java_team_holder_android_generated_ContractProjects_nativeImportRecoveryToken(
    JNIEnv* env, jobject, jlong context_handle, jstring project_id, jstring pin, jstring recovery_token) {
  if (!context_handle || !project_id || !pin || !recovery_token) {
    fail(env, "context, project ID, PIN and recovery token are required"); return;
  }
  std::string project, secret, token;
  if (!utf8(env, project_id, project) || !utf8(env, pin, secret) ||
      !utf8(env, recovery_token, token)) return;
  holder_recovery_token_import_request request{secret.c_str(), token.c_str()};
  Arena arena;
  const holder_project_recovery_token_import_result* result = nullptr;
  holder_error* error = nullptr;
  int status = arena.value ? holder_import_project_recovery_token(
      reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      project.c_str(), &request, arena.value, &result, &error) : HOLDER_ERROR_ALLOCATION;
  if (status != HOLDER_OK) {
    std::string message = error ? holder_error_message(error) : "cannot import recovery token";
    if (error) holder_error_destroy(error);
    fail(env, message.c_str());
  }
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_generated_ContractProjects_nativeImportRecoveryTokenGlobal(
    JNIEnv* env, jobject, jlong context_handle, jstring pin, jstring recovery_token) {
  if (!context_handle || !pin || !recovery_token) {
    fail(env, "context, PIN and recovery token are required"); return nullptr;
  }
  std::string secret, token;
  if (!utf8(env, pin, secret) || !utf8(env, recovery_token, token)) return nullptr;
  holder_recovery_token_import_request request{secret.c_str(), token.c_str()};
  Arena arena;
  const holder_recovery_token_global_import_result* result = nullptr;
  holder_error* error = nullptr;
  int status = arena.value ? holder_import_recovery_token_global(
      reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      &request, arena.value, &result, &error) : HOLDER_ERROR_ALLOCATION;
  if (status != HOLDER_OK) {
    std::string message = error ? holder_error_message(error) : "cannot import recovery token";
    if (error) holder_error_destroy(error);
    fail(env, message.c_str()); return nullptr;
  }
  const auto& value = *result;
  const char* status_name = nullptr;
  switch (value.pull_status) {
    case HOLDER_RECOVERY_TOKEN_GLOBAL_IMPORT_RESULT_PULL_STATUS_NOT_ATTEMPTED: status_name = "not_attempted"; break;
    case HOLDER_RECOVERY_TOKEN_GLOBAL_IMPORT_RESULT_PULL_STATUS_SUCCEEDED: status_name = "succeeded"; break;
    case HOLDER_RECOVERY_TOKEN_GLOBAL_IMPORT_RESULT_PULL_STATUS_FAILED: status_name = "failed"; break;
    default: fail(env, "unknown recovery pull status"); return nullptr;
  }
  jclass item_type = env->FindClass("team/holder/android/generated/ContractRecoveryTokenGlobalImportResult");
  if (!item_type) return nullptr;
  jmethodID item_ctor = env->GetMethodID(item_type, "<init>", "(Ljava/lang/String;ZZZLjava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
  if (!item_ctor) return nullptr;
  jstring v_project_id = string(env, value.project_id);
  jboolean v_project_created = static_cast<jboolean>(value.project_created);
  jboolean v_remote_hint_present = static_cast<jboolean>(value.remote_hint_present);
  jboolean v_remote_configured = static_cast<jboolean>(value.remote_configured);
  jstring v_remote_error = string(env, value.remote_error);
  jstring v_pull_status = string(env, status_name);
  jstring v_pull_error = string(env, value.pull_error);
  jobject item = env->NewObject(item_type, item_ctor, v_project_id, v_project_created, v_remote_hint_present, v_remote_configured, v_remote_error, v_pull_status, v_pull_error);
  if (v_project_id) env->DeleteLocalRef(v_project_id);
  if (v_remote_error) env->DeleteLocalRef(v_remote_error);
  if (v_pull_status) env->DeleteLocalRef(v_pull_status);
  if (v_pull_error) env->DeleteLocalRef(v_pull_error);
  env->DeleteLocalRef(item_type);
  return item;
}
