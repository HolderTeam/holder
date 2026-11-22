// The contract's Git sync operations (contract/interface/git.py). The generated glue checks their
// arguments first; these do the work.
#include "api/Contract.h"

#include "api/CardAccess.h"
#include "api/CoreErrors.h"

#include "capi/Context.h"
#include "card/CardHierarchy.h"
#include "card/CardPaths.h"
#include "card/CardRepo.h"
#include "card/LinkRepo.h"
#include "git/GitOps.h"
#include "git/GitRepo.h"
#include "git/PushResult.h"
#include "git/RemoteProbe.h"
#include "history/CardHistory.h"
#include "model/ProjectSyncState.h"
#include "privacy/ProjectPrivacy.h"
#include "project/ProjectRepo.h"
#include "project/ProjectSyncRepo.h"
#include "resource/AssetEnvelope.h"
#include "sync/ProjectSyncOperation.h"
#include "sync/ProjectSyncPolicy.h"

#include <git2.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <filesystem>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>

namespace holder::contract {

void configure_git_home(std::string path) {
  if (path.empty()) throw InvalidArgument("path must not be empty");

  // libgit2 derives its default home on the first 0->1 init transition. Hold one reference
  // for the process so later GitRepo constructions cannot reset this override.
  git_libgit2_init();
  if (git_libgit2_opts(GIT_OPT_SET_HOMEDIR, path.c_str()) != 0) {
    const git_error* error = git_error_last();
    throw std::runtime_error(
        std::string("git_libgit2_opts(GIT_OPT_SET_HOMEDIR) failed: ") +
        (error && error->message ? error->message : "unknown error")
    );
  }
}

namespace {

holder::model::Project find(holder_context& context, const std::string& project_id) {
  auto project = holder::project::ProjectRepo(context.db).get(project_id);
  if (!project) throw NotFound("project not found: " + project_id);
  return std::move(*project);
}

// The host's Git, or one of core's own with the context's credentials.
holder::git::GitOps& git_for(holder_context& context, std::optional<holder::git::RealGitOps>& own) {
  if (context.git != nullptr) return *context.git;
  auto& git = own.emplace();
  if (context.credential_provider) git.set_credential_provider(context.credential_provider);
  return git;
}

std::optional<std::string> nonempty(const std::optional<std::string>& value) {
  return value && !value->empty() ? value : std::nullopt;
}

PullPhaseStatus pull_status(holder::sync::PullPhaseStatus status) {
  switch (status) {
  case holder::sync::PullPhaseStatus::NotAttempted:
    return PullPhaseStatus::NotAttempted;
  case holder::sync::PullPhaseStatus::Succeeded:
    return PullPhaseStatus::Succeeded;
  case holder::sync::PullPhaseStatus::RemoteUnset:
    return PullPhaseStatus::RemoteUnset;
  case holder::sync::PullPhaseStatus::Failed:
    break;
  }
  return PullPhaseStatus::Failed;
}

PushPhaseStatus push_status(holder::git::PushStatus status) {
  using Status = holder::git::PushStatus;
  switch (status) {
  case Status::Pushed:
    return PushPhaseStatus::Pushed;
  case Status::UpToDate:
    return PushPhaseStatus::UpToDate;
  case Status::AuthFailed:
    return PushPhaseStatus::AuthFailed;
  case Status::NotFound:
    return PushPhaseStatus::NotFound;
  case Status::NetworkError:
    return PushPhaseStatus::NetworkError;
  case Status::NonFastForward:
    return PushPhaseStatus::NonFastForward;
  case Status::RemoteUnset:
    return PushPhaseStatus::RemoteUnset;
  case Status::UnknownError:
    break;
  }
  return PushPhaseStatus::UnknownError;
}

RemoteTestResultStatus probe_status(holder::git::RemoteProbeStatus status) {
  using Status = holder::git::RemoteProbeStatus;
  switch (status) {
  case Status::Reachable:
    return RemoteTestResultStatus::Reachable;
  case Status::AuthFailed:
    return RemoteTestResultStatus::AuthFailed;
  case Status::NotFound:
    return RemoteTestResultStatus::NotFound;
  case Status::NetworkError:
    return RemoteTestResultStatus::NetworkError;
  case Status::InvalidRemoteUrl:
    return RemoteTestResultStatus::InvalidRemoteUrl;
  case Status::RemoteUnset:
    return RemoteTestResultStatus::RemoteUnset;
  case Status::UnknownError:
    break;
  }
  return RemoteTestResultStatus::UnknownError;
}

PullChanges convert_pull_changes(const holder::model::PullChanges& changes) {
  return {
      changes.merged_card_ids,
      changes.kept_out_of_trash_card_ids,
      changes.restored_from_trash_card_ids,
      changes.conflict_copy_ids,
      changes.hidden_cards_repaired
  };
}

SyncState state(holder_context& context, const std::string& project_id) {
  find(context, project_id);
  const auto sync = holder::project::ProjectSyncRepo(context.db)
                        .get(project_id)
                        .value_or(holder::model::ProjectSyncState{});
  holder::card::CardRepo cards(context.db);
  std::vector<ConflictCopy> copies;
  for (const auto& link : holder::card::LinkRepo(context.db).list_conflict_copy_links(project_id)) {
    const auto copy = cards.get(link.from_card_id);
    if (!copy || copy->project_id != project_id || copy->deleted_at) continue;
    std::optional<CardSummary> original;
    if (const auto card = cards.get(link.to_card_id); card && card->project_id == project_id) {
      original = CardSummary{card->card_id, card->title, card->deleted_at};
    }
    copies.push_back(
        {link.to_card_id, std::move(original), {copy->card_id, copy->title, copy->deleted_at}}
    );
  }
  return {
      project_id,
      sync.last_commit_at,
      sync.last_push_at,
      sync.last_pull_at,
      sync.uncommitted_changes_count,
      sync.unpushed_commits_count,
      sync.last_push_status,
      sync.last_pull_status,
      sync.last_sync_error,
      sync.last_sync_error_at,
      sync.retry_count,
      sync.next_retry_at,
      sync.pull_retry_count,
      sync.next_pull_retry_at,
      sync.updated_at > 0 ? std::optional<long long>(sync.updated_at) : std::nullopt,
      sync.last_pull_changes ? std::optional(LastPullChanges{
                                   convert_pull_changes(sync.last_pull_changes->changes),
                                   sync.last_pull_changes->pulled_at
                               })
                             : std::nullopt,
      copies.empty() ? std::nullopt : std::optional(std::move(copies))
  };
}

SyncResult convert_sync_result(const holder::sync::ProjectSyncResult& result) {
  return {
      result.project_id,
      result.succeeded() ? SyncResultStatus::Succeeded : SyncResultStatus::Failed,
      nonempty(result.remote_url),
      {result.pull.attempted,
       pull_status(result.pull.status),
       result.pull.conflicts_resolved,
       result.pull.hidden_cards_repaired,
       convert_pull_changes(result.pull.changes),
       result.pull.error_message},
      {result.push.attempted,
       result.push.attempted ? push_status(result.push.status) : PushPhaseStatus::NotAttempted,
       result.push.ahead_count,
       result.push.behind_count,
       nonempty(result.push.local_head_commit),
       result.push.error_message}
  };
}

DueSyncResult convert_due_result(const holder::sync::ProjectSyncResult& result) {
  auto converted = convert_sync_result(result);
  return {
      std::move(converted.project_id),
      std::move(converted.remote_url),
      std::move(converted.pull),
      std::move(converted.push)
  };
}

// Pulls, pushes or both through core's one sync operation, which records the outcome.
SyncResult run(
    holder_context& context,
    const std::string& project_id,
    bool pull,
    bool push,
    const PushOptions& options
) {
  const auto project = find(context, project_id);
  // An encrypted project with plaintext in it is refused before any Git work. The sync operation
  // checks again before it pushes, and would only record a failure.
  if (push && project.privacy_mode == "encrypted_git")
    holder::privacy::assert_encryption_push_safe(project.root_path);
  std::optional<holder::git::RealGitOps> own;
  const auto result = holder::sync::run_project_sync(
      context.db,
      &context.fts,
      git_for(context, own),
      project_id,
      {.pull = pull,
       .push = push,
       .push_after_failed_pull = false,
       .branch = options.branch.value_or(""),
       .set_upstream = options.set_upstream,
       .now = now_epoch_seconds()}
  );
  return convert_sync_result(result);
}

DueSyncResult run_if_due(
    holder_context& context,
    const std::string& project_id,
    std::int32_t push_interval_seconds,
    std::int32_t pull_interval_seconds
) {
  const auto project = find(context, project_id);
  holder::sync::ProjectSyncResult skipped;
  skipped.project_id = project_id;
  skipped.remote_url = project.git_remote_url;
  if (!project.git_remote_url || project.git_remote_url->empty())
    return convert_due_result(skipped);

  const auto now = now_epoch_seconds();
  const auto state = holder::project::ProjectSyncRepo(context.db).get(project_id);
  holder::sync::PullDecisionInput pull_input{
      .last_pull_at = state ? state->last_pull_at : std::optional<long long>{},
      .next_pull_retry_at = state ? state->next_pull_retry_at : std::optional<long long>{},
      .now = now,
  };
  if (pull_interval_seconds > 0) pull_input.pull_interval_seconds = pull_interval_seconds;
  const bool pull_due = holder::sync::should_attempt_pull(pull_input);
  holder::sync::PushDecisionInput push_input{
      .last_push_at = state ? state->last_push_at : std::optional<long long>{},
      .next_retry_at = state ? state->next_retry_at : std::optional<long long>{},
      .now = now,
  };
  if (push_interval_seconds > 0) push_input.push_interval_seconds = push_interval_seconds;
  const bool push_due = holder::sync::should_attempt_push(push_input);
  if (!pull_due && !push_due) return convert_due_result(skipped);

  std::optional<holder::git::RealGitOps> own;
  const auto result = holder::sync::run_project_sync(
      context.db,
      &context.fts,
      git_for(context, own),
      project_id,
      {.pull = pull_due,
       .push = push_due,
       .push_after_failed_pull = true,
       .branch = "", // LCOV_EXCL_LINE - GCC cleanup-only counter; normal expression is measured on
                     // adjacent lines.
       .set_upstream = true,
       .now = now}
  );
  return convert_due_result(result);
}

RemoteTestResult test_remote(
    holder_context& context,
    const std::string& project_id,
    const RemoteTest& request
) {
  std::optional<holder::git::RealGitOps> own;
  auto& git = git_for(context, own);
  auto operation = git.lock_operation(find(context, project_id).root_path);
  // Read again under the lock, so the remote is the one stored now.
  const auto remote_url = request.remote_url ? request.remote_url
                                             : nonempty(find(context, project_id).git_remote_url);
  holder::git::RemoteProbeResult probe{
      holder::git::RemoteProbeStatus::RemoteUnset,
      false,
      "The project has no Git remote." // LCOV_EXCL_LINE - GCC cleanup-only counter
  };
  if (remote_url) probe = git.probe_remote_url(*remote_url);
  return {
      project_id,
      probe_status(probe.status),
      remote_url,
      probe.remote_has_head,
      probe.status == holder::git::RemoteProbeStatus::Reachable ? std::nullopt
                                                                : nonempty(probe.error_message)
  };
}

ConflictComparison read_conflict_pair(
    holder_context& context,
    const holder::model::Project& project,
    const std::string& copy_id
) {
  const auto links =
      holder::card::LinkRepo(context.db).list_conflict_copy_links(project.project_id);
  const auto link = std::ranges::find(links, copy_id, &holder::model::CardLink::from_card_id);
  if (link == links.end()) throw NotFound("card is not a preserved conflict copy: " + copy_id);
  const auto root = std::filesystem::path(project.root_path);
  const auto card_digest = [&](const CardMetadata& card) {
    const auto rel_path = holder::core::card_rel_path(card.card_id);
    if (card.rel_path != rel_path || !std::filesystem::is_regular_file(root / rel_path))
      throw NotFound("card content missing: " + card.card_id);
    return holder::resource::digest_file(root / rel_path).sha256;
  };
  // The bytes contain the stable IDs, text, placement and durable links and milestones.
  // Unrelated commits do not invalidate a reviewed pair.
  auto token = card_digest(get_card_metadata(context, project.project_id, link->to_card_id)) + ":" +
               card_digest(get_card_metadata(context, project.project_id, copy_id));
  auto original = get_card(context, project.project_id, link->to_card_id);
  auto copy = get_card(context, project.project_id, copy_id);
  if (token != card_digest(original) + ":" + card_digest(copy))
    throw Conflict("The conflict pair changed while being read. Review both versions again.");
  ConflictComparison out{};
  out.original = std::move(original);
  out.copy_card = std::move(copy);
  out.review_token = std::move(token);
  return out;
}

// Both edited-on enums list the same three answers in the same order.
template <class EditedOn> EditedOn edited_on(holder::git::ChangeOrigin origin) {
  switch (origin) {
  case holder::git::ChangeOrigin::ThisDevice:
    return EditedOn::ThisDevice;
  case holder::git::ChangeOrigin::AnotherDevice:
    return EditedOn::AnotherDevice;
  case holder::git::ChangeOrigin::Unknown:
  default:
    return EditedOn::Unknown;
  }
}

bool same_link(const Link& original, const Link& copy) {
  auto target = copy.target_id;
  if (copy.target_type == LinkTargetType::Card &&
      copy.target_project_id == copy.source_project_id && copy.target_id == copy.source_id)
    target = original.source_id;
  return original.target_project_id == copy.target_project_id &&
         original.target_type == copy.target_type && original.target_id == target &&
         original.kind == copy.kind && original.label == copy.label;
}

bool same_milestone(const Milestone& original, const Milestone& copy) {
  return original.kind == copy.kind && original.description == copy.description &&
         std::ranges::is_permutation(original.dates, copy.dates, [](const auto& a, const auto& b) {
           return std::tie(a.role, a.date, a.datetime) == std::tie(b.role, b.date, b.datetime);
         });
}

ConflictCopyCleanupReview cleanup_review(holder_context& context, const ConflictComparison& pair) {
  const auto& project_id = pair.original.project_id;
  const auto& original_id = pair.original.card_id;
  const auto& copy_id = pair.copy_card.card_id;
  ConflictCopyCleanupReview out{};

  const auto original_links = list_card_links(context, project_id, original_id, true);
  for (auto& link : list_card_links(context, project_id, copy_id, true)) {
    if (link.kind != holder::model::kConflictCopyLinkKind &&
        !std::ranges::any_of(original_links, [&](const auto& other) {
          return same_link(other, link);
        }))
      out.copy_links.push_back(std::move(link));
  }
  auto original_milestones = list_card_milestones(context, project_id, original_id);
  for (auto& milestone : list_card_milestones(context, project_id, copy_id)) {
    const auto equivalent = std::ranges::find_if(original_milestones, [&](const auto& other) {
      return same_milestone(other, milestone);
    });
    if (equivalent == original_milestones.end())
      out.copy_milestones.push_back(std::move(milestone));
    else
      original_milestones.erase(equivalent);
  }
  for (auto& link : list_card_backlinks(
           context,
           project_id,
           copy_id,
           ListCardBacklinksScope::AllAvailable,
           false
       )) {
    if (link.source_project_id != project_id || link.source_id != copy_id)
      out.backlinks.push_back(std::move(link));
  }
  for (const auto& tag : pair.copy_card.tags) {
    if (std::ranges::find(pair.original.tags, tag) == pair.original.tags.end())
      out.copy_tags.push_back(tag);
  }

  holder::card::CardRepo cards(context.db);
  auto store = card_store(context);
  const auto copy_card = live_card(store, project_id, copy_id);
  const auto promotion_parent_id = holder::card::reachable_card_parent(cards, copy_card);
  if (promotion_parent_id) {
    const auto parent = live_card(store, project_id, *promotion_parent_id);
    out.promotion_parent = CardSummary{parent.card_id, parent.title, parent.deleted_at};
  }
  for (const auto& child : cards.list_children(project_id, copy_id)) {
    if (!child.deleted_at) out.children.push_back({child.card_id, child.title, child.deleted_at});
  }
  out.placement_differs = pair.original.parent_card_id != pair.copy_card.parent_card_id ||
                          pair.original.sort_key != pair.copy_card.sort_key;
  const auto placements = holder::card::promoted_card_children(cards, copy_card, 0);
  out.preserves_original = std::ranges::none_of(placements, [&](const auto& changed) {
    return changed.card_id == original_id;
  });

  // The pair's file digests already cover outgoing links, milestones, placement and tags.
  // Also guard the backlinks and the placement changes that Trash would apply to other cards.
  using nlohmann::json;
  auto effects = json::array();
  effects.push_back({"conflict-copy-cleanup", project_id, original_id, copy_id, pair.review_token});
  if (out.promotion_parent)
    effects.push_back({out.promotion_parent->card_id, out.promotion_parent->title});
  for (const auto& changed : placements) {
    effects.push_back({changed.card_id, changed.title, changed.sort_key});
  }
  for (const auto& link : out.backlinks) {
    effects.push_back(
        {link.source_project_id,
         link.source_type,
         link.source_id,
         link.kind,
         link.label ? json(*link.label) : json(nullptr),
         link.created_at}
    );
  }
  out.cleanup_token = holder::resource::digest_bytes(effects.dump());
  return out;
}

// The four link target enums list the same types in the same order.
template <class To, class From> To link_type(From type) {
  switch (type) {
  case From::Card:
    return To::Card;
  case From::AiMessage:
    return To::AiMessage;
  case From::AiThread:
    return To::AiThread;
  case From::Resource:
  default:
    return To::Resource;
  }
}

template <class Items, class Match> const auto& reviewed(const Items& items, Match match) {
  const auto found = std::ranges::find_if(items, match);
  if (found == items.end())
    throw Conflict("A detail to carry over is not in the reviewed cleanup. Review it again.");
  return *found;
}

// Carries the chosen copy-only details over to the original before the copy goes to Trash. Every
// choice is checked against the review first, so a bad one changes nothing.
void carry_over(
    holder_context& context,
    const std::string& project_id,
    const std::string& original_id,
    const std::string& copy_id,
    const ConflictCopyCleanupReview& cleanup,
    const TrashConflictCopyRequest& request
) {
  const auto links = request.carry_links.value_or(std::vector<ConflictCopyLinkTarget>{});
  const auto milestones = request.carry_milestones.value_or(
      std::vector<ConflictCopyMilestoneChoice>{}
  );
  const auto tags = request.carry_tags.value_or(std::vector<ConflictCopyTagChoice>{});
  const auto children = request.adopt_children.value_or(std::vector<ConflictCopyChildChoice>{});
  const auto backlinks = request.retarget_backlinks.value_or(
      std::vector<ConflictCopyBacklinkSource>{}
  );

  std::vector<NewLink> new_links;
  for (const auto& choice : links) {
    const auto& link = reviewed(cleanup.copy_links, [&](const Link& item) {
      return item.target_project_id == choice.target_project_id &&
             item.target_type == link_type<LinkTargetType>(choice.target_type) &&
             item.target_id == choice.target_id && item.kind == choice.kind;
    });
    NewLink made;
    made.target_type = link_type<NewLinkTargetType>(link.target_type);
    made.target_id = link.target_id;
    if (link.target_project_id != project_id) made.target_project_id = link.target_project_id;
    made.kind = link.kind;
    made.label = link.label;
    new_links.push_back(std::move(made));
  }
  std::vector<NewMilestone> new_milestones;
  for (const auto& choice : milestones) {
    const auto& milestone = reviewed(cleanup.copy_milestones, [&](const Milestone& item) {
      return item.milestone_id == choice.milestone_id;
    });
    new_milestones.push_back({milestone.dates, milestone.kind, milestone.description});
  }
  for (const auto& choice : tags)
    reviewed(cleanup.copy_tags, [&](const std::string& item) {
      return item == choice.tag;
    });
  for (const auto& choice : children)
    reviewed(cleanup.children, [&](const CardSummary& item) {
      return item.card_id == choice.card_id;
    });
  std::vector<Link> moved_backlinks;
  for (const auto& choice : backlinks) {
    moved_backlinks.push_back(reviewed(cleanup.backlinks, [&](const Link& item) {
      return item.source_type == LinkSourceType::Card &&
             item.source_project_id == choice.source_project_id &&
             item.source_id == choice.source_id && item.kind == choice.kind;
    }));
  }

  for (const auto& link : new_links)
    add_card_link(context, project_id, original_id, link);
  for (const auto& milestone : new_milestones)
    add_card_milestone(context, project_id, original_id, milestone);
  for (const auto& choice : tags)
    add_card_tag(context, project_id, original_id, choice.tag);
  for (const auto& choice : children) {
    CardMove into;
    into.intent = CardMoveIntent::Into;
    into.target_card_id = original_id;
    move_card(context, project_id, choice.card_id, into);
  }
  for (const auto& link : moved_backlinks) {
    NewLink to_original;
    to_original.target_type = NewLinkTargetType::Card;
    to_original.target_id = original_id;
    if (link.source_project_id != project_id) to_original.target_project_id = project_id;
    to_original.kind = link.kind;
    to_original.label = link.label;
    add_card_link(context, link.source_project_id, link.source_id, to_original);
    remove_card_link(
        context,
        link.source_project_id,
        link.source_id,
        RemoveCardLinkTargetType::Card,
        copy_id,
        link.kind,
        link.source_project_id == project_id ? std::nullopt : std::optional<std::string>{project_id}
    );
  }
}

} // namespace

SyncState get_project_sync_state(holder_context& context, std::string project_id) {
  return with_contract_errors([&] {
    return state(context, project_id);
  });
}

ConflictComparison compare_conflict_copy(
    holder_context& context,
    std::string project_id,
    std::string copy_id
) {
  return with_contract_errors([&] {
    const auto project = find(context, project_id);
    const auto mutex = holder::git::repo_mutex_for(project.root_path);
    std::unique_lock lock(*mutex);
    auto result = read_conflict_pair(context, project, copy_id);
    holder::git::GitRepo repo;
    repo.open_existing(project.root_path);
    result.original_edited_on = edited_on<ConflictComparisonOriginalEditedOn>(
        repo.latest_change_origin(holder::core::card_rel_path(result.original.card_id))
    );
    result.copy_edited_on = edited_on<ConflictComparisonCopyEditedOn>(
        repo.latest_change_origin(holder::core::card_rel_path(result.copy_card.card_id))
    );
    lock.unlock();
    if (result.original.content == result.copy_card.content) return result;
    const auto lines = holder::history::compare_card_text(
        result.original.content,
        result.copy_card.content,
        result.truncated
    );
    for (const auto& line : lines) {
      if (line.origin == '-') {
        result.original_changed_lines.push_back({static_cast<std::int32_t>(line.old_line)});
      } else if (line.origin == '+') {
        result.copy_changed_lines.push_back({static_cast<std::int32_t>(line.new_line)});
      }
    }
    return result;
  });
}

ConflictResolutionResult resolve_conflict_copy(
    holder_context& context,
    std::string project_id,
    std::string copy_id,
    const ConflictResolution& resolution
) {
  return with_contract_errors([&] {
    const auto project = find(context, project_id);
    const auto mutex = holder::git::repo_mutex_for(project.root_path);
    std::unique_lock lock(*mutex);
    const auto reviewed = read_conflict_pair(context, project, copy_id);
    if (resolution.review_token != reviewed.review_token)
      throw Conflict(
          "Either the original or the preserved copy changed. Review both versions again."
      );
    card_store(context).save_conflict_resolution(
        project_id,
        reviewed.original.card_id,
        resolution.title,
        resolution.content,
        now_epoch_seconds()
    );
    auto saved = read_conflict_pair(context, project, copy_id);
    auto cleanup = cleanup_review(context, saved);
    return ConflictResolutionResult{std::move(saved.original), std::move(cleanup)};
  });
}

Card separate_conflict_copy(
    holder_context& context,
    std::string project_id,
    std::string copy_id,
    const SeparateConflictCopyRequest& request
) {
  return with_contract_errors([&] {
    const auto project = find(context, project_id);
    const auto mutex = holder::git::repo_mutex_for(project.root_path);
    std::unique_lock lock(*mutex);
    const auto reviewed = read_conflict_pair(context, project, copy_id);
    if (request.review_token != reviewed.review_token)
      throw Conflict(
          "Either the original or the preserved copy changed. Review both versions again."
      );
    card_store(context)
        .separate_conflict_copy(project_id, copy_id, request.title, now_epoch_seconds());
    return get_card(context, project_id, copy_id);
  });
}

CardMetadata trash_conflict_copy(
    holder_context& context,
    std::string project_id,
    std::string copy_id,
    const TrashConflictCopyRequest& confirmation
) {
  return with_contract_errors([&] {
    const auto project = find(context, project_id);
    const auto mutex = holder::git::repo_mutex_for(project.root_path);
    std::unique_lock lock(*mutex);
    const auto pair = read_conflict_pair(context, project, copy_id);
    const auto cleanup = cleanup_review(context, pair);
    if (cleanup.cleanup_token != confirmation.cleanup_token)
      throw Conflict(
          "The saved result, preserved copy or cleanup details changed. Review them again."
      );
    if (!cleanup.preserves_original)
      throw Conflict(
          "Moving the copy to Trash would also move the original. Move the original first."
      );
    carry_over(context, project_id, pair.original.card_id, copy_id, cleanup, confirmation);
    card_store(context).trash(copy_id, now_epoch_seconds());
    return get_card_any_state(context, project_id, copy_id);
  });
}

SyncResult pull_project(holder_context& context, std::string project_id) {
  return with_contract_errors([&] {
    return run(context, project_id, true, false, PushOptions{});
  });
}

SyncResult push_project(
    holder_context& context,
    std::string project_id,
    const PushOptions& options
) {
  return with_contract_errors([&] {
    return run(context, project_id, false, true, options);
  });
}

SyncResult sync_project(
    holder_context& context,
    std::string project_id,
    const PushOptions& options
) {
  return with_contract_errors([&] {
    return run(context, project_id, true, true, options);
  });
}

DueSyncResult sync_project_if_due(
    holder_context& context,
    std::string project_id,
    std::int32_t push_interval_seconds,
    std::int32_t pull_interval_seconds
) {
  return with_contract_errors([&] {
    return run_if_due(context, project_id, push_interval_seconds, pull_interval_seconds);
  });
}

RemoteTestResult test_project_remote(
    holder_context& context,
    std::string project_id,
    const RemoteTest& remote
) {
  return with_contract_errors([&] {
    return test_remote(context, project_id, remote);
  });
}

} // namespace holder::contract
