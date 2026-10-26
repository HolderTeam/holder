// Generated from contract/interface/cards.py by holder_contract.
// Do not edit; change the contract, then run make -C contract update.
//
// The C++ declarations core implements: one function per operation. A missing implementation fails to link.
#pragma once

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

struct holder_context;
struct holder_error;

namespace holder::contract {

/// Base of the error kinds operations throw. Anything else thrown is an unexpected failure.
struct Error : std::runtime_error {
  using std::runtime_error::runtime_error;
};

/// Thrown for an argument the contract's checks cannot catch.
struct InvalidArgument : Error {
  using Error::Error;
};

/// The thing asked for does not exist, or is in the trash.
struct NotFound : Error {
  using Error::Error;
};

/// One page of a listing.
template <class T>
struct Page {
  std::vector<T> items;  ///< The page's items.
  std::optional<std::string> next_cursor;  ///< Pass back to get the next page; null on the last page.
};

/// A card to create.
struct NewCard {
  std::string project_id;  ///< The project to create it in.
  std::string title;  ///< The card's title.
  std::string content = "";  ///< The card's body.
  std::optional<std::string> parent_card_id;  ///< The card's parent; null or omitted for a root card.
  std::optional<double> sort_key;  ///< The card's place among its siblings; omit to let Holder choose.
  std::optional<long long> created_at;  ///< When the card was created, for imports; omit for now.
  std::optional<long long> updated_at;  ///< When the card was last changed, for imports; omit to use created_at.
};

/// A tag written in a card's content.
struct TagOccurrence {
  std::string tag;  ///< The normalized tag.
  long long byte_start{};  ///< Inclusive byte offset of the leading # in the content.
  long long byte_end{};  ///< Exclusive byte offset after the tag's name.
};

/// Everything about a card except its content.
struct CardMetadata {
  std::string card_id;  ///< The card's ID.
  std::string project_id;  ///< The project the card belongs to.
  std::string title;  ///< The card's title.
  std::string rel_path;  ///< The card's file, relative to the project's root.
  std::optional<std::string> parent_card_id;  ///< The card's parent; null for a root card.
  double sort_key{};  ///< The card's place among its siblings; lower comes first.
  long long created_at{};  ///< When the card was created.
  long long updated_at{};  ///< When the card was last changed.
  std::optional<long long> deleted_at;  ///< When the card was moved to the trash; null when it is not in the trash.
};

/// A card, with its content and tags.
struct Card : CardMetadata {
  std::string content;  ///< The card's body.
  std::vector<std::string> tags;  ///< The card's normalized tags.
  std::vector<TagOccurrence> tag_occurrences;  ///< Each tag written in the content, in order.
};

/// A card in a listing.
struct CardListItem : CardMetadata {
  std::optional<std::int32_t> child_count;  ///< The card's live children; present when asked for.
  std::optional<std::string> content;  ///< The card's body; present when asked for.
};

/// Fetch a card.
/// A card in the trash is not found.
/// @throws NotFound
Card get_card(holder_context& context, std::string card_id);

struct ListCardsParentRoots {};
using ListCardsParent = std::variant<ListCardsParentRoots, std::string>;
enum class ListCardsOrder { CardIdAsc, UpdatedDesc, SortKeyAsc };

/// Page through a project's live cards.
/// Filters combine. Each page reads current data; iterating is not a snapshot.
/// @param parent Omit for every level; roots selects root cards; a card ID selects its children.
/// @param tag Only cards with this tag, matched without case and without the leading #. A tag starts with an ASCII letter and holds only ASCII letters, digits, _, / and -; a hex colour such as fff is not a tag.
/// @param order updated_desc breaks ties by descending card ID; sort_key_asc follows sibling order.
/// @param limit At most 1000 when include_content is true.
/// @param cursor The previous page's next_cursor. Keep the project, filters and order unchanged.
/// @param include_content Include each card's body.
/// @param include_child_count Include each card's count of live children.
/// @throws NotFound
Page<CardListItem> list_cards(holder_context& context, std::string project_id, std::optional<ListCardsParent> parent, std::optional<std::string> tag, ListCardsOrder order, std::int32_t limit, std::optional<std::string> cursor, bool include_content, bool include_child_count);

/// Create a card.
/// Fails with NotFound when the project or the parent does not exist.
/// @throws NotFound
Card create_card(holder_context& context, const NewCard& card);

namespace detail {

/// Core supplies this: sets *out_error (when not null) to a new holder_error with message, and
/// returns code.
int fail(holder_error** out_error, int code, const std::string& message);

}  // namespace detail

}  // namespace holder::contract
