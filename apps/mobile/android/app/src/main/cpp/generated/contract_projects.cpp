// Generated from contract/interface/projects.py and contract/interface/app.py by holder_contract.
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

jobject boxed_int(JNIEnv* env, int32_t value) {
  jclass type = env->FindClass("java/lang/Integer");
  jmethodID ctor = env->GetMethodID(type, "<init>", "(I)V");
  jobject result = env->NewObject(type, ctor, static_cast<jint>(value));
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
Java_team_holder_android_generated_ContractProjects_nativeCreateProject(
    JNIEnv* env, jobject, jlong context_handle, jstring name, jstring privacy_mode_value) {
  if (!context_handle || !name) { fail(env, "context and name are required"); return nullptr; }
  std::string project_name, privacy;
  if (!utf8(env, name, project_name) ||
      (privacy_mode_value && !utf8(env, privacy_mode_value, privacy))) return nullptr;
  holder_new_project request{};
  request.name = project_name.c_str();
  // Android's null mode has always meant plain; the contract defaults to encrypted_git.
  if (!privacy_mode_value || privacy == "plain") request.privacy_mode = HOLDER_NEW_PROJECT_PRIVACY_MODE_PLAIN;
  else if (privacy == "encrypted_git") request.privacy_mode = HOLDER_NEW_PROJECT_PRIVACY_MODE_ENCRYPTED_GIT;
  else { fail(env, "unknown project privacy mode"); return nullptr; }
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
