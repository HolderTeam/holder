// Generated from contract/interface/resources/models.py and contract/interface/resources/operations.py by holder_contract.
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

const char* encoding(holder_asset_placement_encoding value) {
  switch (value) {
  case HOLDER_ASSET_PLACEMENT_ENCODING_PLAIN: return "plain";
  case HOLDER_ASSET_PLACEMENT_ENCODING_HOLDER_ASSET_V1: return "holder_asset_v1";
  default: return nullptr;
  }
}

struct JavaTypes {
  JNIEnv* env;
  jclass list = env->FindClass("java/util/ArrayList");
  jmethodID list_ctor = env->GetMethodID(list, "<init>", "(I)V");
  jmethodID add = env->GetMethodID(list, "add", "(Ljava/lang/Object;)Z");
  jclass placement = env->FindClass("team/holder/android/generated/ContractAssetPlacement");
  jmethodID placement_ctor = env->GetMethodID(placement, "<init>",
      "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;JJ)V");
  jclass asset = env->FindClass("team/holder/android/generated/ContractResourceAsset");
  jmethodID asset_ctor = env->GetMethodID(asset, "<init>",
      "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;JLjava/lang/String;Ljava/util/List;JJ)V");
  jclass resource = env->FindClass("team/holder/android/generated/ContractResource");
  jmethodID resource_ctor = env->GetMethodID(resource, "<init>",
      "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/util/List;JJ)V");
  ~JavaTypes() {
    env->DeleteLocalRef(list);
    env->DeleteLocalRef(placement);
    env->DeleteLocalRef(asset);
    env->DeleteLocalRef(resource);
  }
  jobject array(size_t count) { return env->NewObject(list, list_ctor, static_cast<jint>(count)); }
};

jobject placement(JavaTypes& types, const holder_asset_placement& value) {
  JNIEnv* env = types.env;
  const char* stored_as = encoding(value.encoding);
  if (!stored_as) { fail(env, "unsupported asset placement encoding"); return nullptr; }
  jstring id = string(env, value.placement_id);
  jstring location = string(env, value.location_id);
  jstring format = string(env, stored_as);
  jobject out = env->ExceptionCheck() ? nullptr : env->NewObject(types.placement, types.placement_ctor,
      id, location, format, static_cast<jlong>(value.stored_byte_size),
      static_cast<jlong>(value.created_at));
  env->DeleteLocalRef(id);
  env->DeleteLocalRef(location);
  env->DeleteLocalRef(format);
  return out;
}

jobject asset(JavaTypes& types, const holder_resource_asset& value) {
  JNIEnv* env = types.env;
  jstring id = string(env, value.asset_id);
  jstring filename = string(env, value.original_filename);
  jstring media = string(env, value.media_type);
  jstring hash = string(env, value.plaintext_sha256);
  jobject placements = types.array(value.placements.count);
  for (size_t i = 0; i < value.placements.count && !env->ExceptionCheck(); ++i) {
    jobject item = placement(types, value.placements.items[i]);
    if (!env->ExceptionCheck()) env->CallBooleanMethod(placements, types.add, item);
    if (item) env->DeleteLocalRef(item);
  }
  jobject out = env->ExceptionCheck() ? nullptr : env->NewObject(types.asset, types.asset_ctor,
      id, filename, media, static_cast<jlong>(value.byte_size), hash, placements,
      static_cast<jlong>(value.created_at), static_cast<jlong>(value.updated_at));
  env->DeleteLocalRef(id);
  env->DeleteLocalRef(filename);
  env->DeleteLocalRef(media);
  env->DeleteLocalRef(hash);
  env->DeleteLocalRef(placements);
  return out;
}

jobject resource(JavaTypes& types, const holder_resource& value) {
  JNIEnv* env = types.env;
  jstring id = string(env, value.resource_id);
  jstring project = string(env, value.project_id);
  jstring kind = string(env, value.type);
  jstring label = string(env, value.label);
  jobject assets = types.array(value.assets.count);
  for (size_t i = 0; i < value.assets.count && !env->ExceptionCheck(); ++i) {
    jobject item = asset(types, value.assets.items[i]);
    if (!env->ExceptionCheck()) env->CallBooleanMethod(assets, types.add, item);
    if (item) env->DeleteLocalRef(item);
  }
  jobject out = env->ExceptionCheck() ? nullptr : env->NewObject(types.resource, types.resource_ctor,
      id, project, kind, label, assets, static_cast<jlong>(value.created_at),
      static_cast<jlong>(value.updated_at));
  env->DeleteLocalRef(id);
  env->DeleteLocalRef(project);
  env->DeleteLocalRef(kind);
  env->DeleteLocalRef(label);
  env->DeleteLocalRef(assets);
  return out;
}
}  // namespace

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_generated_ContractResources_nativeListResources(
    JNIEnv* env, jobject, jlong context_handle, jstring project_id) {
  if (!context_handle || !project_id) {
    fail(env, "context and project ID are required"); return nullptr;
  }
  std::string project;
  if (!utf8(env, project_id, project)) return nullptr;
  Arena arena;
  const holder_resource_list* resources = nullptr;
  holder_error* error = nullptr;
  const int status = arena.value ? holder_list_resources(
      reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      project.c_str(), arena.value, &resources, &error) : HOLDER_ERROR_ALLOCATION;
  if (status != HOLDER_OK) {
    std::string message = error ? holder_error_message(error) : "cannot allocate resource list";
    if (error) holder_error_destroy(error);
    fail(env, message.c_str()); return nullptr;
  }
  JavaTypes types{env};
  if (env->ExceptionCheck()) return nullptr;
  jobject items = types.array(resources->count);
  for (size_t i = 0; i < resources->count && !env->ExceptionCheck(); ++i) {
    jobject item = resource(types, resources->items[i]);
    if (!env->ExceptionCheck()) env->CallBooleanMethod(items, types.add, item);
    if (item) env->DeleteLocalRef(item);
  }
  return items;
}
