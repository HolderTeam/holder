// Generated from contract/interface/app.py, contract/interface/cards.py, contract/interface/errors.py, contract/interface/links.py by holder_contract.
// Do not edit; change the contract, then run make -C contract update.
//
// The C entry points: argument checks, conversions and error codes.
#include "Contract.h"
#include "holder/holder.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <new>
#include <optional>
#include <stdexcept>
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

// The number of characters in text, or nothing when it is not well-formed UTF-8: no overlong
// forms, surrogates or code points above U+10FFFF.
std::optional<std::size_t> utf8_length(const std::string& text) {
  std::size_t count = 0;
  for (std::size_t i = 0; i < text.size(); ++count) {
    const auto lead = static_cast<unsigned char>(text[i]);
    std::size_t size = 1;
    char32_t point = lead;
    if (lead >= 0xC2 && lead <= 0xDF) {
      size = 2, point = lead & 0x1F;
    } else if (lead >= 0xE0 && lead <= 0xEF) {
      size = 3, point = lead & 0x0F;
    } else if (lead >= 0xF0 && lead <= 0xF4) {
      size = 4, point = lead & 0x07;
    } else if (lead >= 0x80) {
      return std::nullopt;
    }
    if (text.size() - i < size) return std::nullopt;
    for (std::size_t k = 1; k < size; ++k) {
      const auto next = static_cast<unsigned char>(text[i + k]);
      if ((next & 0xC0) != 0x80) return std::nullopt;
      point = (point << 6) | (next & 0x3F);
    }
    if ((size == 3 && (point < 0x800 || (point >= 0xD800 && point <= 0xDFFF))) ||
        (size == 4 && (point < 0x10000 || point > 0x10FFFF)))
      return std::nullopt;
    i += size;
  }
  return count;
}

// Lengths count characters, as the contract does.
std::string text(const char* value, const char* name, std::size_t min_length, std::size_t max_length) {
  require(value != nullptr, std::string(name) + " must not be null");
  std::string out(value);
  const auto length = utf8_length(out);
  require(length.has_value(), std::string(name) + " must be UTF-8");
  require(*length >= min_length, std::string(name) + (min_length == 1 ? " must not be empty"
                                                                       : " must be at least " + std::to_string(min_length) + " characters"));
  require(*length <= max_length, std::string(name) + " must be at most " + std::to_string(max_length) + " characters");
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

double finite(double value, const char* name) {
  require(std::isfinite(value), std::string(name) + " must be a finite number");
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
  } catch (const contract::Conflict& e) {
    return contract::detail::fail(out_error, HOLDER_ERROR_CONFLICT, e.what());
  } catch (const contract::KeyringUnavailable& e) {
    return contract::detail::fail(out_error, HOLDER_ERROR_KEYRING_UNAVAILABLE, e.what());
  } catch (const contract::KeyMissing& e) {
    return contract::detail::fail(out_error, HOLDER_ERROR_KEY_MISSING, e.what());
  } catch (const contract::DataCorrupted& e) {
    return contract::detail::fail(out_error, HOLDER_ERROR_DATA_CORRUPTED, e.what());
  } catch (const contract::InvalidMove& e) {
    return contract::detail::fail(out_error, HOLDER_ERROR_INVALID_MOVE, e.what());
  } catch (const std::bad_alloc&) {
    return contract::detail::fail(out_error, HOLDER_ERROR_ALLOCATION, "allocation failed");
  } catch (const std::exception& e) {
    return contract::detail::fail(out_error, HOLDER_ERROR_RUNTIME, e.what());
  } catch (...) {
    return contract::detail::fail(out_error, HOLDER_ERROR_RUNTIME, "unknown failure");
  }
}

// A partial update's text field: kept, set to its value, or cleared.
contract::Change<std::string> changed_text(const holder_string_change& value, const char* name,
                                           std::size_t min_length, std::size_t max_length) {
  switch (value.change) {
  case HOLDER_KEEP: return std::nullopt;
  case HOLDER_SET: return text(value.value, (std::string(name) + ".value").c_str(), min_length, max_length);
  case HOLDER_CLEAR: return std::optional<std::string>{};
  default: throw ArgumentError(std::string(name) + ".change is not a holder_change value");
  }
}

holder_link_source_type to_c_enum(contract::LinkSourceType value) {
  switch (value) {
  case contract::LinkSourceType::Card: return HOLDER_LINK_SOURCE_TYPE_CARD;
  case contract::LinkSourceType::AiMessage: return HOLDER_LINK_SOURCE_TYPE_AI_MESSAGE;
  }
  throw std::logic_error("not a contract::LinkSourceType value");
}

holder_link_target_type to_c_enum(contract::LinkTargetType value) {
  switch (value) {
  case contract::LinkTargetType::Card: return HOLDER_LINK_TARGET_TYPE_CARD;
  case contract::LinkTargetType::AiMessage: return HOLDER_LINK_TARGET_TYPE_AI_MESSAGE;
  case contract::LinkTargetType::AiThread: return HOLDER_LINK_TARGET_TYPE_AI_THREAD;
  case contract::LinkTargetType::Resource: return HOLDER_LINK_TARGET_TYPE_RESOURCE;
  }
  throw std::logic_error("not a contract::LinkTargetType value");
}

holder_tag_occurrence to_c(holder_arena* arena, const contract::TagOccurrence& value);
holder_tag_occurrence_list to_c_list(holder_arena* arena, const std::vector<contract::TagOccurrence>& values);
holder_card to_c(holder_arena* arena, const contract::Card& value);
holder_card_list_item to_c(holder_arena* arena, const contract::CardListItem& value);
holder_card_list_item_list to_c_list(holder_arena* arena, const std::vector<contract::CardListItem>& values);
holder_card_list_item_page to_c(holder_arena* arena, const contract::Page<contract::CardListItem>& value);
holder_tag_count to_c(holder_arena* arena, const contract::TagCount& value);
holder_tag_count_list to_c_list(holder_arena* arena, const std::vector<contract::TagCount>& values);
holder_link to_c(holder_arena* arena, const contract::Link& value);
holder_link_list to_c_list(holder_arena* arena, const std::vector<contract::Link>& values);

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

holder_tag_count to_c(holder_arena* arena, const contract::TagCount& value) {
  holder_tag_count out{};
  out.tag = copy(arena, value.tag);
  out.card_count = value.card_count;
  return out;
}

holder_tag_count_list to_c_list(holder_arena* arena, const std::vector<contract::TagCount>& values) {
  auto* items = make<holder_tag_count>(arena, values.size());
  for (std::size_t i = 0; i < values.size(); ++i) items[i] = to_c(arena, values[i]);
  return {items, values.size()};
}

holder_link to_c(holder_arena* arena, const contract::Link& value) {
  holder_link out{};
  out.source_project_id = copy(arena, value.source_project_id);
  out.source_type = to_c_enum(value.source_type);
  out.source_id = copy(arena, value.source_id);
  out.target_project_id = copy(arena, value.target_project_id);
  out.target_type = to_c_enum(value.target_type);
  out.target_id = copy(arena, value.target_id);
  out.kind = copy(arena, value.kind);
  out.label = copy_text(arena, value.label);
  out.created_at = value.created_at;
  return out;
}

holder_link_list to_c_list(holder_arena* arena, const std::vector<contract::Link>& values) {
  auto* items = make<holder_link>(arena, values.size());
  for (std::size_t i = 0; i < values.size(); ++i) items[i] = to_c(arena, values[i]);
  return {items, values.size()};
}

contract::NewCard from_c(const holder_new_card& value) {
  contract::NewCard out;
  out.title = text(value.title, "new_card.title", 1, SIZE_MAX);
  out.content = optional_text(value.content, "new_card.content", 0, SIZE_MAX).value_or("");
  out.parent_card_id = optional_text(value.parent_card_id, "new_card.parent_card_id", 1, SIZE_MAX);
  out.sort_key = (value.sort_key ? std::optional<double>(finite(*value.sort_key, "new_card.sort_key")) : std::nullopt);
  out.created_at = (value.created_at ? std::optional<long long>(*value.created_at) : std::nullopt);
  out.updated_at = (value.updated_at ? std::optional<long long>(*value.updated_at) : std::nullopt);
  return out;
}

contract::CardUpdate from_c(const holder_card_update& value) {
  contract::CardUpdate out;
  out.title = optional_text(value.title, "card_update.title", 1, SIZE_MAX);
  out.content = optional_text(value.content, "card_update.content", 0, SIZE_MAX);
  out.parent_card_id = changed_text(value.parent_card_id, "card_update.parent_card_id", 1, SIZE_MAX);
  out.sort_key = (value.sort_key ? std::optional<double>(finite(*value.sort_key, "card_update.sort_key")) : std::nullopt);
  out.updated_at = (value.updated_at ? std::optional<long long>(*value.updated_at) : std::nullopt);
  return out;
}

contract::CardMove from_c(const holder_card_move& value) {
  contract::CardMove out;
  switch (value.intent) {
  case HOLDER_CARD_MOVE_INTENT_INTO: out.intent = contract::CardMoveIntent::Into; break;
  case HOLDER_CARD_MOVE_INTENT_BEFORE: out.intent = contract::CardMoveIntent::Before; break;
  case HOLDER_CARD_MOVE_INTENT_AFTER: out.intent = contract::CardMoveIntent::After; break;
  case HOLDER_CARD_MOVE_INTENT_LEFT: out.intent = contract::CardMoveIntent::Left; break;
  case HOLDER_CARD_MOVE_INTENT_RIGHT: out.intent = contract::CardMoveIntent::Right; break;
  case HOLDER_CARD_MOVE_INTENT_TO_START: out.intent = contract::CardMoveIntent::ToStart; break;
  case HOLDER_CARD_MOVE_INTENT_TO_END: out.intent = contract::CardMoveIntent::ToEnd; break;
  case HOLDER_CARD_MOVE_INTENT_UP_LEVEL: out.intent = contract::CardMoveIntent::UpLevel; break;
  default: throw ArgumentError("card_move.intent is not a holder_card_move_intent value");
  }
  out.target_card_id = optional_text(value.target_card_id, "card_move.target_card_id", 1, SIZE_MAX);
  out.parent_card_id = changed_text(value.parent_card_id, "card_move.parent_card_id", 1, SIZE_MAX);
  return out;
}

contract::NewLink from_c(const holder_new_link& value) {
  contract::NewLink out;
  switch (value.target_type) {
  case HOLDER_NEW_LINK_TARGET_TYPE_CARD: out.target_type = contract::NewLinkTargetType::Card; break;
  case HOLDER_NEW_LINK_TARGET_TYPE_AI_MESSAGE: out.target_type = contract::NewLinkTargetType::AiMessage; break;
  case HOLDER_NEW_LINK_TARGET_TYPE_AI_THREAD: out.target_type = contract::NewLinkTargetType::AiThread; break;
  case HOLDER_NEW_LINK_TARGET_TYPE_RESOURCE: out.target_type = contract::NewLinkTargetType::Resource; break;
  default: throw ArgumentError("new_link.target_type is not a holder_new_link_target_type value");
  }
  out.target_id = text(value.target_id, "new_link.target_id", 1, SIZE_MAX);
  out.target_project_id = optional_text(value.target_project_id, "new_link.target_project_id", 1, SIZE_MAX);
  out.kind = optional_text(value.kind, "new_link.kind", 1, SIZE_MAX).value_or("ref");
  out.label = optional_text(value.label, "new_link.label", 0, SIZE_MAX);
  out.created_at = (value.created_at ? std::optional<long long>(*value.created_at) : std::nullopt);
  return out;
}

}  // namespace

extern "C" {

holder_arena* holder_arena_new(void) { return new (std::nothrow) holder_arena; }
void holder_arena_free(holder_arena* arena) { delete arena; }

int holder_get_card(holder_context* context, const char* project_id, const char* card_id, holder_arena* arena, const holder_card** out_card, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_card != nullptr, "out_card must not be null");
    *out_card = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    const auto card_id_value = text(card_id, "card_id", 1, SIZE_MAX);
    auto* out = make<holder_card>(arena);
    *out = to_c(arena, contract::get_card(*context, project_id_value, card_id_value));
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

int holder_create_card(holder_context* context, const char* project_id, const holder_new_card* card, holder_arena* arena, const holder_card** out_card, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_card != nullptr, "out_card must not be null");
    *out_card = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    require(card != nullptr, "card must not be null");
    const auto card_value = from_c(*card);
    auto* out = make<holder_card>(arena);
    *out = to_c(arena, contract::create_card(*context, project_id_value, card_value));
    *out_card = out;
  });
}

int holder_update_card(holder_context* context, const char* project_id, const char* card_id, const holder_card_update* update, holder_arena* arena, const holder_card** out_card, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_card != nullptr, "out_card must not be null");
    *out_card = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    const auto card_id_value = text(card_id, "card_id", 1, SIZE_MAX);
    require(update != nullptr, "update must not be null");
    const auto update_value = from_c(*update);
    auto* out = make<holder_card>(arena);
    *out = to_c(arena, contract::update_card(*context, project_id_value, card_id_value, update_value));
    *out_card = out;
  });
}

int holder_trash_card(holder_context* context, const char* project_id, const char* card_id, holder_trash_card_children children, holder_arena* arena, const holder_card** out_card, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_card != nullptr, "out_card must not be null");
    *out_card = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    const auto card_id_value = text(card_id, "card_id", 1, SIZE_MAX);
    contract::TrashCardChildren children_value;
    switch (children) {
    case HOLDER_TRASH_CARD_CHILDREN_DEFAULT: children_value = contract::TrashCardChildren::Promote; break;
    case HOLDER_TRASH_CARD_CHILDREN_PROMOTE: children_value = contract::TrashCardChildren::Promote; break;
    case HOLDER_TRASH_CARD_CHILDREN_TRASH_SUBTREE: children_value = contract::TrashCardChildren::TrashSubtree; break;
    default: throw ArgumentError("children is not a holder_trash_card_children value");
    }
    auto* out = make<holder_card>(arena);
    *out = to_c(arena, contract::trash_card(*context, project_id_value, card_id_value, children_value));
    *out_card = out;
  });
}

int holder_restore_card(holder_context* context, const char* project_id, const char* card_id, holder_arena* arena, const holder_card** out_card, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_card != nullptr, "out_card must not be null");
    *out_card = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    const auto card_id_value = text(card_id, "card_id", 1, SIZE_MAX);
    auto* out = make<holder_card>(arena);
    *out = to_c(arena, contract::restore_card(*context, project_id_value, card_id_value));
    *out_card = out;
  });
}

int holder_move_card(holder_context* context, const char* project_id, const char* card_id, const holder_card_move* move, holder_arena* arena, const holder_card** out_card, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_card != nullptr, "out_card must not be null");
    *out_card = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    const auto card_id_value = text(card_id, "card_id", 1, SIZE_MAX);
    require(move != nullptr, "move must not be null");
    const auto move_value = from_c(*move);
    auto* out = make<holder_card>(arena);
    *out = to_c(arena, contract::move_card(*context, project_id_value, card_id_value, move_value));
    *out_card = out;
  });
}

int holder_add_card_tag(holder_context* context, const char* project_id, const char* card_id, const char* tag, holder_arena* arena, const holder_card** out_card, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_card != nullptr, "out_card must not be null");
    *out_card = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    const auto card_id_value = text(card_id, "card_id", 1, SIZE_MAX);
    const auto tag_value = text(tag, "tag", 1, SIZE_MAX);
    auto* out = make<holder_card>(arena);
    *out = to_c(arena, contract::add_card_tag(*context, project_id_value, card_id_value, tag_value));
    *out_card = out;
  });
}

int holder_remove_card_tag(holder_context* context, const char* project_id, const char* card_id, const char* tag, holder_arena* arena, const holder_card** out_card, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_card != nullptr, "out_card must not be null");
    *out_card = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    const auto card_id_value = text(card_id, "card_id", 1, SIZE_MAX);
    const auto tag_value = text(tag, "tag", 1, SIZE_MAX);
    auto* out = make<holder_card>(arena);
    *out = to_c(arena, contract::remove_card_tag(*context, project_id_value, card_id_value, tag_value));
    *out_card = out;
  });
}

int holder_list_project_tags(holder_context* context, const char* project_id, holder_arena* arena, const holder_tag_count_list** out_tag_count_list, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_tag_count_list != nullptr, "out_tag_count_list must not be null");
    *out_tag_count_list = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    auto* out = make<holder_tag_count_list>(arena);
    *out = to_c_list(arena, contract::list_project_tags(*context, project_id_value));
    *out_tag_count_list = out;
  });
}

int holder_list_card_links(holder_context* context, const char* project_id, const char* card_id, bool include_deleted, holder_arena* arena, const holder_link_list** out_link_list, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_link_list != nullptr, "out_link_list must not be null");
    *out_link_list = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    const auto card_id_value = text(card_id, "card_id", 1, SIZE_MAX);
    const auto include_deleted_value = include_deleted;
    auto* out = make<holder_link_list>(arena);
    *out = to_c_list(arena, contract::list_card_links(*context, project_id_value, card_id_value, include_deleted_value));
    *out_link_list = out;
  });
}

int holder_list_card_backlinks(holder_context* context, const char* project_id, const char* card_id, holder_list_card_backlinks_scope scope, bool include_deleted, holder_arena* arena, const holder_link_list** out_link_list, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_link_list != nullptr, "out_link_list must not be null");
    *out_link_list = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    const auto card_id_value = text(card_id, "card_id", 1, SIZE_MAX);
    contract::ListCardBacklinksScope scope_value;
    switch (scope) {
    case HOLDER_LIST_CARD_BACKLINKS_SCOPE_DEFAULT: scope_value = contract::ListCardBacklinksScope::Local; break;
    case HOLDER_LIST_CARD_BACKLINKS_SCOPE_LOCAL: scope_value = contract::ListCardBacklinksScope::Local; break;
    case HOLDER_LIST_CARD_BACKLINKS_SCOPE_EXTERNAL: scope_value = contract::ListCardBacklinksScope::External; break;
    case HOLDER_LIST_CARD_BACKLINKS_SCOPE_ALL_AVAILABLE: scope_value = contract::ListCardBacklinksScope::AllAvailable; break;
    default: throw ArgumentError("scope is not a holder_list_card_backlinks_scope value");
    }
    const auto include_deleted_value = include_deleted;
    auto* out = make<holder_link_list>(arena);
    *out = to_c_list(arena, contract::list_card_backlinks(*context, project_id_value, card_id_value, scope_value, include_deleted_value));
    *out_link_list = out;
  });
}

int holder_add_card_link(holder_context* context, const char* project_id, const char* card_id, const holder_new_link* link, holder_arena* arena, const holder_link** out_link, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_link != nullptr, "out_link must not be null");
    *out_link = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    const auto card_id_value = text(card_id, "card_id", 1, SIZE_MAX);
    require(link != nullptr, "link must not be null");
    const auto link_value = from_c(*link);
    auto* out = make<holder_link>(arena);
    *out = to_c(arena, contract::add_card_link(*context, project_id_value, card_id_value, link_value));
    *out_link = out;
  });
}

int holder_remove_card_link(holder_context* context, const char* project_id, const char* card_id, holder_remove_card_link_target_type target_type, const char* target_id, const char* kind, const char* target_project_id, holder_arena* arena, const holder_link_list** out_link_list, holder_error** out_error) {
  return guarded(arena, out_error, [&] {
    require(context != nullptr, "context must not be null");
    require(out_link_list != nullptr, "out_link_list must not be null");
    *out_link_list = nullptr;
    const auto project_id_value = text(project_id, "project_id", 1, SIZE_MAX);
    const auto card_id_value = text(card_id, "card_id", 1, SIZE_MAX);
    contract::RemoveCardLinkTargetType target_type_value;
    switch (target_type) {
    case HOLDER_REMOVE_CARD_LINK_TARGET_TYPE_CARD: target_type_value = contract::RemoveCardLinkTargetType::Card; break;
    case HOLDER_REMOVE_CARD_LINK_TARGET_TYPE_AI_MESSAGE: target_type_value = contract::RemoveCardLinkTargetType::AiMessage; break;
    case HOLDER_REMOVE_CARD_LINK_TARGET_TYPE_AI_THREAD: target_type_value = contract::RemoveCardLinkTargetType::AiThread; break;
    case HOLDER_REMOVE_CARD_LINK_TARGET_TYPE_RESOURCE: target_type_value = contract::RemoveCardLinkTargetType::Resource; break;
    default: throw ArgumentError("target_type is not a holder_remove_card_link_target_type value");
    }
    const auto target_id_value = text(target_id, "target_id", 1, SIZE_MAX);
    const auto kind_value = optional_text(kind, "kind", 1, SIZE_MAX).value_or("ref");
    const auto target_project_id_value = optional_text(target_project_id, "target_project_id", 1, SIZE_MAX);
    auto* out = make<holder_link_list>(arena);
    *out = to_c_list(arena, contract::remove_card_link(*context, project_id_value, card_id_value, target_type_value, target_id_value, kind_value, target_project_id_value));
    *out_link_list = out;
  });
}

}  // extern "C"
