// Debug-only JNI probe: call the public C API from a native-created thread.
#include "holder/holder_api.h"
#include <jni.h>

#include <cstdint>
#include <string>
#include <thread>

extern "C" JNIEXPORT jint JNICALL
Java_team_holder_android_HostAdapterNativeProbe_calendarFromNativeThread(
    JNIEnv* env, jclass, jlong context_handle, jstring project_id) {
  if (!project_id || !context_handle) return HOLDER_ERROR_INVALID_ARGUMENT;
  const char* project_chars = env->GetStringUTFChars(project_id, nullptr);
  if (!project_chars) return HOLDER_ERROR_ALLOCATION;
  const std::string project(project_chars);
  env->ReleaseStringUTFChars(project_id, project_chars);

  int result = HOLDER_ERROR_RUNTIME;
  std::thread worker([&] {
    holder_arena* arena = holder_arena_new();
    if (!arena) { result = HOLDER_ERROR_ALLOCATION; return; }
    const holder_calendar* calendar = nullptr;
    holder_error* error = nullptr;
    result = holder_get_calendar(
        reinterpret_cast<holder_context*>(static_cast<uintptr_t>(context_handle)),
        project.c_str(), "2026-11-01", "2026-11-02", nullptr,
        arena, &calendar, &error);
    holder_error_destroy(error);
    holder_arena_free(arena);
  });
  worker.join();
  return result;
}
