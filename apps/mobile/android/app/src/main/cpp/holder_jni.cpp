#include "holder/holder.h"

#include <jni.h>

#include <cstdint>
#include <memory>
#include <string>

namespace {

class UtfChars {
 public:
  UtfChars(JNIEnv* env, jstring value) : env_(env), value_(value) {
    if (value_ != nullptr) {
      chars_ = env_->GetStringUTFChars(value_, nullptr);
    }
  }

  UtfChars(const UtfChars&) = delete;
  UtfChars& operator=(const UtfChars&) = delete;

  ~UtfChars() {
    if (chars_ != nullptr) {
      env_->ReleaseStringUTFChars(value_, chars_);
    }
  }

  const char* get() const {
    return chars_;
  }

 private:
  JNIEnv* env_;
  jstring value_;
  const char* chars_ = nullptr;
};

void throw_runtime(JNIEnv* env, const std::string& message) {
  jclass exception_class = env->FindClass("java/lang/RuntimeException");
  if (exception_class != nullptr) {
    env->ThrowNew(exception_class, message.c_str());
  }
}

std::string error_message(holder_error* error) {
  std::string message = holder_error_message(error);
  holder_error_destroy(error);
  return message;
}

jstring string_result(JNIEnv* env, char* value) {
  std::unique_ptr<char, decltype(&holder_string_free)> holder(value, holder_string_free);
  return env->NewStringUTF(holder.get());
}

// Handles are opaque holder_context* values owned by HolderNative for the
// lifetime of the app; see nativeContextOpen/nativeContextClose.
holder_context* context_from_handle(JNIEnv* env, jlong context_handle) {
  if (context_handle == 0) {
    throw_runtime(env, "holder context is not open");
    return nullptr;
  }
  return reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle));
}

} // namespace

extern "C" JNIEXPORT jstring JNICALL
Java_team_holder_android_HolderNative_nativeVersion(JNIEnv* env, jobject /* thiz */) {
  return env->NewStringUTF(holder_version_string());
}

extern "C" JNIEXPORT jlong JNICALL
Java_team_holder_android_HolderNative_nativeContextOpen(
    JNIEnv* env,
    jobject /* thiz */,
    jstring data_dir,
    jstring schema_sql
) {
  UtfChars data_dir_chars(env, data_dir);
  UtfChars schema_sql_chars(env, schema_sql);
  if (data_dir_chars.get() == nullptr || schema_sql_chars.get() == nullptr) {
    throw_runtime(env, "data_dir and schema_sql must not be null");
    return 0;
  }

  holder_context* context = nullptr;
  holder_error* error = nullptr;
  const int rc = holder_context_open(data_dir_chars.get(), schema_sql_chars.get(), &context, &error);
  if (rc != HOLDER_OK) {
    throw_runtime(env, error_message(error));
    return 0;
  }
  return static_cast<jlong>(reinterpret_cast<intptr_t>(context));
}

extern "C" JNIEXPORT void JNICALL
Java_team_holder_android_HolderNative_nativeContextClose(
    JNIEnv* /* env */,
    jobject /* thiz */,
    jlong context_handle
) {
  if (context_handle == 0) {
    return;
  }
  holder_context_destroy(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)));
}

extern "C" JNIEXPORT jstring JNICALL
Java_team_holder_android_HolderNative_nativeDatabaseRebuild(
    JNIEnv* env,
    jobject /* thiz */,
    jstring data_dir,
    jstring schema_sql,
    jboolean dry_run
) {
  UtfChars data_dir_chars(env, data_dir);
  UtfChars schema_sql_chars(env, schema_sql);
  if (data_dir_chars.get() == nullptr || schema_sql_chars.get() == nullptr) {
    throw_runtime(env, "data_dir and schema_sql must not be null");
    return nullptr;
  }

  char* json = nullptr;
  holder_error* error = nullptr;
  const int rc = holder_database_rebuild(
      data_dir_chars.get(), schema_sql_chars.get(), dry_run == JNI_TRUE ? 1 : 0,
      &json, &error
  );
  if (rc != HOLDER_OK) {
    throw_runtime(env, error_message(error));
    return nullptr;
  }
  return string_result(env, json);
}

extern "C" JNIEXPORT jstring JNICALL
Java_team_holder_android_HolderNative_nativeEnsureDefaultProject(
    JNIEnv* env,
    jobject /* thiz */,
    jlong context_handle,
    jstring name,
    jstring welcome_title,
    jstring welcome_content
) {
  holder_context* context = context_from_handle(env, context_handle);
  if (context == nullptr) {
    return nullptr;
  }

  UtfChars name_chars(env, name);
  UtfChars welcome_title_chars(env, welcome_title);
  UtfChars welcome_content_chars(env, welcome_content);
  if (name_chars.get() == nullptr || welcome_title_chars.get() == nullptr) {
    throw_runtime(env, "name and welcome_title must not be null");
    return nullptr;
  }

  char* json = nullptr;
  holder_error* error = nullptr;
  const int rc = holder_ensure_default_project(
      context,
      name_chars.get(),
      welcome_title_chars.get(),
      welcome_content_chars.get(),
      &json,
      &error
  );
  if (rc != HOLDER_OK) {
    throw_runtime(env, error_message(error));
    return nullptr;
  }
  return string_result(env, json);
}

extern "C" JNIEXPORT jstring JNICALL
Java_team_holder_android_HolderNative_nativeBackupSnapshotPage(
    JNIEnv* env,
    jobject /* thiz */,
    jlong context_handle,
    jstring project_id,
    jlong cursor_updated_at,
    jstring cursor_card_id,
    jint limit
) {
  holder_context* context = context_from_handle(env, context_handle);
  if (context == nullptr) {
    return nullptr;
  }

  UtfChars project_id_chars(env, project_id);
  UtfChars cursor_card_id_chars(env, cursor_card_id);
  if (project_id_chars.get() == nullptr) {
    throw_runtime(env, "project_id must not be null");
    return nullptr;
  }

  char* json = nullptr;
  holder_error* error = nullptr;
  const int rc = holder_backup_snapshot_page(
      context,
      project_id_chars.get(),
      static_cast<long long>(cursor_updated_at),
      cursor_card_id_chars.get(),
      static_cast<int>(limit),
      &json,
      &error
  );
  if (rc != HOLDER_OK) {
    throw_runtime(env, error_message(error));
    return nullptr;
  }
  return string_result(env, json);
}

extern "C" JNIEXPORT jstring JNICALL
Java_team_holder_android_HolderNative_nativeBackupRestore(
    JNIEnv* env,
    jobject /* thiz */,
    jlong context_handle,
    jstring project_name,
    jstring privacy_mode,
    jstring cards_json,
    jstring commit_message
) {
  holder_context* context = context_from_handle(env, context_handle);
  if (context == nullptr) {
    return nullptr;
  }

  UtfChars project_name_chars(env, project_name);
  UtfChars privacy_mode_chars(env, privacy_mode);
  UtfChars cards_json_chars(env, cards_json);
  UtfChars commit_message_chars(env, commit_message);
  if (project_name_chars.get() == nullptr || cards_json_chars.get() == nullptr ||
      commit_message_chars.get() == nullptr) {
    throw_runtime(env, "project_name, cards_json and commit_message must not be null");
    return nullptr;
  }

  char* json = nullptr;
  holder_error* error = nullptr;
  const int rc = holder_backup_restore(
      context,
      project_name_chars.get(),
      privacy_mode_chars.get(),
      cards_json_chars.get(),
      commit_message_chars.get(),
      &json,
      &error
  );
  if (rc != HOLDER_OK) {
    throw_runtime(env, error_message(error));
    return nullptr;
  }
  return string_result(env, json);
}

extern "C" JNIEXPORT jstring JNICALL
Java_team_holder_android_HolderNative_nativeCardListEditableTags(
    JNIEnv* env,
    jobject /* thiz */,
    jlong context_handle,
    jstring card_id
) {
  holder_context* context = context_from_handle(env, context_handle);
  if (context == nullptr) {
    return nullptr;
  }

  UtfChars card_id_chars(env, card_id);
  if (card_id_chars.get() == nullptr) {
    throw_runtime(env, "card_id must not be null");
    return nullptr;
  }

  char* json = nullptr;
  holder_error* error = nullptr;
  const int rc = holder_card_list_editable_tags(context, card_id_chars.get(), &json, &error);
  if (rc != HOLDER_OK) {
    throw_runtime(env, error_message(error));
    return nullptr;
  }
  return string_result(env, json);
}

extern "C" JNIEXPORT jstring JNICALL
Java_team_holder_android_HolderNative_nativeLocationList(
    JNIEnv* env,
    jobject /* thiz */,
    jlong context_handle,
    jstring project_id
) {
  holder_context* context = context_from_handle(env, context_handle);
  if (context == nullptr) {
    return nullptr;
  }

  UtfChars project_id_chars(env, project_id);
  if (project_id_chars.get() == nullptr) {
    throw_runtime(env, "project_id must not be null");
    return nullptr;
  }

  char* json = nullptr;
  holder_error* error = nullptr;
  const int rc = holder_location_list(context, project_id_chars.get(), &json, &error);
  if (rc != HOLDER_OK) {
    throw_runtime(env, error_message(error));
    return nullptr;
  }
  return string_result(env, json);
}

extern "C" JNIEXPORT jstring JNICALL
Java_team_holder_android_HolderNative_nativeAssetImportFile(
    JNIEnv* env,
    jobject /* thiz */,
    jlong context_handle,
    jstring project_id,
    jstring card_id,
    jstring location_id,
    jstring source_file_path
) {
  holder_context* context = context_from_handle(env, context_handle);
  if (context == nullptr) {
    return nullptr;
  }

  UtfChars project_id_chars(env, project_id);
  UtfChars card_id_chars(env, card_id);
  UtfChars location_id_chars(env, location_id);
  UtfChars source_file_path_chars(env, source_file_path);
  if (project_id_chars.get() == nullptr || card_id_chars.get() == nullptr ||
      location_id_chars.get() == nullptr || source_file_path_chars.get() == nullptr) {
    throw_runtime(env, "project_id, card_id, location_id, and source_file_path must not be null");
    return nullptr;
  }

  char* json = nullptr;
  holder_error* error = nullptr;
  const int rc = holder_asset_import_file(
      context, project_id_chars.get(), card_id_chars.get(), location_id_chars.get(),
      source_file_path_chars.get(), &json, &error
  );
  if (rc != HOLDER_OK) {
    throw_runtime(env, error_message(error));
    return nullptr;
  }
  return string_result(env, json);
}

extern "C" JNIEXPORT void JNICALL
Java_team_holder_android_HolderNative_nativeAssetRetrieve(
    JNIEnv* env,
    jobject /* thiz */,
    jlong context_handle,
    jstring resource_id,
    jstring asset_id,
    jstring placement_id,
    jstring destination_file_path
) {
  holder_context* context = context_from_handle(env, context_handle);
  if (context == nullptr) {
    return;
  }

  UtfChars resource_id_chars(env, resource_id);
  UtfChars asset_id_chars(env, asset_id);
  UtfChars placement_id_chars(env, placement_id);
  UtfChars destination_file_path_chars(env, destination_file_path);
  if (resource_id_chars.get() == nullptr || asset_id_chars.get() == nullptr ||
      placement_id_chars.get() == nullptr || destination_file_path_chars.get() == nullptr) {
    throw_runtime(env, "resource_id, asset_id, placement_id, and destination_file_path must not be null");
    return;
  }

  holder_error* error = nullptr;
  const int rc = holder_asset_retrieve(
      context, resource_id_chars.get(), asset_id_chars.get(), placement_id_chars.get(),
      destination_file_path_chars.get(), &error
  );
  if (rc != HOLDER_OK) {
    throw_runtime(env, error_message(error));
  }
}
