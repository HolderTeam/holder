// Generated from contract/interface by holder_contract. Do not edit.
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

}  // namespace


extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_generated_ContractOperations_nativeListCards(
    JNIEnv* env, jobject, jlong context_handle, jstring project_id, jstring parent, jstring tag, jstring order, jint limit, jstring cursor, jboolean include_content, jboolean include_child_count) {
  if (!context_handle) { fail(env, "context is required"); return nullptr; }
  if (!project_id) { fail(env, "project_id is required"); return nullptr; }
  std::string project_id_text;
  if (!utf8(env, project_id, project_id_text)) return nullptr;
  std::string parent_text;
  holder_list_cards_parent parent_value{};
  if (parent) {
    if (!utf8(env, parent, parent_text)) return nullptr;
    if (parent_text == "roots") parent_value.kind = HOLDER_LIST_CARDS_PARENT_ROOTS;
    else {
      parent_value.kind = HOLDER_LIST_CARDS_PARENT_CARD_ID;
      parent_value.card_id = parent_text.c_str();
    }
  }
  std::string tag_text;
  if (tag && !utf8(env, tag, tag_text)) return nullptr;
  if (!order) { fail(env, "order is required"); return nullptr; }
  std::string order_text;
  if (!utf8(env, order, order_text)) return nullptr;
  holder_list_cards_order order_value{};
  if (order_text == "card_id_asc") order_value = HOLDER_LIST_CARDS_ORDER_CARD_ID_ASC;
  else if (order_text == "updated_desc") order_value = HOLDER_LIST_CARDS_ORDER_UPDATED_DESC;
  else if (order_text == "sort_key_asc") order_value = HOLDER_LIST_CARDS_ORDER_SORT_KEY_ASC;
  else { fail(env, "invalid order"); return nullptr; }
  int32_t limit_value = static_cast<int32_t>(limit);
  std::string cursor_text;
  if (cursor && !utf8(env, cursor, cursor_text)) return nullptr;
  Arena arena;
  if (!arena.value) { fail(env, "cannot allocate result arena"); return nullptr; }
  const holder_card_list_item_page* page = nullptr;
  holder_error* error = nullptr;
  int status = holder_list_cards(
      reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      project_id_text.c_str(),
      parent_value,
      tag ? tag_text.c_str() : nullptr,
      order_value,
      &limit_value,
      cursor ? cursor_text.c_str() : nullptr,
      include_content == JNI_TRUE,
      include_child_count == JNI_TRUE,
      arena.value, &page, &error);
  if (status != HOLDER_OK) {
    std::string message = error ? holder_error_message(error) : "contract operation failed";
    if (error) holder_error_destroy(error);
    fail(env, message.c_str()); return nullptr;
  }
  jclass item_type = env->FindClass("team/holder/android/generated/ContractCardListItem");
  if (!item_type) return nullptr;
  jmethodID item_ctor = env->GetMethodID(item_type, "<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;DJJLjava/lang/Long;Ljava/lang/Integer;Ljava/lang/String;)V");
  if (!item_ctor) return nullptr;
  jclass list_type = env->FindClass("java/util/ArrayList");
  if (!list_type) return nullptr;
  jmethodID list_ctor = env->GetMethodID(list_type, "<init>", "(I)V");
  jmethodID add = env->GetMethodID(list_type, "add", "(Ljava/lang/Object;)Z");
  if (!list_ctor || !add) return nullptr;
  jobject items = env->NewObject(list_type, list_ctor, static_cast<jint>(page->items.count));
  if (env->ExceptionCheck()) return nullptr;
  for (size_t i = 0; i < page->items.count; ++i) {
    const auto& record = page->items.items[i];
    jstring v_card_id = string(env, record.card_id);
    jstring v_project_id = string(env, record.project_id);
    jstring v_title = string(env, record.title);
    jstring v_rel_path = string(env, record.rel_path);
    jstring v_parent_card_id = string(env, record.parent_card_id);
    jdouble v_sort_key = static_cast<jdouble>(record.sort_key);
    jlong v_created_at = static_cast<jlong>(record.created_at);
    jlong v_updated_at = static_cast<jlong>(record.updated_at);
    jobject v_deleted_at = record.deleted_at ? boxed_long(env, *record.deleted_at) : nullptr;
    jobject v_child_count = record.child_count ? boxed_int(env, *record.child_count) : nullptr;
    jstring v_content = string(env, record.content);
    if (env->ExceptionCheck()) return nullptr;
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
  jclass page_type = env->FindClass("team/holder/android/generated/ContractPage");
  if (!page_type) return nullptr;
  jmethodID page_ctor = env->GetMethodID(page_type, "<init>", "(Ljava/util/List;Ljava/lang/String;)V");
  if (!page_ctor) return nullptr;
  jstring next = string(env, page->next_cursor);
  jobject result = env->NewObject(page_type, page_ctor, items, next);
  if (next) env->DeleteLocalRef(next);
  env->DeleteLocalRef(items);
  env->DeleteLocalRef(page_type);
  env->DeleteLocalRef(list_type);
  env->DeleteLocalRef(item_type);
  return result;
}
