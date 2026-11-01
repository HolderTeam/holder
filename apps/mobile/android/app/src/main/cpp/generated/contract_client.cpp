// Generated from contract/interface by holder_contract. Do not edit; run make -C contract update.
#include "holder/holder.h"
#include <jni.h>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

// The generated Kotlin client (team.holder.android.contract) over core's C API: each
// operation converts its Kotlin arguments to the C API's, calls it, and converts its result
// back, or throws the Kotlin exception for the C error code.

namespace {

struct Arena {
  holder_arena* value = holder_arena_new();
  ~Arena() { holder_arena_free(value); }
};

// What a call's C arguments point into, kept until the call returns.
class Keep {
 public:
  const char* text(std::string value) {
    texts_.push_back(std::move(value));
    return texts_.back().c_str();
  }
  template <class T> T* make(std::size_t count = 1) {
    std::shared_ptr<T[]> block(new T[count]());
    T* data = block.get();
    blocks_.push_back(std::move(block));
    return data;
  }

 private:
  std::deque<std::string> texts_;
  std::vector<std::shared_ptr<void>> blocks_;
};

// Local references made while building one object, deleted when it is built.
class Locals {
 public:
  explicit Locals(JNIEnv* env) : env_(env) {}
  ~Locals() {
    for (jobject ref : refs_)
      if (ref) env_->DeleteLocalRef(ref);
  }
  jobject add(jobject ref) {
    refs_.push_back(ref);
    return ref;
  }

 private:
  JNIEnv* env_;
  std::vector<jobject> refs_;
};

jclass global_class(JNIEnv* env, const char* name) {
  jclass local = env->FindClass(name);
  if (!local) return nullptr;
  auto global = static_cast<jclass>(env->NewGlobalRef(local));
  env->DeleteLocalRef(local);
  return global;
}

void throw_class(JNIEnv* env, const char* name, const char* message) {
  if (env->ExceptionCheck()) return;
  jclass type = env->FindClass(name);
  if (type) env->ThrowNew(type, message);
}

void fail(JNIEnv* env, const char* message) {
  throw_class(env, "team/holder/android/contract/InvalidArgumentException", message);
}

void throw_status(JNIEnv* env, int status, holder_error* error) {
  std::string message = error ? holder_error_message(error) : "contract operation failed";
  if (error) holder_error_destroy(error);
  const char* type = "team/holder/android/contract/UnexpectedFailureException";
  switch (status) {
  case 4000: type = "team/holder/android/contract/InvalidArgumentException"; break;
  case 4001: type = "team/holder/android/contract/RecoveryTokenInvalidException"; break;
  case 4002: type = "team/holder/android/contract/RebuildFailedException"; break;
  case 4040: type = "team/holder/android/contract/NotFoundException"; break;
  case 4090: type = "team/holder/android/contract/ConflictException"; break;
  case 4091: type = "team/holder/android/contract/KeyMissingException"; break;
  case 4092: type = "team/holder/android/contract/HierarchyInvalidException"; break;
  case 4093: type = "team/holder/android/contract/HistoryKeyUnavailableException"; break;
  case 4094: type = "team/holder/android/contract/RevisionAmbiguousException"; break;
  case 4095: type = "team/holder/android/contract/NotTrashedException"; break;
  case 4130: type = "team/holder/android/contract/HistoryResponseTooLargeException"; break;
  case 4220: type = "team/holder/android/contract/InvalidMoveException"; break;
  case 5000: type = "team/holder/android/contract/UnexpectedFailureException"; break;
  case 5001: type = "team/holder/android/contract/AllocationFailureException"; break;
  case 5002: type = "team/holder/android/contract/DataCorruptedException"; break;
  case 5030: type = "team/holder/android/contract/KeyringUnavailableException"; break;
  case 5031: type = "team/holder/android/contract/HistoryUnavailableException"; break;
  default: break;
  }
  throw_class(env, type, message.c_str());
}

// NewStringUTF expects modified UTF-8; the contract uses ordinary UTF-8.
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

// A Kotlin string as a C string the call can use, NULL for null.
bool c_text(JNIEnv* env, jstring value, Keep& keep, const char*& out) {
  if (!value) {
    out = nullptr;
    return true;
  }
  std::string text;
  if (!utf8(env, value, text)) return false;
  out = keep.text(std::move(text));
  return true;
}

[[maybe_unused]] jobject box_int32(JNIEnv* env, int32_t value) {
  static const jclass type = global_class(env, "java/lang/Integer");
  static const jmethodID of = env->GetStaticMethodID(type, "valueOf", "(I)Ljava/lang/Integer;");
  return env->CallStaticObjectMethod(type, of, static_cast<jint>(value));
}

[[maybe_unused]] jobject box_int64(JNIEnv* env, int64_t value) {
  static const jclass type = global_class(env, "java/lang/Long");
  static const jmethodID of = env->GetStaticMethodID(type, "valueOf", "(J)Ljava/lang/Long;");
  return env->CallStaticObjectMethod(type, of, static_cast<jlong>(value));
}

[[maybe_unused]] jobject box_float64(JNIEnv* env, double value) {
  static const jclass type = global_class(env, "java/lang/Double");
  static const jmethodID of = env->GetStaticMethodID(type, "valueOf", "(D)Ljava/lang/Double;");
  return env->CallStaticObjectMethod(type, of, static_cast<jdouble>(value));
}

[[maybe_unused]] jobject box_bool(JNIEnv* env, bool value) {
  static const jclass type = global_class(env, "java/lang/Boolean");
  static const jmethodID of = env->GetStaticMethodID(type, "valueOf", "(Z)Ljava/lang/Boolean;");
  return env->CallStaticObjectMethod(type, of, static_cast<jboolean>(value ? JNI_TRUE : JNI_FALSE));
}

[[maybe_unused]] int32_t unbox_int32(JNIEnv* env, jobject value) {
  static const jclass type = global_class(env, "java/lang/Integer");
  static const jmethodID get = env->GetMethodID(type, "intValue", "()I");
  return static_cast<int32_t>(env->CallIntMethod(value, get));
}

[[maybe_unused]] int64_t unbox_int64(JNIEnv* env, jobject value) {
  static const jclass type = global_class(env, "java/lang/Long");
  static const jmethodID get = env->GetMethodID(type, "longValue", "()J");
  return static_cast<int64_t>(env->CallLongMethod(value, get));
}

[[maybe_unused]] double unbox_float64(JNIEnv* env, jobject value) {
  static const jclass type = global_class(env, "java/lang/Double");
  static const jmethodID get = env->GetMethodID(type, "doubleValue", "()D");
  return static_cast<double>(env->CallDoubleMethod(value, get));
}

[[maybe_unused]] bool unbox_bool(JNIEnv* env, jobject value) {
  static const jclass type = global_class(env, "java/lang/Boolean");
  static const jmethodID get = env->GetMethodID(type, "booleanValue", "()Z");
  return env->CallBooleanMethod(value, get) == JNI_TRUE;
}

// A Kotlin enum's position, counting from 0; C counts the same choices from 1.
[[maybe_unused]] int ordinal(JNIEnv* env, jobject value) {
  static const jclass type = global_class(env, "java/lang/Enum");
  static const jmethodID get = env->GetMethodID(type, "ordinal", "()I");
  return static_cast<int>(env->CallIntMethod(value, get));
}

jobject new_list(JNIEnv* env, std::size_t size) {
  static const jclass type = global_class(env, "java/util/ArrayList");
  static const jmethodID make = env->GetMethodID(type, "<init>", "(I)V");
  return env->NewObject(type, make, static_cast<jint>(size));
}

bool list_add(JNIEnv* env, jobject list, jobject item) {
  static const jclass type = global_class(env, "java/util/List");
  static const jmethodID add = env->GetMethodID(type, "add", "(Ljava/lang/Object;)Z");
  env->CallBooleanMethod(list, add, item);
  if (item) env->DeleteLocalRef(item);
  return !env->ExceptionCheck();
}

template <class T, class F>
jobject to_java_list(JNIEnv* env, const T* items, std::size_t count, F convert) {
  jobject list = new_list(env, count);
  if (!list) return nullptr;
  for (std::size_t i = 0; i < count; ++i) {
    jobject item = convert(items[i]);
    if (env->ExceptionCheck() || !list_add(env, list, item)) {
      env->DeleteLocalRef(list);
      return nullptr;
    }
  }
  return list;
}

template <class P, class F>
bool from_java_list(JNIEnv* env, jobject list, Keep& keep, P*& items, std::size_t& count, F convert) {
  using T = std::remove_const_t<P>;
  static const jclass type = global_class(env, "java/util/List");
  static const jmethodID size_of = env->GetMethodID(type, "size", "()I");
  static const jmethodID get = env->GetMethodID(type, "get", "(I)Ljava/lang/Object;");
  const jint size = env->CallIntMethod(list, size_of);
  if (env->ExceptionCheck()) return false;
  T* out = keep.make<T>(static_cast<std::size_t>(size));
  for (jint i = 0; i < size; ++i) {
    jobject item = env->CallObjectMethod(list, get, i);
    if (env->ExceptionCheck()) return false;
    const bool converted = item ? convert(item, out[i]) : (fail(env, "a list item must not be null"), false);
    if (item) env->DeleteLocalRef(item);
    if (!converted) return false;
  }
  items = out;
  count = static_cast<std::size_t>(size);
  return true;
}

jobject new_map(JNIEnv* env) {
  static const jclass type = global_class(env, "java/util/LinkedHashMap");
  static const jmethodID make = env->GetMethodID(type, "<init>", "()V");
  return env->NewObject(type, make);
}

bool map_put(JNIEnv* env, jobject map, jobject key, jobject value) {
  static const jclass type = global_class(env, "java/util/Map");
  static const jmethodID put = env->GetMethodID(type, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
  jobject previous = env->CallObjectMethod(map, put, key, value);
  if (previous) env->DeleteLocalRef(previous);
  if (key) env->DeleteLocalRef(key);
  if (value) env->DeleteLocalRef(value);
  return !env->ExceptionCheck();
}

template <class M> jobject to_java_text_map(JNIEnv* env, const M& value) {
  jobject map = new_map(env);
  if (!map) return nullptr;
  for (std::size_t i = 0; i < value.count; ++i) {
    if (!map_put(env, map, string(env, value.items[i].key), string(env, value.items[i].values))) {
      env->DeleteLocalRef(map);
      return nullptr;
    }
  }
  return map;
}

template <class M> jobject to_java_list_map(JNIEnv* env, const M& value) {
  jobject map = new_map(env);
  if (!map) return nullptr;
  for (std::size_t i = 0; i < value.count; ++i) {
    const auto& values = value.items[i].values;
    jobject list = to_java_list(env, values.items, values.count, [&](const char* text) -> jobject { return string(env, text); });
    if (!list || !map_put(env, map, string(env, value.items[i].key), list)) {
      env->DeleteLocalRef(map);
      return nullptr;
    }
  }
  return map;
}

// Calls convert(key, value) for each entry of a Kotlin map, in its iteration order.
template <class F> bool for_each_entry(JNIEnv* env, jobject map, F convert) {
  static const jclass map_type = global_class(env, "java/util/Map");
  static const jclass set_type = global_class(env, "java/util/Set");
  static const jclass iterator_type = global_class(env, "java/util/Iterator");
  static const jclass entry_type = global_class(env, "java/util/Map$Entry");
  static const jmethodID entries = env->GetMethodID(map_type, "entrySet", "()Ljava/util/Set;");
  static const jmethodID iterator = env->GetMethodID(set_type, "iterator", "()Ljava/util/Iterator;");
  static const jmethodID has_next = env->GetMethodID(iterator_type, "hasNext", "()Z");
  static const jmethodID next = env->GetMethodID(iterator_type, "next", "()Ljava/lang/Object;");
  static const jmethodID key_of = env->GetMethodID(entry_type, "getKey", "()Ljava/lang/Object;");
  static const jmethodID value_of = env->GetMethodID(entry_type, "getValue", "()Ljava/lang/Object;");
  Locals locals(env);
  jobject set = locals.add(env->CallObjectMethod(map, entries));
  if (!set) return false;
  jobject it = locals.add(env->CallObjectMethod(set, iterator));
  if (!it) return false;
  while (env->CallBooleanMethod(it, has_next) == JNI_TRUE) {
    Locals entry_locals(env);
    jobject entry = entry_locals.add(env->CallObjectMethod(it, next));
    jobject key = entry_locals.add(env->CallObjectMethod(entry, key_of));
    jobject value = entry_locals.add(env->CallObjectMethod(entry, value_of));
    if (env->ExceptionCheck()) return false;
    if (!key || !value) { fail(env, "a map key or value must not be null"); return false; }
    if (!convert(static_cast<jstring>(key), value)) return false;
  }
  return !env->ExceptionCheck();
}

jint map_size(JNIEnv* env, jobject map) {
  static const jclass type = global_class(env, "java/util/Map");
  static const jmethodID size = env->GetMethodID(type, "size", "()I");
  return env->CallIntMethod(map, size);
}

template <class M> bool from_java_text_map(JNIEnv* env, jobject map, Keep& keep, M& out) {
  using Entry = std::remove_cv_t<std::remove_pointer_t<decltype(out.items)>>;
  const jint size = map_size(env, map);
  if (env->ExceptionCheck()) return false;
  Entry* entries = keep.make<Entry>(static_cast<std::size_t>(size));
  std::size_t i = 0;
  if (!for_each_entry(env, map, [&](jstring key, jobject value) {
        if (i >= static_cast<std::size_t>(size)) return false;
        Entry& entry = entries[i++];
        return c_text(env, key, keep, entry.key) && c_text(env, static_cast<jstring>(value), keep, entry.values);
      }))
    return false;
  out.items = entries;
  out.count = i;
  return true;
}

template <class M> bool from_java_list_map(JNIEnv* env, jobject map, Keep& keep, M& out) {
  using Entry = std::remove_cv_t<std::remove_pointer_t<decltype(out.items)>>;
  const jint size = map_size(env, map);
  if (env->ExceptionCheck()) return false;
  Entry* entries = keep.make<Entry>(static_cast<std::size_t>(size));
  std::size_t i = 0;
  if (!for_each_entry(env, map, [&](jstring key, jobject value) {
        if (i >= static_cast<std::size_t>(size)) return false;
        Entry& entry = entries[i++];
        return c_text(env, key, keep, entry.key) &&
               from_java_list(env, value, keep, entry.values.items, entry.values.count,
                              [&](jobject text, const char*& element) { return c_text(env, static_cast<jstring>(text), keep, element); });
      }))
    return false;
  out.items = entries;
  out.count = i;
  return true;
}

bool from_java_change(JNIEnv* env, jobject change, Keep& keep, holder_string_change& out) {
  static const jclass set = global_class(env, "team/holder/android/contract/Change$Set");
  static const jclass clear = global_class(env, "team/holder/android/contract/Change$Clear");
  static const jfieldID value = env->GetFieldID(set, "value", "Ljava/lang/Object;");
  out = holder_string_change{};
  if (change && env->IsInstanceOf(change, set)) {
    jobject text = env->GetObjectField(change, value);
    const bool converted = text && c_text(env, static_cast<jstring>(text), keep, out.value);
    if (text) env->DeleteLocalRef(text);
    if (!converted) {
      if (!env->ExceptionCheck()) fail(env, "a set change needs a value");
      return false;
    }
    out.change = HOLDER_SET;
  } else if (change && env->IsInstanceOf(change, clear)) {
    out.change = HOLDER_CLEAR;
  } else {
    out.change = HOLDER_KEEP;
  }
  return true;
}

// ProjectStorageLocationProvider, from its C value, which counts from 1.
jobject to_java_holder_project_storage_location_provider(JNIEnv* env, int value) {
  static const jclass type = global_class(env, "team/holder/android/contract/ProjectStorageLocationProvider");
  static const jmethodID values = env->GetStaticMethodID(type, "values", "()[Lteam/holder/android/contract/ProjectStorageLocationProvider;");
  auto entries = static_cast<jobjectArray>(env->CallStaticObjectMethod(type, values));
  if (!entries) return nullptr;
  jobject entry = env->GetObjectArrayElement(entries, value - 1);
  env->DeleteLocalRef(entries);
  return entry;
}

// CardBreadcrumbKind, from its C value, which counts from 1.
jobject to_java_holder_card_breadcrumb_kind(JNIEnv* env, int value) {
  static const jclass type = global_class(env, "team/holder/android/contract/CardBreadcrumbKind");
  static const jmethodID values = env->GetStaticMethodID(type, "values", "()[Lteam/holder/android/contract/CardBreadcrumbKind;");
  auto entries = static_cast<jobjectArray>(env->CallStaticObjectMethod(type, values));
  if (!entries) return nullptr;
  jobject entry = env->GetObjectArrayElement(entries, value - 1);
  env->DeleteLocalRef(entries);
  return entry;
}

// CardReferenceResolveDataStatus, from its C value, which counts from 1.
jobject to_java_holder_card_reference_resolve_data_status(JNIEnv* env, int value) {
  static const jclass type = global_class(env, "team/holder/android/contract/CardReferenceResolveDataStatus");
  static const jmethodID values = env->GetStaticMethodID(type, "values", "()[Lteam/holder/android/contract/CardReferenceResolveDataStatus;");
  auto entries = static_cast<jobjectArray>(env->CallStaticObjectMethod(type, values));
  if (!entries) return nullptr;
  jobject entry = env->GetObjectArrayElement(entries, value - 1);
  env->DeleteLocalRef(entries);
  return entry;
}

// LinkSourceType, from its C value, which counts from 1.
jobject to_java_holder_link_source_type(JNIEnv* env, int value) {
  static const jclass type = global_class(env, "team/holder/android/contract/LinkSourceType");
  static const jmethodID values = env->GetStaticMethodID(type, "values", "()[Lteam/holder/android/contract/LinkSourceType;");
  auto entries = static_cast<jobjectArray>(env->CallStaticObjectMethod(type, values));
  if (!entries) return nullptr;
  jobject entry = env->GetObjectArrayElement(entries, value - 1);
  env->DeleteLocalRef(entries);
  return entry;
}

// LinkTargetType, from its C value, which counts from 1.
jobject to_java_holder_link_target_type(JNIEnv* env, int value) {
  static const jclass type = global_class(env, "team/holder/android/contract/LinkTargetType");
  static const jmethodID values = env->GetStaticMethodID(type, "values", "()[Lteam/holder/android/contract/LinkTargetType;");
  auto entries = static_cast<jobjectArray>(env->CallStaticObjectMethod(type, values));
  if (!entries) return nullptr;
  jobject entry = env->GetObjectArrayElement(entries, value - 1);
  env->DeleteLocalRef(entries);
  return entry;
}

// AssetPlacementEncoding, from its C value, which counts from 1.
jobject to_java_holder_asset_placement_encoding(JNIEnv* env, int value) {
  static const jclass type = global_class(env, "team/holder/android/contract/AssetPlacementEncoding");
  static const jmethodID values = env->GetStaticMethodID(type, "values", "()[Lteam/holder/android/contract/AssetPlacementEncoding;");
  auto entries = static_cast<jobjectArray>(env->CallStaticObjectMethod(type, values));
  if (!entries) return nullptr;
  jobject entry = env->GetObjectArrayElement(entries, value - 1);
  env->DeleteLocalRef(entries);
  return entry;
}

// ProjectPrivacyMode, from its C value, which counts from 1.
jobject to_java_holder_project_privacy_mode(JNIEnv* env, int value) {
  static const jclass type = global_class(env, "team/holder/android/contract/ProjectPrivacyMode");
  static const jmethodID values = env->GetStaticMethodID(type, "values", "()[Lteam/holder/android/contract/ProjectPrivacyMode;");
  auto entries = static_cast<jobjectArray>(env->CallStaticObjectMethod(type, values));
  if (!entries) return nullptr;
  jobject entry = env->GetObjectArrayElement(entries, value - 1);
  env->DeleteLocalRef(entries);
  return entry;
}

// ProjectEncryptionCheckPrivacyMode, from its C value, which counts from 1.
jobject to_java_holder_project_encryption_check_privacy_mode(JNIEnv* env, int value) {
  static const jclass type = global_class(env, "team/holder/android/contract/ProjectEncryptionCheckPrivacyMode");
  static const jmethodID values = env->GetStaticMethodID(type, "values", "()[Lteam/holder/android/contract/ProjectEncryptionCheckPrivacyMode;");
  auto entries = static_cast<jobjectArray>(env->CallStaticObjectMethod(type, values));
  if (!entries) return nullptr;
  jobject entry = env->GetObjectArrayElement(entries, value - 1);
  env->DeleteLocalRef(entries);
  return entry;
}

// RecoverySnapshotLinkTargetType, from its C value, which counts from 1.
jobject to_java_holder_recovery_snapshot_link_target_type(JNIEnv* env, int value) {
  static const jclass type = global_class(env, "team/holder/android/contract/RecoverySnapshotLinkTargetType");
  static const jmethodID values = env->GetStaticMethodID(type, "values", "()[Lteam/holder/android/contract/RecoverySnapshotLinkTargetType;");
  auto entries = static_cast<jobjectArray>(env->CallStaticObjectMethod(type, values));
  if (!entries) return nullptr;
  jobject entry = env->GetObjectArrayElement(entries, value - 1);
  env->DeleteLocalRef(entries);
  return entry;
}

// RecoverySnapshotPlacementEncoding, from its C value, which counts from 1.
jobject to_java_holder_recovery_snapshot_placement_encoding(JNIEnv* env, int value) {
  static const jclass type = global_class(env, "team/holder/android/contract/RecoverySnapshotPlacementEncoding");
  static const jmethodID values = env->GetStaticMethodID(type, "values", "()[Lteam/holder/android/contract/RecoverySnapshotPlacementEncoding;");
  auto entries = static_cast<jobjectArray>(env->CallStaticObjectMethod(type, values));
  if (!entries) return nullptr;
  jobject entry = env->GetObjectArrayElement(entries, value - 1);
  env->DeleteLocalRef(entries);
  return entry;
}

// RecoverySnapshotLocationProvider, from its C value, which counts from 1.
jobject to_java_holder_recovery_snapshot_location_provider(JNIEnv* env, int value) {
  static const jclass type = global_class(env, "team/holder/android/contract/RecoverySnapshotLocationProvider");
  static const jmethodID values = env->GetStaticMethodID(type, "values", "()[Lteam/holder/android/contract/RecoverySnapshotLocationProvider;");
  auto entries = static_cast<jobjectArray>(env->CallStaticObjectMethod(type, values));
  if (!entries) return nullptr;
  jobject entry = env->GetObjectArrayElement(entries, value - 1);
  env->DeleteLocalRef(entries);
  return entry;
}

// RecoverySnapshotCardPrivacyMode, from its C value, which counts from 1.
jobject to_java_holder_recovery_snapshot_card_privacy_mode(JNIEnv* env, int value) {
  static const jclass type = global_class(env, "team/holder/android/contract/RecoverySnapshotCardPrivacyMode");
  static const jmethodID values = env->GetStaticMethodID(type, "values", "()[Lteam/holder/android/contract/RecoverySnapshotCardPrivacyMode;");
  auto entries = static_cast<jobjectArray>(env->CallStaticObjectMethod(type, values));
  if (!entries) return nullptr;
  jobject entry = env->GetObjectArrayElement(entries, value - 1);
  env->DeleteLocalRef(entries);
  return entry;
}

// RecoveryTokenGlobalImportResultPullStatus, from its C value, which counts from 1.
jobject to_java_holder_recovery_token_global_import_result_pull_status(JNIEnv* env, int value) {
  static const jclass type = global_class(env, "team/holder/android/contract/RecoveryTokenGlobalImportResultPullStatus");
  static const jmethodID values = env->GetStaticMethodID(type, "values", "()[Lteam/holder/android/contract/RecoveryTokenGlobalImportResultPullStatus;");
  auto entries = static_cast<jobjectArray>(env->CallStaticObjectMethod(type, values));
  if (!entries) return nullptr;
  jobject entry = env->GetObjectArrayElement(entries, value - 1);
  env->DeleteLocalRef(entries);
  return entry;
}

// PullPhaseStatus, from its C value, which counts from 1.
jobject to_java_holder_pull_phase_status(JNIEnv* env, int value) {
  static const jclass type = global_class(env, "team/holder/android/contract/PullPhaseStatus");
  static const jmethodID values = env->GetStaticMethodID(type, "values", "()[Lteam/holder/android/contract/PullPhaseStatus;");
  auto entries = static_cast<jobjectArray>(env->CallStaticObjectMethod(type, values));
  if (!entries) return nullptr;
  jobject entry = env->GetObjectArrayElement(entries, value - 1);
  env->DeleteLocalRef(entries);
  return entry;
}

// PushPhaseStatus, from its C value, which counts from 1.
jobject to_java_holder_push_phase_status(JNIEnv* env, int value) {
  static const jclass type = global_class(env, "team/holder/android/contract/PushPhaseStatus");
  static const jmethodID values = env->GetStaticMethodID(type, "values", "()[Lteam/holder/android/contract/PushPhaseStatus;");
  auto entries = static_cast<jobjectArray>(env->CallStaticObjectMethod(type, values));
  if (!entries) return nullptr;
  jobject entry = env->GetObjectArrayElement(entries, value - 1);
  env->DeleteLocalRef(entries);
  return entry;
}

// SyncResultStatus, from its C value, which counts from 1.
jobject to_java_holder_sync_result_status(JNIEnv* env, int value) {
  static const jclass type = global_class(env, "team/holder/android/contract/SyncResultStatus");
  static const jmethodID values = env->GetStaticMethodID(type, "values", "()[Lteam/holder/android/contract/SyncResultStatus;");
  auto entries = static_cast<jobjectArray>(env->CallStaticObjectMethod(type, values));
  if (!entries) return nullptr;
  jobject entry = env->GetObjectArrayElement(entries, value - 1);
  env->DeleteLocalRef(entries);
  return entry;
}

// RemoteTestResultStatus, from its C value, which counts from 1.
jobject to_java_holder_remote_test_result_status(JNIEnv* env, int value) {
  static const jclass type = global_class(env, "team/holder/android/contract/RemoteTestResultStatus");
  static const jmethodID values = env->GetStaticMethodID(type, "values", "()[Lteam/holder/android/contract/RemoteTestResultStatus;");
  auto entries = static_cast<jobjectArray>(env->CallStaticObjectMethod(type, values));
  if (!entries) return nullptr;
  jobject entry = env->GetObjectArrayElement(entries, value - 1);
  env->DeleteLocalRef(entries);
  return entry;
}

// HistoryAffectedObjectKind, from its C value, which counts from 1.
jobject to_java_holder_history_affected_object_kind(JNIEnv* env, int value) {
  static const jclass type = global_class(env, "team/holder/android/contract/HistoryAffectedObjectKind");
  static const jmethodID values = env->GetStaticMethodID(type, "values", "()[Lteam/holder/android/contract/HistoryAffectedObjectKind;");
  auto entries = static_cast<jobjectArray>(env->CallStaticObjectMethod(type, values));
  if (!entries) return nullptr;
  jobject entry = env->GetObjectArrayElement(entries, value - 1);
  env->DeleteLocalRef(entries);
  return entry;
}

// TrashItemType, from its C value, which counts from 1.
jobject to_java_holder_trash_item_type(JNIEnv* env, int value) {
  static const jclass type = global_class(env, "team/holder/android/contract/TrashItemType");
  static const jmethodID values = env->GetStaticMethodID(type, "values", "()[Lteam/holder/android/contract/TrashItemType;");
  auto entries = static_cast<jobjectArray>(env->CallStaticObjectMethod(type, values));
  if (!entries) return nullptr;
  jobject entry = env->GetObjectArrayElement(entries, value - 1);
  env->DeleteLocalRef(entries);
  return entry;
}

jobject to_java_holder_project_storage_location(JNIEnv* env, const holder_project_storage_location& value);
jobject to_java_holder_storage_location_list(JNIEnv* env, const holder_storage_location_list& value);
jobject to_java_holder_location_delete_result(JNIEnv* env, const holder_location_delete_result& value);
jobject to_java_holder_tag_occurrence(JNIEnv* env, const holder_tag_occurrence& value);
jobject to_java_holder_card(JNIEnv* env, const holder_card& value);
jobject to_java_holder_card_metadata(JNIEnv* env, const holder_card_metadata& value);
jobject to_java_holder_card_breadcrumb(JNIEnv* env, const holder_card_breadcrumb& value);
jobject to_java_holder_card_reference_candidate(JNIEnv* env, const holder_card_reference_candidate& value);
jobject to_java_holder_card_reference_resolve_data(JNIEnv* env, const holder_card_reference_resolve_data& value);
jobject to_java_holder_card_list_item(JNIEnv* env, const holder_card_list_item& value);
jobject to_java_holder_card_list_item_page(JNIEnv* env, const holder_card_list_item_page& value);
jobject to_java_holder_tag_count(JNIEnv* env, const holder_tag_count& value);
jobject to_java_holder_link_kind_labels(JNIEnv* env, const holder_link_kind_labels& value);
jobject to_java_holder_link(JNIEnv* env, const holder_link& value);
jobject to_java_holder_milestone_date(JNIEnv* env, const holder_milestone_date& value);
bool from_java_holder_milestone_date(JNIEnv* env, jobject object, holder_milestone_date& out, Keep& keep);
jobject to_java_holder_milestone(JNIEnv* env, const holder_milestone& value);
jobject to_java_holder_calendar_milestone(JNIEnv* env, const holder_calendar_milestone& value);
jobject to_java_holder_calendar(JNIEnv* env, const holder_calendar& value);
jobject to_java_holder_asset_import(JNIEnv* env, const holder_asset_import& value);
jobject to_java_holder_asset_file(JNIEnv* env, const holder_asset_file& value);
jobject to_java_holder_asset_placement(JNIEnv* env, const holder_asset_placement& value);
jobject to_java_holder_resource_asset(JNIEnv* env, const holder_resource_asset& value);
jobject to_java_holder_resource(JNIEnv* env, const holder_resource& value);
jobject to_java_holder_project_list_item(JNIEnv* env, const holder_project_list_item& value);
jobject to_java_holder_project(JNIEnv* env, const holder_project& value);
jobject to_java_holder_encryption_safety_check(JNIEnv* env, const holder_encryption_safety_check& value);
jobject to_java_holder_project_encryption_check(JNIEnv* env, const holder_project_encryption_check& value);
jobject to_java_holder_hierarchy_repair_move(JNIEnv* env, const holder_hierarchy_repair_move& value);
jobject to_java_holder_hierarchy_repair(JNIEnv* env, const holder_hierarchy_repair& value);
jobject to_java_holder_rebuild_result(JNIEnv* env, const holder_rebuild_result& value);
jobject to_java_holder_reindex_result(JNIEnv* env, const holder_reindex_result& value);
jobject to_java_holder_recovery_snapshot_link(JNIEnv* env, const holder_recovery_snapshot_link& value);
jobject to_java_holder_recovery_snapshot_milestone(JNIEnv* env, const holder_recovery_snapshot_milestone& value);
jobject to_java_holder_recovery_snapshot_placement(JNIEnv* env, const holder_recovery_snapshot_placement& value);
jobject to_java_holder_recovery_snapshot_asset(JNIEnv* env, const holder_recovery_snapshot_asset& value);
jobject to_java_holder_recovery_snapshot_resource(JNIEnv* env, const holder_recovery_snapshot_resource& value);
jobject to_java_holder_recovery_snapshot_location(JNIEnv* env, const holder_recovery_snapshot_location& value);
jobject to_java_holder_recovery_snapshot_card(JNIEnv* env, const holder_recovery_snapshot_card& value);
jobject to_java_holder_recovery_snapshot_page(JNIEnv* env, const holder_recovery_snapshot_page& value);
jobject to_java_holder_project_recovery_token_export_result(JNIEnv* env, const holder_project_recovery_token_export_result& value);
jobject to_java_holder_project_recovery_token_import_result(JNIEnv* env, const holder_project_recovery_token_import_result& value);
jobject to_java_holder_recovery_token_global_import_result(JNIEnv* env, const holder_recovery_token_global_import_result& value);
jobject to_java_holder_sync_state(JNIEnv* env, const holder_sync_state& value);
jobject to_java_holder_pull_phase(JNIEnv* env, const holder_pull_phase& value);
jobject to_java_holder_push_phase(JNIEnv* env, const holder_push_phase& value);
jobject to_java_holder_sync_result(JNIEnv* env, const holder_sync_result& value);
jobject to_java_holder_due_sync_result(JNIEnv* env, const holder_due_sync_result& value);
jobject to_java_holder_remote_test_result(JNIEnv* env, const holder_remote_test_result& value);
jobject to_java_holder_history_author(JNIEnv* env, const holder_history_author& value);
jobject to_java_holder_history_affected_path(JNIEnv* env, const holder_history_affected_path& value);
jobject to_java_holder_history_affected_object(JNIEnv* env, const holder_history_affected_object& value);
jobject to_java_holder_project_activity(JNIEnv* env, const holder_project_activity& value);
jobject to_java_holder_project_history_page(JNIEnv* env, const holder_project_history_page& value);
jobject to_java_holder_card_history_save(JNIEnv* env, const holder_card_history_save& value);
jobject to_java_holder_card_history_entry(JNIEnv* env, const holder_card_history_entry& value);
jobject to_java_holder_card_history_page(JNIEnv* env, const holder_card_history_page& value);
jobject to_java_holder_card_history_version(JNIEnv* env, const holder_card_history_version& value);
jobject to_java_holder_card_history_diff_line(JNIEnv* env, const holder_card_history_diff_line& value);
jobject to_java_holder_card_history_comparison(JNIEnv* env, const holder_card_history_comparison& value);
jobject to_java_holder_card_history_snapshot(JNIEnv* env, const holder_card_history_snapshot& value);
jobject to_java_holder_card_history_restore_result(JNIEnv* env, const holder_card_history_restore_result& value);
jobject to_java_holder_trash_item(JNIEnv* env, const holder_trash_item& value);
jobject to_java_holder_trash_empty_result(JNIEnv* env, const holder_trash_empty_result& value);
jobject to_java_holder_trash_delete_result(JNIEnv* env, const holder_trash_delete_result& value);
jobject to_java_holder_search_card(JNIEnv* env, const holder_search_card& value);
jobject to_java_holder_search_message(JNIEnv* env, const holder_search_message& value);
bool from_java_holder_new_storage_location(JNIEnv* env, jobject object, holder_new_storage_location& out, Keep& keep);
bool from_java_holder_storage_location_update(JNIEnv* env, jobject object, holder_storage_location_update& out, Keep& keep);
bool from_java_holder_card_reference_resolve_request(JNIEnv* env, jobject object, holder_card_reference_resolve_request& out, Keep& keep);
bool from_java_holder_new_card(JNIEnv* env, jobject object, holder_new_card& out, Keep& keep);
bool from_java_holder_card_update(JNIEnv* env, jobject object, holder_card_update& out, Keep& keep);
bool from_java_holder_card_move(JNIEnv* env, jobject object, holder_card_move& out, Keep& keep);
bool from_java_holder_new_link(JNIEnv* env, jobject object, holder_new_link& out, Keep& keep);
bool from_java_holder_new_milestone(JNIEnv* env, jobject object, holder_new_milestone& out, Keep& keep);
bool from_java_holder_milestone_update(JNIEnv* env, jobject object, holder_milestone_update& out, Keep& keep);
bool from_java_holder_asset_import_request(JNIEnv* env, jobject object, holder_asset_import_request& out, Keep& keep);
bool from_java_holder_new_resource(JNIEnv* env, jobject object, holder_new_resource& out, Keep& keep);
bool from_java_holder_resource_update(JNIEnv* env, jobject object, holder_resource_update& out, Keep& keep);
bool from_java_holder_new_project(JNIEnv* env, jobject object, holder_new_project& out, Keep& keep);
bool from_java_holder_project_update(JNIEnv* env, jobject object, holder_project_update& out, Keep& keep);
bool from_java_holder_hierarchy_repair_request(JNIEnv* env, jobject object, holder_hierarchy_repair_request& out, Keep& keep);
bool from_java_holder_rebuild_request(JNIEnv* env, jobject object, holder_rebuild_request& out, Keep& keep);
bool from_java_holder_recovery_snapshot_restore(JNIEnv* env, jobject object, holder_recovery_snapshot_restore& out, Keep& keep);
bool from_java_holder_recovery_token_export_request(JNIEnv* env, jobject object, holder_recovery_token_export_request& out, Keep& keep);
bool from_java_holder_recovery_token_import_request(JNIEnv* env, jobject object, holder_recovery_token_import_request& out, Keep& keep);
bool from_java_holder_push_options(JNIEnv* env, jobject object, holder_push_options& out, Keep& keep);
bool from_java_holder_remote_test(JNIEnv* env, jobject object, holder_remote_test& out, Keep& keep);

jobject to_java_holder_project_storage_location(JNIEnv* env, const holder_project_storage_location& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/ProjectStorageLocation");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Lteam/holder/android/contract/ProjectStorageLocationProvider;Ljava/util/Map;ZLjava/lang/String;JJ)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[9] = {};
  args[0].l = locals.add(string(env, value.location_id));
  if (env->ExceptionCheck()) return nullptr;
  args[1].l = locals.add(string(env, value.project_id));
  if (env->ExceptionCheck()) return nullptr;
  args[2].l = locals.add(string(env, value.name));
  if (env->ExceptionCheck()) return nullptr;
  args[3].l = locals.add(to_java_holder_project_storage_location_provider(env, static_cast<int>(value.provider)));
  if (env->ExceptionCheck()) return nullptr;
  args[4].l = locals.add(to_java_text_map(env, value.configuration));
  if (env->ExceptionCheck()) return nullptr;
  args[5].z = static_cast<jboolean>(value.bound ? JNI_TRUE : JNI_FALSE);
  args[6].l = locals.add(string(env, value.binding_preview));
  if (env->ExceptionCheck()) return nullptr;
  args[7].j = static_cast<jlong>(value.created_at);
  args[8].j = static_cast<jlong>(value.updated_at);
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_storage_location_list(JNIEnv* env, const holder_storage_location_list& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/StorageLocationList");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(Ljava/util/List;Ljava/lang/String;)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[2] = {};
  args[0].l = locals.add(to_java_list(env, (value.data).items, (value.data).count, [&](const auto& item) -> jobject { return to_java_holder_project_storage_location(env, item); }));
  if (env->ExceptionCheck()) return nullptr;
  args[1].l = locals.add(string(env, value.preferred_location_id));
  if (env->ExceptionCheck()) return nullptr;
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_location_delete_result(JNIEnv* env, const holder_location_delete_result& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/LocationDeleteResult");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(Ljava/lang/String;)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[1] = {};
  args[0].l = locals.add(string(env, value.location_id));
  if (env->ExceptionCheck()) return nullptr;
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_tag_occurrence(JNIEnv* env, const holder_tag_occurrence& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/TagOccurrence");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(Ljava/lang/String;JJ)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[3] = {};
  args[0].l = locals.add(string(env, value.tag));
  if (env->ExceptionCheck()) return nullptr;
  args[1].j = static_cast<jlong>(value.byte_start);
  args[2].j = static_cast<jlong>(value.byte_end);
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_card(JNIEnv* env, const holder_card& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/Card");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;DJJLjava/lang/Long;Ljava/lang/String;Ljava/util/List;Ljava/util/List;Ljava/util/List;)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[13] = {};
  args[0].l = locals.add(string(env, value.card_id));
  if (env->ExceptionCheck()) return nullptr;
  args[1].l = locals.add(string(env, value.project_id));
  if (env->ExceptionCheck()) return nullptr;
  args[2].l = locals.add(string(env, value.title));
  if (env->ExceptionCheck()) return nullptr;
  args[3].l = locals.add(string(env, value.rel_path));
  if (env->ExceptionCheck()) return nullptr;
  args[4].l = locals.add(string(env, value.parent_card_id));
  if (env->ExceptionCheck()) return nullptr;
  args[5].d = static_cast<jdouble>(value.sort_key);
  args[6].j = static_cast<jlong>(value.created_at);
  args[7].j = static_cast<jlong>(value.updated_at);
  args[8].l = locals.add((value.deleted_at ? box_int64(env, *value.deleted_at) : nullptr));
  if (env->ExceptionCheck()) return nullptr;
  args[9].l = locals.add(string(env, value.content));
  if (env->ExceptionCheck()) return nullptr;
  args[10].l = locals.add(to_java_list(env, (value.tags).items, (value.tags).count, [&](const auto& item) -> jobject { return string(env, item); }));
  if (env->ExceptionCheck()) return nullptr;
  args[11].l = locals.add(to_java_list(env, (value.tag_occurrences).items, (value.tag_occurrences).count, [&](const auto& item) -> jobject { return to_java_holder_tag_occurrence(env, item); }));
  if (env->ExceptionCheck()) return nullptr;
  args[12].l = locals.add(to_java_list(env, (value.removable_tags).items, (value.removable_tags).count, [&](const auto& item) -> jobject { return string(env, item); }));
  if (env->ExceptionCheck()) return nullptr;
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_card_metadata(JNIEnv* env, const holder_card_metadata& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/CardMetadata");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;DJJLjava/lang/Long;)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[9] = {};
  args[0].l = locals.add(string(env, value.card_id));
  if (env->ExceptionCheck()) return nullptr;
  args[1].l = locals.add(string(env, value.project_id));
  if (env->ExceptionCheck()) return nullptr;
  args[2].l = locals.add(string(env, value.title));
  if (env->ExceptionCheck()) return nullptr;
  args[3].l = locals.add(string(env, value.rel_path));
  if (env->ExceptionCheck()) return nullptr;
  args[4].l = locals.add(string(env, value.parent_card_id));
  if (env->ExceptionCheck()) return nullptr;
  args[5].d = static_cast<jdouble>(value.sort_key);
  args[6].j = static_cast<jlong>(value.created_at);
  args[7].j = static_cast<jlong>(value.updated_at);
  args[8].l = locals.add((value.deleted_at ? box_int64(env, *value.deleted_at) : nullptr));
  if (env->ExceptionCheck()) return nullptr;
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_card_breadcrumb(JNIEnv* env, const holder_card_breadcrumb& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/CardBreadcrumb");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(Lteam/holder/android/contract/CardBreadcrumbKind;Ljava/lang/String;Ljava/lang/String;)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[3] = {};
  args[0].l = locals.add(to_java_holder_card_breadcrumb_kind(env, static_cast<int>(value.kind)));
  if (env->ExceptionCheck()) return nullptr;
  args[1].l = locals.add(string(env, value.id));
  if (env->ExceptionCheck()) return nullptr;
  args[2].l = locals.add(string(env, value.title));
  if (env->ExceptionCheck()) return nullptr;
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_card_reference_candidate(JNIEnv* env, const holder_card_reference_candidate& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/CardReferenceCandidate");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/Long;)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[3] = {};
  args[0].l = locals.add(string(env, value.card_id));
  if (env->ExceptionCheck()) return nullptr;
  args[1].l = locals.add(string(env, value.title));
  if (env->ExceptionCheck()) return nullptr;
  args[2].l = locals.add((value.deleted_at ? box_int64(env, *value.deleted_at) : nullptr));
  if (env->ExceptionCheck()) return nullptr;
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_card_reference_resolve_data(JNIEnv* env, const holder_card_reference_resolve_data& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/CardReferenceResolveData");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(Lteam/holder/android/contract/CardReferenceResolveDataStatus;Ljava/lang/String;Lteam/holder/android/contract/CardReferenceCandidate;Ljava/util/List;)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[4] = {};
  args[0].l = locals.add(to_java_holder_card_reference_resolve_data_status(env, static_cast<int>(value.status)));
  if (env->ExceptionCheck()) return nullptr;
  args[1].l = locals.add(string(env, value.match_kind));
  if (env->ExceptionCheck()) return nullptr;
  args[2].l = locals.add((value.card ? to_java_holder_card_reference_candidate(env, *value.card) : nullptr));
  if (env->ExceptionCheck()) return nullptr;
  args[3].l = locals.add((value.candidates ? to_java_list(env, value.candidates->items, value.candidates->count, [&](const auto& item) -> jobject { return to_java_holder_card_reference_candidate(env, item); }) : nullptr));
  if (env->ExceptionCheck()) return nullptr;
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_card_list_item(JNIEnv* env, const holder_card_list_item& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/CardListItem");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;DJJLjava/lang/Long;Ljava/lang/Integer;Ljava/lang/String;)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[11] = {};
  args[0].l = locals.add(string(env, value.card_id));
  if (env->ExceptionCheck()) return nullptr;
  args[1].l = locals.add(string(env, value.project_id));
  if (env->ExceptionCheck()) return nullptr;
  args[2].l = locals.add(string(env, value.title));
  if (env->ExceptionCheck()) return nullptr;
  args[3].l = locals.add(string(env, value.rel_path));
  if (env->ExceptionCheck()) return nullptr;
  args[4].l = locals.add(string(env, value.parent_card_id));
  if (env->ExceptionCheck()) return nullptr;
  args[5].d = static_cast<jdouble>(value.sort_key);
  args[6].j = static_cast<jlong>(value.created_at);
  args[7].j = static_cast<jlong>(value.updated_at);
  args[8].l = locals.add((value.deleted_at ? box_int64(env, *value.deleted_at) : nullptr));
  if (env->ExceptionCheck()) return nullptr;
  args[9].l = locals.add((value.child_count ? box_int32(env, *value.child_count) : nullptr));
  if (env->ExceptionCheck()) return nullptr;
  args[10].l = locals.add(string(env, value.content));
  if (env->ExceptionCheck()) return nullptr;
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_card_list_item_page(JNIEnv* env, const holder_card_list_item_page& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/Page");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(Ljava/util/List;Ljava/lang/String;)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[2] = {};
  args[0].l = locals.add(to_java_list(env, (value.items).items, (value.items).count, [&](const auto& item) -> jobject { return to_java_holder_card_list_item(env, item); }));
  if (env->ExceptionCheck()) return nullptr;
  args[1].l = locals.add(string(env, value.next_cursor));
  if (env->ExceptionCheck()) return nullptr;
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_tag_count(JNIEnv* env, const holder_tag_count& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/TagCount");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(Ljava/lang/String;I)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[2] = {};
  args[0].l = locals.add(string(env, value.tag));
  if (env->ExceptionCheck()) return nullptr;
  args[1].i = static_cast<jint>(value.card_count);
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_link_kind_labels(JNIEnv* env, const holder_link_kind_labels& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/LinkKindLabels");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[3] = {};
  args[0].l = locals.add(string(env, value.kind));
  if (env->ExceptionCheck()) return nullptr;
  args[1].l = locals.add(string(env, value.forward_label));
  if (env->ExceptionCheck()) return nullptr;
  args[2].l = locals.add(string(env, value.reverse_label));
  if (env->ExceptionCheck()) return nullptr;
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_link(JNIEnv* env, const holder_link& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/Link");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(Ljava/lang/String;Lteam/holder/android/contract/LinkSourceType;Ljava/lang/String;Ljava/lang/String;Lteam/holder/android/contract/LinkTargetType;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;J)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[9] = {};
  args[0].l = locals.add(string(env, value.source_project_id));
  if (env->ExceptionCheck()) return nullptr;
  args[1].l = locals.add(to_java_holder_link_source_type(env, static_cast<int>(value.source_type)));
  if (env->ExceptionCheck()) return nullptr;
  args[2].l = locals.add(string(env, value.source_id));
  if (env->ExceptionCheck()) return nullptr;
  args[3].l = locals.add(string(env, value.target_project_id));
  if (env->ExceptionCheck()) return nullptr;
  args[4].l = locals.add(to_java_holder_link_target_type(env, static_cast<int>(value.target_type)));
  if (env->ExceptionCheck()) return nullptr;
  args[5].l = locals.add(string(env, value.target_id));
  if (env->ExceptionCheck()) return nullptr;
  args[6].l = locals.add(string(env, value.kind));
  if (env->ExceptionCheck()) return nullptr;
  args[7].l = locals.add(string(env, value.label));
  if (env->ExceptionCheck()) return nullptr;
  args[8].j = static_cast<jlong>(value.created_at);
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_milestone_date(JNIEnv* env, const holder_milestone_date& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/MilestoneDate");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[3] = {};
  args[0].l = locals.add(string(env, value.role));
  if (env->ExceptionCheck()) return nullptr;
  args[1].l = locals.add(string(env, value.date));
  if (env->ExceptionCheck()) return nullptr;
  args[2].l = locals.add(string(env, value.datetime));
  if (env->ExceptionCheck()) return nullptr;
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_milestone(JNIEnv* env, const holder_milestone& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/Milestone");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/util/List;Ljava/lang/String;Ljava/lang/String;JJ)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[8] = {};
  args[0].l = locals.add(string(env, value.milestone_id));
  if (env->ExceptionCheck()) return nullptr;
  args[1].l = locals.add(string(env, value.project_id));
  if (env->ExceptionCheck()) return nullptr;
  args[2].l = locals.add(string(env, value.card_id));
  if (env->ExceptionCheck()) return nullptr;
  args[3].l = locals.add(to_java_list(env, (value.dates).items, (value.dates).count, [&](const auto& item) -> jobject { return to_java_holder_milestone_date(env, item); }));
  if (env->ExceptionCheck()) return nullptr;
  args[4].l = locals.add(string(env, value.kind));
  if (env->ExceptionCheck()) return nullptr;
  args[5].l = locals.add(string(env, value.description));
  if (env->ExceptionCheck()) return nullptr;
  args[6].j = static_cast<jlong>(value.created_at);
  args[7].j = static_cast<jlong>(value.updated_at);
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_calendar_milestone(JNIEnv* env, const holder_calendar_milestone& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/CalendarMilestone");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/util/List;Ljava/lang/String;Ljava/lang/String;JJLjava/lang/String;)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[9] = {};
  args[0].l = locals.add(string(env, value.milestone_id));
  if (env->ExceptionCheck()) return nullptr;
  args[1].l = locals.add(string(env, value.project_id));
  if (env->ExceptionCheck()) return nullptr;
  args[2].l = locals.add(string(env, value.card_id));
  if (env->ExceptionCheck()) return nullptr;
  args[3].l = locals.add(to_java_list(env, (value.dates).items, (value.dates).count, [&](const auto& item) -> jobject { return to_java_holder_milestone_date(env, item); }));
  if (env->ExceptionCheck()) return nullptr;
  args[4].l = locals.add(string(env, value.kind));
  if (env->ExceptionCheck()) return nullptr;
  args[5].l = locals.add(string(env, value.description));
  if (env->ExceptionCheck()) return nullptr;
  args[6].j = static_cast<jlong>(value.created_at);
  args[7].j = static_cast<jlong>(value.updated_at);
  args[8].l = locals.add(string(env, value.card_title));
  if (env->ExceptionCheck()) return nullptr;
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_calendar(JNIEnv* env, const holder_calendar& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/Calendar");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(Ljava/util/List;Ljava/util/List;Ljava/util/List;)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[3] = {};
  args[0].l = locals.add(to_java_list(env, (value.milestones).items, (value.milestones).count, [&](const auto& item) -> jobject { return to_java_holder_calendar_milestone(env, item); }));
  if (env->ExceptionCheck()) return nullptr;
  args[1].l = locals.add(to_java_list(env, (value.created_cards).items, (value.created_cards).count, [&](const auto& item) -> jobject { return to_java_holder_card_metadata(env, item); }));
  if (env->ExceptionCheck()) return nullptr;
  args[2].l = locals.add(to_java_list(env, (value.updated_cards).items, (value.updated_cards).count, [&](const auto& item) -> jobject { return to_java_holder_card_metadata(env, item); }));
  if (env->ExceptionCheck()) return nullptr;
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_asset_import(JNIEnv* env, const holder_asset_import& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/AssetImport");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(Ljava/lang/String;Ljava/lang/String;ZZ)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[4] = {};
  args[0].l = locals.add(string(env, value.resource_id));
  if (env->ExceptionCheck()) return nullptr;
  args[1].l = locals.add(string(env, value.asset_id));
  if (env->ExceptionCheck()) return nullptr;
  args[2].z = static_cast<jboolean>(value.duplicate_reused ? JNI_TRUE : JNI_FALSE);
  args[3].z = static_cast<jboolean>(value.link_created ? JNI_TRUE : JNI_FALSE);
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_asset_file(JNIEnv* env, const holder_asset_file& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/AssetFile");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;J)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[4] = {};
  args[0].l = locals.add(string(env, value.path));
  if (env->ExceptionCheck()) return nullptr;
  args[1].l = locals.add(string(env, value.original_filename));
  if (env->ExceptionCheck()) return nullptr;
  args[2].l = locals.add(string(env, value.media_type));
  if (env->ExceptionCheck()) return nullptr;
  args[3].j = static_cast<jlong>(value.byte_size);
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_asset_placement(JNIEnv* env, const holder_asset_placement& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/AssetPlacement");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(Ljava/lang/String;Ljava/lang/String;Lteam/holder/android/contract/AssetPlacementEncoding;JJ)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[5] = {};
  args[0].l = locals.add(string(env, value.placement_id));
  if (env->ExceptionCheck()) return nullptr;
  args[1].l = locals.add(string(env, value.location_id));
  if (env->ExceptionCheck()) return nullptr;
  args[2].l = locals.add(to_java_holder_asset_placement_encoding(env, static_cast<int>(value.encoding)));
  if (env->ExceptionCheck()) return nullptr;
  args[3].j = static_cast<jlong>(value.stored_byte_size);
  args[4].j = static_cast<jlong>(value.created_at);
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_resource_asset(JNIEnv* env, const holder_resource_asset& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/ResourceAsset");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;JLjava/lang/String;Ljava/util/List;JJ)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[8] = {};
  args[0].l = locals.add(string(env, value.asset_id));
  if (env->ExceptionCheck()) return nullptr;
  args[1].l = locals.add(string(env, value.original_filename));
  if (env->ExceptionCheck()) return nullptr;
  args[2].l = locals.add(string(env, value.media_type));
  if (env->ExceptionCheck()) return nullptr;
  args[3].j = static_cast<jlong>(value.byte_size);
  args[4].l = locals.add(string(env, value.plaintext_sha256));
  if (env->ExceptionCheck()) return nullptr;
  args[5].l = locals.add(to_java_list(env, (value.placements).items, (value.placements).count, [&](const auto& item) -> jobject { return to_java_holder_asset_placement(env, item); }));
  if (env->ExceptionCheck()) return nullptr;
  args[6].j = static_cast<jlong>(value.created_at);
  args[7].j = static_cast<jlong>(value.updated_at);
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_resource(JNIEnv* env, const holder_resource& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/Resource");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/util/Map;Ljava/util/List;JJ)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[8] = {};
  args[0].l = locals.add(string(env, value.resource_id));
  if (env->ExceptionCheck()) return nullptr;
  args[1].l = locals.add(string(env, value.project_id));
  if (env->ExceptionCheck()) return nullptr;
  args[2].l = locals.add(string(env, value.type));
  if (env->ExceptionCheck()) return nullptr;
  args[3].l = locals.add(string(env, value.label));
  if (env->ExceptionCheck()) return nullptr;
  args[4].l = locals.add(to_java_list_map(env, value.metadata));
  if (env->ExceptionCheck()) return nullptr;
  args[5].l = locals.add(to_java_list(env, (value.assets).items, (value.assets).count, [&](const auto& item) -> jobject { return to_java_holder_resource_asset(env, item); }));
  if (env->ExceptionCheck()) return nullptr;
  args[6].j = static_cast<jlong>(value.created_at);
  args[7].j = static_cast<jlong>(value.updated_at);
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_project_list_item(JNIEnv* env, const holder_project_list_item& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/ProjectListItem");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Lteam/holder/android/contract/ProjectPrivacyMode;Ljava/lang/String;JJLjava/lang/Integer;Ljava/lang/Integer;)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[11] = {};
  args[0].l = locals.add(string(env, value.project_id));
  if (env->ExceptionCheck()) return nullptr;
  args[1].l = locals.add(string(env, value.name));
  if (env->ExceptionCheck()) return nullptr;
  args[2].l = locals.add(string(env, value.root_path));
  if (env->ExceptionCheck()) return nullptr;
  args[3].l = locals.add(string(env, value.git_remote_url));
  if (env->ExceptionCheck()) return nullptr;
  args[4].l = locals.add(string(env, value.git_provider));
  if (env->ExceptionCheck()) return nullptr;
  args[5].l = locals.add(to_java_holder_project_privacy_mode(env, static_cast<int>(value.privacy_mode)));
  if (env->ExceptionCheck()) return nullptr;
  args[6].l = locals.add(string(env, value.project_key_id));
  if (env->ExceptionCheck()) return nullptr;
  args[7].j = static_cast<jlong>(value.created_at);
  args[8].j = static_cast<jlong>(value.updated_at);
  args[9].l = locals.add((value.card_count ? box_int32(env, *value.card_count) : nullptr));
  if (env->ExceptionCheck()) return nullptr;
  args[10].l = locals.add((value.root_card_count ? box_int32(env, *value.root_card_count) : nullptr));
  if (env->ExceptionCheck()) return nullptr;
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_project(JNIEnv* env, const holder_project& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/Project");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Lteam/holder/android/contract/ProjectPrivacyMode;Ljava/lang/String;JJ)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[9] = {};
  args[0].l = locals.add(string(env, value.project_id));
  if (env->ExceptionCheck()) return nullptr;
  args[1].l = locals.add(string(env, value.name));
  if (env->ExceptionCheck()) return nullptr;
  args[2].l = locals.add(string(env, value.root_path));
  if (env->ExceptionCheck()) return nullptr;
  args[3].l = locals.add(string(env, value.git_remote_url));
  if (env->ExceptionCheck()) return nullptr;
  args[4].l = locals.add(string(env, value.git_provider));
  if (env->ExceptionCheck()) return nullptr;
  args[5].l = locals.add(to_java_holder_project_privacy_mode(env, static_cast<int>(value.privacy_mode)));
  if (env->ExceptionCheck()) return nullptr;
  args[6].l = locals.add(string(env, value.project_key_id));
  if (env->ExceptionCheck()) return nullptr;
  args[7].j = static_cast<jlong>(value.created_at);
  args[8].j = static_cast<jlong>(value.updated_at);
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_encryption_safety_check(JNIEnv* env, const holder_encryption_safety_check& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/EncryptionSafetyCheck");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(ZIILjava/util/List;Ljava/lang/String;)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[5] = {};
  args[0].z = static_cast<jboolean>(value.ok ? JNI_TRUE : JNI_FALSE);
  args[1].i = static_cast<jint>(value.checked_files);
  args[2].i = static_cast<jint>(value.unsafe_files);
  args[3].l = locals.add(to_java_list(env, (value.unsafe_paths).items, (value.unsafe_paths).count, [&](const auto& item) -> jobject { return string(env, item); }));
  if (env->ExceptionCheck()) return nullptr;
  args[4].l = locals.add(string(env, value.message));
  if (env->ExceptionCheck()) return nullptr;
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_project_encryption_check(JNIEnv* env, const holder_project_encryption_check& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/ProjectEncryptionCheck");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(Ljava/lang/String;Lteam/holder/android/contract/ProjectEncryptionCheckPrivacyMode;Lteam/holder/android/contract/EncryptionSafetyCheck;)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[3] = {};
  args[0].l = locals.add(string(env, value.project_id));
  if (env->ExceptionCheck()) return nullptr;
  args[1].l = locals.add(to_java_holder_project_encryption_check_privacy_mode(env, static_cast<int>(value.privacy_mode)));
  if (env->ExceptionCheck()) return nullptr;
  args[2].l = locals.add(to_java_holder_encryption_safety_check(env, value.check));
  if (env->ExceptionCheck()) return nullptr;
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_hierarchy_repair_move(JNIEnv* env, const holder_hierarchy_repair_move& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/HierarchyRepairMove");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(Ljava/lang/String;Ljava/lang/String;)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[2] = {};
  args[0].l = locals.add(string(env, value.card_id));
  if (env->ExceptionCheck()) return nullptr;
  args[1].l = locals.add(string(env, value.parent_card_id));
  if (env->ExceptionCheck()) return nullptr;
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_hierarchy_repair(JNIEnv* env, const holder_hierarchy_repair& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/HierarchyRepair");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(ZLjava/util/List;ILjava/util/List;)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[4] = {};
  args[0].z = static_cast<jboolean>(value.applied ? JNI_TRUE : JNI_FALSE);
  args[1].l = locals.add(to_java_list(env, (value.moved).items, (value.moved).count, [&](const auto& item) -> jobject { return to_java_holder_hierarchy_repair_move(env, item); }));
  if (env->ExceptionCheck()) return nullptr;
  args[2].i = static_cast<jint>(value.changed_card_count);
  args[3].l = locals.add(to_java_list(env, (value.cycle_card_ids).items, (value.cycle_card_ids).count, [&](const auto& item) -> jobject { return string(env, item); }));
  if (env->ExceptionCheck()) return nullptr;
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_rebuild_result(JNIEnv* env, const holder_rebuild_result& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/RebuildResult");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(Ljava/lang/String;IIII)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[5] = {};
  args[0].l = locals.add(string(env, value.project_id));
  if (env->ExceptionCheck()) return nullptr;
  args[1].i = static_cast<jint>(value.cards);
  args[2].i = static_cast<jint>(value.ai_messages);
  args[3].i = static_cast<jint>(value.ai_threads);
  args[4].i = static_cast<jint>(value.links);
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_reindex_result(JNIEnv* env, const holder_reindex_result& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/ReindexResult");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(Ljava/lang/String;)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[1] = {};
  args[0].l = locals.add(string(env, value.message));
  if (env->ExceptionCheck()) return nullptr;
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_recovery_snapshot_link(JNIEnv* env, const holder_recovery_snapshot_link& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/RecoverySnapshotLink");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(Lteam/holder/android/contract/RecoverySnapshotLinkTargetType;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[5] = {};
  args[0].l = locals.add(to_java_holder_recovery_snapshot_link_target_type(env, static_cast<int>(value.target_type)));
  if (env->ExceptionCheck()) return nullptr;
  args[1].l = locals.add(string(env, value.target_id));
  if (env->ExceptionCheck()) return nullptr;
  args[2].l = locals.add(string(env, value.target_project_id));
  if (env->ExceptionCheck()) return nullptr;
  args[3].l = locals.add(string(env, value.kind));
  if (env->ExceptionCheck()) return nullptr;
  args[4].l = locals.add(string(env, value.label));
  if (env->ExceptionCheck()) return nullptr;
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_recovery_snapshot_milestone(JNIEnv* env, const holder_recovery_snapshot_milestone& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/RecoverySnapshotMilestone");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(Ljava/util/List;Ljava/lang/String;Ljava/lang/String;)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[3] = {};
  args[0].l = locals.add(to_java_list(env, (value.dates).items, (value.dates).count, [&](const auto& item) -> jobject { return to_java_holder_milestone_date(env, item); }));
  if (env->ExceptionCheck()) return nullptr;
  args[1].l = locals.add(string(env, value.kind));
  if (env->ExceptionCheck()) return nullptr;
  args[2].l = locals.add(string(env, value.description));
  if (env->ExceptionCheck()) return nullptr;
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_recovery_snapshot_placement(JNIEnv* env, const holder_recovery_snapshot_placement& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/RecoverySnapshotPlacement");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Lteam/holder/android/contract/RecoverySnapshotPlacementEncoding;JLjava/lang/String;J)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[7] = {};
  args[0].l = locals.add(string(env, value.placement_id));
  if (env->ExceptionCheck()) return nullptr;
  args[1].l = locals.add(string(env, value.location_id));
  if (env->ExceptionCheck()) return nullptr;
  args[2].l = locals.add(string(env, value.object_key));
  if (env->ExceptionCheck()) return nullptr;
  args[3].l = locals.add(to_java_holder_recovery_snapshot_placement_encoding(env, static_cast<int>(value.encoding)));
  if (env->ExceptionCheck()) return nullptr;
  args[4].j = static_cast<jlong>(value.stored_byte_size);
  args[5].l = locals.add(string(env, value.stored_sha256));
  if (env->ExceptionCheck()) return nullptr;
  args[6].j = static_cast<jlong>(value.created_at);
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_recovery_snapshot_asset(JNIEnv* env, const holder_recovery_snapshot_asset& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/RecoverySnapshotAsset");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;JLjava/lang/String;Ljava/util/List;JJ)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[8] = {};
  args[0].l = locals.add(string(env, value.asset_id));
  if (env->ExceptionCheck()) return nullptr;
  args[1].l = locals.add(string(env, value.original_filename));
  if (env->ExceptionCheck()) return nullptr;
  args[2].l = locals.add(string(env, value.media_type));
  if (env->ExceptionCheck()) return nullptr;
  args[3].j = static_cast<jlong>(value.byte_size);
  args[4].l = locals.add(string(env, value.plaintext_sha256));
  if (env->ExceptionCheck()) return nullptr;
  args[5].l = locals.add(to_java_list(env, (value.placements).items, (value.placements).count, [&](const auto& item) -> jobject { return to_java_holder_recovery_snapshot_placement(env, item); }));
  if (env->ExceptionCheck()) return nullptr;
  args[6].j = static_cast<jlong>(value.created_at);
  args[7].j = static_cast<jlong>(value.updated_at);
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_recovery_snapshot_resource(JNIEnv* env, const holder_recovery_snapshot_resource& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/RecoverySnapshotResource");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/util/Map;Ljava/util/List;JJ)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[7] = {};
  args[0].l = locals.add(string(env, value.resource_id));
  if (env->ExceptionCheck()) return nullptr;
  args[1].l = locals.add(string(env, value.type));
  if (env->ExceptionCheck()) return nullptr;
  args[2].l = locals.add(string(env, value.label));
  if (env->ExceptionCheck()) return nullptr;
  args[3].l = locals.add(to_java_list_map(env, value.metadata));
  if (env->ExceptionCheck()) return nullptr;
  args[4].l = locals.add(to_java_list(env, (value.assets).items, (value.assets).count, [&](const auto& item) -> jobject { return to_java_holder_recovery_snapshot_asset(env, item); }));
  if (env->ExceptionCheck()) return nullptr;
  args[5].j = static_cast<jlong>(value.created_at);
  args[6].j = static_cast<jlong>(value.updated_at);
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_recovery_snapshot_location(JNIEnv* env, const holder_recovery_snapshot_location& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/RecoverySnapshotLocation");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(Ljava/lang/String;Ljava/lang/String;Lteam/holder/android/contract/RecoverySnapshotLocationProvider;Ljava/util/Map;JJ)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[6] = {};
  args[0].l = locals.add(string(env, value.location_id));
  if (env->ExceptionCheck()) return nullptr;
  args[1].l = locals.add(string(env, value.name));
  if (env->ExceptionCheck()) return nullptr;
  args[2].l = locals.add(to_java_holder_recovery_snapshot_location_provider(env, static_cast<int>(value.provider)));
  if (env->ExceptionCheck()) return nullptr;
  args[3].l = locals.add(to_java_text_map(env, value.configuration));
  if (env->ExceptionCheck()) return nullptr;
  args[4].j = static_cast<jlong>(value.created_at);
  args[5].j = static_cast<jlong>(value.updated_at);
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_recovery_snapshot_card(JNIEnv* env, const holder_recovery_snapshot_card& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/RecoverySnapshotCard");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Lteam/holder/android/contract/RecoverySnapshotCardPrivacyMode;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;DJJLjava/util/List;Ljava/util/List;Ljava/util/List;Ljava/util/List;)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[14] = {};
  args[0].l = locals.add(string(env, value.card_id));
  if (env->ExceptionCheck()) return nullptr;
  args[1].l = locals.add(string(env, value.project_id));
  if (env->ExceptionCheck()) return nullptr;
  args[2].l = locals.add(string(env, value.project_name));
  if (env->ExceptionCheck()) return nullptr;
  args[3].l = locals.add(to_java_holder_recovery_snapshot_card_privacy_mode(env, static_cast<int>(value.privacy_mode)));
  if (env->ExceptionCheck()) return nullptr;
  args[4].l = locals.add(string(env, value.title));
  if (env->ExceptionCheck()) return nullptr;
  args[5].l = locals.add(string(env, value.content));
  if (env->ExceptionCheck()) return nullptr;
  args[6].l = locals.add(string(env, value.parent_card_id));
  if (env->ExceptionCheck()) return nullptr;
  args[7].d = static_cast<jdouble>(value.sort_key);
  args[8].j = static_cast<jlong>(value.created_at);
  args[9].j = static_cast<jlong>(value.updated_at);
  args[10].l = locals.add(to_java_list(env, (value.links).items, (value.links).count, [&](const auto& item) -> jobject { return to_java_holder_recovery_snapshot_link(env, item); }));
  if (env->ExceptionCheck()) return nullptr;
  args[11].l = locals.add(to_java_list(env, (value.milestones).items, (value.milestones).count, [&](const auto& item) -> jobject { return to_java_holder_recovery_snapshot_milestone(env, item); }));
  if (env->ExceptionCheck()) return nullptr;
  args[12].l = locals.add(to_java_list(env, (value.resources).items, (value.resources).count, [&](const auto& item) -> jobject { return to_java_holder_recovery_snapshot_resource(env, item); }));
  if (env->ExceptionCheck()) return nullptr;
  args[13].l = locals.add(to_java_list(env, (value.locations).items, (value.locations).count, [&](const auto& item) -> jobject { return to_java_holder_recovery_snapshot_location(env, item); }));
  if (env->ExceptionCheck()) return nullptr;
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_recovery_snapshot_page(JNIEnv* env, const holder_recovery_snapshot_page& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/RecoverySnapshotPage");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(Ljava/util/List;Ljava/util/List;Ljava/lang/String;)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[3] = {};
  args[0].l = locals.add(to_java_list(env, (value.cards).items, (value.cards).count, [&](const auto& item) -> jobject { return to_java_holder_recovery_snapshot_card(env, item); }));
  if (env->ExceptionCheck()) return nullptr;
  args[1].l = locals.add(to_java_list(env, (value.lines).items, (value.lines).count, [&](const auto& item) -> jobject { return string(env, item); }));
  if (env->ExceptionCheck()) return nullptr;
  args[2].l = locals.add(string(env, value.next_cursor));
  if (env->ExceptionCheck()) return nullptr;
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_project_recovery_token_export_result(JNIEnv* env, const holder_project_recovery_token_export_result& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/ProjectRecoveryTokenExportResult");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[3] = {};
  args[0].l = locals.add(string(env, value.project_id));
  if (env->ExceptionCheck()) return nullptr;
  args[1].l = locals.add(string(env, value.key_id));
  if (env->ExceptionCheck()) return nullptr;
  args[2].l = locals.add(string(env, value.recovery_token));
  if (env->ExceptionCheck()) return nullptr;
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_project_recovery_token_import_result(JNIEnv* env, const holder_project_recovery_token_import_result& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/ProjectRecoveryTokenImportResult");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(Ljava/lang/String;)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[1] = {};
  args[0].l = locals.add(string(env, value.project_id));
  if (env->ExceptionCheck()) return nullptr;
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_recovery_token_global_import_result(JNIEnv* env, const holder_recovery_token_global_import_result& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/RecoveryTokenGlobalImportResult");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(Ljava/lang/String;ZZZLjava/lang/String;Lteam/holder/android/contract/RecoveryTokenGlobalImportResultPullStatus;Ljava/lang/String;)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[7] = {};
  args[0].l = locals.add(string(env, value.project_id));
  if (env->ExceptionCheck()) return nullptr;
  args[1].z = static_cast<jboolean>(value.project_created ? JNI_TRUE : JNI_FALSE);
  args[2].z = static_cast<jboolean>(value.remote_hint_present ? JNI_TRUE : JNI_FALSE);
  args[3].z = static_cast<jboolean>(value.remote_configured ? JNI_TRUE : JNI_FALSE);
  args[4].l = locals.add(string(env, value.remote_error));
  if (env->ExceptionCheck()) return nullptr;
  args[5].l = locals.add(to_java_holder_recovery_token_global_import_result_pull_status(env, static_cast<int>(value.pull_status)));
  if (env->ExceptionCheck()) return nullptr;
  args[6].l = locals.add(string(env, value.pull_error));
  if (env->ExceptionCheck()) return nullptr;
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_sync_state(JNIEnv* env, const holder_sync_state& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/SyncState");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(Ljava/lang/String;Ljava/lang/Long;Ljava/lang/Long;Ljava/lang/Long;IILjava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/Long;ILjava/lang/Long;ILjava/lang/Long;Ljava/lang/Long;)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[15] = {};
  args[0].l = locals.add(string(env, value.project_id));
  if (env->ExceptionCheck()) return nullptr;
  args[1].l = locals.add((value.last_commit_at ? box_int64(env, *value.last_commit_at) : nullptr));
  if (env->ExceptionCheck()) return nullptr;
  args[2].l = locals.add((value.last_push_at ? box_int64(env, *value.last_push_at) : nullptr));
  if (env->ExceptionCheck()) return nullptr;
  args[3].l = locals.add((value.last_pull_at ? box_int64(env, *value.last_pull_at) : nullptr));
  if (env->ExceptionCheck()) return nullptr;
  args[4].i = static_cast<jint>(value.uncommitted_changes_count);
  args[5].i = static_cast<jint>(value.unpushed_commits_count);
  args[6].l = locals.add(string(env, value.last_push_status));
  if (env->ExceptionCheck()) return nullptr;
  args[7].l = locals.add(string(env, value.last_pull_status));
  if (env->ExceptionCheck()) return nullptr;
  args[8].l = locals.add(string(env, value.last_sync_error));
  if (env->ExceptionCheck()) return nullptr;
  args[9].l = locals.add((value.last_sync_error_at ? box_int64(env, *value.last_sync_error_at) : nullptr));
  if (env->ExceptionCheck()) return nullptr;
  args[10].i = static_cast<jint>(value.retry_count);
  args[11].l = locals.add((value.next_retry_at ? box_int64(env, *value.next_retry_at) : nullptr));
  if (env->ExceptionCheck()) return nullptr;
  args[12].i = static_cast<jint>(value.pull_retry_count);
  args[13].l = locals.add((value.next_pull_retry_at ? box_int64(env, *value.next_pull_retry_at) : nullptr));
  if (env->ExceptionCheck()) return nullptr;
  args[14].l = locals.add((value.updated_at ? box_int64(env, *value.updated_at) : nullptr));
  if (env->ExceptionCheck()) return nullptr;
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_pull_phase(JNIEnv* env, const holder_pull_phase& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/PullPhase");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(ZLteam/holder/android/contract/PullPhaseStatus;IILjava/lang/String;)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[5] = {};
  args[0].z = static_cast<jboolean>(value.attempted ? JNI_TRUE : JNI_FALSE);
  args[1].l = locals.add(to_java_holder_pull_phase_status(env, static_cast<int>(value.status)));
  if (env->ExceptionCheck()) return nullptr;
  args[2].i = static_cast<jint>(value.conflicts_resolved);
  args[3].i = static_cast<jint>(value.hidden_cards_repaired);
  args[4].l = locals.add(string(env, value.error_message));
  if (env->ExceptionCheck()) return nullptr;
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_push_phase(JNIEnv* env, const holder_push_phase& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/PushPhase");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(ZLteam/holder/android/contract/PushPhaseStatus;IILjava/lang/String;Ljava/lang/String;)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[6] = {};
  args[0].z = static_cast<jboolean>(value.attempted ? JNI_TRUE : JNI_FALSE);
  args[1].l = locals.add(to_java_holder_push_phase_status(env, static_cast<int>(value.status)));
  if (env->ExceptionCheck()) return nullptr;
  args[2].i = static_cast<jint>(value.ahead_count);
  args[3].i = static_cast<jint>(value.behind_count);
  args[4].l = locals.add(string(env, value.local_head_commit));
  if (env->ExceptionCheck()) return nullptr;
  args[5].l = locals.add(string(env, value.error_message));
  if (env->ExceptionCheck()) return nullptr;
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_sync_result(JNIEnv* env, const holder_sync_result& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/SyncResult");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(Ljava/lang/String;Lteam/holder/android/contract/SyncResultStatus;Ljava/lang/String;Lteam/holder/android/contract/PullPhase;Lteam/holder/android/contract/PushPhase;)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[5] = {};
  args[0].l = locals.add(string(env, value.project_id));
  if (env->ExceptionCheck()) return nullptr;
  args[1].l = locals.add(to_java_holder_sync_result_status(env, static_cast<int>(value.status)));
  if (env->ExceptionCheck()) return nullptr;
  args[2].l = locals.add(string(env, value.remote_url));
  if (env->ExceptionCheck()) return nullptr;
  args[3].l = locals.add(to_java_holder_pull_phase(env, value.pull));
  if (env->ExceptionCheck()) return nullptr;
  args[4].l = locals.add(to_java_holder_push_phase(env, value.push));
  if (env->ExceptionCheck()) return nullptr;
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_due_sync_result(JNIEnv* env, const holder_due_sync_result& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/DueSyncResult");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(Ljava/lang/String;Ljava/lang/String;Lteam/holder/android/contract/PullPhase;Lteam/holder/android/contract/PushPhase;)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[4] = {};
  args[0].l = locals.add(string(env, value.project_id));
  if (env->ExceptionCheck()) return nullptr;
  args[1].l = locals.add(string(env, value.remote_url));
  if (env->ExceptionCheck()) return nullptr;
  args[2].l = locals.add(to_java_holder_pull_phase(env, value.pull));
  if (env->ExceptionCheck()) return nullptr;
  args[3].l = locals.add(to_java_holder_push_phase(env, value.push));
  if (env->ExceptionCheck()) return nullptr;
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_remote_test_result(JNIEnv* env, const holder_remote_test_result& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/RemoteTestResult");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(Ljava/lang/String;Lteam/holder/android/contract/RemoteTestResultStatus;Ljava/lang/String;ZLjava/lang/String;)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[5] = {};
  args[0].l = locals.add(string(env, value.project_id));
  if (env->ExceptionCheck()) return nullptr;
  args[1].l = locals.add(to_java_holder_remote_test_result_status(env, static_cast<int>(value.status)));
  if (env->ExceptionCheck()) return nullptr;
  args[2].l = locals.add(string(env, value.remote_url));
  if (env->ExceptionCheck()) return nullptr;
  args[3].z = static_cast<jboolean>(value.remote_has_head ? JNI_TRUE : JNI_FALSE);
  args[4].l = locals.add(string(env, value.error_message));
  if (env->ExceptionCheck()) return nullptr;
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_history_author(JNIEnv* env, const holder_history_author& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/HistoryAuthor");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(Ljava/lang/String;Ljava/lang/String;)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[2] = {};
  args[0].l = locals.add(string(env, value.name));
  if (env->ExceptionCheck()) return nullptr;
  args[1].l = locals.add(string(env, value.email));
  if (env->ExceptionCheck()) return nullptr;
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_history_affected_path(JNIEnv* env, const holder_history_affected_path& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/HistoryAffectedPath");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[3] = {};
  args[0].l = locals.add(string(env, value.path));
  if (env->ExceptionCheck()) return nullptr;
  args[1].l = locals.add(string(env, value.title));
  if (env->ExceptionCheck()) return nullptr;
  args[2].l = locals.add(string(env, value.detail));
  if (env->ExceptionCheck()) return nullptr;
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_history_affected_object(JNIEnv* env, const holder_history_affected_object& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/HistoryAffectedObject");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(Lteam/holder/android/contract/HistoryAffectedObjectKind;Ljava/util/List;Ljava/util/List;)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[3] = {};
  args[0].l = locals.add(to_java_holder_history_affected_object_kind(env, static_cast<int>(value.kind)));
  if (env->ExceptionCheck()) return nullptr;
  args[1].l = locals.add(to_java_list(env, (value.paths).items, (value.paths).count, [&](const auto& item) -> jobject { return string(env, item); }));
  if (env->ExceptionCheck()) return nullptr;
  args[2].l = locals.add(to_java_list(env, (value.items).items, (value.items).count, [&](const auto& item) -> jobject { return to_java_holder_history_affected_path(env, item); }));
  if (env->ExceptionCheck()) return nullptr;
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_project_activity(JNIEnv* env, const holder_project_activity& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/ProjectActivity");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(Ljava/lang/String;Ljava/util/List;Lteam/holder/android/contract/HistoryAuthor;JJLjava/lang/String;Ljava/util/List;Z)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[8] = {};
  args[0].l = locals.add(string(env, value.oid));
  if (env->ExceptionCheck()) return nullptr;
  args[1].l = locals.add(to_java_list(env, (value.parent_oids).items, (value.parent_oids).count, [&](const auto& item) -> jobject { return string(env, item); }));
  if (env->ExceptionCheck()) return nullptr;
  args[2].l = locals.add(to_java_holder_history_author(env, value.author));
  if (env->ExceptionCheck()) return nullptr;
  args[3].j = static_cast<jlong>(value.authored_at);
  args[4].j = static_cast<jlong>(value.committed_at);
  args[5].l = locals.add(string(env, value.message));
  if (env->ExceptionCheck()) return nullptr;
  args[6].l = locals.add(to_java_list(env, (value.affected_objects).items, (value.affected_objects).count, [&](const auto& item) -> jobject { return to_java_holder_history_affected_object(env, item); }));
  if (env->ExceptionCheck()) return nullptr;
  args[7].z = static_cast<jboolean>(value.is_merge ? JNI_TRUE : JNI_FALSE);
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_project_history_page(JNIEnv* env, const holder_project_history_page& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/ProjectHistoryPage");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(Ljava/lang/String;Ljava/util/List;Ljava/lang/String;Z)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[4] = {};
  args[0].l = locals.add(string(env, value.head_oid));
  if (env->ExceptionCheck()) return nullptr;
  args[1].l = locals.add(to_java_list(env, (value.activities).items, (value.activities).count, [&](const auto& item) -> jobject { return to_java_holder_project_activity(env, item); }));
  if (env->ExceptionCheck()) return nullptr;
  args[2].l = locals.add(string(env, value.next_cursor));
  if (env->ExceptionCheck()) return nullptr;
  args[3].z = static_cast<jboolean>(value.scan_limited ? JNI_TRUE : JNI_FALSE);
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_card_history_save(JNIEnv* env, const holder_card_history_save& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/CardHistorySave");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(Ljava/lang/String;Ljava/util/List;JJLjava/lang/String;)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[5] = {};
  args[0].l = locals.add(string(env, value.oid));
  if (env->ExceptionCheck()) return nullptr;
  args[1].l = locals.add(to_java_list(env, (value.parent_oids).items, (value.parent_oids).count, [&](const auto& item) -> jobject { return string(env, item); }));
  if (env->ExceptionCheck()) return nullptr;
  args[2].j = static_cast<jlong>(value.authored_at);
  args[3].j = static_cast<jlong>(value.committed_at);
  args[4].l = locals.add(string(env, value.message));
  if (env->ExceptionCheck()) return nullptr;
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_card_history_entry(JNIEnv* env, const holder_card_history_entry& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/CardHistoryEntry");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/util/List;Ljava/util/List;Lteam/holder/android/contract/HistoryAuthor;JJLjava/lang/String;Ljava/lang/String;IZLjava/util/List;)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[12] = {};
  args[0].l = locals.add(string(env, value.first_oid));
  if (env->ExceptionCheck()) return nullptr;
  args[1].l = locals.add(string(env, value.last_oid));
  if (env->ExceptionCheck()) return nullptr;
  args[2].l = locals.add(to_java_list(env, (value.parent_oids).items, (value.parent_oids).count, [&](const auto& item) -> jobject { return string(env, item); }));
  if (env->ExceptionCheck()) return nullptr;
  args[3].l = locals.add(to_java_list(env, (value.visible_parent_oids).items, (value.visible_parent_oids).count, [&](const auto& item) -> jobject { return string(env, item); }));
  if (env->ExceptionCheck()) return nullptr;
  args[4].l = locals.add(to_java_holder_history_author(env, value.author));
  if (env->ExceptionCheck()) return nullptr;
  args[5].j = static_cast<jlong>(value.started_at);
  args[6].j = static_cast<jlong>(value.ended_at);
  args[7].l = locals.add(string(env, value.kind));
  if (env->ExceptionCheck()) return nullptr;
  args[8].l = locals.add(string(env, value.summary));
  if (env->ExceptionCheck()) return nullptr;
  args[9].i = static_cast<jint>(value.commit_count);
  args[10].z = static_cast<jboolean>(value.is_merge ? JNI_TRUE : JNI_FALSE);
  args[11].l = locals.add(to_java_list(env, (value.saves).items, (value.saves).count, [&](const auto& item) -> jobject { return to_java_holder_card_history_save(env, item); }));
  if (env->ExceptionCheck()) return nullptr;
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_card_history_page(JNIEnv* env, const holder_card_history_page& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/CardHistoryPage");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(Ljava/lang/String;Ljava/util/List;Ljava/lang/String;Z)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[4] = {};
  args[0].l = locals.add(string(env, value.head_oid));
  if (env->ExceptionCheck()) return nullptr;
  args[1].l = locals.add(to_java_list(env, (value.entries).items, (value.entries).count, [&](const auto& item) -> jobject { return to_java_holder_card_history_entry(env, item); }));
  if (env->ExceptionCheck()) return nullptr;
  args[2].l = locals.add(string(env, value.next_cursor));
  if (env->ExceptionCheck()) return nullptr;
  args[3].z = static_cast<jboolean>(value.scan_limited ? JNI_TRUE : JNI_FALSE);
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_card_history_version(JNIEnv* env, const holder_card_history_version& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/CardHistoryVersion");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(ZLjava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[4] = {};
  args[0].z = static_cast<jboolean>(value.exists ? JNI_TRUE : JNI_FALSE);
  args[1].l = locals.add(string(env, value.oid));
  if (env->ExceptionCheck()) return nullptr;
  args[2].l = locals.add(string(env, value.title));
  if (env->ExceptionCheck()) return nullptr;
  args[3].l = locals.add(string(env, value.body));
  if (env->ExceptionCheck()) return nullptr;
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_card_history_diff_line(JNIEnv* env, const holder_card_history_diff_line& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/CardHistoryDiffLine");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/Long;Ljava/lang/Long;)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[4] = {};
  args[0].l = locals.add(string(env, value.origin));
  if (env->ExceptionCheck()) return nullptr;
  args[1].l = locals.add(string(env, value.text));
  if (env->ExceptionCheck()) return nullptr;
  args[2].l = locals.add((value.old_line ? box_int64(env, *value.old_line) : nullptr));
  if (env->ExceptionCheck()) return nullptr;
  args[3].l = locals.add((value.new_line ? box_int64(env, *value.new_line) : nullptr));
  if (env->ExceptionCheck()) return nullptr;
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_card_history_comparison(JNIEnv* env, const holder_card_history_comparison& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/CardHistoryComparison");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(Lteam/holder/android/contract/CardHistoryVersion;Lteam/holder/android/contract/CardHistoryVersion;Ljava/lang/String;Ljava/util/List;Z)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[5] = {};
  args[0].l = locals.add(to_java_holder_card_history_version(env, value.before));
  if (env->ExceptionCheck()) return nullptr;
  args[1].l = locals.add(to_java_holder_card_history_version(env, value.after));
  if (env->ExceptionCheck()) return nullptr;
  args[2].l = locals.add(string(env, value.summary));
  if (env->ExceptionCheck()) return nullptr;
  args[3].l = locals.add(to_java_list(env, (value.lines).items, (value.lines).count, [&](const auto& item) -> jobject { return to_java_holder_card_history_diff_line(env, item); }));
  if (env->ExceptionCheck()) return nullptr;
  args[4].z = static_cast<jboolean>(value.truncated ? JNI_TRUE : JNI_FALSE);
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_card_history_snapshot(JNIEnv* env, const holder_card_history_snapshot& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/CardHistorySnapshot");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(Ljava/lang/String;Lteam/holder/android/contract/CardHistoryVersion;)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[2] = {};
  args[0].l = locals.add(string(env, value.card_id));
  if (env->ExceptionCheck()) return nullptr;
  args[1].l = locals.add(to_java_holder_card_history_version(env, value.snapshot));
  if (env->ExceptionCheck()) return nullptr;
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_card_history_restore_result(JNIEnv* env, const holder_card_history_restore_result& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/CardHistoryRestoreResult");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;JLjava/lang/Long;)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[6] = {};
  args[0].l = locals.add(string(env, value.card_id));
  if (env->ExceptionCheck()) return nullptr;
  args[1].l = locals.add(string(env, value.restored_from_oid));
  if (env->ExceptionCheck()) return nullptr;
  args[2].l = locals.add(string(env, value.result_oid));
  if (env->ExceptionCheck()) return nullptr;
  args[3].l = locals.add(string(env, value.title));
  if (env->ExceptionCheck()) return nullptr;
  args[4].j = static_cast<jlong>(value.updated_at);
  args[5].l = locals.add((value.deleted_at ? box_int64(env, *value.deleted_at) : nullptr));
  if (env->ExceptionCheck()) return nullptr;
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_trash_item(JNIEnv* env, const holder_trash_item& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/TrashItem");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(Lteam/holder/android/contract/TrashItemType;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;JLjava/lang/Long;Ljava/lang/String;)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[10] = {};
  args[0].l = locals.add(to_java_holder_trash_item_type(env, static_cast<int>(value.type)));
  if (env->ExceptionCheck()) return nullptr;
  args[1].l = locals.add(string(env, value.project_id));
  if (env->ExceptionCheck()) return nullptr;
  args[2].l = locals.add(string(env, value.card_id));
  if (env->ExceptionCheck()) return nullptr;
  args[3].l = locals.add(string(env, value.message_id));
  if (env->ExceptionCheck()) return nullptr;
  args[4].l = locals.add(string(env, value.thread_id));
  if (env->ExceptionCheck()) return nullptr;
  args[5].l = locals.add(string(env, value.title));
  if (env->ExceptionCheck()) return nullptr;
  args[6].l = locals.add(string(env, value.role));
  if (env->ExceptionCheck()) return nullptr;
  args[7].j = static_cast<jlong>(value.deleted_at);
  args[8].l = locals.add((value.created_at ? box_int64(env, *value.created_at) : nullptr));
  if (env->ExceptionCheck()) return nullptr;
  args[9].l = locals.add(string(env, value.rel_path));
  if (env->ExceptionCheck()) return nullptr;
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_trash_empty_result(JNIEnv* env, const holder_trash_empty_result& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/TrashEmptyResult");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(Ljava/lang/String;)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[1] = {};
  args[0].l = locals.add(string(env, value.project_id));
  if (env->ExceptionCheck()) return nullptr;
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_trash_delete_result(JNIEnv* env, const holder_trash_delete_result& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/TrashDeleteResult");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(Ljava/lang/String;Ljava/lang/String;)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[2] = {};
  args[0].l = locals.add(string(env, value.card_id));
  if (env->ExceptionCheck()) return nullptr;
  args[1].l = locals.add(string(env, value.message_id));
  if (env->ExceptionCheck()) return nullptr;
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_search_card(JNIEnv* env, const holder_search_card& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/SearchCard");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(Ljava/lang/String;Ljava/lang/String;JJLjava/lang/String;D)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[6] = {};
  args[0].l = locals.add(string(env, value.card_id));
  if (env->ExceptionCheck()) return nullptr;
  args[1].l = locals.add(string(env, value.title));
  if (env->ExceptionCheck()) return nullptr;
  args[2].j = static_cast<jlong>(value.updated_at);
  args[3].j = static_cast<jlong>(value.created_at);
  args[4].l = locals.add(string(env, value.snippet));
  if (env->ExceptionCheck()) return nullptr;
  args[5].d = static_cast<jdouble>(value.rank);
  return env->NewObjectA(type, constructor, args);
}

jobject to_java_holder_search_message(JNIEnv* env, const holder_search_message& value) {
  static const jclass type = global_class(env, "team/holder/android/contract/SearchMessage");
  if (!type) return nullptr;
  static const jmethodID constructor = env->GetMethodID(type, "<init>", "(Ljava/lang/String;JLjava/lang/String;D)V");
  if (!constructor) return nullptr;
  Locals locals(env);
  jvalue args[4] = {};
  args[0].l = locals.add(string(env, value.message_id));
  if (env->ExceptionCheck()) return nullptr;
  args[1].j = static_cast<jlong>(value.created_at);
  args[2].l = locals.add(string(env, value.snippet));
  if (env->ExceptionCheck()) return nullptr;
  args[3].d = static_cast<jdouble>(value.rank);
  return env->NewObjectA(type, constructor, args);
}

bool from_java_holder_milestone_date(JNIEnv* env, jobject object, holder_milestone_date& out, Keep& keep) {
  static const jclass type = global_class(env, "team/holder/android/contract/MilestoneDate");
  if (!type) return false;
  static const jfieldID field_role = env->GetFieldID(type, "role", "Ljava/lang/String;");
  static const jfieldID field_date = env->GetFieldID(type, "date", "Ljava/lang/String;");
  static const jfieldID field_datetime = env->GetFieldID(type, "datetime", "Ljava/lang/String;");
  out = holder_milestone_date{};
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_role));
    if (env->ExceptionCheck()) return false;
    if (!c_text(env, static_cast<jstring>(item), keep, out.role)) return false;
  }
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_date));
    if (env->ExceptionCheck()) return false;
    if (!c_text(env, static_cast<jstring>(item), keep, out.date)) return false;
  }
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_datetime));
    if (env->ExceptionCheck()) return false;
    if (!c_text(env, static_cast<jstring>(item), keep, out.datetime)) return false;
  }
  return !env->ExceptionCheck();
}

bool from_java_holder_new_storage_location(JNIEnv* env, jobject object, holder_new_storage_location& out, Keep& keep) {
  static const jclass type = global_class(env, "team/holder/android/contract/NewStorageLocation");
  if (!type) return false;
  static const jfieldID field_location_id = env->GetFieldID(type, "locationId", "Ljava/lang/String;");
  static const jfieldID field_project_id = env->GetFieldID(type, "projectId", "Ljava/lang/String;");
  static const jfieldID field_name = env->GetFieldID(type, "name", "Ljava/lang/String;");
  static const jfieldID field_provider = env->GetFieldID(type, "provider", "Lteam/holder/android/contract/NewStorageLocationProvider;");
  static const jfieldID field_configuration = env->GetFieldID(type, "configuration", "Ljava/util/Map;");
  static const jfieldID field_created_at = env->GetFieldID(type, "createdAt", "Ljava/lang/Long;");
  static const jfieldID field_updated_at = env->GetFieldID(type, "updatedAt", "Ljava/lang/Long;");
  out = holder_new_storage_location{};
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_location_id));
    if (env->ExceptionCheck()) return false;
    if (!c_text(env, static_cast<jstring>(item), keep, out.location_id)) return false;
  }
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_project_id));
    if (env->ExceptionCheck()) return false;
    if (!c_text(env, static_cast<jstring>(item), keep, out.project_id)) return false;
  }
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_name));
    if (env->ExceptionCheck()) return false;
    if (!c_text(env, static_cast<jstring>(item), keep, out.name)) return false;
  }
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_provider));
    if (env->ExceptionCheck()) return false;
    out.provider = item ? static_cast<holder_new_storage_location_provider>(ordinal(env, item) + 1) : static_cast<holder_new_storage_location_provider>(0);
  }
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_configuration));
    if (env->ExceptionCheck()) return false;
    if (item) {
      auto* held = keep.make<holder_location_configuration>();
      if (!from_java_text_map(env, item, keep, (*held))) return false;
      out.configuration = held;
    } else {
      out.configuration = nullptr;
    }
  }
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_created_at));
    if (env->ExceptionCheck()) return false;
    if (item) {
      auto* held = keep.make<std::remove_cv_t<std::remove_pointer_t<std::remove_cv_t<decltype(out.created_at)>>>>();
      *held = unbox_int64(env, item);
      out.created_at = held;
    } else {
      out.created_at = nullptr;
    }
  }
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_updated_at));
    if (env->ExceptionCheck()) return false;
    if (item) {
      auto* held = keep.make<std::remove_cv_t<std::remove_pointer_t<std::remove_cv_t<decltype(out.updated_at)>>>>();
      *held = unbox_int64(env, item);
      out.updated_at = held;
    } else {
      out.updated_at = nullptr;
    }
  }
  return !env->ExceptionCheck();
}

bool from_java_holder_storage_location_update(JNIEnv* env, jobject object, holder_storage_location_update& out, Keep& keep) {
  static const jclass type = global_class(env, "team/holder/android/contract/StorageLocationUpdate");
  if (!type) return false;
  static const jfieldID field_name = env->GetFieldID(type, "name", "Ljava/lang/String;");
  static const jfieldID field_configuration = env->GetFieldID(type, "configuration", "Ljava/util/Map;");
  static const jfieldID field_updated_at = env->GetFieldID(type, "updatedAt", "Ljava/lang/Long;");
  out = holder_storage_location_update{};
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_name));
    if (env->ExceptionCheck()) return false;
    if (!c_text(env, static_cast<jstring>(item), keep, out.name)) return false;
  }
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_configuration));
    if (env->ExceptionCheck()) return false;
    if (item) {
      auto* held = keep.make<holder_location_configuration>();
      if (!from_java_text_map(env, item, keep, (*held))) return false;
      out.configuration = held;
    } else {
      out.configuration = nullptr;
    }
  }
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_updated_at));
    if (env->ExceptionCheck()) return false;
    if (item) {
      auto* held = keep.make<std::remove_cv_t<std::remove_pointer_t<std::remove_cv_t<decltype(out.updated_at)>>>>();
      *held = unbox_int64(env, item);
      out.updated_at = held;
    } else {
      out.updated_at = nullptr;
    }
  }
  return !env->ExceptionCheck();
}

bool from_java_holder_card_reference_resolve_request(JNIEnv* env, jobject object, holder_card_reference_resolve_request& out, Keep& keep) {
  static const jclass type = global_class(env, "team/holder/android/contract/CardReferenceResolveRequest");
  if (!type) return false;
  static const jfieldID field_project_id = env->GetFieldID(type, "projectId", "Ljava/lang/String;");
  static const jfieldID field_reference = env->GetFieldID(type, "reference", "Ljava/lang/String;");
  static const jfieldID field_scope = env->GetFieldID(type, "scope", "Lteam/holder/android/contract/CardReferenceResolveRequestScope;");
  out = holder_card_reference_resolve_request{};
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_project_id));
    if (env->ExceptionCheck()) return false;
    if (!c_text(env, static_cast<jstring>(item), keep, out.project_id)) return false;
  }
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_reference));
    if (env->ExceptionCheck()) return false;
    if (!c_text(env, static_cast<jstring>(item), keep, out.reference)) return false;
  }
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_scope));
    if (env->ExceptionCheck()) return false;
    out.scope = item ? static_cast<holder_card_reference_resolve_request_scope>(ordinal(env, item) + 1) : static_cast<holder_card_reference_resolve_request_scope>(0);
  }
  return !env->ExceptionCheck();
}

bool from_java_holder_new_card(JNIEnv* env, jobject object, holder_new_card& out, Keep& keep) {
  static const jclass type = global_class(env, "team/holder/android/contract/NewCard");
  if (!type) return false;
  static const jfieldID field_title = env->GetFieldID(type, "title", "Ljava/lang/String;");
  static const jfieldID field_content = env->GetFieldID(type, "content", "Ljava/lang/String;");
  static const jfieldID field_parent_card_id = env->GetFieldID(type, "parentCardId", "Ljava/lang/String;");
  static const jfieldID field_sort_key = env->GetFieldID(type, "sortKey", "Ljava/lang/Double;");
  static const jfieldID field_created_at = env->GetFieldID(type, "createdAt", "Ljava/lang/Long;");
  static const jfieldID field_updated_at = env->GetFieldID(type, "updatedAt", "Ljava/lang/Long;");
  out = holder_new_card{};
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_title));
    if (env->ExceptionCheck()) return false;
    if (!c_text(env, static_cast<jstring>(item), keep, out.title)) return false;
  }
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_content));
    if (env->ExceptionCheck()) return false;
    if (!c_text(env, static_cast<jstring>(item), keep, out.content)) return false;
  }
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_parent_card_id));
    if (env->ExceptionCheck()) return false;
    if (!c_text(env, static_cast<jstring>(item), keep, out.parent_card_id)) return false;
  }
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_sort_key));
    if (env->ExceptionCheck()) return false;
    if (item) {
      auto* held = keep.make<std::remove_cv_t<std::remove_pointer_t<std::remove_cv_t<decltype(out.sort_key)>>>>();
      *held = unbox_float64(env, item);
      out.sort_key = held;
    } else {
      out.sort_key = nullptr;
    }
  }
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_created_at));
    if (env->ExceptionCheck()) return false;
    if (item) {
      auto* held = keep.make<std::remove_cv_t<std::remove_pointer_t<std::remove_cv_t<decltype(out.created_at)>>>>();
      *held = unbox_int64(env, item);
      out.created_at = held;
    } else {
      out.created_at = nullptr;
    }
  }
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_updated_at));
    if (env->ExceptionCheck()) return false;
    if (item) {
      auto* held = keep.make<std::remove_cv_t<std::remove_pointer_t<std::remove_cv_t<decltype(out.updated_at)>>>>();
      *held = unbox_int64(env, item);
      out.updated_at = held;
    } else {
      out.updated_at = nullptr;
    }
  }
  return !env->ExceptionCheck();
}

bool from_java_holder_card_update(JNIEnv* env, jobject object, holder_card_update& out, Keep& keep) {
  static const jclass type = global_class(env, "team/holder/android/contract/CardUpdate");
  if (!type) return false;
  static const jfieldID field_title = env->GetFieldID(type, "title", "Ljava/lang/String;");
  static const jfieldID field_content = env->GetFieldID(type, "content", "Ljava/lang/String;");
  static const jfieldID field_parent_card_id = env->GetFieldID(type, "parentCardId", "Lteam/holder/android/contract/Change;");
  static const jfieldID field_sort_key = env->GetFieldID(type, "sortKey", "Ljava/lang/Double;");
  static const jfieldID field_updated_at = env->GetFieldID(type, "updatedAt", "Ljava/lang/Long;");
  out = holder_card_update{};
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_title));
    if (env->ExceptionCheck()) return false;
    if (!c_text(env, static_cast<jstring>(item), keep, out.title)) return false;
  }
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_content));
    if (env->ExceptionCheck()) return false;
    if (!c_text(env, static_cast<jstring>(item), keep, out.content)) return false;
  }
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_parent_card_id));
    if (env->ExceptionCheck()) return false;
    if (!from_java_change(env, item, keep, out.parent_card_id)) return false;
  }
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_sort_key));
    if (env->ExceptionCheck()) return false;
    if (item) {
      auto* held = keep.make<std::remove_cv_t<std::remove_pointer_t<std::remove_cv_t<decltype(out.sort_key)>>>>();
      *held = unbox_float64(env, item);
      out.sort_key = held;
    } else {
      out.sort_key = nullptr;
    }
  }
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_updated_at));
    if (env->ExceptionCheck()) return false;
    if (item) {
      auto* held = keep.make<std::remove_cv_t<std::remove_pointer_t<std::remove_cv_t<decltype(out.updated_at)>>>>();
      *held = unbox_int64(env, item);
      out.updated_at = held;
    } else {
      out.updated_at = nullptr;
    }
  }
  return !env->ExceptionCheck();
}

bool from_java_holder_card_move(JNIEnv* env, jobject object, holder_card_move& out, Keep& keep) {
  static const jclass type = global_class(env, "team/holder/android/contract/CardMove");
  if (!type) return false;
  static const jfieldID field_intent = env->GetFieldID(type, "intent", "Lteam/holder/android/contract/CardMoveIntent;");
  static const jfieldID field_target_card_id = env->GetFieldID(type, "targetCardId", "Ljava/lang/String;");
  static const jfieldID field_parent_card_id = env->GetFieldID(type, "parentCardId", "Lteam/holder/android/contract/Change;");
  out = holder_card_move{};
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_intent));
    if (env->ExceptionCheck()) return false;
    out.intent = item ? static_cast<holder_card_move_intent>(ordinal(env, item) + 1) : static_cast<holder_card_move_intent>(0);
  }
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_target_card_id));
    if (env->ExceptionCheck()) return false;
    if (!c_text(env, static_cast<jstring>(item), keep, out.target_card_id)) return false;
  }
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_parent_card_id));
    if (env->ExceptionCheck()) return false;
    if (!from_java_change(env, item, keep, out.parent_card_id)) return false;
  }
  return !env->ExceptionCheck();
}

bool from_java_holder_new_link(JNIEnv* env, jobject object, holder_new_link& out, Keep& keep) {
  static const jclass type = global_class(env, "team/holder/android/contract/NewLink");
  if (!type) return false;
  static const jfieldID field_target_type = env->GetFieldID(type, "targetType", "Lteam/holder/android/contract/NewLinkTargetType;");
  static const jfieldID field_target_id = env->GetFieldID(type, "targetId", "Ljava/lang/String;");
  static const jfieldID field_target_project_id = env->GetFieldID(type, "targetProjectId", "Ljava/lang/String;");
  static const jfieldID field_kind = env->GetFieldID(type, "kind", "Ljava/lang/String;");
  static const jfieldID field_label = env->GetFieldID(type, "label", "Ljava/lang/String;");
  static const jfieldID field_created_at = env->GetFieldID(type, "createdAt", "Ljava/lang/Long;");
  out = holder_new_link{};
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_target_type));
    if (env->ExceptionCheck()) return false;
    out.target_type = item ? static_cast<holder_new_link_target_type>(ordinal(env, item) + 1) : static_cast<holder_new_link_target_type>(0);
  }
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_target_id));
    if (env->ExceptionCheck()) return false;
    if (!c_text(env, static_cast<jstring>(item), keep, out.target_id)) return false;
  }
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_target_project_id));
    if (env->ExceptionCheck()) return false;
    if (!c_text(env, static_cast<jstring>(item), keep, out.target_project_id)) return false;
  }
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_kind));
    if (env->ExceptionCheck()) return false;
    if (!c_text(env, static_cast<jstring>(item), keep, out.kind)) return false;
  }
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_label));
    if (env->ExceptionCheck()) return false;
    if (!c_text(env, static_cast<jstring>(item), keep, out.label)) return false;
  }
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_created_at));
    if (env->ExceptionCheck()) return false;
    if (item) {
      auto* held = keep.make<std::remove_cv_t<std::remove_pointer_t<std::remove_cv_t<decltype(out.created_at)>>>>();
      *held = unbox_int64(env, item);
      out.created_at = held;
    } else {
      out.created_at = nullptr;
    }
  }
  return !env->ExceptionCheck();
}

bool from_java_holder_new_milestone(JNIEnv* env, jobject object, holder_new_milestone& out, Keep& keep) {
  static const jclass type = global_class(env, "team/holder/android/contract/NewMilestone");
  if (!type) return false;
  static const jfieldID field_dates = env->GetFieldID(type, "dates", "Ljava/util/List;");
  static const jfieldID field_kind = env->GetFieldID(type, "kind", "Ljava/lang/String;");
  static const jfieldID field_description = env->GetFieldID(type, "description", "Ljava/lang/String;");
  out = holder_new_milestone{};
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_dates));
    if (env->ExceptionCheck()) return false;
    if (!item) { fail(env, "a collection is required"); return false; }
    if (!from_java_list(env, item, keep, out.dates.items, out.dates.count,
                        [&](jobject item, auto& element) { return from_java_holder_milestone_date(env, item, element, keep); }))
      return false;
  }
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_kind));
    if (env->ExceptionCheck()) return false;
    if (!c_text(env, static_cast<jstring>(item), keep, out.kind)) return false;
  }
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_description));
    if (env->ExceptionCheck()) return false;
    if (!c_text(env, static_cast<jstring>(item), keep, out.description)) return false;
  }
  return !env->ExceptionCheck();
}

bool from_java_holder_milestone_update(JNIEnv* env, jobject object, holder_milestone_update& out, Keep& keep) {
  static const jclass type = global_class(env, "team/holder/android/contract/MilestoneUpdate");
  if (!type) return false;
  static const jfieldID field_dates = env->GetFieldID(type, "dates", "Ljava/util/List;");
  static const jfieldID field_kind = env->GetFieldID(type, "kind", "Lteam/holder/android/contract/Change;");
  static const jfieldID field_description = env->GetFieldID(type, "description", "Lteam/holder/android/contract/Change;");
  out = holder_milestone_update{};
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_dates));
    if (env->ExceptionCheck()) return false;
    if (item) {
      auto* held = keep.make<holder_milestone_date_list>();
      if (!from_java_list(env, item, keep, (*held).items, (*held).count,
                          [&](jobject item, auto& element) { return from_java_holder_milestone_date(env, item, element, keep); }))
        return false;
      out.dates = held;
    } else {
      out.dates = nullptr;
    }
  }
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_kind));
    if (env->ExceptionCheck()) return false;
    if (!from_java_change(env, item, keep, out.kind)) return false;
  }
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_description));
    if (env->ExceptionCheck()) return false;
    if (!from_java_change(env, item, keep, out.description)) return false;
  }
  return !env->ExceptionCheck();
}

bool from_java_holder_asset_import_request(JNIEnv* env, jobject object, holder_asset_import_request& out, Keep& keep) {
  static const jclass type = global_class(env, "team/holder/android/contract/AssetImportRequest");
  if (!type) return false;
  static const jfieldID field_project_id = env->GetFieldID(type, "projectId", "Ljava/lang/String;");
  static const jfieldID field_card_id = env->GetFieldID(type, "cardId", "Ljava/lang/String;");
  static const jfieldID field_location_id = env->GetFieldID(type, "locationId", "Ljava/lang/String;");
  static const jfieldID field_source_path = env->GetFieldID(type, "sourcePath", "Ljava/lang/String;");
  out = holder_asset_import_request{};
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_project_id));
    if (env->ExceptionCheck()) return false;
    if (!c_text(env, static_cast<jstring>(item), keep, out.project_id)) return false;
  }
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_card_id));
    if (env->ExceptionCheck()) return false;
    if (!c_text(env, static_cast<jstring>(item), keep, out.card_id)) return false;
  }
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_location_id));
    if (env->ExceptionCheck()) return false;
    if (!c_text(env, static_cast<jstring>(item), keep, out.location_id)) return false;
  }
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_source_path));
    if (env->ExceptionCheck()) return false;
    if (!c_text(env, static_cast<jstring>(item), keep, out.source_path)) return false;
  }
  return !env->ExceptionCheck();
}

bool from_java_holder_new_resource(JNIEnv* env, jobject object, holder_new_resource& out, Keep& keep) {
  static const jclass type = global_class(env, "team/holder/android/contract/NewResource");
  if (!type) return false;
  static const jfieldID field_resource_id = env->GetFieldID(type, "resourceId", "Ljava/lang/String;");
  static const jfieldID field_type = env->GetFieldID(type, "type", "Ljava/lang/String;");
  static const jfieldID field_label = env->GetFieldID(type, "label", "Ljava/lang/String;");
  static const jfieldID field_metadata = env->GetFieldID(type, "metadata", "Ljava/util/Map;");
  static const jfieldID field_created_at = env->GetFieldID(type, "createdAt", "Ljava/lang/Long;");
  static const jfieldID field_updated_at = env->GetFieldID(type, "updatedAt", "Ljava/lang/Long;");
  out = holder_new_resource{};
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_resource_id));
    if (env->ExceptionCheck()) return false;
    if (!c_text(env, static_cast<jstring>(item), keep, out.resource_id)) return false;
  }
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_type));
    if (env->ExceptionCheck()) return false;
    if (!c_text(env, static_cast<jstring>(item), keep, out.type)) return false;
  }
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_label));
    if (env->ExceptionCheck()) return false;
    if (!c_text(env, static_cast<jstring>(item), keep, out.label)) return false;
  }
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_metadata));
    if (env->ExceptionCheck()) return false;
    if (item) {
      auto* held = keep.make<holder_resource_metadata>();
      if (!from_java_list_map(env, item, keep, (*held))) return false;
      out.metadata = held;
    } else {
      out.metadata = nullptr;
    }
  }
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_created_at));
    if (env->ExceptionCheck()) return false;
    if (item) {
      auto* held = keep.make<std::remove_cv_t<std::remove_pointer_t<std::remove_cv_t<decltype(out.created_at)>>>>();
      *held = unbox_int64(env, item);
      out.created_at = held;
    } else {
      out.created_at = nullptr;
    }
  }
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_updated_at));
    if (env->ExceptionCheck()) return false;
    if (item) {
      auto* held = keep.make<std::remove_cv_t<std::remove_pointer_t<std::remove_cv_t<decltype(out.updated_at)>>>>();
      *held = unbox_int64(env, item);
      out.updated_at = held;
    } else {
      out.updated_at = nullptr;
    }
  }
  return !env->ExceptionCheck();
}

bool from_java_holder_resource_update(JNIEnv* env, jobject object, holder_resource_update& out, Keep& keep) {
  static const jclass type = global_class(env, "team/holder/android/contract/ResourceUpdate");
  if (!type) return false;
  static const jfieldID field_type = env->GetFieldID(type, "type", "Ljava/lang/String;");
  static const jfieldID field_label = env->GetFieldID(type, "label", "Ljava/lang/String;");
  static const jfieldID field_metadata = env->GetFieldID(type, "metadata", "Ljava/util/Map;");
  static const jfieldID field_updated_at = env->GetFieldID(type, "updatedAt", "Ljava/lang/Long;");
  out = holder_resource_update{};
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_type));
    if (env->ExceptionCheck()) return false;
    if (!c_text(env, static_cast<jstring>(item), keep, out.type)) return false;
  }
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_label));
    if (env->ExceptionCheck()) return false;
    if (!c_text(env, static_cast<jstring>(item), keep, out.label)) return false;
  }
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_metadata));
    if (env->ExceptionCheck()) return false;
    if (item) {
      auto* held = keep.make<holder_resource_metadata>();
      if (!from_java_list_map(env, item, keep, (*held))) return false;
      out.metadata = held;
    } else {
      out.metadata = nullptr;
    }
  }
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_updated_at));
    if (env->ExceptionCheck()) return false;
    if (item) {
      auto* held = keep.make<std::remove_cv_t<std::remove_pointer_t<std::remove_cv_t<decltype(out.updated_at)>>>>();
      *held = unbox_int64(env, item);
      out.updated_at = held;
    } else {
      out.updated_at = nullptr;
    }
  }
  return !env->ExceptionCheck();
}

bool from_java_holder_new_project(JNIEnv* env, jobject object, holder_new_project& out, Keep& keep) {
  static const jclass type = global_class(env, "team/holder/android/contract/NewProject");
  if (!type) return false;
  static const jfieldID field_project_id = env->GetFieldID(type, "projectId", "Ljava/lang/String;");
  static const jfieldID field_name = env->GetFieldID(type, "name", "Ljava/lang/String;");
  static const jfieldID field_root_path = env->GetFieldID(type, "rootPath", "Ljava/lang/String;");
  static const jfieldID field_git_remote_url = env->GetFieldID(type, "gitRemoteUrl", "Ljava/lang/String;");
  static const jfieldID field_git_provider = env->GetFieldID(type, "gitProvider", "Ljava/lang/String;");
  static const jfieldID field_privacy_mode = env->GetFieldID(type, "privacyMode", "Lteam/holder/android/contract/NewProjectPrivacyMode;");
  static const jfieldID field_created_at = env->GetFieldID(type, "createdAt", "Ljava/lang/Long;");
  static const jfieldID field_updated_at = env->GetFieldID(type, "updatedAt", "Ljava/lang/Long;");
  out = holder_new_project{};
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_project_id));
    if (env->ExceptionCheck()) return false;
    if (!c_text(env, static_cast<jstring>(item), keep, out.project_id)) return false;
  }
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_name));
    if (env->ExceptionCheck()) return false;
    if (!c_text(env, static_cast<jstring>(item), keep, out.name)) return false;
  }
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_root_path));
    if (env->ExceptionCheck()) return false;
    if (!c_text(env, static_cast<jstring>(item), keep, out.root_path)) return false;
  }
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_git_remote_url));
    if (env->ExceptionCheck()) return false;
    if (!c_text(env, static_cast<jstring>(item), keep, out.git_remote_url)) return false;
  }
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_git_provider));
    if (env->ExceptionCheck()) return false;
    if (!c_text(env, static_cast<jstring>(item), keep, out.git_provider)) return false;
  }
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_privacy_mode));
    if (env->ExceptionCheck()) return false;
    out.privacy_mode = item ? static_cast<holder_new_project_privacy_mode>(ordinal(env, item) + 1) : static_cast<holder_new_project_privacy_mode>(0);
  }
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_created_at));
    if (env->ExceptionCheck()) return false;
    if (item) {
      auto* held = keep.make<std::remove_cv_t<std::remove_pointer_t<std::remove_cv_t<decltype(out.created_at)>>>>();
      *held = unbox_int64(env, item);
      out.created_at = held;
    } else {
      out.created_at = nullptr;
    }
  }
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_updated_at));
    if (env->ExceptionCheck()) return false;
    if (item) {
      auto* held = keep.make<std::remove_cv_t<std::remove_pointer_t<std::remove_cv_t<decltype(out.updated_at)>>>>();
      *held = unbox_int64(env, item);
      out.updated_at = held;
    } else {
      out.updated_at = nullptr;
    }
  }
  return !env->ExceptionCheck();
}

bool from_java_holder_project_update(JNIEnv* env, jobject object, holder_project_update& out, Keep& keep) {
  static const jclass type = global_class(env, "team/holder/android/contract/ProjectUpdate");
  if (!type) return false;
  static const jfieldID field_name = env->GetFieldID(type, "name", "Ljava/lang/String;");
  static const jfieldID field_git_remote_url = env->GetFieldID(type, "gitRemoteUrl", "Lteam/holder/android/contract/Change;");
  static const jfieldID field_git_provider = env->GetFieldID(type, "gitProvider", "Lteam/holder/android/contract/Change;");
  out = holder_project_update{};
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_name));
    if (env->ExceptionCheck()) return false;
    if (!c_text(env, static_cast<jstring>(item), keep, out.name)) return false;
  }
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_git_remote_url));
    if (env->ExceptionCheck()) return false;
    if (!from_java_change(env, item, keep, out.git_remote_url)) return false;
  }
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_git_provider));
    if (env->ExceptionCheck()) return false;
    if (!from_java_change(env, item, keep, out.git_provider)) return false;
  }
  return !env->ExceptionCheck();
}

bool from_java_holder_hierarchy_repair_request(JNIEnv* env, jobject object, holder_hierarchy_repair_request& out, Keep& keep) {
  static const jclass type = global_class(env, "team/holder/android/contract/HierarchyRepairRequest");
  if (!type) return false;
  static const jfieldID field_apply = env->GetFieldID(type, "apply", "Z");
  out = holder_hierarchy_repair_request{};
  {
    auto* held = keep.make<std::remove_cv_t<std::remove_pointer_t<decltype(out.apply)>>>();
    *held = env->GetBooleanField(object, field_apply) == JNI_TRUE;
    out.apply = held;
  }
  return !env->ExceptionCheck();
}

bool from_java_holder_rebuild_request(JNIEnv* env, jobject object, holder_rebuild_request& out, Keep& keep) {
  static const jclass type = global_class(env, "team/holder/android/contract/RebuildRequest");
  if (!type) return false;
  static const jfieldID field_project_id = env->GetFieldID(type, "projectId", "Ljava/lang/String;");
  out = holder_rebuild_request{};
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_project_id));
    if (env->ExceptionCheck()) return false;
    if (!c_text(env, static_cast<jstring>(item), keep, out.project_id)) return false;
  }
  return !env->ExceptionCheck();
}

bool from_java_holder_recovery_snapshot_restore(JNIEnv* env, jobject object, holder_recovery_snapshot_restore& out, Keep& keep) {
  static const jclass type = global_class(env, "team/holder/android/contract/RecoverySnapshotRestore");
  if (!type) return false;
  static const jfieldID field_project_name = env->GetFieldID(type, "projectName", "Ljava/lang/String;");
  static const jfieldID field_privacy_mode = env->GetFieldID(type, "privacyMode", "Lteam/holder/android/contract/RecoverySnapshotRestorePrivacyMode;");
  static const jfieldID field_snapshot = env->GetFieldID(type, "snapshot", "Ljava/lang/String;");
  static const jfieldID field_commit_message = env->GetFieldID(type, "commitMessage", "Ljava/lang/String;");
  out = holder_recovery_snapshot_restore{};
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_project_name));
    if (env->ExceptionCheck()) return false;
    if (!c_text(env, static_cast<jstring>(item), keep, out.project_name)) return false;
  }
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_privacy_mode));
    if (env->ExceptionCheck()) return false;
    out.privacy_mode = item ? static_cast<holder_recovery_snapshot_restore_privacy_mode>(ordinal(env, item) + 1) : static_cast<holder_recovery_snapshot_restore_privacy_mode>(0);
  }
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_snapshot));
    if (env->ExceptionCheck()) return false;
    if (!c_text(env, static_cast<jstring>(item), keep, out.snapshot)) return false;
  }
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_commit_message));
    if (env->ExceptionCheck()) return false;
    if (!c_text(env, static_cast<jstring>(item), keep, out.commit_message)) return false;
  }
  return !env->ExceptionCheck();
}

bool from_java_holder_recovery_token_export_request(JNIEnv* env, jobject object, holder_recovery_token_export_request& out, Keep& keep) {
  static const jclass type = global_class(env, "team/holder/android/contract/RecoveryTokenExportRequest");
  if (!type) return false;
  static const jfieldID field_pin = env->GetFieldID(type, "pin", "Ljava/lang/String;");
  out = holder_recovery_token_export_request{};
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_pin));
    if (env->ExceptionCheck()) return false;
    if (!c_text(env, static_cast<jstring>(item), keep, out.pin)) return false;
  }
  return !env->ExceptionCheck();
}

bool from_java_holder_recovery_token_import_request(JNIEnv* env, jobject object, holder_recovery_token_import_request& out, Keep& keep) {
  static const jclass type = global_class(env, "team/holder/android/contract/RecoveryTokenImportRequest");
  if (!type) return false;
  static const jfieldID field_pin = env->GetFieldID(type, "pin", "Ljava/lang/String;");
  static const jfieldID field_recovery_token = env->GetFieldID(type, "recoveryToken", "Ljava/lang/String;");
  out = holder_recovery_token_import_request{};
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_pin));
    if (env->ExceptionCheck()) return false;
    if (!c_text(env, static_cast<jstring>(item), keep, out.pin)) return false;
  }
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_recovery_token));
    if (env->ExceptionCheck()) return false;
    if (!c_text(env, static_cast<jstring>(item), keep, out.recovery_token)) return false;
  }
  return !env->ExceptionCheck();
}

bool from_java_holder_push_options(JNIEnv* env, jobject object, holder_push_options& out, Keep& keep) {
  static const jclass type = global_class(env, "team/holder/android/contract/PushOptions");
  if (!type) return false;
  static const jfieldID field_branch = env->GetFieldID(type, "branch", "Ljava/lang/String;");
  static const jfieldID field_set_upstream = env->GetFieldID(type, "setUpstream", "Z");
  out = holder_push_options{};
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_branch));
    if (env->ExceptionCheck()) return false;
    if (!c_text(env, static_cast<jstring>(item), keep, out.branch)) return false;
  }
  {
    auto* held = keep.make<std::remove_cv_t<std::remove_pointer_t<decltype(out.set_upstream)>>>();
    *held = env->GetBooleanField(object, field_set_upstream) == JNI_TRUE;
    out.set_upstream = held;
  }
  return !env->ExceptionCheck();
}

bool from_java_holder_remote_test(JNIEnv* env, jobject object, holder_remote_test& out, Keep& keep) {
  static const jclass type = global_class(env, "team/holder/android/contract/RemoteTest");
  if (!type) return false;
  static const jfieldID field_remote_url = env->GetFieldID(type, "remoteUrl", "Ljava/lang/String;");
  out = holder_remote_test{};
  {
    Locals locals(env);
    jobject item = locals.add(env->GetObjectField(object, field_remote_url));
    if (env->ExceptionCheck()) return false;
    if (!c_text(env, static_cast<jstring>(item), keep, out.remote_url)) return false;
  }
  return !env->ExceptionCheck();
}

}  // namespace

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Locations_listStorageLocations(JNIEnv* env, jobject, jlong context_handle, jstring a_project_id) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_project_id = nullptr;
  if (!c_text(env, a_project_id, keep, p_project_id)) return nullptr;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_storage_location_list* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_list_storage_locations(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_project_id,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_holder_storage_location_list(env, (*out));
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Locations_getStorageLocation(JNIEnv* env, jobject, jlong context_handle, jstring a_location_id) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_location_id = nullptr;
  if (!c_text(env, a_location_id, keep, p_location_id)) return nullptr;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_project_storage_location* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_get_storage_location(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_location_id,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_holder_project_storage_location(env, (*out));
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Locations_createStorageLocation(JNIEnv* env, jobject, jlong context_handle, jobject a_request) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  if (!a_request) { fail(env, "request is required"); return nullptr; }
  auto* p_request = keep.make<holder_new_storage_location>();
  if (!from_java_holder_new_storage_location(env, a_request, *p_request, keep)) return nullptr;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_project_storage_location* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_create_storage_location(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_request,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_holder_project_storage_location(env, (*out));
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Locations_updateStorageLocation(JNIEnv* env, jobject, jlong context_handle, jstring a_location_id, jobject a_request) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_location_id = nullptr;
  if (!c_text(env, a_location_id, keep, p_location_id)) return nullptr;
  if (!a_request) { fail(env, "request is required"); return nullptr; }
  auto* p_request = keep.make<holder_storage_location_update>();
  if (!from_java_holder_storage_location_update(env, a_request, *p_request, keep)) return nullptr;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_project_storage_location* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_update_storage_location(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_location_id,
      p_request,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_holder_project_storage_location(env, (*out));
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Locations_deleteStorageLocation(JNIEnv* env, jobject, jlong context_handle, jstring a_location_id) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_location_id = nullptr;
  if (!c_text(env, a_location_id, keep, p_location_id)) return nullptr;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_location_delete_result* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_delete_storage_location(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_location_id,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_holder_location_delete_result(env, (*out));
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Cards_getCard(JNIEnv* env, jobject, jlong context_handle, jstring a_project_id, jstring a_card_id) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_project_id = nullptr;
  if (!c_text(env, a_project_id, keep, p_project_id)) return nullptr;
  const char* p_card_id = nullptr;
  if (!c_text(env, a_card_id, keep, p_card_id)) return nullptr;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_card* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_get_card(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_project_id,
      p_card_id,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_holder_card(env, (*out));
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Cards_getCardMetadata(JNIEnv* env, jobject, jlong context_handle, jstring a_project_id, jstring a_card_id) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_project_id = nullptr;
  if (!c_text(env, a_project_id, keep, p_project_id)) return nullptr;
  const char* p_card_id = nullptr;
  if (!c_text(env, a_card_id, keep, p_card_id)) return nullptr;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_card_metadata* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_get_card_metadata(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_project_id,
      p_card_id,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_holder_card_metadata(env, (*out));
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Cards_getCardBreadcrumbs(JNIEnv* env, jobject, jlong context_handle, jstring a_project_id, jstring a_card_id) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_project_id = nullptr;
  if (!c_text(env, a_project_id, keep, p_project_id)) return nullptr;
  const char* p_card_id = nullptr;
  if (!c_text(env, a_card_id, keep, p_card_id)) return nullptr;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_card_breadcrumb_list* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_get_card_breadcrumbs(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_project_id,
      p_card_id,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_list(env, ((*out)).items, ((*out)).count, [&](const auto& item) -> jobject { return to_java_holder_card_breadcrumb(env, item); });
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Cards_resolveCardReference(JNIEnv* env, jobject, jlong context_handle, jobject a_request) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  if (!a_request) { fail(env, "request is required"); return nullptr; }
  auto* p_request = keep.make<holder_card_reference_resolve_request>();
  if (!from_java_holder_card_reference_resolve_request(env, a_request, *p_request, keep)) return nullptr;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_card_reference_resolve_data* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_resolve_card_reference(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_request,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_holder_card_reference_resolve_data(env, (*out));
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Cards_listCards(JNIEnv* env, jobject, jlong context_handle, jstring a_project_id, jobject a_parent, jstring a_tag, jobject a_order, jint a_limit, jstring a_cursor, jboolean a_include_content, jboolean a_include_child_count) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_project_id = nullptr;
  if (!c_text(env, a_project_id, keep, p_project_id)) return nullptr;
  holder_list_cards_parent p_parent{};
  if (a_parent) {
    static const jclass choice_1 = global_class(env, "team/holder/android/contract/ListCardsParent$Roots");
    static const jclass text_class = global_class(env, "team/holder/android/contract/ListCardsParent$CardId");
    static const jfieldID text_field = env->GetFieldID(text_class, "value", "Ljava/lang/String;");
    if (env->IsInstanceOf(a_parent, choice_1)) p_parent.kind = HOLDER_LIST_CARDS_PARENT_ROOTS;
    else {
      auto text = static_cast<jstring>(env->GetObjectField(a_parent, text_field));
      const bool converted = c_text(env, text, keep, p_parent.card_id);
      if (text) env->DeleteLocalRef(text);
      if (!converted) return nullptr;
      p_parent.kind = HOLDER_LIST_CARDS_PARENT_CARD_ID;
    }
  }
  const char* p_tag = nullptr;
  if (!c_text(env, a_tag, keep, p_tag)) return nullptr;
  const holder_list_cards_order p_order = a_order ? static_cast<holder_list_cards_order>(ordinal(env, a_order) + 1) : static_cast<holder_list_cards_order>(0);
  int32_t v_limit = static_cast<int32_t>(a_limit);
  const int32_t* p_limit = &v_limit;
  const char* p_cursor = nullptr;
  if (!c_text(env, a_cursor, keep, p_cursor)) return nullptr;
  const bool p_include_content = a_include_content == JNI_TRUE;
  const bool p_include_child_count = a_include_child_count == JNI_TRUE;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_card_list_item_page* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_list_cards(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_project_id,
      p_parent,
      p_tag,
      p_order,
      p_limit,
      p_cursor,
      p_include_content,
      p_include_child_count,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_holder_card_list_item_page(env, (*out));
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Cards_createCard(JNIEnv* env, jobject, jlong context_handle, jstring a_project_id, jobject a_card) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_project_id = nullptr;
  if (!c_text(env, a_project_id, keep, p_project_id)) return nullptr;
  if (!a_card) { fail(env, "card is required"); return nullptr; }
  auto* p_card = keep.make<holder_new_card>();
  if (!from_java_holder_new_card(env, a_card, *p_card, keep)) return nullptr;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_card* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_create_card(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_project_id,
      p_card,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_holder_card(env, (*out));
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Cards_updateCard(JNIEnv* env, jobject, jlong context_handle, jstring a_project_id, jstring a_card_id, jobject a_update) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_project_id = nullptr;
  if (!c_text(env, a_project_id, keep, p_project_id)) return nullptr;
  const char* p_card_id = nullptr;
  if (!c_text(env, a_card_id, keep, p_card_id)) return nullptr;
  if (!a_update) { fail(env, "update is required"); return nullptr; }
  auto* p_update = keep.make<holder_card_update>();
  if (!from_java_holder_card_update(env, a_update, *p_update, keep)) return nullptr;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_card* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_update_card(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_project_id,
      p_card_id,
      p_update,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_holder_card(env, (*out));
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Cards_trashCard(JNIEnv* env, jobject, jlong context_handle, jstring a_project_id, jstring a_card_id, jobject a_children) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_project_id = nullptr;
  if (!c_text(env, a_project_id, keep, p_project_id)) return nullptr;
  const char* p_card_id = nullptr;
  if (!c_text(env, a_card_id, keep, p_card_id)) return nullptr;
  const holder_trash_card_children p_children = a_children ? static_cast<holder_trash_card_children>(ordinal(env, a_children) + 1) : static_cast<holder_trash_card_children>(0);
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_card* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_trash_card(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_project_id,
      p_card_id,
      p_children,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_holder_card(env, (*out));
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Cards_restoreCard(JNIEnv* env, jobject, jlong context_handle, jstring a_project_id, jstring a_card_id) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_project_id = nullptr;
  if (!c_text(env, a_project_id, keep, p_project_id)) return nullptr;
  const char* p_card_id = nullptr;
  if (!c_text(env, a_card_id, keep, p_card_id)) return nullptr;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_card* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_restore_card(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_project_id,
      p_card_id,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_holder_card(env, (*out));
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Cards_moveCard(JNIEnv* env, jobject, jlong context_handle, jstring a_project_id, jstring a_card_id, jobject a_move) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_project_id = nullptr;
  if (!c_text(env, a_project_id, keep, p_project_id)) return nullptr;
  const char* p_card_id = nullptr;
  if (!c_text(env, a_card_id, keep, p_card_id)) return nullptr;
  if (!a_move) { fail(env, "move is required"); return nullptr; }
  auto* p_move = keep.make<holder_card_move>();
  if (!from_java_holder_card_move(env, a_move, *p_move, keep)) return nullptr;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_card* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_move_card(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_project_id,
      p_card_id,
      p_move,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_holder_card(env, (*out));
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Cards_addCardTag(JNIEnv* env, jobject, jlong context_handle, jstring a_project_id, jstring a_card_id, jstring a_tag) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_project_id = nullptr;
  if (!c_text(env, a_project_id, keep, p_project_id)) return nullptr;
  const char* p_card_id = nullptr;
  if (!c_text(env, a_card_id, keep, p_card_id)) return nullptr;
  const char* p_tag = nullptr;
  if (!c_text(env, a_tag, keep, p_tag)) return nullptr;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_card* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_add_card_tag(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_project_id,
      p_card_id,
      p_tag,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_holder_card(env, (*out));
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Cards_removeCardTag(JNIEnv* env, jobject, jlong context_handle, jstring a_project_id, jstring a_card_id, jstring a_tag) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_project_id = nullptr;
  if (!c_text(env, a_project_id, keep, p_project_id)) return nullptr;
  const char* p_card_id = nullptr;
  if (!c_text(env, a_card_id, keep, p_card_id)) return nullptr;
  const char* p_tag = nullptr;
  if (!c_text(env, a_tag, keep, p_tag)) return nullptr;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_card* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_remove_card_tag(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_project_id,
      p_card_id,
      p_tag,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_holder_card(env, (*out));
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Cards_listProjectTags(JNIEnv* env, jobject, jlong context_handle, jstring a_project_id) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_project_id = nullptr;
  if (!c_text(env, a_project_id, keep, p_project_id)) return nullptr;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_tag_count_list* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_list_project_tags(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_project_id,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_list(env, ((*out)).items, ((*out)).count, [&](const auto& item) -> jobject { return to_java_holder_tag_count(env, item); });
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Links_listLinkKinds(JNIEnv* env, jobject, jlong context_handle) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_link_kind_labels_list* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_list_link_kinds(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_list(env, ((*out)).items, ((*out)).count, [&](const auto& item) -> jobject { return to_java_holder_link_kind_labels(env, item); });
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Links_listCardLinks(JNIEnv* env, jobject, jlong context_handle, jstring a_project_id, jstring a_card_id, jboolean a_include_deleted) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_project_id = nullptr;
  if (!c_text(env, a_project_id, keep, p_project_id)) return nullptr;
  const char* p_card_id = nullptr;
  if (!c_text(env, a_card_id, keep, p_card_id)) return nullptr;
  const bool p_include_deleted = a_include_deleted == JNI_TRUE;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_link_list* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_list_card_links(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_project_id,
      p_card_id,
      p_include_deleted,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_list(env, ((*out)).items, ((*out)).count, [&](const auto& item) -> jobject { return to_java_holder_link(env, item); });
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Links_listCardBacklinks(JNIEnv* env, jobject, jlong context_handle, jstring a_project_id, jstring a_card_id, jobject a_scope, jboolean a_include_deleted) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_project_id = nullptr;
  if (!c_text(env, a_project_id, keep, p_project_id)) return nullptr;
  const char* p_card_id = nullptr;
  if (!c_text(env, a_card_id, keep, p_card_id)) return nullptr;
  const holder_list_card_backlinks_scope p_scope = a_scope ? static_cast<holder_list_card_backlinks_scope>(ordinal(env, a_scope) + 1) : static_cast<holder_list_card_backlinks_scope>(0);
  const bool p_include_deleted = a_include_deleted == JNI_TRUE;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_link_list* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_list_card_backlinks(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_project_id,
      p_card_id,
      p_scope,
      p_include_deleted,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_list(env, ((*out)).items, ((*out)).count, [&](const auto& item) -> jobject { return to_java_holder_link(env, item); });
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Links_addCardLink(JNIEnv* env, jobject, jlong context_handle, jstring a_project_id, jstring a_card_id, jobject a_link) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_project_id = nullptr;
  if (!c_text(env, a_project_id, keep, p_project_id)) return nullptr;
  const char* p_card_id = nullptr;
  if (!c_text(env, a_card_id, keep, p_card_id)) return nullptr;
  if (!a_link) { fail(env, "link is required"); return nullptr; }
  auto* p_link = keep.make<holder_new_link>();
  if (!from_java_holder_new_link(env, a_link, *p_link, keep)) return nullptr;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_link* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_add_card_link(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_project_id,
      p_card_id,
      p_link,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_holder_link(env, (*out));
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Links_removeCardLink(JNIEnv* env, jobject, jlong context_handle, jstring a_project_id, jstring a_card_id, jobject a_target_type, jstring a_target_id, jstring a_kind, jstring a_target_project_id) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_project_id = nullptr;
  if (!c_text(env, a_project_id, keep, p_project_id)) return nullptr;
  const char* p_card_id = nullptr;
  if (!c_text(env, a_card_id, keep, p_card_id)) return nullptr;
  const holder_remove_card_link_target_type p_target_type = a_target_type ? static_cast<holder_remove_card_link_target_type>(ordinal(env, a_target_type) + 1) : static_cast<holder_remove_card_link_target_type>(0);
  const char* p_target_id = nullptr;
  if (!c_text(env, a_target_id, keep, p_target_id)) return nullptr;
  const char* p_kind = nullptr;
  if (!c_text(env, a_kind, keep, p_kind)) return nullptr;
  const char* p_target_project_id = nullptr;
  if (!c_text(env, a_target_project_id, keep, p_target_project_id)) return nullptr;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_link_list* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_remove_card_link(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_project_id,
      p_card_id,
      p_target_type,
      p_target_id,
      p_kind,
      p_target_project_id,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_list(env, ((*out)).items, ((*out)).count, [&](const auto& item) -> jobject { return to_java_holder_link(env, item); });
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Milestones_listCardMilestones(JNIEnv* env, jobject, jlong context_handle, jstring a_project_id, jstring a_card_id) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_project_id = nullptr;
  if (!c_text(env, a_project_id, keep, p_project_id)) return nullptr;
  const char* p_card_id = nullptr;
  if (!c_text(env, a_card_id, keep, p_card_id)) return nullptr;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_milestone_list* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_list_card_milestones(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_project_id,
      p_card_id,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_list(env, ((*out)).items, ((*out)).count, [&](const auto& item) -> jobject { return to_java_holder_milestone(env, item); });
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Milestones_addCardMilestone(JNIEnv* env, jobject, jlong context_handle, jstring a_project_id, jstring a_card_id, jobject a_milestone) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_project_id = nullptr;
  if (!c_text(env, a_project_id, keep, p_project_id)) return nullptr;
  const char* p_card_id = nullptr;
  if (!c_text(env, a_card_id, keep, p_card_id)) return nullptr;
  if (!a_milestone) { fail(env, "milestone is required"); return nullptr; }
  auto* p_milestone = keep.make<holder_new_milestone>();
  if (!from_java_holder_new_milestone(env, a_milestone, *p_milestone, keep)) return nullptr;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_milestone* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_add_card_milestone(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_project_id,
      p_card_id,
      p_milestone,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_holder_milestone(env, (*out));
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Milestones_updateCardMilestone(JNIEnv* env, jobject, jlong context_handle, jstring a_project_id, jstring a_card_id, jstring a_milestone_id, jobject a_update) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_project_id = nullptr;
  if (!c_text(env, a_project_id, keep, p_project_id)) return nullptr;
  const char* p_card_id = nullptr;
  if (!c_text(env, a_card_id, keep, p_card_id)) return nullptr;
  const char* p_milestone_id = nullptr;
  if (!c_text(env, a_milestone_id, keep, p_milestone_id)) return nullptr;
  if (!a_update) { fail(env, "update is required"); return nullptr; }
  auto* p_update = keep.make<holder_milestone_update>();
  if (!from_java_holder_milestone_update(env, a_update, *p_update, keep)) return nullptr;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_milestone* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_update_card_milestone(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_project_id,
      p_card_id,
      p_milestone_id,
      p_update,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_holder_milestone(env, (*out));
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Milestones_removeCardMilestone(JNIEnv* env, jobject, jlong context_handle, jstring a_project_id, jstring a_card_id, jstring a_milestone_id) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_project_id = nullptr;
  if (!c_text(env, a_project_id, keep, p_project_id)) return nullptr;
  const char* p_card_id = nullptr;
  if (!c_text(env, a_card_id, keep, p_card_id)) return nullptr;
  const char* p_milestone_id = nullptr;
  if (!c_text(env, a_milestone_id, keep, p_milestone_id)) return nullptr;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_milestone_list* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_remove_card_milestone(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_project_id,
      p_card_id,
      p_milestone_id,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_list(env, ((*out)).items, ((*out)).count, [&](const auto& item) -> jobject { return to_java_holder_milestone(env, item); });
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Milestones_listMilestonesByDay(JNIEnv* env, jobject, jlong context_handle, jstring a_project_id, jstring a_first_day, jstring a_last_day, jstring a_role) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_project_id = nullptr;
  if (!c_text(env, a_project_id, keep, p_project_id)) return nullptr;
  const char* p_first_day = nullptr;
  if (!c_text(env, a_first_day, keep, p_first_day)) return nullptr;
  const char* p_last_day = nullptr;
  if (!c_text(env, a_last_day, keep, p_last_day)) return nullptr;
  const char* p_role = nullptr;
  if (!c_text(env, a_role, keep, p_role)) return nullptr;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_milestone_list* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_list_milestones_by_day(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_project_id,
      p_first_day,
      p_last_day,
      p_role,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_list(env, ((*out)).items, ((*out)).count, [&](const auto& item) -> jobject { return to_java_holder_milestone(env, item); });
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Milestones_listMilestonesByInstant(JNIEnv* env, jobject, jlong context_handle, jstring a_project_id, jlong a_first_instant, jlong a_last_instant, jstring a_role) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_project_id = nullptr;
  if (!c_text(env, a_project_id, keep, p_project_id)) return nullptr;
  const int64_t p_first_instant = static_cast<int64_t>(a_first_instant);
  const int64_t p_last_instant = static_cast<int64_t>(a_last_instant);
  const char* p_role = nullptr;
  if (!c_text(env, a_role, keep, p_role)) return nullptr;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_milestone_list* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_list_milestones_by_instant(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_project_id,
      p_first_instant,
      p_last_instant,
      p_role,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_list(env, ((*out)).items, ((*out)).count, [&](const auto& item) -> jobject { return to_java_holder_milestone(env, item); });
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Milestones_getCalendar(JNIEnv* env, jobject, jlong context_handle, jstring a_project_id, jlong a_first_instant, jlong a_last_instant, jstring a_first_day, jstring a_last_day) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_project_id = nullptr;
  if (!c_text(env, a_project_id, keep, p_project_id)) return nullptr;
  const int64_t p_first_instant = static_cast<int64_t>(a_first_instant);
  const int64_t p_last_instant = static_cast<int64_t>(a_last_instant);
  const char* p_first_day = nullptr;
  if (!c_text(env, a_first_day, keep, p_first_day)) return nullptr;
  const char* p_last_day = nullptr;
  if (!c_text(env, a_last_day, keep, p_last_day)) return nullptr;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_calendar* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_get_calendar(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_project_id,
      p_first_instant,
      p_last_instant,
      p_first_day,
      p_last_day,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_holder_calendar(env, (*out));
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Resources_importAssetFile(JNIEnv* env, jobject, jlong context_handle, jobject a_request) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  if (!a_request) { fail(env, "request is required"); return nullptr; }
  auto* p_request = keep.make<holder_asset_import_request>();
  if (!from_java_holder_asset_import_request(env, a_request, *p_request, keep)) return nullptr;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_asset_import* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_import_asset_file(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_request,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_holder_asset_import(env, (*out));
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Resources_retrieveAssetFile(JNIEnv* env, jobject, jlong context_handle, jstring a_resource_id, jstring a_asset_id, jstring a_placement_id, jstring a_destination_path) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_resource_id = nullptr;
  if (!c_text(env, a_resource_id, keep, p_resource_id)) return nullptr;
  const char* p_asset_id = nullptr;
  if (!c_text(env, a_asset_id, keep, p_asset_id)) return nullptr;
  const char* p_placement_id = nullptr;
  if (!c_text(env, a_placement_id, keep, p_placement_id)) return nullptr;
  const char* p_destination_path = nullptr;
  if (!c_text(env, a_destination_path, keep, p_destination_path)) return nullptr;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_asset_file* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_retrieve_asset_file(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_resource_id,
      p_asset_id,
      p_placement_id,
      p_destination_path,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_holder_asset_file(env, (*out));
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Resources_listResources(JNIEnv* env, jobject, jlong context_handle, jstring a_project_id) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_project_id = nullptr;
  if (!c_text(env, a_project_id, keep, p_project_id)) return nullptr;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_resource_list* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_list_resources(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_project_id,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_list(env, ((*out)).items, ((*out)).count, [&](const auto& item) -> jobject { return to_java_holder_resource(env, item); });
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Resources_getResource(JNIEnv* env, jobject, jlong context_handle, jstring a_project_id, jstring a_resource_id) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_project_id = nullptr;
  if (!c_text(env, a_project_id, keep, p_project_id)) return nullptr;
  const char* p_resource_id = nullptr;
  if (!c_text(env, a_resource_id, keep, p_resource_id)) return nullptr;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_resource* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_get_resource(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_project_id,
      p_resource_id,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_holder_resource(env, (*out));
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Resources_createResource(JNIEnv* env, jobject, jlong context_handle, jstring a_project_id, jobject a_resource) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_project_id = nullptr;
  if (!c_text(env, a_project_id, keep, p_project_id)) return nullptr;
  if (!a_resource) { fail(env, "resource is required"); return nullptr; }
  auto* p_resource = keep.make<holder_new_resource>();
  if (!from_java_holder_new_resource(env, a_resource, *p_resource, keep)) return nullptr;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_resource* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_create_resource(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_project_id,
      p_resource,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_holder_resource(env, (*out));
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Resources_updateResource(JNIEnv* env, jobject, jlong context_handle, jstring a_project_id, jstring a_resource_id, jobject a_update) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_project_id = nullptr;
  if (!c_text(env, a_project_id, keep, p_project_id)) return nullptr;
  const char* p_resource_id = nullptr;
  if (!c_text(env, a_resource_id, keep, p_resource_id)) return nullptr;
  if (!a_update) { fail(env, "update is required"); return nullptr; }
  auto* p_update = keep.make<holder_resource_update>();
  if (!from_java_holder_resource_update(env, a_update, *p_update, keep)) return nullptr;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_resource* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_update_resource(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_project_id,
      p_resource_id,
      p_update,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_holder_resource(env, (*out));
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Resources_deleteResource(JNIEnv* env, jobject, jlong context_handle, jstring a_project_id, jstring a_resource_id) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_project_id = nullptr;
  if (!c_text(env, a_project_id, keep, p_project_id)) return nullptr;
  const char* p_resource_id = nullptr;
  if (!c_text(env, a_resource_id, keep, p_resource_id)) return nullptr;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_resource* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_delete_resource(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_project_id,
      p_resource_id,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_holder_resource(env, (*out));
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Resources_listResourceBacklinks(JNIEnv* env, jobject, jlong context_handle, jstring a_project_id, jstring a_resource_id, jboolean a_include_deleted) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_project_id = nullptr;
  if (!c_text(env, a_project_id, keep, p_project_id)) return nullptr;
  const char* p_resource_id = nullptr;
  if (!c_text(env, a_resource_id, keep, p_resource_id)) return nullptr;
  const bool p_include_deleted = a_include_deleted == JNI_TRUE;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_link_list* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_list_resource_backlinks(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_project_id,
      p_resource_id,
      p_include_deleted,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_list(env, ((*out)).items, ((*out)).count, [&](const auto& item) -> jobject { return to_java_holder_link(env, item); });
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Resources_listCardResources(JNIEnv* env, jobject, jlong context_handle, jstring a_project_id, jstring a_card_id) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_project_id = nullptr;
  if (!c_text(env, a_project_id, keep, p_project_id)) return nullptr;
  const char* p_card_id = nullptr;
  if (!c_text(env, a_card_id, keep, p_card_id)) return nullptr;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_resource_list* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_list_card_resources(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_project_id,
      p_card_id,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_list(env, ((*out)).items, ((*out)).count, [&](const auto& item) -> jobject { return to_java_holder_resource(env, item); });
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Resources_attachCardResource(JNIEnv* env, jobject, jlong context_handle, jstring a_project_id, jstring a_card_id, jstring a_resource_id) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_project_id = nullptr;
  if (!c_text(env, a_project_id, keep, p_project_id)) return nullptr;
  const char* p_card_id = nullptr;
  if (!c_text(env, a_card_id, keep, p_card_id)) return nullptr;
  const char* p_resource_id = nullptr;
  if (!c_text(env, a_resource_id, keep, p_resource_id)) return nullptr;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_resource_list* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_attach_card_resource(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_project_id,
      p_card_id,
      p_resource_id,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_list(env, ((*out)).items, ((*out)).count, [&](const auto& item) -> jobject { return to_java_holder_resource(env, item); });
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Resources_detachCardResource(JNIEnv* env, jobject, jlong context_handle, jstring a_project_id, jstring a_card_id, jstring a_resource_id) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_project_id = nullptr;
  if (!c_text(env, a_project_id, keep, p_project_id)) return nullptr;
  const char* p_card_id = nullptr;
  if (!c_text(env, a_card_id, keep, p_card_id)) return nullptr;
  const char* p_resource_id = nullptr;
  if (!c_text(env, a_resource_id, keep, p_resource_id)) return nullptr;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_resource_list* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_detach_card_resource(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_project_id,
      p_card_id,
      p_resource_id,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_list(env, ((*out)).items, ((*out)).count, [&](const auto& item) -> jobject { return to_java_holder_resource(env, item); });
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Projects_listProjects(JNIEnv* env, jobject, jlong context_handle, jstring a_name, jobject a_updated_after, jobject a_updated_before, jobject a_order, jint a_limit, jint a_offset, jboolean a_include_card_counts) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_name = nullptr;
  if (!c_text(env, a_name, keep, p_name)) return nullptr;
  int64_t* p_updated_after = nullptr;
  if (a_updated_after) {
    p_updated_after = keep.make<int64_t>();
    *p_updated_after = unbox_int64(env, a_updated_after);
  }
  int64_t* p_updated_before = nullptr;
  if (a_updated_before) {
    p_updated_before = keep.make<int64_t>();
    *p_updated_before = unbox_int64(env, a_updated_before);
  }
  const holder_list_projects_order p_order = a_order ? static_cast<holder_list_projects_order>(ordinal(env, a_order) + 1) : static_cast<holder_list_projects_order>(0);
  int32_t v_limit = static_cast<int32_t>(a_limit);
  const int32_t* p_limit = &v_limit;
  const int32_t p_offset = static_cast<int32_t>(a_offset);
  const bool p_include_card_counts = a_include_card_counts == JNI_TRUE;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_project_list_item_list* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_list_projects(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_name,
      p_updated_after,
      p_updated_before,
      p_order,
      p_limit,
      p_offset,
      p_include_card_counts,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_list(env, ((*out)).items, ((*out)).count, [&](const auto& item) -> jobject { return to_java_holder_project_list_item(env, item); });
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Projects_getProject(JNIEnv* env, jobject, jlong context_handle, jstring a_project_id) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_project_id = nullptr;
  if (!c_text(env, a_project_id, keep, p_project_id)) return nullptr;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_project* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_get_project(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_project_id,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_holder_project(env, (*out));
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Projects_createProject(JNIEnv* env, jobject, jlong context_handle, jobject a_project) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  if (!a_project) { fail(env, "project is required"); return nullptr; }
  auto* p_project = keep.make<holder_new_project>();
  if (!from_java_holder_new_project(env, a_project, *p_project, keep)) return nullptr;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_project* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_create_project(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_project,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_holder_project(env, (*out));
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Projects_updateProject(JNIEnv* env, jobject, jlong context_handle, jstring a_project_id, jobject a_update) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_project_id = nullptr;
  if (!c_text(env, a_project_id, keep, p_project_id)) return nullptr;
  if (!a_update) { fail(env, "update is required"); return nullptr; }
  auto* p_update = keep.make<holder_project_update>();
  if (!from_java_holder_project_update(env, a_update, *p_update, keep)) return nullptr;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_project* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_update_project(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_project_id,
      p_update,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_holder_project(env, (*out));
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Projects_deleteProject(JNIEnv* env, jobject, jlong context_handle, jstring a_project_id) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_project_id = nullptr;
  if (!c_text(env, a_project_id, keep, p_project_id)) return nullptr;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_project* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_delete_project(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_project_id,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_holder_project(env, (*out));
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Maintenance_checkProjectEncryption(JNIEnv* env, jobject, jlong context_handle, jstring a_project_id) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_project_id = nullptr;
  if (!c_text(env, a_project_id, keep, p_project_id)) return nullptr;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_project_encryption_check* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_check_project_encryption(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_project_id,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_holder_project_encryption_check(env, (*out));
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Maintenance_repairProjectHierarchy(JNIEnv* env, jobject, jlong context_handle, jstring a_project_id, jobject a_request) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_project_id = nullptr;
  if (!c_text(env, a_project_id, keep, p_project_id)) return nullptr;
  if (!a_request) { fail(env, "request is required"); return nullptr; }
  auto* p_request = keep.make<holder_hierarchy_repair_request>();
  if (!from_java_holder_hierarchy_repair_request(env, a_request, *p_request, keep)) return nullptr;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_hierarchy_repair* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_repair_project_hierarchy(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_project_id,
      p_request,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_holder_hierarchy_repair(env, (*out));
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Maintenance_rebuildProject(JNIEnv* env, jobject, jlong context_handle, jobject a_request) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  if (!a_request) { fail(env, "request is required"); return nullptr; }
  auto* p_request = keep.make<holder_rebuild_request>();
  if (!from_java_holder_rebuild_request(env, a_request, *p_request, keep)) return nullptr;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_rebuild_result* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_rebuild_project(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_request,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_holder_rebuild_result(env, (*out));
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Maintenance_reindexSearch(JNIEnv* env, jobject, jlong context_handle) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_reindex_result* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_reindex_search(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_holder_reindex_result(env, (*out));
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Recovery_readRecoverySnapshot(JNIEnv* env, jobject, jlong context_handle, jstring a_project_id, jstring a_cursor, jint a_limit) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_project_id = nullptr;
  if (!c_text(env, a_project_id, keep, p_project_id)) return nullptr;
  const char* p_cursor = nullptr;
  if (!c_text(env, a_cursor, keep, p_cursor)) return nullptr;
  int32_t v_limit = static_cast<int32_t>(a_limit);
  const int32_t* p_limit = &v_limit;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_recovery_snapshot_page* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_read_recovery_snapshot(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_project_id,
      p_cursor,
      p_limit,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_holder_recovery_snapshot_page(env, (*out));
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Recovery_restoreRecoverySnapshot(JNIEnv* env, jobject, jlong context_handle, jobject a_request) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  if (!a_request) { fail(env, "request is required"); return nullptr; }
  auto* p_request = keep.make<holder_recovery_snapshot_restore>();
  if (!from_java_holder_recovery_snapshot_restore(env, a_request, *p_request, keep)) return nullptr;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_project* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_restore_recovery_snapshot(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_request,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_holder_project(env, (*out));
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Recovery_exportProjectRecoveryToken(JNIEnv* env, jobject, jlong context_handle, jstring a_project_id, jobject a_request) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_project_id = nullptr;
  if (!c_text(env, a_project_id, keep, p_project_id)) return nullptr;
  if (!a_request) { fail(env, "request is required"); return nullptr; }
  auto* p_request = keep.make<holder_recovery_token_export_request>();
  if (!from_java_holder_recovery_token_export_request(env, a_request, *p_request, keep)) return nullptr;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_project_recovery_token_export_result* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_export_project_recovery_token(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_project_id,
      p_request,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_holder_project_recovery_token_export_result(env, (*out));
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Recovery_importProjectRecoveryToken(JNIEnv* env, jobject, jlong context_handle, jstring a_project_id, jobject a_request) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_project_id = nullptr;
  if (!c_text(env, a_project_id, keep, p_project_id)) return nullptr;
  if (!a_request) { fail(env, "request is required"); return nullptr; }
  auto* p_request = keep.make<holder_recovery_token_import_request>();
  if (!from_java_holder_recovery_token_import_request(env, a_request, *p_request, keep)) return nullptr;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_project_recovery_token_import_result* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_import_project_recovery_token(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_project_id,
      p_request,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_holder_project_recovery_token_import_result(env, (*out));
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Recovery_importRecoveryTokenGlobal(JNIEnv* env, jobject, jlong context_handle, jobject a_request) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  if (!a_request) { fail(env, "request is required"); return nullptr; }
  auto* p_request = keep.make<holder_recovery_token_import_request>();
  if (!from_java_holder_recovery_token_import_request(env, a_request, *p_request, keep)) return nullptr;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_recovery_token_global_import_result* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_import_recovery_token_global(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_request,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_holder_recovery_token_global_import_result(env, (*out));
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Git_getProjectSyncState(JNIEnv* env, jobject, jlong context_handle, jstring a_project_id) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_project_id = nullptr;
  if (!c_text(env, a_project_id, keep, p_project_id)) return nullptr;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_sync_state* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_get_project_sync_state(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_project_id,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_holder_sync_state(env, (*out));
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Git_pullProject(JNIEnv* env, jobject, jlong context_handle, jstring a_project_id) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_project_id = nullptr;
  if (!c_text(env, a_project_id, keep, p_project_id)) return nullptr;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_sync_result* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_pull_project(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_project_id,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_holder_sync_result(env, (*out));
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Git_pushProject(JNIEnv* env, jobject, jlong context_handle, jstring a_project_id, jobject a_options) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_project_id = nullptr;
  if (!c_text(env, a_project_id, keep, p_project_id)) return nullptr;
  if (!a_options) { fail(env, "options is required"); return nullptr; }
  auto* p_options = keep.make<holder_push_options>();
  if (!from_java_holder_push_options(env, a_options, *p_options, keep)) return nullptr;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_sync_result* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_push_project(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_project_id,
      p_options,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_holder_sync_result(env, (*out));
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Git_syncProject(JNIEnv* env, jobject, jlong context_handle, jstring a_project_id, jobject a_options) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_project_id = nullptr;
  if (!c_text(env, a_project_id, keep, p_project_id)) return nullptr;
  if (!a_options) { fail(env, "options is required"); return nullptr; }
  auto* p_options = keep.make<holder_push_options>();
  if (!from_java_holder_push_options(env, a_options, *p_options, keep)) return nullptr;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_sync_result* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_sync_project(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_project_id,
      p_options,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_holder_sync_result(env, (*out));
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Git_syncProjectIfDue(JNIEnv* env, jobject, jlong context_handle, jstring a_project_id, jint a_push_interval_seconds, jint a_pull_interval_seconds) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_project_id = nullptr;
  if (!c_text(env, a_project_id, keep, p_project_id)) return nullptr;
  const int32_t p_push_interval_seconds = static_cast<int32_t>(a_push_interval_seconds);
  const int32_t p_pull_interval_seconds = static_cast<int32_t>(a_pull_interval_seconds);
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_due_sync_result* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_sync_project_if_due(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_project_id,
      p_push_interval_seconds,
      p_pull_interval_seconds,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_holder_due_sync_result(env, (*out));
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Git_testProjectRemote(JNIEnv* env, jobject, jlong context_handle, jstring a_project_id, jobject a_remote) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_project_id = nullptr;
  if (!c_text(env, a_project_id, keep, p_project_id)) return nullptr;
  if (!a_remote) { fail(env, "remote is required"); return nullptr; }
  auto* p_remote = keep.make<holder_remote_test>();
  if (!from_java_holder_remote_test(env, a_remote, *p_remote, keep)) return nullptr;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_remote_test_result* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_test_project_remote(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_project_id,
      p_remote,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_holder_remote_test_result(env, (*out));
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_History_listProjectHistory(JNIEnv* env, jobject, jlong context_handle, jstring a_project_id, jint a_limit, jstring a_cursor, jobject a_kind) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_project_id = nullptr;
  if (!c_text(env, a_project_id, keep, p_project_id)) return nullptr;
  int32_t v_limit = static_cast<int32_t>(a_limit);
  const int32_t* p_limit = &v_limit;
  const char* p_cursor = nullptr;
  if (!c_text(env, a_cursor, keep, p_cursor)) return nullptr;
  const holder_list_project_history_kind p_kind = a_kind ? static_cast<holder_list_project_history_kind>(ordinal(env, a_kind) + 1) : static_cast<holder_list_project_history_kind>(0);
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_project_history_page* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_list_project_history(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_project_id,
      p_limit,
      p_cursor,
      p_kind,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_holder_project_history_page(env, (*out));
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_History_listCardHistory(JNIEnv* env, jobject, jlong context_handle, jstring a_project_id, jstring a_card_id, jint a_limit, jstring a_cursor) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_project_id = nullptr;
  if (!c_text(env, a_project_id, keep, p_project_id)) return nullptr;
  const char* p_card_id = nullptr;
  if (!c_text(env, a_card_id, keep, p_card_id)) return nullptr;
  int32_t v_limit = static_cast<int32_t>(a_limit);
  const int32_t* p_limit = &v_limit;
  const char* p_cursor = nullptr;
  if (!c_text(env, a_cursor, keep, p_cursor)) return nullptr;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_card_history_page* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_list_card_history(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_project_id,
      p_card_id,
      p_limit,
      p_cursor,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_holder_card_history_page(env, (*out));
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_History_compareCardHistory(JNIEnv* env, jobject, jlong context_handle, jstring a_project_id, jstring a_card_id, jstring a_to_oid, jstring a_from_oid, jobject a_mode) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_project_id = nullptr;
  if (!c_text(env, a_project_id, keep, p_project_id)) return nullptr;
  const char* p_card_id = nullptr;
  if (!c_text(env, a_card_id, keep, p_card_id)) return nullptr;
  const char* p_to_oid = nullptr;
  if (!c_text(env, a_to_oid, keep, p_to_oid)) return nullptr;
  const char* p_from_oid = nullptr;
  if (!c_text(env, a_from_oid, keep, p_from_oid)) return nullptr;
  const holder_compare_card_history_mode p_mode = a_mode ? static_cast<holder_compare_card_history_mode>(ordinal(env, a_mode) + 1) : static_cast<holder_compare_card_history_mode>(0);
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_card_history_comparison* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_compare_card_history(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_project_id,
      p_card_id,
      p_to_oid,
      p_from_oid,
      p_mode,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_holder_card_history_comparison(env, (*out));
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_History_getCardHistorySnapshot(JNIEnv* env, jobject, jlong context_handle, jstring a_project_id, jstring a_card_id, jstring a_oid) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_project_id = nullptr;
  if (!c_text(env, a_project_id, keep, p_project_id)) return nullptr;
  const char* p_card_id = nullptr;
  if (!c_text(env, a_card_id, keep, p_card_id)) return nullptr;
  const char* p_oid = nullptr;
  if (!c_text(env, a_oid, keep, p_oid)) return nullptr;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_card_history_snapshot* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_get_card_history_snapshot(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_project_id,
      p_card_id,
      p_oid,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_holder_card_history_snapshot(env, (*out));
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_History_restoreCardHistorySnapshot(JNIEnv* env, jobject, jlong context_handle, jstring a_project_id, jstring a_card_id, jstring a_oid) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_project_id = nullptr;
  if (!c_text(env, a_project_id, keep, p_project_id)) return nullptr;
  const char* p_card_id = nullptr;
  if (!c_text(env, a_card_id, keep, p_card_id)) return nullptr;
  const char* p_oid = nullptr;
  if (!c_text(env, a_oid, keep, p_oid)) return nullptr;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_card_history_restore_result* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_restore_card_history_snapshot(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_project_id,
      p_card_id,
      p_oid,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_holder_card_history_restore_result(env, (*out));
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Trash_listTrash(JNIEnv* env, jobject, jlong context_handle, jstring a_project_id, jstring a_type) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_project_id = nullptr;
  if (!c_text(env, a_project_id, keep, p_project_id)) return nullptr;
  const char* p_type = nullptr;
  if (!c_text(env, a_type, keep, p_type)) return nullptr;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_trash_item_list* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_list_trash(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_project_id,
      p_type,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_list(env, ((*out)).items, ((*out)).count, [&](const auto& item) -> jobject { return to_java_holder_trash_item(env, item); });
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Trash_emptyTrash(JNIEnv* env, jobject, jlong context_handle, jstring a_project_id, jobject a_type) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_project_id = nullptr;
  if (!c_text(env, a_project_id, keep, p_project_id)) return nullptr;
  const holder_empty_trash_type p_type = a_type ? static_cast<holder_empty_trash_type>(ordinal(env, a_type) + 1) : static_cast<holder_empty_trash_type>(0);
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_trash_empty_result* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_empty_trash(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_project_id,
      p_type,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_holder_trash_empty_result(env, (*out));
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Trash_deleteTrashItem(JNIEnv* env, jobject, jlong context_handle, jobject a_type, jstring a_id) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const holder_delete_trash_item_type p_type = a_type ? static_cast<holder_delete_trash_item_type>(ordinal(env, a_type) + 1) : static_cast<holder_delete_trash_item_type>(0);
  const char* p_id = nullptr;
  if (!c_text(env, a_id, keep, p_id)) return nullptr;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_trash_delete_result* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_delete_trash_item(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_type,
      p_id,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_holder_trash_delete_result(env, (*out));
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Trash_deleteProjectTrashItem(JNIEnv* env, jobject, jlong context_handle, jstring a_project_id, jobject a_type, jstring a_id) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_project_id = nullptr;
  if (!c_text(env, a_project_id, keep, p_project_id)) return nullptr;
  const holder_delete_project_trash_item_type p_type = a_type ? static_cast<holder_delete_project_trash_item_type>(ordinal(env, a_type) + 1) : static_cast<holder_delete_project_trash_item_type>(0);
  const char* p_id = nullptr;
  if (!c_text(env, a_id, keep, p_id)) return nullptr;
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_trash_delete_result* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_delete_project_trash_item(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_project_id,
      p_type,
      p_id,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_holder_trash_delete_result(env, (*out));
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Search_searchCards(JNIEnv* env, jobject, jlong context_handle, jstring a_project_id, jstring a_q, jint a_limit, jint a_offset) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_project_id = nullptr;
  if (!c_text(env, a_project_id, keep, p_project_id)) return nullptr;
  const char* p_q = nullptr;
  if (!c_text(env, a_q, keep, p_q)) return nullptr;
  int32_t v_limit = static_cast<int32_t>(a_limit);
  const int32_t* p_limit = &v_limit;
  const int32_t p_offset = static_cast<int32_t>(a_offset);
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_search_card_list* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_search_cards(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_project_id,
      p_q,
      p_limit,
      p_offset,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_list(env, ((*out)).items, ((*out)).count, [&](const auto& item) -> jobject { return to_java_holder_search_card(env, item); });
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_contract_Search_searchAiMessages(JNIEnv* env, jobject, jlong context_handle, jstring a_project_id, jstring a_q, jint a_limit, jint a_offset) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  Keep keep;
  const char* p_project_id = nullptr;
  if (!c_text(env, a_project_id, keep, p_project_id)) return nullptr;
  const char* p_q = nullptr;
  if (!c_text(env, a_q, keep, p_q)) return nullptr;
  int32_t v_limit = static_cast<int32_t>(a_limit);
  const int32_t* p_limit = &v_limit;
  const int32_t p_offset = static_cast<int32_t>(a_offset);
  Arena arena;
  if (!arena.value) { throw_class(env, "team/holder/android/contract/AllocationFailureException", "cannot allocate a result arena"); return nullptr; }
  const holder_search_message_list* out = nullptr;
  holder_error* error = nullptr;
  const int status = holder_search_ai_messages(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      p_project_id,
      p_q,
      p_limit,
      p_offset,
      arena.value, &out, &error);
  if (status != HOLDER_OK) { throw_status(env, status, error); return nullptr; }
  return to_java_list(env, ((*out)).items, ((*out)).count, [&](const auto& item) -> jobject { return to_java_holder_search_message(env, item); });
}
