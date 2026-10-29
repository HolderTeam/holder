// Generated from contract/interface/app.py, contract/interface/card_references.py, contract/interface/cards.py, contract/interface/errors.py, contract/interface/git.py, contract/interface/history.py, contract/interface/links.py, contract/interface/maintenance.py, contract/interface/milestones.py, contract/interface/projects.py, contract/interface/rebuild.py, contract/interface/recovery.py, contract/interface/resources.py, contract/interface/search.py, contract/interface/trash.py by holder_contract.
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

/// A card's ancestor chain is broken, cyclic or too deep to follow.
struct HierarchyInvalid : Error {
  using Error::Error;
};

/// The project's Git history cannot be read right now.
struct HistoryUnavailable : Error {
  using Error::Error;
};

/// This device cannot use the encrypted project's key to read its history.
struct HistoryKeyUnavailable : Error {
  using Error::Error;
};

/// A commit ID prefix matches more than one revision; use a longer prefix or the full ID.
struct RevisionAmbiguous : Error {
  using Error::Error;
};

/// The item still exists but is not in Trash, so permanent removal is refused.
struct NotTrashed : Error {
  using Error::Error;
};

/// The bounded History response would exceed 2 MiB.
struct HistoryResponseTooLarge : Error {
  using Error::Error;
};

/// The recovery token or its PIN is invalid, or the token belongs to another project.
struct RecoveryTokenInvalid : Error {
  using Error::Error;
};

/// A project's files cannot be read back into the database.
struct RebuildFailed : Error {
  using Error::Error;
};

/// One page of a listing.
template <class T>
struct Page {
  std::vector<T> items;  ///< The page's items.
  std::optional<std::string> next_cursor;  ///< Pass back to get the next page; null on the last page.
};

/// Which cards may match.
enum class CardReferenceResolveRequestScope { Live, Trashed, Either };

/// A reference to resolve among one project's cards.
struct CardReferenceResolveRequest {
  std::string project_id;  ///< The project to search.
  std::string reference;  ///< A full canonical UUID, eligible UUID prefix, or exact card title.
  CardReferenceResolveRequestScope scope;  ///< Which cards may match.
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

/// Ask for a repair plan, or apply it.
struct HierarchyRepairRequest {
  bool apply = false;  ///< Apply the repair. Without it, only report.
};

/// PIN used to wrap the project key.
struct RecoveryTokenExportRequest {
  std::string pin;  ///< User-provided PIN used to wrap or unwrap the key.
};

/// PIN and serialized .hrk token to restore.
struct RecoveryTokenImportRequest : RecoveryTokenExportRequest {
  std::string recovery_token;  ///< Serialized .hrk token JSON.
};

/// The project whose files should be read into the database.
struct RebuildRequest {
  std::string project_id;  ///< The project to rebuild.
};

/// How to push.
struct PushOptions {
  std::optional<std::string> branch;  ///< The branch to push; omit for the repository's default.
  bool set_upstream = true;  ///< Whether the remote branch becomes the local branch's upstream.
};

/// A remote to test.
struct RemoteTest {
  std::optional<std::string> remote_url;  ///< The Git remote to test; omit for the project's own.
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

/// Whether this step names the project or a card.
enum class CardBreadcrumbKind { Project, Card };

/// One project or live card in a card's path from its project root.
struct CardBreadcrumb {
  CardBreadcrumbKind kind;  ///< Whether this step names the project or a card.
  std::string id;  ///< The project or card ID.
  std::string title;  ///< The project or card title.
};

/// A card that matches a reference.
struct CardReferenceCandidate {
  std::string card_id;  ///< The card's canonical full UUID.
  std::string title;  ///< The card's title.
  std::optional<long long> deleted_at;  ///< When the card entered Trash, or null while live.
};

/// The resolution outcome.
enum class CardReferenceResolveDataStatus { Resolved, Ambiguous, NotFound };

/// A resolved, ambiguous, or absent card reference.
struct CardReferenceResolveData {
  CardReferenceResolveDataStatus status;  ///< The resolution outcome.
  std::optional<std::string> match_kind;  ///< How a reference matched: full_id, id_prefix or exact_title. Absent when no card matched.
  std::optional<CardReferenceCandidate> card;  ///< The matching card for a resolved reference.
  std::optional<std::vector<CardReferenceCandidate>> candidates;  ///< Two candidate cards for an ambiguous reference.
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

/// Whether an encrypted project's card files are safe to push.
struct EncryptionSafetyCheck {
  bool ok{};  ///< Whether every checked file passed.
  std::int32_t checked_files{};  ///< Card files examined.
  std::int32_t unsafe_files{};  ///< Card files that failed the safety check.
  std::vector<std::string> unsafe_paths;  ///< Project-relative paths of unsafe files.
  std::string message;  ///< A short explanation of the result.
};

/// How the project's files are stored.
enum class ProjectEncryptionCheckPrivacyMode { EncryptedGit, Plain };

/// A project's privacy mode and the result of checking its files.
struct ProjectEncryptionCheck {
  std::string project_id;  ///< The project checked.
  ProjectEncryptionCheckPrivacyMode privacy_mode;  ///< How the project's files are stored.
  EncryptionSafetyCheck check;  ///< The safety check's result.
};

/// A hidden branch's top card and its new parent.
struct HierarchyRepairMove {
  std::string card_id;  ///< The hidden branch's top card.
  std::optional<std::string> parent_card_id;  ///< Its new parent, or null for a project root.
};

/// A plan to make hidden live cards reachable, and whether it was applied.
struct HierarchyRepair {
  bool applied{};  ///< Whether changes were written.
  std::vector<HierarchyRepairMove> moved;  ///< The hidden branches whose top cards move.
  std::int32_t changed_card_count{};  ///< Cards moved or re-spaced to make room.
  std::vector<std::string> cycle_card_ids;  ///< Cards in parent cycles; left for manual repair.
};

/// A portable encrypted project key.
struct ProjectRecoveryTokenExportResult {
  std::string project_id;  ///< The project whose key was exported.
  std::string key_id;  ///< The encrypted project's key ID.
  std::string recovery_token;  ///< Serialized .hrk token JSON.
};

/// The project whose key was restored.
struct ProjectRecoveryTokenImportResult {
  std::string project_id;  ///< The project whose key was restored.
};

/// Outcome of the attempted pull.
enum class RecoveryTokenGlobalImportResultPullStatus { NotAttempted, Succeeded, Failed };

/// The result of restoring a token to this device.
struct RecoveryTokenGlobalImportResult : ProjectRecoveryTokenImportResult {
  bool project_created{};  ///< Whether this device made the project from the token.
  bool remote_hint_present{};  ///< Whether the token names a Git remote.
  bool remote_configured{};  ///< Whether that remote was configured.
  std::optional<std::string> remote_error;  ///< Why remote configuration failed, or null.
  RecoveryTokenGlobalImportResultPullStatus pull_status;  ///< Outcome of the attempted pull.
  std::optional<std::string> pull_error;  ///< Why pulling failed, or null.
};

/// The numbers of entities restored from a project's files.
struct RebuildResult {
  std::string project_id;  ///< The project rebuilt.
  std::int32_t cards{};  ///< Cards restored.
  std::int32_t ai_messages{};  ///< AI messages restored.
  std::int32_t ai_threads{};  ///< AI threads restored.
  std::int32_t links{};  ///< Links restored.
};

/// Completion of a full database search-index rebuild.
struct ReindexResult {
  std::string message;  ///< A short completion message.
};

/// What this device last recorded about a project's sync: its last pull and push, what is waiting to go, and
/// when it will retry.
struct SyncState {
  std::string project_id;  ///< The project.
  std::optional<long long> last_commit_at;  ///< When the project's repository last had a commit; null when unknown.
  std::optional<long long> last_push_at;  ///< When the project was last pushed; null when never.
  std::optional<long long> last_pull_at;  ///< When the project was last pulled; null when never.
  std::int32_t uncommitted_changes_count{};  ///< Files changed and not yet committed.
  std::int32_t unpushed_commits_count{};  ///< Commits not yet on the remote.
  std::optional<std::string> last_push_status;  ///< How the last push ended, as a PushStatus; null when never.
  std::optional<std::string> last_pull_status;  ///< How the last pull ended; null when never.
  std::optional<std::string> last_sync_error;  ///< The last failure's message; null when the last sync succeeded.
  std::optional<long long> last_sync_error_at;  ///< When the last failure happened; null when none is recorded.
  std::int32_t retry_count{};  ///< Failed pushes in a row.
  std::optional<long long> next_retry_at;  ///< When a push will next be tried; null when none is waiting.
  std::int32_t pull_retry_count{};  ///< Failed pulls in a row.
  std::optional<long long> next_pull_retry_at;  ///< When a pull will next be tried; null when none is waiting.
  std::optional<long long> updated_at;  ///< When this state was last recorded; null when nothing has been.
};

/// How the pull ended.
enum class PullPhaseStatus { NotAttempted, Succeeded, RemoteUnset, Failed };

/// What a pull did.
struct PullPhase {
  bool attempted{};  ///< Whether a pull was tried.
  PullPhaseStatus status;  ///< How the pull ended.
  std::int32_t conflicts_resolved{};  ///< Cards changed both here and on the remote, merged by Holder's card rules.
  std::int32_t hidden_cards_repaired{};  ///< Live cards the pull left under a missing or trashed parent, made reachable again.
  std::optional<std::string> error_message;  ///< Why the pull failed; null when it did not.
};

/// How the push ended.
enum class PushPhaseStatus { NotAttempted, Pushed, UpToDate, AuthFailed, NotFound, NetworkError, NonFastForward, RemoteUnset, UnknownError };

/// What a push did.
struct PushPhase {
  bool attempted{};  ///< Whether a push was tried.
  PushPhaseStatus status;  ///< How the push ended.
  std::int32_t ahead_count{};  ///< Commits here that the remote does not have.
  std::int32_t behind_count{};  ///< Commits on the remote that are not here.
  std::optional<std::string> local_head_commit;  ///< The commit this device's branch is at; null when unknown.
  std::optional<std::string> error_message;  ///< Why the push failed; null when it did not.
};

/// Whether every phase tried succeeded.
enum class SyncResultStatus { Succeeded, Failed };

/// What a pull, a push or a sync did, as recorded in the project's sync state. A Git failure is an outcome
/// here, not an error.
struct SyncResult {
  std::string project_id;  ///< The project.
  SyncResultStatus status;  ///< Whether every phase tried succeeded.
  std::optional<std::string> remote_url;  ///< The Git remote used; null when the project has none.
  PullPhase pull;  ///< The pull; not attempted for a push.
  PushPhase push;  ///< The push; not attempted for a pull, or after a pull that failed.
};

/// What testing the remote found; remote_unset when there was none to test.
enum class RemoteTestResultStatus { Reachable, AuthFailed, NotFound, NetworkError, InvalidRemoteUrl, RemoteUnset, UnknownError };

/// Whether a Git remote can be reached with this device's credentials.
struct RemoteTestResult {
  std::string project_id;  ///< The project.
  RemoteTestResultStatus status;  ///< What testing the remote found; remote_unset when there was none to test.
  std::optional<std::string> remote_url;  ///< The remote tested; null when there was none.
  bool remote_has_head{};  ///< Whether the remote has any commits.
  std::optional<std::string> error_message;  ///< Why the remote could not be reached; null when it could.
};

/// The author recorded on a Git commit.
struct HistoryAuthor {
  std::string name;  ///< The author's name.
  std::string email;  ///< The author's email address.
};

/// A changed path with any display details found in its historical version.
struct HistoryAffectedPath {
  std::string path;  ///< The path relative to the project root.
  std::optional<std::string> title;  ///< A historical title, when available.
  std::optional<std::string> detail;  ///< A historical detail, when available.
};

/// What kind of object these paths belong to.
enum class HistoryAffectedObjectKind { Card, Resource, Location, AiData, ProjectSettings, Unknown };

/// Paths changed together in one commit, grouped by Holder object kind.
struct HistoryAffectedObject {
  HistoryAffectedObjectKind kind;  ///< What kind of object these paths belong to.
  std::vector<std::string> paths;  ///< The changed paths, retained for existing clients.
  std::vector<HistoryAffectedPath> items;  ///< The changed paths with historical display details.
};

/// One Git commit affecting a project.
struct ProjectActivity {
  std::string oid;  ///< The full commit object ID.
  std::vector<std::string> parent_oids;  ///< The commit's parents.
  HistoryAuthor author;  ///< The commit's author.
  long long authored_at{};  ///< When the commit was authored.
  long long committed_at{};  ///< When the commit was committed.
  std::string message;  ///< The commit message.
  std::vector<HistoryAffectedObject> affected_objects;  ///< Changed paths grouped by Holder object kind.
  bool is_merge{};  ///< Whether the commit has multiple parents.
};

/// One bounded page of project activity, newest first.
struct ProjectHistoryPage {
  std::optional<std::string> head_oid;  ///< The project HEAD captured for this page; null for an empty repository.
  std::vector<ProjectActivity> activities;  ///< The activities on this page, newest first.
  std::optional<std::string> next_cursor;  ///< The cursor for another page; null at the end.
  bool scan_limited{};  ///< Whether History reached its bounded scan before the end.
};

/// One exact commit saved for a card.
struct CardHistorySave {
  std::string oid;  ///< The full commit ID.
  std::vector<std::string> parent_oids;  ///< The commit's parents.
  long long authored_at{};  ///< When the commit was authored.
  long long committed_at{};  ///< When the commit was committed.
  std::string message;  ///< The commit message.
};

/// One displayed card change, possibly grouping adjacent autosaves.
struct CardHistoryEntry {
  std::string first_oid;  ///< The first commit in the entry.
  std::string last_oid;  ///< The last commit in the entry.
  std::vector<std::string> parent_oids;  ///< Parents of the first commit.
  std::vector<std::string> visible_parent_oids;  ///< Direct parents represented on this page; omitted connections must not be inferred.
  HistoryAuthor author;  ///< The author's name and email.
  long long started_at{};  ///< When the first commit was authored.
  long long ended_at{};  ///< When the last commit was committed.
  std::string kind;  ///< The kind of change shown.
  std::string summary;  ///< A short description of the change.
  std::int32_t commit_count{};  ///< The number of commits grouped in this entry.
  bool is_merge{};  ///< Whether this entry includes a merge commit.
  std::vector<CardHistorySave> saves;  ///< Exact saved commits in chronological order.
};

/// One bounded page of saved card changes, newest first.
struct CardHistoryPage {
  std::optional<std::string> head_oid;  ///< The project HEAD captured for this page; null for an empty repository.
  std::vector<CardHistoryEntry> entries;  ///< The card changes on this page.
  std::optional<std::string> next_cursor;  ///< The cursor for another page; null at the end.
  bool scan_limited{};  ///< Whether History reached its bounded scan before the end.
};

/// A card's saved state at one commit, including an explicit absent state.
struct CardHistoryVersion {
  bool exists{};  ///< Whether the card existed in this snapshot.
  std::string oid;  ///< The full commit ID.
  std::string title;  ///< The card's saved title, empty when absent.
  std::string body;  ///< The card's saved body, empty when absent.
};

/// One line in a card comparison.
struct CardHistoryDiffLine {
  std::string origin;  ///< Space for unchanged, plus for added, minus for removed.
  std::string text;  ///< The line's text.
  std::optional<long long> old_line;  ///< Its old line number; null for an addition.
  std::optional<long long> new_line;  ///< Its new line number; null for a removal.
};

/// A bounded line comparison of two saved card states.
struct CardHistoryComparison {
  CardHistoryVersion before;  ///< The earlier state.
  CardHistoryVersion after;  ///< The later state.
  std::string summary;  ///< A short description of the difference.
  std::vector<CardHistoryDiffLine> lines;  ///< The comparison's lines.
  bool truncated{};  ///< Whether the line comparison was cut short.
};

/// A card's state at a selected revision.
struct CardHistorySnapshot {
  std::string card_id;  ///< The card being inspected.
  CardHistoryVersion snapshot;  ///< The state at the selected commit.
};

/// A card restored from a saved state by creating a new commit.
struct CardHistoryRestoreResult {
  std::string card_id;  ///< The restored card.
  std::string restored_from_oid;  ///< The full commit ID of the selected historical snapshot.
  std::string result_oid;  ///< The full commit ID created by restoring it.
  std::string title;  ///< The card's title after restoration.
  long long updated_at{};  ///< When the restored card was updated.
  std::optional<long long> deleted_at;  ///< When the restored card is in Trash; null when live.
};

/// The kind of item.
enum class TrashItemType { Card, AiMessage };

/// A trashed card or AI message.
struct TrashItem {
  TrashItemType type;  ///< The kind of item.
  std::string project_id;  ///< The project that owns the item.
  std::optional<std::string> card_id;  ///< The card ID, present for a card.
  std::optional<std::string> message_id;  ///< The message ID, present for an AI message.
  std::optional<std::string> thread_id;  ///< The AI thread containing a message.
  std::optional<std::string> title;  ///< The card title, if this is a card.
  std::optional<std::string> role;  ///< The AI message role, if this is a message.
  long long deleted_at{};  ///< When the item entered Trash.
  std::optional<long long> created_at;  ///< When the AI message was created.
  std::optional<std::string> rel_path;  ///< The card's relative file path.
};

/// The project whose Trash was emptied.
struct TrashEmptyResult {
  std::string project_id;  ///< The requested project, including when it had no trashed items.
};

/// The ID of a permanently removed item.
struct TrashDeleteResult {
  std::optional<std::string> card_id;  ///< The removed card's ID, or null for a message.
  std::optional<std::string> message_id;  ///< The removed AI message's ID, or null for a card.
};

/// A card matching a full-text query.
struct SearchCard {
  std::string card_id;  ///< The matching card's ID.
  std::string title;  ///< The card's current title.
  long long updated_at{};  ///< When the card was last updated.
  long long created_at{};  ///< When the card was created.
  std::string snippet;  ///< A highlighted excerpt from the match.
  double rank{};  ///< The FTS relevance score; lower scores rank first.
};

/// An AI message matching a full-text query.
struct SearchMessage {
  std::string message_id;  ///< The matching message's ID.
  long long created_at{};  ///< When the message was created.
  std::string snippet;  ///< A highlighted excerpt from the match.
  double rank{};  ///< The FTS relevance score; lower scores rank first.
};

/// Fetch a card.
/// A card in the trash or in another project is not found.
/// @throws NotFound, KeyringUnavailable, KeyMissing, DataCorrupted
Card get_card(holder_context& context, std::string project_id, std::string card_id);

/// Read a live card's path from its project root.
/// The project comes first, then live ancestor cards and the requested card. A missing or trashed
/// card is not found; a broken or cyclic ancestor chain is invalid.
/// @throws NotFound, HierarchyInvalid
std::vector<CardBreadcrumb> get_card_breadcrumbs(holder_context& context, std::string project_id, std::string card_id);

/// Resolve a card reference within a project and deletion scope.
/// @throws NotFound
CardReferenceResolveData resolve_card_reference(holder_context& context, const CardReferenceResolveRequest& request);

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

/// Check whether an encrypted project's card files are safe to push.
/// A plain project needs no check and reports a successful result with zero files.
/// @throws NotFound
ProjectEncryptionCheck check_project_encryption(holder_context& context, std::string project_id);

/// Find hidden live card branches, and optionally move them into the reachable tree.
/// An empty body plans without writing. Applying moves each hidden branch to its nearest reachable
/// ancestor in one commit; a second apply changes nothing. Parent cycles are reported and left alone.
/// @throws NotFound, KeyringUnavailable, KeyMissing, DataCorrupted
HierarchyRepair repair_project_hierarchy(holder_context& context, std::string project_id, const HierarchyRepairRequest& request);

/// Wrap an encrypted project's key in a portable token protected by a PIN.
/// @throws NotFound, KeyringUnavailable, KeyMissing, DataCorrupted
ProjectRecoveryTokenExportResult export_project_recovery_token(holder_context& context, std::string project_id, const RecoveryTokenExportRequest& request);

/// Restore an encrypted project's key from a token for that project.
/// @throws NotFound, RecoveryTokenInvalid, KeyringUnavailable, DataCorrupted
ProjectRecoveryTokenImportResult import_project_recovery_token(holder_context& context, std::string project_id, const RecoveryTokenImportRequest& request);

/// Restore a token, finding its project here or making one from the token's metadata.
/// If the token names a Git remote, configure it and attempt a pull. Remote and pull failures
/// are reported in the result. Returns 201 when the project was created, otherwise 200.
/// @throws RecoveryTokenInvalid, KeyringUnavailable, DataCorrupted
RecoveryTokenGlobalImportResult import_recovery_token_global(holder_context& context, const RecoveryTokenImportRequest& request);

/// Rebuild a project's database state from its files.
/// A missing project is NotFound. Files that cannot be read back give RebuildFailed.
/// @throws NotFound, RebuildFailed
RebuildResult rebuild_project(holder_context& context, const RebuildRequest& request);

/// Rebuild all search indexes from database state.
ReindexResult reindex_search(holder_context& context);

/// What this device last recorded about a project's sync.
/// @throws NotFound
SyncState get_project_sync_state(holder_context& context, std::string project_id);

/// Pull a project's checked-out branch from its Git remote.
/// Changes fast-forward, or merge card by card where both sides changed, and the project's index is updated
/// before the result returns. A Git failure is an outcome in the result, recorded in the sync state. Git work on
/// one project runs one at a time.
/// @throws NotFound, KeyringUnavailable, KeyMissing, DataCorrupted
SyncResult pull_project(holder_context& context, std::string project_id);

/// Push a project to its Git remote.
/// An encrypted project is checked first, so no plaintext card leaves the device: one whose files are not safe
/// to push is DataCorrupted, and nothing is pushed. A Git failure is an outcome in the result, recorded in the
/// sync state.
/// @throws NotFound, KeyringUnavailable, KeyMissing, DataCorrupted
SyncResult push_project(holder_context& context, std::string project_id, const PushOptions& options);

/// Pull a project, then push it.
/// The push is tried only when the pull succeeds. Otherwise as pull_project and push_project.
/// @throws NotFound, KeyringUnavailable, KeyMissing, DataCorrupted
SyncResult sync_project(holder_context& context, std::string project_id, const PushOptions& options);

/// Test whether a Git remote can be reached, with this device's credentials.
/// Lists the remote's refs and nothing more: nothing is fetched, and neither the project nor its repository
/// changes.
/// @throws NotFound
RemoteTestResult test_project_remote(holder_context& context, std::string project_id, const RemoteTest& remote);

enum class ListProjectHistoryKind { Card, Resource, Location, AiData, ProjectSettings, Unknown };

/// Return bounded, cursor-paginated Git activities for a project.
/// Each commit groups its changed paths by Holder object kind. Unrecognised paths remain visible as unknown.
/// A missing decryption key is HistoryKeyUnavailable; other history read failures are HistoryUnavailable.
/// @param cursor A full commit OID from next_cursor.
/// @param kind Only activities affecting this object kind.
/// @throws NotFound, HistoryKeyUnavailable, HistoryResponseTooLarge, HistoryUnavailable
ProjectHistoryPage list_project_history(holder_context& context, std::string project_id, std::int32_t limit, std::optional<std::string> cursor, std::optional<ListProjectHistoryKind> kind);

/// List saved history for a card.
/// Returns commits reachable from the captured project HEAD that changed the card. Adjacent autosaves may be
/// grouped into one displayed entry.
/// @param cursor A full commit ID from next_cursor.
/// @throws NotFound, HistoryKeyUnavailable, HistoryResponseTooLarge, HistoryUnavailable
CardHistoryPage list_card_history(holder_context& context, std::string project_id, std::string card_id, std::int32_t limit, std::optional<std::string> cursor);

enum class CompareCardHistoryMode { Since, Change };

/// Compare saved card versions or one revision's change.
/// In since mode, compares the selected before and after snapshots. In change mode, compares the first parent
/// of to_oid with to_oid. A root commit has a previously nonexistent card as its before state; a merge uses only
/// its first parent.
/// @param to_oid The ending revision, or the change to inspect.
/// @param from_oid Required in since mode; ignored in change mode.
/// @param mode Compare two snapshots or one introduced change.
/// @throws NotFound, RevisionAmbiguous, HistoryKeyUnavailable, HistoryResponseTooLarge, HistoryUnavailable
CardHistoryComparison compare_card_history(holder_context& context, std::string project_id, std::string card_id, std::string to_oid, std::optional<std::string> from_oid, CompareCardHistoryMode mode);

/// Read a card snapshot at a selected revision, including an explicit absent state.
/// @param oid The revision containing the card snapshot.
/// @throws NotFound, RevisionAmbiguous, HistoryKeyUnavailable, HistoryResponseTooLarge, HistoryUnavailable
CardHistorySnapshot get_card_history_snapshot(holder_context& context, std::string project_id, std::string card_id, std::string oid);

/// Restore a card to a saved snapshot.
/// Restores the complete snapshot as a new commit, preserving the old history. Lifecycle rules still apply:
/// restoring a trashed snapshot promotes live children, and restoring a live snapshot places the card under a
/// reachable parent.
/// @param oid The revision containing the snapshot to restore.
/// @throws NotFound, RevisionAmbiguous, HistoryKeyUnavailable, HistoryUnavailable
CardHistoryRestoreResult restore_card_history_snapshot(holder_context& context, std::string project_id, std::string card_id, std::string oid);

/// List a project's trashed cards and AI messages.
/// A project that is not registered has an empty Trash.
/// @param project_id The project whose Trash is listed.
/// @param type card, ai_message, or all; an unknown type lists nothing.
std::vector<TrashItem> list_trash(holder_context& context, std::string project_id, std::optional<std::string> type);

enum class EmptyTrashType { Card, AiMessage, All };

/// Permanently remove a project's trashed items.
/// An empty or unregistered project succeeds without changes. Old Git revisions of cards remain available.
/// @param project_id The project whose Trash is emptied.
/// @param type Which item kinds to remove.
/// @throws KeyringUnavailable, KeyMissing, DataCorrupted
TrashEmptyResult empty_trash(holder_context& context, std::string project_id, EmptyTrashType type);

enum class DeleteTrashItemType { Card, AiMessage };

/// Permanently remove a trashed item by its global ID (legacy route).
/// @throws NotFound, NotTrashed, KeyringUnavailable, KeyMissing, DataCorrupted
TrashDeleteResult delete_trash_item(holder_context& context, DeleteTrashItemType type, std::string id);

enum class DeleteProjectTrashItemType { Card, AiMessage };

/// Permanently remove a trashed item from the named project.
/// Old Git revisions of cards remain available.
/// @throws NotFound, NotTrashed, KeyringUnavailable, KeyMissing, DataCorrupted
TrashDeleteResult delete_project_trash_item(holder_context& context, std::string project_id, DeleteProjectTrashItemType type, std::string id);

/// Search a project's cards, ordered by relevance, best first.
/// @param project_id The project to search.
/// @param q An SQLite FTS5 query for card titles and bodies.
std::vector<SearchCard> search_cards(holder_context& context, std::string project_id, std::string q, std::int32_t limit, std::int32_t offset);

/// Search a project's AI messages, ordered by relevance, best first.
/// @param project_id The project to search.
/// @param q An SQLite FTS5 query for AI message content.
std::vector<SearchMessage> search_ai_messages(holder_context& context, std::string project_id, std::string q, std::int32_t limit, std::int32_t offset);

namespace detail {

/// Core supplies this: sets *out_error (when not null) to a new holder_error with message, and
/// returns code.
int fail(holder_error** out_error, int code, const std::string& message);

}  // namespace detail

}  // namespace holder::contract
