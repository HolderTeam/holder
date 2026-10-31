// Generated from contract/interface/cards.py and contract/interface/app.py by holder_contract.
// Do not edit; run make -C contract update.
#include "holder/holder.h"
#include <jni.h>
#include <cstdint>
#include <string>
#include <vector>

namespace {
struct Arena {
  holder_arena* value = holder_arena_new();
  ~Arena() { holder_arena_free(value); }
};

void fail(JNIEnv* env, const char* message) {
  jclass type = env->FindClass("java/lang/RuntimeException");
  if (type) env->ThrowNew(type, message);
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

jobject boxed_long(JNIEnv* env, int64_t value) {
  jclass type = env->FindClass("java/lang/Long");
  jmethodID ctor = env->GetMethodID(type, "<init>", "(J)V");
  jobject result = env->NewObject(type, ctor, static_cast<jlong>(value));
  env->DeleteLocalRef(type);
  return result;
}

jobject boxed_int(JNIEnv* env, int32_t value) {
  jclass type = env->FindClass("java/lang/Integer");
  jmethodID ctor = env->GetMethodID(type, "<init>", "(I)V");
  jobject result = env->NewObject(type, ctor, static_cast<jint>(value));
  env->DeleteLocalRef(type);
  return result;
}

}  // namespace

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_generated_ContractCards_nativeListCardsPage(
    JNIEnv* env, jobject, jlong context_handle, jstring project_id, jstring cursor) {
  if (!context_handle || !project_id) { fail(env, "context and project ID are required"); return nullptr; }
  const char* project = env->GetStringUTFChars(project_id, nullptr);
  const char* position = cursor ? env->GetStringUTFChars(cursor, nullptr) : nullptr;
  if (!project || (cursor && !position)) {
    if (project) env->ReleaseStringUTFChars(project_id, project);
    return nullptr;
  }
  Arena arena;
  const holder_card_list_item_page* page = nullptr;
  holder_error* error = nullptr;
  const int32_t limit = 5000;
  int status = arena.value ? holder_list_cards(
      reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)), project,
      {}, nullptr, HOLDER_LIST_CARDS_ORDER_SORT_KEY_ASC, &limit, position, false, false,
      arena.value, &page, &error) : HOLDER_ERROR_ALLOCATION;
  if (position) env->ReleaseStringUTFChars(cursor, position);
  env->ReleaseStringUTFChars(project_id, project);
  if (status != HOLDER_OK) {
    std::string message = error ? holder_error_message(error) : "cannot allocate card page";
    if (error) holder_error_destroy(error);
    fail(env, message.c_str()); return nullptr;
  }
  jclass item_type = env->FindClass("team/holder/android/generated/ContractCardListItem");
  jmethodID item_ctor = env->GetMethodID(item_type, "<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;DJJLjava/lang/Long;Ljava/lang/Integer;Ljava/lang/String;)V");
  jclass list_type = env->FindClass("java/util/ArrayList");
  jmethodID list_ctor = env->GetMethodID(list_type, "<init>", "(I)V");
  jmethodID add = env->GetMethodID(list_type, "add", "(Ljava/lang/Object;)Z");
  jobject items = env->NewObject(list_type, list_ctor, static_cast<jint>(page->items.count));
  if (env->ExceptionCheck()) return nullptr;
  for (size_t i = 0; i < page->items.count; ++i) {
    const auto& card = page->items.items[i];
    jstring v_card_id = string(env, card.card_id);
    jstring v_project_id = string(env, card.project_id);
    jstring v_title = string(env, card.title);
    jstring v_rel_path = string(env, card.rel_path);
    jstring v_parent_card_id = string(env, card.parent_card_id);
    jdouble v_sort_key = static_cast<jdouble>(card.sort_key);
    jlong v_created_at = static_cast<jlong>(card.created_at);
    jlong v_updated_at = static_cast<jlong>(card.updated_at);
    jobject v_deleted_at = card.deleted_at ? boxed_long(env, *card.deleted_at) : nullptr;
    jobject v_child_count = card.child_count ? boxed_int(env, *card.child_count) : nullptr;
    jstring v_content = string(env, card.content);
    jobject item = env->NewObject(item_type, item_ctor, v_card_id, v_project_id, v_title, v_rel_path, v_parent_card_id, v_sort_key, v_created_at, v_updated_at, v_deleted_at, v_child_count, v_content);
    if (v_card_id) env->DeleteLocalRef(v_card_id);
    if (v_project_id) env->DeleteLocalRef(v_project_id);
    if (v_title) env->DeleteLocalRef(v_title);
    if (v_rel_path) env->DeleteLocalRef(v_rel_path);
    if (v_parent_card_id) env->DeleteLocalRef(v_parent_card_id);
    if (v_deleted_at) env->DeleteLocalRef(v_deleted_at);
    if (v_child_count) env->DeleteLocalRef(v_child_count);
    if (v_content) env->DeleteLocalRef(v_content);
    if (env->ExceptionCheck()) return nullptr;
    env->CallBooleanMethod(items, add, item);
    env->DeleteLocalRef(item);
    if (env->ExceptionCheck()) return nullptr;
  }
  jclass page_type = env->FindClass("team/holder/android/generated/ContractCardPage");
  jmethodID page_ctor = env->GetMethodID(page_type, "<init>", "(Ljava/util/List;Ljava/lang/String;)V");
  jstring next = string(env, page->next_cursor);
  return env->NewObject(page_type, page_ctor, items, next);
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_generated_ContractCards_nativeGetCardMetadata(
    JNIEnv* env, jobject, jlong context_handle, jstring project_id, jstring card_id) {
  if (!context_handle || !project_id || !card_id) {
    fail(env, "context, project ID and card ID are required"); return nullptr;
  }
  const char* project = env->GetStringUTFChars(project_id, nullptr);
  const char* id = env->GetStringUTFChars(card_id, nullptr);
  if (!project || !id) {
    if (project) env->ReleaseStringUTFChars(project_id, project);
    if (id) env->ReleaseStringUTFChars(card_id, id);
    return nullptr;
  }
  Arena arena;
  const holder_card_metadata* value = nullptr;
  holder_error* error = nullptr;
  int status = arena.value ? holder_get_card_metadata(
      reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      project, id, arena.value, &value, &error) : HOLDER_ERROR_ALLOCATION;
  env->ReleaseStringUTFChars(card_id, id);
  env->ReleaseStringUTFChars(project_id, project);
  if (status != HOLDER_OK) {
    std::string message = error ? holder_error_message(error) : "cannot allocate card metadata";
    if (error) holder_error_destroy(error);
    fail(env, message.c_str()); return nullptr;
  }
  const auto& card = *value;
  jclass item_type = env->FindClass("team/holder/android/generated/ContractCardMetadata");
  jmethodID item_ctor = env->GetMethodID(item_type, "<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;DJJLjava/lang/Long;)V");
  jstring v_card_id = string(env, card.card_id);
  jstring v_project_id = string(env, card.project_id);
  jstring v_title = string(env, card.title);
  jstring v_rel_path = string(env, card.rel_path);
  jstring v_parent_card_id = string(env, card.parent_card_id);
  jdouble v_sort_key = static_cast<jdouble>(card.sort_key);
  jlong v_created_at = static_cast<jlong>(card.created_at);
  jlong v_updated_at = static_cast<jlong>(card.updated_at);
  jobject v_deleted_at = card.deleted_at ? boxed_long(env, *card.deleted_at) : nullptr;
  jobject result = env->NewObject(item_type, item_ctor, v_card_id, v_project_id, v_title, v_rel_path, v_parent_card_id, v_sort_key, v_created_at, v_updated_at, v_deleted_at);
  if (v_card_id) env->DeleteLocalRef(v_card_id);
  if (v_project_id) env->DeleteLocalRef(v_project_id);
  if (v_title) env->DeleteLocalRef(v_title);
  if (v_rel_path) env->DeleteLocalRef(v_rel_path);
  if (v_parent_card_id) env->DeleteLocalRef(v_parent_card_id);
  if (v_deleted_at) env->DeleteLocalRef(v_deleted_at);
  return result;
}
