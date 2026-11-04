// Generated from contract/interface by compiler. Do not edit; run make -C contract update.
#include "holder/holder_api.h"
#include <jni.h>

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <new>

namespace {
struct SignerContext {
  JavaVM* vm = nullptr;
  jobject signer = nullptr;
  jmethodID sign = nullptr;
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

int fail(JNIEnv* env, char** out_error, const char* message) {
  if (env && env->ExceptionCheck()) env->ExceptionClear();
  if (out_error) {
    const size_t length = std::strlen(message) + 1;
    *out_error = static_cast<char*>(std::malloc(length));
    if (*out_error) std::memcpy(*out_error, message, length);
  }
  return 1;
}

int sign(void* user_data, const unsigned char* data, size_t data_len,
         unsigned char** out_value, size_t* out_value_len, char** out_error) {
  auto* ctx = static_cast<SignerContext*>(user_data);
  AttachedEnv attached(ctx->vm);
  JNIEnv* env = attached.get();
  if (!env) return fail(nullptr, out_error, "could not attach JNI thread");
  if (data_len > static_cast<size_t>(std::numeric_limits<jsize>::max()))
    return fail(env, out_error, "SSH challenge is too large");
  jbyteArray input = env->NewByteArray(static_cast<jsize>(data_len));
  if (!input || env->ExceptionCheck()) return fail(env, out_error, "could not allocate challenge");
  env->SetByteArrayRegion(input, 0, static_cast<jsize>(data_len), reinterpret_cast<const jbyte*>(data));
  if (env->ExceptionCheck()) { env->DeleteLocalRef(input); return fail(env, out_error, "could not copy challenge"); }
  auto signature = static_cast<jbyteArray>(env->CallObjectMethod(ctx->signer, ctx->sign, input));
  env->DeleteLocalRef(input);
  if (env->ExceptionCheck() || !signature) return fail(env, out_error, "SSH signing callback failed");
  const jsize length = env->GetArrayLength(signature);
  if (length <= 0) { env->DeleteLocalRef(signature); return fail(env, out_error, "empty SSH signature"); }
  auto* bytes = static_cast<unsigned char*>(std::malloc(static_cast<size_t>(length)));
  if (!bytes) { env->DeleteLocalRef(signature); return fail(env, out_error, "could not allocate signature"); }
  env->GetByteArrayRegion(signature, 0, length, reinterpret_cast<jbyte*>(bytes));
  env->DeleteLocalRef(signature);
  if (env->ExceptionCheck()) { std::free(bytes); return fail(env, out_error, "could not copy signature"); }
  *out_value = bytes;
  *out_value_len = static_cast<size_t>(length);
  return 0;
}

void destroy(void* user_data) {
  auto* ctx = static_cast<SignerContext*>(user_data);
  AttachedEnv attached(ctx->vm);
  if (JNIEnv* env = attached.get()) env->DeleteGlobalRef(ctx->signer);
  delete ctx;
}
} // namespace

extern "C" JNIEXPORT void JNICALL
Java_team_holder_android_contract_SignerHosts_registerSshSigner(
    JNIEnv* env, jclass, jlong context_handle, jstring username, jbyteArray public_key_blob, jobject signer) {
  if (!context_handle || !username || !public_key_blob || !signer) {
    env->ThrowNew(env->FindClass("java/lang/IllegalArgumentException"), "signer registration fields are required");
    return;
  }
  auto* ctx = new (std::nothrow) SignerContext();
  if (!ctx) { env->ThrowNew(env->FindClass("java/lang/OutOfMemoryError"), "signer context allocation failed"); return; }
  env->GetJavaVM(&ctx->vm);
  ctx->signer = env->NewGlobalRef(signer);
  jclass type = env->GetObjectClass(signer);
  if (type && !env->ExceptionCheck()) ctx->sign = env->GetMethodID(type, "sign", "([B)[B");
  if (type) env->DeleteLocalRef(type);
  if (!ctx->signer || !ctx->sign || env->ExceptionCheck()) {
    env->ExceptionClear();
    if (ctx->signer) env->DeleteGlobalRef(ctx->signer);
    delete ctx;
    env->ThrowNew(env->FindClass("java/lang/IllegalArgumentException"), "invalid SSH signer");
    return;
  }
  const char* name = env->GetStringUTFChars(username, nullptr);
  if (!name) { destroy(ctx); return; }
  const jsize length = env->GetArrayLength(public_key_blob);
  jbyte* bytes = env->GetByteArrayElements(public_key_blob, nullptr);
  if (!bytes) { env->ReleaseStringUTFChars(username, name); destroy(ctx); return; }
  const holder_ssh_signer_callbacks callbacks{sign};
  holder_error* error = nullptr;
  // Core owns ctx from this call onward, including on failure.
  const int status = holder_register_ssh_signer(
      reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)), name,
      reinterpret_cast<const unsigned char*>(bytes), static_cast<size_t>(length),
      &callbacks, ctx, destroy, &error);
  env->ReleaseByteArrayElements(public_key_blob, bytes, JNI_ABORT);
  env->ReleaseStringUTFChars(username, name);
  if (status != HOLDER_OK) {
    env->ThrowNew(env->FindClass("java/lang/IllegalStateException"),
                  error ? holder_error_message(error) : "SSH signer registration failed");
  }
  if (error) holder_error_destroy(error);
}
