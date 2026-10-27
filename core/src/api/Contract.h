// Generated from contract/interface/app.py, contract/interface/cards.py, contract/interface/errors.py, contract/interface/links.py, contract/interface/milestones.py, contract/interface/projects.py, contract/interface/resources.py by holder_contract.
// Do not edit; change the contract, then run make -C contract update.
//
// The C++ declarations core implements: one function per operation. A missing implementation fails to link.
#pragma once

#include <cstdint>
#include <map>
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

/// The request conflicts with what exists: what would be created already exists, such as a card with the same ID or file, or the thing to change is not in the state the request needs, such as restoring a card that is not in the trash, or removing a tag that a card would still carry in its prose.
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

/// What the link points at.
enum class NewLinkTargetType { Card, AiMessage, AiThread, Resource };

/// A link to make from a card. Its source is the card it is made from.
struct NewLink {
  NewLinkTargetType target_type;  ///< What the link points at.
  std::string target_id;  ///< The ID of what the link points at.
  std::optional<std::string> target_project_id;  ///< The project of a card the link points at; omit for the card's own project. Only a card can be in another project, which need not be on this device.
  std::string kind = "ref";  ///< What the link means.
  std::optional<std::string> label;  ///< A note on the link; null or omitted for none.
  std::optional<long long> created_at;  ///< When the link was made, for imports; omit for now. A link that already exists keeps its own.
};

/// One of a milestone's dates: a role and exactly one of a day or an instant.
struct MilestoneDate {
  std::string role;  ///< What the date is for.
  std::optional<std::string> date;  ///< The calendar day; present when the date is a day.
  std::optional<std::string> datetime;  ///< The instant; present when the date is an instant.
};

/// A milestone to put on a card.
struct NewMilestone {
  std::vector<MilestoneDate> dates;  ///< The milestone's dates, in order; empty for a milestone without a date.
  std::optional<std::string> kind;  ///< What kind of milestone it is; null or omitted for none.
  std::optional<std::string> description;  ///< A note on the milestone; null or omitted for none.
};

/// A change to a milestone. Each field left out keeps its value.
struct MilestoneUpdate {
  std::optional<std::vector<MilestoneDate>> dates;  ///< The milestone's new dates, replacing all of them; empty for none.
  Change<std::string> kind;  ///< What kind of milestone it is; null for none.
  Change<std::string> description;  ///< A note on the milestone; null for none.
};

/// A resource to make.
struct NewResource {
  std::optional<std::string> resource_id;  ///< The resource's ID, for imports; omit for a new one.
  std::string type;  ///< What kind of thing the resource is.
  std::string label;  ///< The resource's name.
  std::optional<std::map<std::string, std::vector<std::string>>> metadata;  ///< What is known about the resource; omit for nothing. A name without values is left out.
  std::optional<long long> created_at;  ///< When the resource was made, for imports; omit for now.
  std::optional<long long> updated_at;  ///< When the resource was last changed, for imports; omit for when it was made.
};

/// A change to a resource. Each field left out keeps its value.
struct ResourceUpdate {
  std::optional<std::string> type;  ///< What kind of thing the resource is.
  std::optional<std::string> label;  ///< The resource's name.
  std::optional<std::map<std::string, std::vector<std::string>>> metadata;  ///< Names whose values change: each name's values replace its old ones, an empty list removes the name, and names left out are kept.
  std::optional<long long> updated_at;  ///< When the resource was changed, for imports; omit for now.
};

/// How the project's files are stored.
enum class NewProjectPrivacyMode { EncryptedGit, Plain };

/// A project to make.
struct NewProject {
  std::optional<std::string> project_id;  ///< The project's ID, for imports; omit for a new one.
  std::string name;  ///< The project's name.
  std::optional<std::string> root_path;  ///< The directory for the project's repository, an absolute path; omit for one under this device's projects directory, named for the project.
  std::optional<std::string> git_remote_url;  ///< The Git remote to sync with; null or omitted for none. One Git does not accept is an invalid argument.
  std::optional<std::string> git_provider;  ///< Who hosts the Git remote; null or omitted when unknown.
  NewProjectPrivacyMode privacy_mode = NewProjectPrivacyMode::EncryptedGit;  ///< How the project's files are stored.
  std::optional<long long> created_at;  ///< When the project was made, for imports, after the epoch; omit for now.
  std::optional<long long> updated_at;  ///< When the project's settings last changed, for imports, after the epoch; omit for when it was made.
};

/// A change to a project's settings. Each field left out keeps its value.
struct ProjectUpdate {
  std::optional<std::string> name;  ///< The project's name.
  Change<std::string> git_remote_url;  ///< The Git remote to sync with; null for none.
  Change<std::string> git_provider;  ///< Who hosts the Git remote; null when unknown.
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

/// A tag and how many of a project's live cards carry it.
struct TagCount {
  std::string tag;  ///< The tag, normalized: in lower case.
  std::int32_t card_count{};  ///< The live cards in the project that carry the tag: at least one, or the tag is not listed.
};

/// What the link is from.
enum class LinkSourceType { Card, AiMessage };

/// What the link points at.
enum class LinkTargetType { Card, AiMessage, AiThread, Resource };

/// A link from a card, or from an AI message, to something in a project. A link names both of its ends in full:
/// each end's project, type and ID.
struct Link {
  std::string source_project_id;  ///< The project of the card or AI message the link is from.
  LinkSourceType source_type;  ///< What the link is from.
  std::string source_id;  ///< The ID of the card or AI message the link is from.
  std::string target_project_id;  ///< The project of what the link points at: the source's own project, or, for a card, another project, which need not be on this device. A link grants no access to its target.
  LinkTargetType target_type;  ///< What the link points at.
  std::string target_id;  ///< The ID of what the link points at.
  std::string kind;  ///< What the link means.
  std::optional<std::string> label;  ///< A note on the link; null when it has none.
  long long created_at{};  ///< When the link was made.
};

/// A milestone on a card: its dates, which may repeat a role or be none, and what it is.
struct Milestone {
  std::string milestone_id;  ///< The milestone's ID.
  std::string project_id;  ///< The project of the card the milestone is on.
  std::string card_id;  ///< The card the milestone is on.
  std::vector<MilestoneDate> dates;  ///< The milestone's dates, in the order they were given.
  std::optional<std::string> kind;  ///< What kind of milestone it is; null when it has none.
  std::optional<std::string> description;  ///< A note on the milestone; null when it has none.
  long long created_at{};  ///< When the milestone was made.
  long long updated_at{};  ///< When the milestone was last changed.
};

/// A milestone on a calendar, with the title of its card.
struct CalendarMilestone : Milestone {
  std::string card_title;  ///< The title of the card the milestone is on.
};

/// What happens in a project over a window of time: its milestones, and the cards made and changed.
struct Calendar {
  std::vector<CalendarMilestone> milestones;  ///< The milestones with a date in the window, as a day in its days or an instant in its instants, each once, by when they were made and then by ID.
  std::vector<CardMetadata> created_cards;  ///< The live cards made in the window, by when they were made and then by ID.
  std::vector<CardMetadata> updated_cards;  ///< The live cards last changed in the window, other than by being made, by when and then by ID.
};

/// How the copy is stored: as the file is, or encrypted for the project.
enum class AssetPlacementEncoding { Plain, HolderAssetV1 };

/// One stored copy of an asset.
struct AssetPlacement {
  std::string placement_id;  ///< The copy's ID.
  std::string location_id;  ///< The storage location that holds the copy.
  AssetPlacementEncoding encoding;  ///< How the copy is stored: as the file is, or encrypted for the project.
  long long stored_byte_size{};  ///< The size of the stored copy, in bytes.
  long long created_at{};  ///< When the copy was stored.
};

/// A file that belongs to a resource.
struct ResourceAsset {
  std::string asset_id;  ///< The asset's ID.
  std::string original_filename;  ///< The file's name when it was imported.
  std::string media_type;  ///< The file's media type, such as application/pdf.
  long long byte_size{};  ///< The file's size, in bytes.
  std::string plaintext_sha256;  ///< The SHA-256 of the file's content, in lower-case hex.
  std::vector<AssetPlacement> placements;  ///< The stored copies of the file.
  long long created_at{};  ///< When the asset was imported.
  long long updated_at{};  ///< When the asset was last changed.
};

/// A resource in a project, with its files.
struct Resource {
  std::string resource_id;  ///< The resource's ID.
  std::string project_id;  ///< The project the resource belongs to.
  std::string type;  ///< What kind of thing the resource is.
  std::string label;  ///< The resource's name.
  std::map<std::string, std::vector<std::string>> metadata;  ///< What is known about the resource, by name.
  std::vector<ResourceAsset> assets;  ///< The resource's files.
  long long created_at{};  ///< When the resource was made.
  long long updated_at{};  ///< When the resource was last changed.
};

/// How the project's files are stored.
enum class ProjectPrivacyMode { EncryptedGit, Plain };

/// A project on this device.
struct Project {
  std::string project_id;  ///< The project's ID.
  std::string name;  ///< The project's name.
  std::string root_path;  ///< The directory that holds the project's repository on this device.
  std::optional<std::string> git_remote_url;  ///< The Git remote the project syncs with; null when it has none.
  std::optional<std::string> git_provider;  ///< Who hosts the Git remote, such as github; null when unknown.
  ProjectPrivacyMode privacy_mode;  ///< How the project's files are stored.
  std::optional<std::string> project_key_id;  ///< The ID of an encrypted project's key; null for a plain project.
  long long created_at{};  ///< When the project was made.
  long long updated_at{};  ///< When the project's settings last changed.
};

/// A project in a listing.
struct ProjectListItem : Project {
  std::optional<std::int32_t> card_count;  ///< The project's live cards; present when asked for.
  std::optional<std::int32_t> root_card_count;  ///< The project's live root cards; present when asked for.
};

/// Fetch a card.
/// A card in the trash or in another project is not found.
/// @throws NotFound, KeyringUnavailable, KeyMissing, DataCorrupted
Card get_card(holder_context& context, std::string project_id, std::string card_id);

struct ListCardsParentRoots {};
using ListCardsParent = std::variant<ListCardsParentRoots, std::string>;
enum class ListCardsOrder { CardIdAsc, UpdatedDesc, SortKeyAsc };

/// Page through a project's live cards.
/// Filters combine. Each page reads current data; iterating is not a snapshot.
/// @param parent Omit for every level; roots selects root cards; a card ID selects its children.
/// @param tag Only cards with this tag.
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
Card create_card(holder_context& context, std::string project_id, const NewCard& card);

/// Change a card.
/// Each field left out keeps its value, and an update that leaves every field out changes nothing. The title and
/// content are written first, then the parent and place, each as its own change in the project's history. A card
/// in the trash or in another project is not found, and neither is a new parent that does not exist or is in the
/// trash; a parent under the card itself is an InvalidMove.
/// @throws NotFound, InvalidMove, KeyringUnavailable, KeyMissing, DataCorrupted
Card update_card(holder_context& context, std::string project_id, std::string card_id, const CardUpdate& update);

enum class TrashCardChildren { Promote, TrashSubtree };

/// Move a card to the trash.
/// The card and every card that changes with it are written as one change in the project's history. A card already
/// in the trash, or in another project, is not found. Returns the card as it is in the trash.
/// @param children promote moves the card's live children into its place among its siblings, keeping their own descendants; trash_subtree moves every live descendant to the trash with it.
/// @throws NotFound, KeyringUnavailable, KeyMissing, DataCorrupted
Card trash_card(holder_context& context, std::string project_id, std::string card_id, TrashCardChildren children);

/// Restore a card from the trash.
/// Restores this card only. It returns to its parent when that parent is live and reachable, otherwise to its
/// nearest live, reachable ancestor or the top level. Children promoted when it was trashed stay where they are;
/// descendants trashed with it stay in the trash, and come back under it when restored. A card in another
/// project is not found, and one that is not in the trash is a Conflict.
/// @throws NotFound, Conflict, KeyringUnavailable, KeyMissing, DataCorrupted
Card restore_card(holder_context& context, std::string project_id, std::string card_id);

/// Move a card in its project's hierarchy.
/// When the card's new neighbours leave no room between their places, nearby siblings are re-spaced in the same
/// change, keeping their order; other cards' sort_key values a caller holds may then be out of date. A card in the
/// trash or in another project is not found, and neither is a target or parent that does not exist or is in the
/// trash. A move that changes nothing writes nothing.
/// @throws NotFound, InvalidMove, KeyringUnavailable, KeyMissing, DataCorrupted
Card move_card(holder_context& context, std::string project_id, std::string card_id, const CardMove& move);

/// Tag a card.
/// The tag is added to the card's trailing tag line, which is made when the card has none. A card that already
/// carries the tag, anywhere in its content, is left as it is. A card in the trash or in another project is not
/// found.
/// @throws NotFound, KeyringUnavailable, KeyMissing, DataCorrupted
Card add_card_tag(holder_context& context, std::string project_id, std::string card_id, std::string tag);

/// Take a tag off a card.
/// The tag is removed from the card's trailing tag line, and the line with it when it is left empty, so that the
/// card no longer carries it. A card that does not carry the tag is left as it is. Holder does not rewrite prose,
/// so a card that would still carry the tag in its prose is a Conflict, and is left as it is. A card in the trash
/// or in another project is not found.
/// @throws NotFound, Conflict, KeyringUnavailable, KeyMissing, DataCorrupted
Card remove_card_tag(holder_context& context, std::string project_id, std::string card_id, std::string tag);

/// List a project's tags.
/// Each tag carried by a live card in the project, with how many carry it, by descending count and then by tag.
/// @throws NotFound
std::vector<TagCount> list_project_tags(holder_context& context, std::string project_id);

/// List a card's links.
/// The links the card makes, newest first. A card in the trash or in another project is not found.
/// @param include_deleted Include links to targets in the trash. A link to a card in another project is listed whether or not that card can be found here.
/// @throws NotFound
std::vector<Link> list_card_links(holder_context& context, std::string project_id, std::string card_id, bool include_deleted);

enum class ListCardBacklinksScope { Local, External, AllAvailable };

/// List the links to a card.
/// Links from cards and AI messages, newest first. External links come only from projects on this device, so
/// they are never all there may be. A card in the trash or in another project is not found.
/// @param scope local lists links from the card's own project; external, links from the other projects on this device; all_available, both.
/// @param include_deleted Include links from cards and AI messages in the trash.
/// @throws NotFound
std::vector<Link> list_card_backlinks(holder_context& context, std::string project_id, std::string card_id, ListCardBacklinksScope scope, bool include_deleted);

/// Link a card to something.
/// A link is one target, of one type in one project, with one kind. Making a link that already exists sets its
/// label and keeps when it was made. A target in the card's project must exist and not be in the trash; a card in
/// another project may be one this device does not have. A card in the trash or in another project is not found,
/// and neither is a target that does not exist or is in the trash.
/// @throws NotFound, KeyringUnavailable, KeyMissing, DataCorrupted
Link add_card_link(holder_context& context, std::string project_id, std::string card_id, const NewLink& link);

enum class RemoveCardLinkTargetType { Card, AiMessage, AiThread, Resource };

/// Remove a link from a card.
/// Removes the one link to that target, of that type in that project, with that kind, and returns the card's
/// remaining links as list_card_links lists them by default. A card without that link is left as it is. A card in
/// the trash or in another project is not found.
/// @param target_project_id Omit for the card's own project.
/// @throws NotFound, KeyringUnavailable, KeyMissing, DataCorrupted
std::vector<Link> remove_card_link(holder_context& context, std::string project_id, std::string card_id, RemoveCardLinkTargetType target_type, std::string target_id, std::string kind, std::optional<std::string> target_project_id);

/// List a card's milestones.
/// By when they were made, then by ID. A card in the trash or in another project is not found.
/// @throws NotFound
std::vector<Milestone> list_card_milestones(holder_context& context, std::string project_id, std::string card_id);

/// Put a milestone on a card.
/// The milestone gets a new ID. Each date must be a real day or instant. A card in the trash or in another project
/// is not found.
/// @throws NotFound, KeyringUnavailable, KeyMissing, DataCorrupted
Milestone add_card_milestone(holder_context& context, std::string project_id, std::string card_id, const NewMilestone& milestone);

/// Change a milestone.
/// Each field left out keeps its value, and an update that changes nothing writes nothing. Each date must be a real
/// day or instant. A card in the trash or in another project is not found, and neither is a milestone that is not
/// on the card.
/// @throws NotFound, KeyringUnavailable, KeyMissing, DataCorrupted
Milestone update_card_milestone(holder_context& context, std::string project_id, std::string card_id, std::string milestone_id, const MilestoneUpdate& update);

/// Take a milestone off a card.
/// Returns the card's remaining milestones, as list_card_milestones lists them. A card without the milestone is
/// left as it is. A card in the trash or in another project is not found.
/// @throws NotFound, KeyringUnavailable, KeyMissing, DataCorrupted
std::vector<Milestone> remove_card_milestone(holder_context& context, std::string project_id, std::string card_id, std::string milestone_id);

/// List a project's milestones with a day in a range.
/// The range holds first_day, last_day and the days between; it is empty when first_day is after last_day. A
/// milestone is listed once, by its first day in the range and then by ID. Milestones on cards in the trash are
/// left out.
/// @param role Only dates with this role; omit for any.
/// @throws NotFound
std::vector<Milestone> list_milestones_by_day(holder_context& context, std::string project_id, std::string first_day, std::string last_day, std::optional<std::string> role);

/// List a project's milestones with an instant in a range.
/// The range holds first_instant, last_instant and the instants between; it is empty when first_instant is after
/// last_instant. A milestone is listed once, by its first instant in the range and then by ID. Milestones on cards
/// in the trash are left out.
/// @param role Only dates with this role; omit for any.
/// @throws NotFound
std::vector<Milestone> list_milestones_by_instant(holder_context& context, std::string project_id, long long first_instant, long long last_instant, std::optional<std::string> role);

/// What happens in a project over a window of time.
/// The caller gives the window twice, as instants and as the days they fall on where the caller is, because a day
/// has no time zone: a milestone is in the window when one of its days is in the days, or one of its instants in
/// the instants. Cards are made and changed at instants. Either range is empty when its start is after its end.
/// @param first_day The day first_instant falls on where the caller is.
/// @param last_day The day last_instant falls on where the caller is.
/// @throws NotFound
Calendar get_calendar(holder_context& context, std::string project_id, long long first_instant, long long last_instant, std::string first_day, std::string last_day);

/// List a project's resources.
/// By when they last changed, latest first, then by ID.
/// @throws NotFound
std::vector<Resource> list_resources(holder_context& context, std::string project_id);

/// Fetch a resource.
/// A resource in another project is not found.
/// @throws NotFound
Resource get_resource(holder_context& context, std::string project_id, std::string resource_id);

/// Make a resource.
/// A project that does not exist is not found. Resource IDs are unique on this device, so an ID already in use,
/// in this project or another, is a Conflict.
/// @throws NotFound, Conflict, KeyringUnavailable, KeyMissing, DataCorrupted
Resource create_resource(holder_context& context, std::string project_id, const NewResource& resource);

/// Change a resource.
/// Each field left out keeps its value. A resource in another project is not found.
/// @throws NotFound, KeyringUnavailable, KeyMissing, DataCorrupted
Resource update_resource(holder_context& context, std::string project_id, std::string resource_id, const ResourceUpdate& update);

/// Delete a resource.
/// The resource goes, and so do the links to it from cards. Its files' stored copies are left where they are.
/// Returns the resource as it was. A resource in another project is not found.
/// @throws NotFound, KeyringUnavailable, KeyMissing, DataCorrupted
Resource delete_resource(holder_context& context, std::string project_id, std::string resource_id);

/// List the links to a resource.
/// Links of every kind, attachments among them, from cards and AI messages in the resource's project, newest
/// first. A resource in another project is not found.
/// @param include_deleted Include links from cards and AI messages in the trash.
/// @throws NotFound
std::vector<Link> list_resource_backlinks(holder_context& context, std::string project_id, std::string resource_id, bool include_deleted);

/// List the resources attached to a card.
/// By when they last changed, latest first, then by ID. A card in the trash or in another project is not found.
/// @throws NotFound
std::vector<Resource> list_card_resources(holder_context& context, std::string project_id, std::string card_id);

/// Attach a resource to a card.
/// An attachment is the card's link of kind attachment to the resource. A card that has it already is left as it
/// is. Returns the card's attached resources, as list_card_resources lists them. A card in the trash or in another
/// project is not found, and neither is a resource that is not in the card's project.
/// @throws NotFound, KeyringUnavailable, KeyMissing, DataCorrupted
std::vector<Resource> attach_card_resource(holder_context& context, std::string project_id, std::string card_id, std::string resource_id);

/// Detach a resource from a card.
/// The resource itself is kept. A card without the attachment is left as it is, and a resource that no longer
/// exists can still be detached. Returns the card's attached resources, as list_card_resources lists them. A card
/// in the trash or in another project is not found.
/// @throws NotFound, KeyringUnavailable, KeyMissing, DataCorrupted
std::vector<Resource> detach_card_resource(holder_context& context, std::string project_id, std::string card_id, std::string resource_id);

enum class ListProjectsOrder { UpdatedAtDesc, UpdatedAtAsc, CreatedAtDesc, CreatedAtAsc, NameDesc, NameAsc };

/// List the projects on this device.
/// Filters combine. Projects that tie in the order keep the order they were stored in.
/// @param name Only projects whose name contains this text.
/// @param updated_after Only projects last changed at or after this instant.
/// @param updated_before Only projects last changed at or before this instant.
/// @param order Omit for Home first, then the latest changed.
/// @param offset How many of the ordered projects to skip.
/// @param include_card_counts Include each project's counts of live cards.
std::vector<ProjectListItem> list_projects(holder_context& context, std::optional<std::string> name, std::optional<long long> updated_after, std::optional<long long> updated_before, std::optional<ListProjectsOrder> order, std::int32_t limit, std::int32_t offset, bool include_card_counts);

/// Fetch a project.
/// @throws NotFound
Project get_project(holder_context& context, std::string project_id);

/// Make a project.
/// Its repository is made, with its settings, and an encrypted project's key. An ID a project on this device has
/// is a Conflict. Making a project that was removed from this device makes it known here again.
/// @throws Conflict, KeyringUnavailable, KeyMissing, DataCorrupted
Project create_project(holder_context& context, const NewProject& project);

/// Change a project's settings.
/// Each field left out keeps its value, and an update that changes nothing writes nothing. The settings are
/// committed to the project's repository; a Git remote is set there before it is stored, and one Git does not
/// accept is an invalid argument.
/// @throws NotFound, KeyringUnavailable, KeyMissing, DataCorrupted
Project update_project(holder_context& context, std::string project_id, const ProjectUpdate& update);

/// Remove a project from this device.
/// The project's files and keys are kept, and its removal is remembered, so that looking for projects does not
/// bring it back; making it again does. Returns the project as it was.
/// @throws NotFound
Project delete_project(holder_context& context, std::string project_id);

namespace detail {

/// Core supplies this: sets *out_error (when not null) to a new holder_error with message, and
/// returns code.
int fail(holder_error** out_error, int code, const std::string& message);

}  // namespace detail

}  // namespace holder::contract
