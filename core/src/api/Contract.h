// Generated from contract/interface/app.py, contract/interface/cards.py, contract/interface/errors.py by holder_contract.
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

/// A field of a partial update: nullopt keeps it, an empty value clears it, and a value sets it.
template <class T>
using Change = std::optional<std::optional<T>>;

/// The thing asked for does not exist, or is in the trash.
struct NotFound : Error {
  using Error::Error;
};

/// The request conflicts with what exists: what would be created already exists, such as a card with the same ID or file, or the thing to change is not in the state the request needs, such as restoring a card that is not in the trash.
struct Conflict : Error {
  using Error::Error;
};

/// The keyring that holds an encrypted project's key cannot be used: it is locked, or this device has none.
struct KeyringUnavailable : Error {
  using Error::Error;
};

/// An encrypted project's key is not on this device; the project's recovery token restores it.
struct KeyMissing : Error {
  using Error::Error;
};

/// A project's stored data is damaged or unsafe: an encrypted file does not decrypt, or holds plaintext.
struct DataCorrupted : Error {
  using Error::Error;
};

/// A card cannot go where it was asked to: under itself or one of its descendants, beside a card that cannot be its sibling, or up a level from the top.
struct InvalidMove : Error {
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

/// Changes to a card. Each field left out keeps its value.
struct CardUpdate {
  std::optional<std::string> title;  ///< The card's new title.
  std::optional<std::string> content;  ///< The card's new body.
  Change<std::string> parent_card_id;  ///< The card's new parent; null makes it a root card.
  std::optional<double> sort_key;  ///< The card's new place among its siblings.
  std::optional<long long> updated_at;  ///< When the card was changed, for imports; omit for now.
};

/// into makes the card target_card_id's last child; before and after place it beside target_card_id, as its sibling; left and right swap it with its previous or next sibling; to_start and to_end make it the first or last of its siblings; up_level makes it its grandparent's last child.
enum class CardMoveIntent { Into, Before, After, Left, Right, ToStart, ToEnd, UpLevel };

/// Where to move a card, relative to another card or among its siblings.
struct CardMove {
  CardMoveIntent intent;  ///< into makes the card target_card_id's last child; before and after place it beside target_card_id, as its sibling; left and right swap it with its previous or next sibling; to_start and to_end make it the first or last of its siblings; up_level makes it its grandparent's last child.
  std::optional<std::string> target_card_id;  ///< The card to move into, before or after; required for those intents, and ignored by the others.
  Change<std::string> parent_card_id;  ///< For left, right, to_start and to_end: the parent among whose children to place it, null for the root cards; omit for its current parent. Ignored by the other intents.
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
/// @throws NotFound, KeyringUnavailable, KeyMissing, DataCorrupted
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
/// @throws NotFound, KeyringUnavailable, KeyMissing, DataCorrupted
Page<CardListItem> list_cards(holder_context& context, std::string project_id, std::optional<ListCardsParent> parent, std::optional<std::string> tag, ListCardsOrder order, std::int32_t limit, std::optional<std::string> cursor, bool include_content, bool include_child_count);

/// Create a card.
/// Fails with NotFound when the project or the parent does not exist.
/// @throws NotFound, Conflict, KeyringUnavailable, KeyMissing, DataCorrupted
Card create_card(holder_context& context, const NewCard& card);

/// Change a card.
/// Each field left out keeps its value, and an update that leaves every field out changes nothing. The title and
/// content are written first, then the parent and place, each as its own change in the project's history. A card
/// in the trash is not found, and neither is a new parent that does not exist or is in the trash; a parent under
/// the card itself is an InvalidMove.
/// @throws NotFound, InvalidMove, KeyringUnavailable, KeyMissing, DataCorrupted
Card update_card(holder_context& context, std::string card_id, const CardUpdate& update);

enum class TrashCardChildren { Promote, TrashSubtree };

/// Move a card to the trash.
/// The card and every card that changes with it are written as one change in the project's history. A card already
/// in the trash is not found. Returns the card as it is in the trash.
/// @param children promote moves the card's live children into its place among its siblings, keeping their own descendants; trash_subtree moves every live descendant to the trash with it.
/// @throws NotFound, KeyringUnavailable, KeyMissing, DataCorrupted
Card trash_card(holder_context& context, std::string card_id, TrashCardChildren children);

/// Restore a card from the trash.
/// Restores this card only. It returns to its parent when that parent is live and reachable, otherwise to its
/// nearest live, reachable ancestor or the top level. Children promoted when it was trashed stay where they are;
/// descendants trashed with it stay in the trash, and come back under it when restored. A card that is not in the
/// trash is a Conflict.
/// @throws NotFound, Conflict, KeyringUnavailable, KeyMissing, DataCorrupted
Card restore_card(holder_context& context, std::string card_id);

/// Move a card in its project's hierarchy.
/// When the card's new neighbours leave no room between their places, nearby siblings are re-spaced in the same
/// change, keeping their order; other cards' sort_key values a caller holds may then be out of date. A card in the
/// trash is not found, and neither is a target or parent that does not exist, is in the trash or is in another
/// project. A move that changes nothing writes nothing.
/// @throws NotFound, InvalidMove, KeyringUnavailable, KeyMissing, DataCorrupted
Card move_card(holder_context& context, std::string card_id, const CardMove& move);

namespace detail {

/// Core supplies this: sets *out_error (when not null) to a new holder_error with message, and
/// returns code.
int fail(holder_error** out_error, int code, const std::string& message);

}  // namespace detail

}  // namespace holder::contract
