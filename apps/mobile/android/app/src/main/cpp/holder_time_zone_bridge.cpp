// Bridges core's time-zone provider (holder_time_zone_set_provider) to AndroidTimeZones on the
// Kotlin side. Android's C++ library has no time-zone database, so core asks java.time, which uses
// Android's own zone data. See holder/holder.h for the ownership and threading contract.

#include "holder/holder.h"

#include <jni.h>

#include <climits>
#include <cstring>

namespace {

struct TimeZoneBridgeContext {
  JavaVM* jvm = nullptr;
  jclass zones_class = nullptr;  // A global reference.
  jmethodID offset_method = nullptr;
  jmethodID current_method = nullptr;
};

// The callbacks may run on any thread; a thread is detached only by whoever attached it.
class Env {
 public:
  explicit Env(JavaVM* jvm) : jvm_(jvm) {
    const jint rc = jvm_->GetEnv(reinterpret_cast<void**>(&env_), JNI_VERSION_1_6);
    if (rc == JNI_EDETACHED && jvm_->AttachCurrentThread(&env_, nullptr) == JNI_OK) {
      attached_ = true;
    } else if (rc != JNI_OK) {
      env_ = nullptr;
    }
  }
  Env(const Env&) = delete;
  Env& operator=(const Env&) = delete;
  ~Env() {
    if (attached_) jvm_->DetachCurrentThread();
  }
  JNIEnv* get() const { return env_; }

 private:
  JavaVM* jvm_;
  JNIEnv* env_ = nullptr;
  bool attached_ = false;
};

int time_zone_offset(void* user_data, const char* zone, long long utc_seconds, long long* out_offset_seconds) {
  auto* ctx = static_cast<TimeZoneBridgeContext*>(user_data);
  Env env(ctx->jvm);
  if (env.get() == nullptr) return 1;
  jstring name = env.get()->NewStringUTF(zone);
  if (name == nullptr) {
    env.get()->ExceptionClear();
    return 1;
  }
  const jlong offset = env.get()->CallStaticLongMethod(ctx->zones_class, ctx->offset_method, name,
                                                       static_cast<jlong>(utc_seconds));
  env.get()->DeleteLocalRef(name);
  if (env.get()->ExceptionCheck()) {
    env.get()->ExceptionClear();
    return 1;
  }
  if (offset == LLONG_MIN) return 1;  // AndroidTimeZones.UNKNOWN_ZONE
  *out_offset_seconds = offset;
  return 0;
}

int time_zone_current(void* user_data, char* out_zone, size_t capacity) {
  auto* ctx = static_cast<TimeZoneBridgeContext*>(user_data);
  Env env(ctx->jvm);
  if (env.get() == nullptr || capacity == 0) return 1;
  auto name = static_cast<jstring>(env.get()->CallStaticObjectMethod(ctx->zones_class, ctx->current_method));
  if (env.get()->ExceptionCheck() || name == nullptr) {
    env.get()->ExceptionClear();
    return 1;
  }
  const char* chars = env.get()->GetStringUTFChars(name, nullptr);
  if (chars == nullptr) {
    env.get()->ExceptionClear();
    env.get()->DeleteLocalRef(name);
    return 1;
  }
  const bool fits = std::strlen(chars) < capacity;
  if (fits) std::strcpy(out_zone, chars);
  env.get()->ReleaseStringUTFChars(name, chars);
  env.get()->DeleteLocalRef(name);
  return fits ? 0 : 1;
}

void time_zone_destroy(void* user_data) {
  auto* ctx = static_cast<TimeZoneBridgeContext*>(user_data);
  {
    Env env(ctx->jvm);
    if (env.get() != nullptr) env.get()->DeleteGlobalRef(ctx->zones_class);
  }
  delete ctx;
}

} // namespace

extern "C" JNIEXPORT jint JNICALL
Java_team_holder_android_time_AndroidTimeZones_nativeRegisterProvider(JNIEnv* env, jclass clazz) {
  auto* ctx = new TimeZoneBridgeContext();
  env->GetJavaVM(&ctx->jvm);
  ctx->zones_class = static_cast<jclass>(env->NewGlobalRef(clazz));
  ctx->offset_method = env->GetStaticMethodID(clazz, "offsetSeconds", "(Ljava/lang/String;J)J");
  ctx->current_method = env->GetStaticMethodID(clazz, "currentZone", "()Ljava/lang/String;");
  if (ctx->offset_method == nullptr || ctx->current_method == nullptr) {
    env->ExceptionClear();
    env->DeleteGlobalRef(ctx->zones_class);
    delete ctx;
    return HOLDER_ERROR_RUNTIME;
  }
  holder_error* error = nullptr;
  // From here core owns ctx and releases it exactly once, whatever the result.
  const int rc = holder_time_zone_set_provider(&time_zone_offset, &time_zone_current, ctx, &time_zone_destroy, &error);
  if (error != nullptr) holder_error_destroy(error);
  return rc;
}
