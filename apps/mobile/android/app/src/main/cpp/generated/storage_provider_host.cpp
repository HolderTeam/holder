// Generated from contract/interface by compiler. Do not edit; run make -C contract update.
#include "holder/holder_api.h"
#include <jni.h>

#include <cstdlib>
#include <cstring>
#include <new>

namespace {
struct StorageContext {
  JavaVM* vm = nullptr;
  jobject provider = nullptr;
  jclass failure_class = nullptr;
  jmethodID put = nullptr;
  jmethodID get = nullptr;
  jmethodID exists = nullptr;
  jmethodID remove = nullptr;
  jmethodID failure_code = nullptr;
  jmethodID failure_message = nullptr;
};

class AttachedEnv {
 public:
  explicit AttachedEnv(JavaVM* vm) : vm_(vm) {
    const jint status = vm->GetEnv(reinterpret_cast<void**>(&env_), JNI_VERSION_1_6);
    if (status == JNI_EDETACHED) {
      if (vm->AttachCurrentThread(&env_, nullptr) == JNI_OK) attached_ = true;
      else env_ = nullptr;
    } else if (status != JNI_OK) env_ = nullptr;
  }
  ~AttachedEnv() { if (attached_) vm_->DetachCurrentThread(); }
  JNIEnv* get() const { return env_; }
 private:
  JavaVM* vm_;
  JNIEnv* env_ = nullptr;
  bool attached_ = false;
};

char* copy_text(const char* text) {
  const size_t size = std::strlen(text) + 1;
  auto* copy = static_cast<char*>(std::malloc(size));
  if (copy) std::memcpy(copy, text, size);
  return copy;
}

int fail(JNIEnv* env, StorageContext* ctx, int* out_code, char** out_error, const char* fallback) {
  *out_code = HOLDER_STORAGE_ERROR_UNAVAILABLE;
  const char* message = fallback;
  jthrowable thrown = env ? env->ExceptionOccurred() : nullptr;
  if (thrown) {
    env->ExceptionClear();
    if (env->IsInstanceOf(thrown, ctx->failure_class)) {
      const jint code = env->CallIntMethod(thrown, ctx->failure_code);
      if (!env->ExceptionCheck() && code >= HOLDER_STORAGE_ERROR_UNAVAILABLE &&
          code <= HOLDER_STORAGE_ERROR_TRANSIENT) *out_code = code;
    }
    if (env->ExceptionCheck()) env->ExceptionClear();
    auto detail = static_cast<jstring>(env->CallObjectMethod(thrown, ctx->failure_message));
    if (!env->ExceptionCheck() && detail) {
      const char* chars = env->GetStringUTFChars(detail, nullptr);
      if (chars) {
        *out_error = copy_text(chars);
        env->ReleaseStringUTFChars(detail, chars);
      }
      env->DeleteLocalRef(detail);
    }
    if (env->ExceptionCheck()) env->ExceptionClear();
    env->DeleteLocalRef(thrown);
  }
  if (!*out_error) *out_error = copy_text(message);
  return 1;
}

jstring name(JNIEnv* env, const char* value) { return env->NewStringUTF(value ? value : ""); }

int put(void* user_data, const char* key, const char* staged, int64_t size, const char* sha256,
        int* out_code, char** out_error) {
  auto* ctx = static_cast<StorageContext*>(user_data);
  AttachedEnv attached(ctx->vm);
  JNIEnv* env = attached.get();
  if (!env) return fail(nullptr, ctx, out_code, out_error, "could not attach JNI thread");
  jstring j_key = name(env, key), j_staged = name(env, staged), j_hash = name(env, sha256);
  if (j_key && j_staged && j_hash && !env->ExceptionCheck())
    env->CallVoidMethod(ctx->provider, ctx->put, j_key, j_staged, static_cast<jlong>(size), j_hash);
  const bool failed = env->ExceptionCheck() || !j_key || !j_staged || !j_hash;
  const int status = failed ? fail(env, ctx, out_code, out_error, "storage put failed") : 0;
  if (j_hash) env->DeleteLocalRef(j_hash);
  if (j_staged) env->DeleteLocalRef(j_staged);
  if (j_key) env->DeleteLocalRef(j_key);
  return status;
}

int get(void* user_data, const char* key, const char* destination,
        int* out_code, char** out_error) {
  auto* ctx = static_cast<StorageContext*>(user_data);
  AttachedEnv attached(ctx->vm);
  JNIEnv* env = attached.get();
  if (!env) return fail(nullptr, ctx, out_code, out_error, "could not attach JNI thread");
  jstring j_key = name(env, key), j_destination = name(env, destination);
  if (j_key && j_destination && !env->ExceptionCheck())
    env->CallVoidMethod(ctx->provider, ctx->get, j_key, j_destination);
  const bool failed = env->ExceptionCheck() || !j_key || !j_destination;
  const int status = failed ? fail(env, ctx, out_code, out_error, "storage get failed") : 0;
  if (j_destination) env->DeleteLocalRef(j_destination);
  if (j_key) env->DeleteLocalRef(j_key);
  return status;
}

int exists(void* user_data, const char* key, int* out_value,
           int* out_code, char** out_error) {
  auto* ctx = static_cast<StorageContext*>(user_data);
  AttachedEnv attached(ctx->vm);
  JNIEnv* env = attached.get();
  if (!env) return fail(nullptr, ctx, out_code, out_error, "could not attach JNI thread");
  jstring j_key = name(env, key);
  jboolean found = JNI_FALSE;
  if (j_key && !env->ExceptionCheck()) found = env->CallBooleanMethod(ctx->provider, ctx->exists, j_key);
  const bool failed = env->ExceptionCheck() || !j_key;
  const int status = failed ? fail(env, ctx, out_code, out_error, "storage exists failed") : 0;
  if (j_key) env->DeleteLocalRef(j_key);
  if (!failed) *out_value = found ? 1 : 0;
  return status;
}

int remove(void* user_data, const char* key, int* out_code, char** out_error) {
  auto* ctx = static_cast<StorageContext*>(user_data);
  AttachedEnv attached(ctx->vm);
  JNIEnv* env = attached.get();
  if (!env) return fail(nullptr, ctx, out_code, out_error, "could not attach JNI thread");
  jstring j_key = name(env, key);
  if (j_key && !env->ExceptionCheck()) env->CallVoidMethod(ctx->provider, ctx->remove, j_key);
  const bool failed = env->ExceptionCheck() || !j_key;
  const int status = failed ? fail(env, ctx, out_code, out_error, "storage remove failed") : 0;
  if (j_key) env->DeleteLocalRef(j_key);
  return status;
}

void destroy(void* user_data) {
  auto* ctx = static_cast<StorageContext*>(user_data);
  AttachedEnv attached(ctx->vm);
  if (JNIEnv* env = attached.get()) {
    env->DeleteGlobalRef(ctx->provider);
    env->DeleteGlobalRef(ctx->failure_class);
  }
  delete ctx;
}
} // namespace

extern "C" JNIEXPORT void JNICALL
Java_team_holder_android_contract_StorageHosts_registerStorageProvider(
    JNIEnv* env, jclass, jobject kind, jobject provider) {
  if (!kind || !provider) {
    env->ThrowNew(env->FindClass("java/lang/IllegalArgumentException"), "kind and provider are required");
    return;
  }
  // The Kotlin enum lists the C enum's members in order, and the C enum starts at one.
  jint ordinal = -1;
  if (jclass kind_class = env->GetObjectClass(kind)) {
    if (jmethodID method = env->GetMethodID(kind_class, "ordinal", "()I")) ordinal = env->CallIntMethod(kind, method);
    env->DeleteLocalRef(kind_class);
  }
  if (env->ExceptionCheck() || ordinal < 0) {
    env->ExceptionClear();
    env->ThrowNew(env->FindClass("java/lang/IllegalArgumentException"), "invalid kind");
    return;
  }
  const auto c_kind = static_cast<holder_register_storage_provider_kind>(ordinal + 1);
  auto* ctx = new (std::nothrow) StorageContext();
  if (!ctx) { env->ThrowNew(env->FindClass("java/lang/OutOfMemoryError"), "storage context allocation failed"); return; }
  env->GetJavaVM(&ctx->vm);
  ctx->provider = env->NewGlobalRef(provider);
  jclass type = env->GetObjectClass(provider);
  if (type && !env->ExceptionCheck()) {
    ctx->put = env->GetMethodID(type, "put", "(Ljava/lang/String;Ljava/lang/String;JLjava/lang/String;)V");
    ctx->get = env->GetMethodID(type, "get", "(Ljava/lang/String;Ljava/lang/String;)V");
    ctx->exists = env->GetMethodID(type, "exists", "(Ljava/lang/String;)Z");
    ctx->remove = env->GetMethodID(type, "remove", "(Ljava/lang/String;)V");
    env->DeleteLocalRef(type);
  }
  jclass failure = env->FindClass("team/holder/android/contract/StorageFailure");
  if (failure && !env->ExceptionCheck()) {
    ctx->failure_class = static_cast<jclass>(env->NewGlobalRef(failure));
    ctx->failure_code = env->GetMethodID(failure, "getCode", "()I");
    ctx->failure_message = env->GetMethodID(failure, "getMessage", "()Ljava/lang/String;");
    env->DeleteLocalRef(failure);
  }
  if (env->ExceptionCheck() || !ctx->provider || !ctx->failure_class || !ctx->put || !ctx->get ||
      !ctx->exists || !ctx->remove || !ctx->failure_code || !ctx->failure_message) {
    env->ExceptionClear();
    if (ctx->provider) env->DeleteGlobalRef(ctx->provider);
    if (ctx->failure_class) env->DeleteGlobalRef(ctx->failure_class);
    delete ctx;
    env->ThrowNew(env->FindClass("java/lang/IllegalArgumentException"), "invalid storage provider");
    return;
  }
  const holder_storage_provider_callbacks callbacks{put, get, exists, remove};
  holder_error* error = nullptr;
  // Core owns ctx from this call onward, including on failure.
  const int status = holder_register_storage_provider(c_kind, &callbacks, ctx, destroy, &error);
  if (status != HOLDER_OK) {
    env->ThrowNew(env->FindClass("java/lang/IllegalStateException"),
                  error ? holder_error_message(error) : "storage provider registration failed");
  }
  if (error) holder_error_destroy(error);
}
