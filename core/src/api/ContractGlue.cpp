// Generated from contract/interface/cards.py by holder_contract.
// Do not edit; change the contract, then run make -C contract update.
//
// The C entry points: argument checks, conversions and error codes.
#include "Contract.h"
#include "holder/holder.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <new>
#include <string>
#include <utility>
#include <vector>

namespace contract = holder::contract;

struct holder_arena {
  std::vector<std::unique_ptr<std::byte[]>> blocks;
};

namespace {

template <class T>
T* make(holder_arena* arena, std::size_t count = 1) {
  if (count == 0) return nullptr;
  auto block = std::make_unique<std::byte[]>(sizeof(T) * count);
  T* items = reinterpret_cast<T*>(block.get());
  std::uninitialized_value_construct_n(items, count);
  arena->blocks.push_back(std::move(block));
  return items;
}

const char* copy(holder_arena* arena, const std::string& text) {
  char* out = make<char>(arena, text.size() + 1);
  std::memcpy(out, text.data(), text.size());
  return out;
}

template <class C, class T>
const C* copy_value(holder_arena* arena, const std::optional<T>& value) {
  if (!value) return nullptr;
  C* out = make<C>(arena);
  *out = static_cast<C>(*value);
  return out;
}

const char* copy_text(holder_arena* arena, const std::optional<std::string>& value) {
  return value ? copy(arena, *value) : nullptr;
}

holder_string_list copy_strings(holder_arena* arena, const std::vector<std::string>& values) {
  auto* items = make<const char*>(arena, values.size());
  for (std::size_t i = 0; i < values.size(); ++i) items[i] = copy(arena, values[i]);
  return {items, values.size()};
}

struct ArgumentError : std::invalid_argument {
  using std::invalid_argument::invalid_argument;
};

void require(bool ok, const std::string& message) {
  if (!ok) throw ArgumentError(message);
}

std::string text(const char* value, const char* name, std::size_t min_length, std::size_t max_length) {
  require(value != nullptr, std::string(name) + " must not be null");
  std::string out(value);
  require(out.size() >= min_length, std::string(name) + (min_length == 1 ? " must not be empty"
                                                                         : " must be at least " + std::to_string(min_length) + " bytes"));
  require(out.size() <= max_length, std::string(name) + " must be at most " + std::to_string(max_length) + " bytes");
  return out;
}

std::optional<std::string> optional_text(const char* value, const char* name, std::size_t min_length,
                                         std::size_t max_length) {
  if (value == nullptr) return std::nullopt;
  return text(value, name, min_length, max_length);
}

template <class T>
T in_range(T value, const char* name, T minimum, T maximum) {
  require(value >= minimum && value <= maximum, std::string(name) + " must be from " + std::to_string(minimum) +
                                                    " to " + std::to_string(maximum));
  return value;
}

template <class Body>
int guarded(holder_arena* arena, holder_error** out_error, Body&& body) {
  if (out_error != nullptr) *out_error = nullptr;
  if (arena == nullptr) return contract::detail::fail(out_error, HOLDER_ERROR_INVALID_ARGUMENT, "arena must not be null");
  try {
    body();
    return HOLDER_OK;
  } catch (const ArgumentError& e) {
    return contract::detail::fail(out_error, HOLDER_ERROR_INVALID_ARGUMENT, e.what());
  } catch (const contract::InvalidArgument& e) {
    return contract::detail::fail(out_error, HOLDER_ERROR_INVALID_ARGUMENT, e.what());
  } catch (const contract::NotFound& e) {
    return contract::detail::fail(out_error, HOLDER_ERROR_NOT_FOUND, e.what());
  } catch (const std::bad_alloc&) {
    return contract::detail::fail(out_error, HOLDER_ERROR_ALLOCATION, "allocation failed");
  } catch (const std::exception& e) {
    return contract::detail::fail(out_error, HOLDER_ERROR_RUNTIME, e.what());
  } catch (...) {
    return contract::detail::fail(out_error, HOLDER_ERROR_RUNTIME, "unknown failure");
  }
}

holder_tag_occurrence to_c(holder_arena* arena, const contract::TagOccurrence& value);
holder_tag_occurrence_list to_c_list(holder_arena* arena, const std::vector<contract::TagOccurrence>& values);
holder_card to_c(holder_arena* arena, const contract::Card& value);
holder_card_list_item to_c(holder_arena* arena, const contract::CardListItem& value);
holder_card_list_item_list to_c_list(holder_arena* arena, const std::vector<contract::CardListItem>& values);
holder_card_list_item_page to_c(holder_arena* arena, const contract::Page<contract::CardListItem>& value);

holder_tag_occurrence to_c(holder_arena* arena, const contract::TagOccurrence& value) {
  holder_tag_occurrence out{};
  out.tag = copy(arena, value.tag);
  out.byte_start = value.byte_start;
  out.byte_end = value.byte_end;
  return out;
}

holder_tag_occurrence_list to_c_list(holder_arena* arena, const std::vector<contract::TagOccurrence>& values) {
  auto* items = make<holder_tag_occurrence>(arena, values.size());
  for (std::size_t i = 0; i < values.size(); ++i) items[i] = to_c(arena, values[i]);
  return {items, values.size()};
}

holder_card to_c(holder_arena* arena, const contract::Card& value) {
  holder_card out{};
  out.card_id = copy(arena, value.card_id);
  out.project_id = copy(arena, value.project_id);
  out.title = copy(arena, value.title);
  out.rel_path = copy(arena, value.rel_path);
  out.parent_card_id = copy_text(arena, value.parent_card_id);
  out.sort_key = value.sort_key;
  out.created_at = value.created_at;
  out.updated_at = value.updated_at;
  out.deleted_at = copy_value<int64_t>(arena, value.deleted_at);
  out.content = copy(arena, value.content);
  out.tags = copy_strings(arena, value.tags);
  out.tag_occurrences = to_c_list(arena, value.tag_occurrences);
  return out;
}

holder_card_list_item to_c(holder_arena* arena, const contract::CardListItem& value) {
  holder_card_list_item out{};
  out.card_id = copy(arena, value.card_id);
  out.project_id = copy(arena, value.project_id);
  out.title = copy(arena, value.title);
  out.rel_path = copy(arena, value.rel_path);
  out.parent_card_id = copy_text(arena, value.parent_card_id);
  out.sort_key = value.sort_key;
  out.created_at = value.created_at;
  out.updated_at = value.updated_at;
  out.deleted_at = copy_value<int64_t>(arena, value.deleted_at);
  out.child_count = copy_value<int32_t>(arena, value.child_count);
  out.content = copy_text(arena, value.content);
  return out;
}

holder_card_list_item_list to_c_list(holder_arena* arena, const std::vector<contract::CardListItem>& values) {
  auto* items = make<holder_card_list_item>(arena, values.size());
  for (std::size_t i = 0; i < values.size(); ++i) items[i] = to_c(arena, values[i]);
  return {items, values.size()};
}

holder_card_list_item_page to_c(holder_arena* arena, const contract::Page<contract::CardListItem>& value) {
  holder_card_list_item_page out{};
  out.items = to_c_list(arena, value.items);
  out.next_cursor = copy_text(arena, value.next_cursor);
  return out;
}

contract::NewCard from_c(const holder_new_card& value) {
  contract::NewCard out;
  out.project_id = text(value.project_id, "new_card.project_id", 1, SIZE_MAX);
  out.title = text(value.title, "new_card.title", 1, SIZE_MAX);
  out.content = optional_text(value.content, "new_card.content", 0, SIZE_MAX).value_or("");
  out.parent_card_id = optional_text(value.parent_card_id, "new_card.parent_card_id", 1, SIZE_MAX);
  out.sort_key = (value.sort_key ? std::optional<double>(*value.sort_key) : std::nullopt);
  out.created_at = (value.created_at ? std::optional<long long>(*value.created_at) : std::nullopt);
  out.updated_at = (value.updated_at ? std::optional<long long>(*value.updated_at) : std::nullopt);
  return out;
}

}  // namespace

extern "C" {

holder_arena* holder_arena_new(void) { return new (std::nothrow) holder_arena; }
void holder_arena_free(holder_arena* arena) { delete arena; }

int holder_get_card(holder_context* context, const char* card_id, holder_arena* arena, const holder_card** out_card, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_card != nullptr, "out_card must not be null");
    *out_card = nullptr;
    const auto card_id_value = text(card_id, "card_id", 1, SIZE_MAX);
    auto* out = make<holder_card>(arena);
    *out = to_c(arena, contract::get_card(*context, card_id_value));
    *out_card = out;
  });
}

int holder_list_cards(holder_context* context, const char* project_id, holder_list_cards_parent parent, const char* tag, holder_list_cards_order order, const int32_t* limit, const char* cursor, bool include_content, bool include_child_count, holder_arena* arena, const holder_card_list_item_page** out_page, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_page != nullptr, "out_page must not be null");
    *out_page = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    std::optional<contract::ListCardsParent> parent_value;
    switch (parent.kind) {
    case HOLDER_LIST_CARDS_PARENT_NONE: break;
    case HOLDER_LIST_CARDS_PARENT_ROOTS: parent_value = contract::ListCardsParentRoots{}; break;
    case HOLDER_LIST_CARDS_PARENT_CARD_ID: parent_value = text(parent.card_id, "parent.card_id", 1, SIZE_MAX); break;
    default: throw ArgumentError("parent.kind is not a holder_list_cards_parent_kind value");
    }
    const auto tag_value = optional_text(tag, "tag", 1, SIZE_MAX);
    contract::ListCardsOrder order_value;
    switch (order) {
    case HOLDER_LIST_CARDS_ORDER_DEFAULT: order_value = contract::ListCardsOrder::CardIdAsc; break;
    case HOLDER_LIST_CARDS_ORDER_CARD_ID_ASC: order_value = contract::ListCardsOrder::CardIdAsc; break;
    case HOLDER_LIST_CARDS_ORDER_UPDATED_DESC: order_value = contract::ListCardsOrder::UpdatedDesc; break;
    case HOLDER_LIST_CARDS_ORDER_SORT_KEY_ASC: order_value = contract::ListCardsOrder::SortKeyAsc; break;
    default: throw ArgumentError("order is not a holder_list_cards_order value");
    }
    const auto limit_value = (limit ? in_range<std::int32_t>(*limit, "limit", 1, 5000) : std::int32_t{200});
    const auto cursor_value = optional_text(cursor, "cursor", 1, 8192);
    const auto include_content_value = include_content;
    const auto include_child_count_value = include_child_count;
    auto* out = make<holder_card_list_item_page>(arena);
    *out = to_c(arena, contract::list_cards(*context, project_id_value, parent_value, tag_value, order_value, limit_value, cursor_value, include_content_value, include_child_count_value));
    *out_page = out;
  });
}

int holder_create_card(holder_context* context, const holder_new_card* card, holder_arena* arena, const holder_card** out_card, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_card != nullptr, "out_card must not be null");
    *out_card = nullptr;
    require(card != nullptr, "card must not be null");
    const auto card_value = from_c(*card);
    auto* out = make<holder_card>(arena);
    *out = to_c(arena, contract::create_card(*context, card_value));
    *out_card = out;
  });
}

}  // extern "C"
