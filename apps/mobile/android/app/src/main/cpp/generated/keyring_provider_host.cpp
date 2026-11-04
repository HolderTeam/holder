// Generated from contract/interface by compiler. Do not edit; run make -C contract update.
// Bridges holder-core's platform keyring seam to
// AndroidKeyringStore on the Kotlin side, backed by Keystore-wrapped
// EncryptedSharedPreferences -- Android's substitute for the desktop platform
// keyrings (libsecret/Keychain/Windows Credential Manager) holder-core otherwise
// relies on for encrypted_git project keys and other secrets. See holder/holder_api.h
// for the C ABI's exact ownership/threading contract this implements against; it
// is the same contract the generated SSH signer adapter implements.

#include "holder/holder_api.h"

#include <jni.h>

#include <cstdlib>
#include <cstring>
#include <new>
#include <string>

namespace {

// Owns everything the lookup/store/remove/destroy callbacks below need for as
// long as holder-core keeps this provider installed -- which may span many
// keyring operations, on different threads, over the app's lifetime.
// Constructed at registration; destroyed exactly once by keyring_destroy.
struct KeyringBridgeContext {
  JavaVM* jvm = nullptr;
  jobject store_global_ref = nullptr;
  jmethodID lookup_method = nullptr;
  jmethodID store_method = nullptr;
  jmethodID remove_method = nullptr;
};

// JNIEnv pointers are thread-local, so each callback obtains its own:
// JNIEnv* is thread-local, these callbacks can run on any thread over the
// provider's lifetime, and a thread must only be detached by whoever attached
// it.
JNIEnv* get_env_attaching_if_needed(JavaVM* jvm, bool* out_attached_here) {
  *out_attached_here = false;
  JNIEnv* env = nullptr;
  const jint rc = jvm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
  if (rc == JNI_OK) {
    return env;
  }
  if (rc == JNI_EDETACHED) {
    if (jvm->AttachCurrentThread(&env, nullptr) == JNI_OK) {
      *out_attached_here = true;
      return env;
    }
  }
  return nullptr;
}

void detach_if_attached_here(JavaVM* jvm, bool attached_here) {
  if (attached_here) {
    jvm->DetachCurrentThread();
  }
}

char* malloc_dup(const std::string& value) {
  auto* buf = static_cast<char*>(malloc(value.size() + 1));
  if (buf == nullptr) return nullptr;
  memcpy(buf, value.c_str(), value.size() + 1);
  return buf;
}

void capture_exception(JNIEnv* env, char** out_error) {
  jthrowable failure = env->ExceptionOccurred();
  if (!failure) return;
  env->ExceptionClear();
  jclass type = env->GetObjectClass(failure);
  jmethodID to_string = type ? env->GetMethodID(type, "toString", "()Ljava/lang/String;") : nullptr;
  jstring description = to_string ? static_cast<jstring>(env->CallObjectMethod(failure, to_string)) : nullptr;
  if (description && !env->ExceptionCheck()) {
    const char* chars = env->GetStringUTFChars(description, nullptr);
    if (chars) {
      *out_error = malloc_dup(chars);
      env->ReleaseStringUTFChars(description, chars);
    }
  }
  if (env->ExceptionCheck()) env->ExceptionClear();
  if (description) env->DeleteLocalRef(description);
  if (type) env->DeleteLocalRef(type);
  env->DeleteLocalRef(failure);
}

int keyring_lookup(
    void* user_data,
    int32_t kind,
    const char* service,
    const char* account,
    const char* project_id,
    char** out_value,
    char** out_error
) {
  auto* ctx = static_cast<KeyringBridgeContext*>(user_data);
  *out_value = nullptr;

  bool attached_here = false;
  JNIEnv* env = get_env_attaching_if_needed(ctx->jvm, &attached_here);
  if (env == nullptr) return -1;

  int result = -1;
  jstring j_service = env->NewStringUTF(service != nullptr ? service : "");
  jstring j_account = env->NewStringUTF(account != nullptr ? account : "");
  jstring j_project_id = project_id != nullptr ? env->NewStringUTF(project_id) : nullptr;

  auto j_secret = static_cast<jstring>(env->CallObjectMethod(
      ctx->store_global_ref,
      ctx->lookup_method,
      static_cast<jint>(kind),
      j_service,
      j_account,
      j_project_id
  ));

  if (env->ExceptionCheck()) {
    capture_exception(env, out_error);
  } else {
    result = 0;
    if (j_secret != nullptr) {
      const char* chars = env->GetStringUTFChars(j_secret, nullptr);
      if (chars != nullptr) {
        *out_value = malloc_dup(std::string(chars));
        if (*out_value == nullptr) result = -1;
        env->ReleaseStringUTFChars(j_secret, chars);
      } else {
        env->ExceptionClear();
        result = -1;
      }
    }
  }

  if (j_secret != nullptr) env->DeleteLocalRef(j_secret);
  if (j_project_id != nullptr) env->DeleteLocalRef(j_project_id);
  env->DeleteLocalRef(j_account);
  env->DeleteLocalRef(j_service);

  detach_if_attached_here(ctx->jvm, attached_here);
  return result;
}

int keyring_store(
    void* user_data,
    int32_t kind,
    const char* service,
    const char* account,
    const char* project_id,
    const char* label,
    const char* secret,
    char** out_error
) {
  auto* ctx = static_cast<KeyringBridgeContext*>(user_data);

  bool attached_here = false;
  JNIEnv* env = get_env_attaching_if_needed(ctx->jvm, &attached_here);
  if (env == nullptr) return -1;

  int result = -1;
  jstring j_service = env->NewStringUTF(service != nullptr ? service : "");
  jstring j_account = env->NewStringUTF(account != nullptr ? account : "");
  jstring j_project_id = project_id != nullptr ? env->NewStringUTF(project_id) : nullptr;
  jstring j_label = env->NewStringUTF(label != nullptr ? label : "");
  jstring j_secret = env->NewStringUTF(secret != nullptr ? secret : "");

  env->CallVoidMethod(
      ctx->store_global_ref,
      ctx->store_method,
      static_cast<jint>(kind),
      j_service,
      j_account,
      j_project_id,
      j_label,
      j_secret
  );

  if (env->ExceptionCheck()) {
    capture_exception(env, out_error);
  } else {
    result = 0;
  }

  env->DeleteLocalRef(j_secret);
  env->DeleteLocalRef(j_label);
  if (j_project_id != nullptr) env->DeleteLocalRef(j_project_id);
  env->DeleteLocalRef(j_account);
  env->DeleteLocalRef(j_service);

  detach_if_attached_here(ctx->jvm, attached_here);
  return result;
}

int keyring_remove(
    void* user_data,
    int32_t kind,
    const char* service,
    const char* account,
    const char* project_id,
    char** out_error
) {
  auto* ctx = static_cast<KeyringBridgeContext*>(user_data);

  bool attached_here = false;
  JNIEnv* env = get_env_attaching_if_needed(ctx->jvm, &attached_here);
  if (env == nullptr) return -1;

  int result = -1;
  jstring j_service = env->NewStringUTF(service != nullptr ? service : "");
  jstring j_account = env->NewStringUTF(account != nullptr ? account : "");
  jstring j_project_id = project_id != nullptr ? env->NewStringUTF(project_id) : nullptr;

  env->CallVoidMethod(
      ctx->store_global_ref,
      ctx->remove_method,
      static_cast<jint>(kind),
      j_service,
      j_account,
      j_project_id
  );

  if (env->ExceptionCheck()) {
    capture_exception(env, out_error);
  } else {
    result = 0;
  }

  if (j_project_id != nullptr) env->DeleteLocalRef(j_project_id);
  env->DeleteLocalRef(j_account);
  env->DeleteLocalRef(j_service);

  detach_if_attached_here(ctx->jvm, attached_here);
  return result;
}

void keyring_destroy(void* user_data) {
  auto* ctx = static_cast<KeyringBridgeContext*>(user_data);

  bool attached_here = false;
  JNIEnv* env = get_env_attaching_if_needed(ctx->jvm, &attached_here);
  if (env != nullptr) {
    env->DeleteGlobalRef(ctx->store_global_ref);
    detach_if_attached_here(ctx->jvm, attached_here);
  }
  // If we couldn't even attach, the JVM is likely shutting down; leaking one
  // global ref in that case is preferable to crashing.

  delete ctx;
}

} // namespace

// Installs a Kotlin KeyringProvider as core's process-wide provider.
extern "C" JNIEXPORT void JNICALL
Java_team_holder_android_contract_KeyringHosts_registerKeyringProvider(
    JNIEnv* env,
    jclass,
    jobject provider
) {
  if (provider == nullptr) {
    env->ThrowNew(env->FindClass("java/lang/IllegalArgumentException"), "provider is required");
    return;
  }
  auto* ctx = new (std::nothrow) KeyringBridgeContext();
  if (!ctx) {
    env->ThrowNew(env->FindClass("java/lang/OutOfMemoryError"), "keyring context allocation failed");
    return;
  }
  env->GetJavaVM(&ctx->jvm);
  ctx->store_global_ref = env->NewGlobalRef(provider);

  jclass clazz = env->GetObjectClass(provider);
  if (clazz) ctx->lookup_method = env->GetMethodID(
      clazz,
      "lookup",
      "(ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;"
  );
  if (clazz && !env->ExceptionCheck()) ctx->store_method = env->GetMethodID(
      clazz,
      "store",
      "(ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V"
  );
  if (clazz && !env->ExceptionCheck()) ctx->remove_method =
      env->GetMethodID(clazz, "remove", "(ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
  if (clazz) env->DeleteLocalRef(clazz);

  if (ctx->store_global_ref == nullptr || ctx->lookup_method == nullptr ||
      ctx->store_method == nullptr || ctx->remove_method == nullptr) {
    env->ExceptionClear();
    if (ctx->store_global_ref) env->DeleteGlobalRef(ctx->store_global_ref);
    delete ctx;
    env->ThrowNew(env->FindClass("java/lang/IllegalArgumentException"), "invalid keyring provider");
    return;
  }

  holder_error* error = nullptr;
  // Per holder_register_keyring_provider's contract, ctx is always released exactly
  // once from this point on -- by holder-core, regardless of the return code --
  // so there is nothing to clean up here on failure.
  const holder_keyring_provider_callbacks callbacks{keyring_lookup, keyring_store, keyring_remove};
  const int rc = holder_register_keyring_provider(&callbacks, ctx, &keyring_destroy, &error);
  if (rc != HOLDER_OK) {
    env->ThrowNew(env->FindClass("java/lang/IllegalStateException"),
                  error != nullptr ? holder_error_message(error) : "keyring registration failed");
  }
  if (error != nullptr) holder_error_destroy(error);
}
