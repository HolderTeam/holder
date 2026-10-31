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

extern "C" JNIEXPORT jstring JNICALL
Java_team_holder_android_generated_ContractCards_nativeGetCardContent(
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
  const holder_card* value = nullptr;
  holder_error* error = nullptr;
  int status = arena.value ? holder_get_card(
      reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      project, id, arena.value, &value, &error) : HOLDER_ERROR_ALLOCATION;
  env->ReleaseStringUTFChars(card_id, id);
  env->ReleaseStringUTFChars(project_id, project);
  if (status != HOLDER_OK) {
    std::string message = error ? holder_error_message(error) : "cannot read card content";
    if (error) holder_error_destroy(error);
    fail(env, message.c_str()); return nullptr;
  }
  return string(env, value->content);
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_generated_ContractCards_nativeUpdateCard(
    JNIEnv* env, jobject, jlong context_handle, jstring project_id, jstring card_id, jstring title, jstring content) {
  if (!context_handle || !project_id || !card_id || !title || !content) {
    fail(env, "context, project ID, card ID and required fields are needed"); return nullptr;
  }
  std::string project, id, card_title, body;
  if (!utf8(env, project_id, project) || !utf8(env, card_id, id) ||
      !utf8(env, title, card_title) || !utf8(env, content, body)) return nullptr;
  holder_card_update request{};
  request.title = card_title.empty() ? nullptr : card_title.c_str();
  request.content = body.c_str();
  Arena arena;
  const holder_card* value = nullptr;
  holder_error* error = nullptr;
  int status = arena.value ? holder_update_card(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)), project.c_str(), id.c_str(), &request, arena.value, &value, &error) : HOLDER_ERROR_ALLOCATION;
  if (status != HOLDER_OK) {
    std::string message = error ? holder_error_message(error) : "cannot allocate card for nativeUpdateCard";
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

extern "C" JNIEXPORT void JNICALL
Java_team_holder_android_generated_ContractCards_nativeTrashCard(
    JNIEnv* env, jobject, jlong context_handle, jstring project_id, jstring card_id) {
  if (!context_handle || !project_id || !card_id) {
    fail(env, "context, project ID, card ID and required fields are needed"); return;
  }
  std::string project, id;
  if (!utf8(env, project_id, project) || !utf8(env, card_id, id)) return;
  Arena arena;
  const holder_card* value = nullptr;
  holder_error* error = nullptr;
  int status = arena.value ? holder_trash_card(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)), project.c_str(), id.c_str(), HOLDER_TRASH_CARD_CHILDREN_PROMOTE, arena.value, &value, &error) : HOLDER_ERROR_ALLOCATION;
  if (status != HOLDER_OK) {
    std::string message = error ? holder_error_message(error) : "cannot allocate card for nativeTrashCard";
    if (error) holder_error_destroy(error);
    fail(env, message.c_str()); return;
  }
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_generated_ContractCards_nativeRestoreCard(
    JNIEnv* env, jobject, jlong context_handle, jstring project_id, jstring card_id) {
  if (!context_handle || !project_id || !card_id) {
    fail(env, "context, project ID, card ID and required fields are needed"); return nullptr;
  }
  std::string project, id;
  if (!utf8(env, project_id, project) || !utf8(env, card_id, id)) return nullptr;
  Arena arena;
  const holder_card* value = nullptr;
  holder_error* error = nullptr;
  int status = arena.value ? holder_restore_card(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)), project.c_str(), id.c_str(), arena.value, &value, &error) : HOLDER_ERROR_ALLOCATION;
  if (status != HOLDER_OK) {
    std::string message = error ? holder_error_message(error) : "cannot allocate card for nativeRestoreCard";
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

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_generated_ContractCards_nativeListProjectTags(
    JNIEnv* env, jobject, jlong context_handle, jstring project_id) {
  if (!context_handle || !project_id) {
    fail(env, "context and required list parameters are needed"); return nullptr;
  }
  std::string project;
  if (!utf8(env, project_id, project)) return nullptr;
  Arena arena;
  const holder_tag_count_list* page = nullptr;
  holder_error* error = nullptr;
  int status = arena.value ? holder_list_project_tags(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)), project.c_str(), arena.value, &page, &error) : HOLDER_ERROR_ALLOCATION;
  if (status != HOLDER_OK) {
    std::string message = error ? holder_error_message(error) : "cannot run list_project_tags";
    if (error) holder_error_destroy(error);
    fail(env, message.c_str()); return nullptr;
  }
  jclass item_type = env->FindClass("team/holder/android/generated/ContractTagCount");
  jmethodID item_ctor = env->GetMethodID(item_type, "<init>", "(Ljava/lang/String;I)V");
  jclass list_type = env->FindClass("java/util/ArrayList");
  jmethodID list_ctor = env->GetMethodID(list_type, "<init>", "(I)V");
  jmethodID add = env->GetMethodID(list_type, "add", "(Ljava/lang/Object;)Z");
  jobject items = env->NewObject(list_type, list_ctor, static_cast<jint>(page->count));
  if (env->ExceptionCheck()) return nullptr;
  for (size_t i = 0; i < page->count; ++i) {
    const auto& value = page->items[i];
    jstring v_tag = string(env, value.tag);
    jint v_card_count = static_cast<jint>(value.card_count);
    jobject item = env->NewObject(item_type, item_ctor, v_tag, v_card_count);
    if (v_tag) env->DeleteLocalRef(v_tag);
    if (env->ExceptionCheck()) return nullptr;
    env->CallBooleanMethod(items, add, item);
    env->DeleteLocalRef(item);
    if (env->ExceptionCheck()) return nullptr;
  }
  return items;
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_generated_ContractCards_nativeSearchCards(
    JNIEnv* env, jobject, jlong context_handle, jstring project_id, jstring q, jint limit) {
  if (!context_handle || !project_id || !q) {
    fail(env, "context and required list parameters are needed"); return nullptr;
  }
  std::string project, query;
  if (!utf8(env, project_id, project) || !utf8(env, q, query)) return nullptr;
  const int32_t count = static_cast<int32_t>(limit);
  Arena arena;
  const holder_search_card_list* page = nullptr;
  holder_error* error = nullptr;
  int status = arena.value ? holder_search_cards(reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)), project.c_str(), query.c_str(), &count, 0, arena.value, &page, &error) : HOLDER_ERROR_ALLOCATION;
  if (status != HOLDER_OK) {
    std::string message = error ? holder_error_message(error) : "cannot run search_cards";
    if (error) holder_error_destroy(error);
    fail(env, message.c_str()); return nullptr;
  }
  jclass item_type = env->FindClass("team/holder/android/generated/ContractSearchCard");
  jmethodID item_ctor = env->GetMethodID(item_type, "<init>", "(Ljava/lang/String;Ljava/lang/String;JJLjava/lang/String;D)V");
  jclass list_type = env->FindClass("java/util/ArrayList");
  jmethodID list_ctor = env->GetMethodID(list_type, "<init>", "(I)V");
  jmethodID add = env->GetMethodID(list_type, "add", "(Ljava/lang/Object;)Z");
  jobject items = env->NewObject(list_type, list_ctor, static_cast<jint>(page->count));
  if (env->ExceptionCheck()) return nullptr;
  for (size_t i = 0; i < page->count; ++i) {
    const auto& value = page->items[i];
    jstring v_card_id = string(env, value.card_id);
    jstring v_title = string(env, value.title);
    jlong v_updated_at = static_cast<jlong>(value.updated_at);
    jlong v_created_at = static_cast<jlong>(value.created_at);
    jstring v_snippet = string(env, value.snippet);
    jdouble v_rank = static_cast<jdouble>(value.rank);
    jobject item = env->NewObject(item_type, item_ctor, v_card_id, v_title, v_updated_at, v_created_at, v_snippet, v_rank);
    if (v_card_id) env->DeleteLocalRef(v_card_id);
    if (v_title) env->DeleteLocalRef(v_title);
    if (v_snippet) env->DeleteLocalRef(v_snippet);
    if (env->ExceptionCheck()) return nullptr;
    env->CallBooleanMethod(items, add, item);
    env->DeleteLocalRef(item);
    if (env->ExceptionCheck()) return nullptr;
  }
  return items;
}

extern "C" JNIEXPORT jobject JNICALL
Java_team_holder_android_generated_ContractCards_nativeCreateCard(
    JNIEnv* env, jobject, jlong context_handle, jstring project_id, jstring title,
    jstring content, jstring parent_card_id) {
  if (!context_handle || !project_id || !title || !content) {
    fail(env, "context, project ID, title and content are required"); return nullptr;
  }
  std::string project, card_title, body, parent;
  if (!utf8(env, project_id, project) || !utf8(env, title, card_title) ||
      !utf8(env, content, body) ||
      (parent_card_id && !utf8(env, parent_card_id, parent))) return nullptr;
  holder_new_card request{};
  request.title = card_title.c_str();
  request.content = body.c_str();
  request.parent_card_id = parent_card_id ? parent.c_str() : nullptr;
  Arena arena;
  const holder_card* value = nullptr;
  holder_error* error = nullptr;
  int status = arena.value ? holder_create_card(
      reinterpret_cast<holder_context*>(static_cast<intptr_t>(context_handle)),
      project.c_str(), &request, arena.value, &value, &error) : HOLDER_ERROR_ALLOCATION;
  if (status != HOLDER_OK) {
    std::string message = error ? holder_error_message(error) : "cannot allocate card";
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
