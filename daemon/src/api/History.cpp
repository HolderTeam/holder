#include "api/History.h"

#include "api/generated/RuntimeJson.h"
#include "api/support/Time.h"

#include "card/CardStore.h"
#include "git/GitRepo.h"
#include "git/RepoLocks.h"
#include "git/RevisionReferenceResolver.h"
#include "history/CardHistory.h"
#include "privacy/PrivacyError.h"
#include "project/ProjectRepo.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace holder::api {
namespace {

constexpr std::size_t kMaxHistoryResponseBytes = 2 * 1024 * 1024;

// Runs a history read, keeping its declared failures and reporting any other as unavailable.
template <typename Read>
auto guarded(Read read) {
  try {
    return read();
  } catch (const CodedError&) {
    throw;
  } catch (const std::invalid_argument&) {
    throw;
  } catch (const holder::privacy::PrivacyError& error) {
    if (history_key_unavailable(error)) {
      throw HistoryConflict("history_key_unavailable", error.what());
    }
    throw HistoryUnavailable(error.what());
  } catch (const std::exception& error) {
    throw HistoryUnavailable(error.what());
  }
}

template <typename Response>
Response bounded(Response response) {
  if (rig_json::encode(response).dump().size() > kMaxHistoryResponseBytes) {
    throw HistoryResponseTooLarge{};
  }
  return response;
}

holder::model::Project project_or_throw(holder::platform::Db& db, const std::string& project_id) {
  auto project = holder::project::ProjectRepo(db).get(project_id);
  if (!project) throw HistoryNotFound("not_found", "Project not found.");
  return std::move(*project);
}

std::optional<std::string> history_cursor(const std::optional<std::string>& cursor) {
  if (!cursor || cursor->empty()) return std::nullopt;
  const bool full_oid =
      cursor->size() == 40 && std::all_of(cursor->begin(), cursor->end(), [](unsigned char ch) {
        return std::isxdigit(ch) != 0;
      });
  if (!full_oid) throw std::invalid_argument("cursor must be a full commit OID");
  return cursor;
}

void require_resolved(const holder::git::RevisionReferenceResult& result,
                      const std::string& reference,
                      const std::string& parameter) {
  switch (result.status) {
  case holder::git::RevisionReferenceStatus::Resolved:
    return;
  case holder::git::RevisionReferenceStatus::Ambiguous:
    throw HistoryConflict("revision_ambiguous",
                          parameter + " revision reference is ambiguous: " + reference);
  case holder::git::RevisionReferenceStatus::NotFound:
    throw HistoryNotFound("revision_not_found",
                          parameter + " revision was not found: " + reference);
  }
  throw std::runtime_error("unknown revision reference status"); // LCOV_EXCL_LINE
}

std::string resolve(const holder::model::Project& project,
                    const std::string& reference,
                    const std::string& parameter) {
  holder::git::GitRepo repo;
  repo.open_existing(project.root_path);
  const auto result = holder::git::RevisionReferenceResolver(repo).resolve(reference);
  require_resolved(result, reference, parameter);
  return *result.oid;
}

CardHistoryAuthor author(const std::string& name, const std::string& email) {
  return {name, email};
}

CardHistoryVersion version_item(const holder::history::CardVersion& version) {
  return {version.exists, version.oid, version.title, version.body};
}

CardHistoryEntry entry_item(const holder::history::CardHistoryEntry& entry) {
  CardHistoryEntry item{
      entry.first_oid,
      entry.last_oid,
      entry.parent_oids,
      entry.visible_parent_oids,
      author(entry.author_name, entry.author_email),
      entry.started_at,
      entry.ended_at,
      entry.kind,
      entry.summary,
      static_cast<int>(entry.commit_count),
      entry.is_merge,
      {},
  };
  for (const auto& save : entry.saves) {
    item.saves.push_back(
        {save.oid, save.parent_oids, save.authored_at, save.committed_at, save.message});
  }
  return item;
}

std::optional<long long> line_number(long long value) {
  return value < 0 ? std::nullopt : std::optional<long long>(value);
}

} // namespace

bool history_key_unavailable(const holder::privacy::PrivacyError& error) {
  return error.code() == holder::privacy::PrivacyErrorCode::KeyMaterialMissing ||
         error.code() == holder::privacy::PrivacyErrorCode::KeyringUnavailable;
}

CardHistoryPageResponse get_card_history(holder::platform::Db& db,
                                         std::string project_id,
                                         std::string card_id,
                                         int limit,
                                         std::optional<std::string> cursor) {
  return guarded([&] {
    const auto project = project_or_throw(db, project_id);
    const auto page = holder::history::CardHistoryService().list(
        project, card_id, static_cast<std::size_t>(limit), history_cursor(cursor));
    CardHistoryPageResponse response{
        true, {page.head_oid, {}, page.next_cursor, page.scan_limited}};
    for (const auto& entry : page.entries) {
      response.data.entries.push_back(entry_item(entry));
    }
    return bounded(std::move(response));
  });
}

CardHistoryComparisonResponse compare_card_history(holder::platform::Db& db,
                                                   std::string project_id,
                                                   std::string card_id,
                                                   std::optional<RevisionReference> from,
                                                   RevisionReference to,
                                                   std::string mode) {
  return guarded([&] {
    const auto project = project_or_throw(db, project_id);
    if (mode == "since" && !from) {
      throw std::invalid_argument("from and to revision references are required.");
    }
    holder::history::CardHistoryService history;
    holder::history::CardHistoryComparison comparison;
    if (mode == "change") {
      auto change = history.compare_change(project, card_id, to.value);
      require_resolved(change.revision, to.value, "to");
      // LCOV_EXCL_START: a resolved comparison is an invariant of compare_change.
      if (!change.comparison) {
        throw std::runtime_error("resolved history change is missing its comparison");
      }
      // LCOV_EXCL_STOP
      comparison = std::move(*change.comparison);
    } else {
      const auto from_oid = resolve(project, from->value, "from");
      const auto to_oid = resolve(project, to.value, "to");
      comparison = history.compare(project, card_id, from_oid, to_oid);
    }
    CardHistoryComparisonResponse response{
        true,
        {version_item(comparison.from),
         version_item(comparison.to),
         comparison.summary,
         {},
         comparison.truncated}};
    for (const auto& line : comparison.lines) {
      response.data.lines.push_back({std::string(1, line.origin),
                                     line.text,
                                     line_number(line.old_line),
                                     line_number(line.new_line)});
    }
    return bounded(std::move(response));
  });
}

CardHistorySnapshotResponse get_card_history_snapshot(holder::platform::Db& db,
                                                      std::string project_id,
                                                      std::string card_id,
                                                      RevisionReference oid) {
  return guarded([&] {
    const auto project = project_or_throw(db, project_id);
    const auto resolved = resolve(project, oid.value, "oid");
    const auto snapshot =
        holder::history::CardHistoryService().compare(project, card_id, std::nullopt, resolved).to;
    return bounded(CardHistorySnapshotResponse{true, {card_id, version_item(snapshot)}});
  });
}

CardHistoryRestoreResponse post_projects_project_id_history_cards_card_id_restore(
    holder::platform::Db& db,
    holder::card::CardStore* card_store,
    std::string project_id,
    std::string card_id,
    RevisionReference oid) {
  return guarded([&] {
    const auto project = project_or_throw(db, project_id);
    if (card_store == nullptr) throw CardStoreUnavailable{};
    // Resolve and restore under the repository lock, so the revision cannot move in between.
    const auto repo_mutex = holder::git::repo_mutex_for(project.root_path);
    const std::lock_guard<std::recursive_mutex> repo_guard(*repo_mutex);
    const auto resolved = resolve(project, oid.value, "oid");
    const auto card = card_store->get(card_id);
    if (!card || card->project_id != project.project_id)
      throw HistoryNotFound("not_found", "Card not found.");
    card_store->restore_version(card_id, resolved, support::now_epoch_seconds());
    const auto restored = card_store->get(card_id);
    if (!restored) throw std::runtime_error("restored card is missing"); // LCOV_EXCL_LINE
    holder::git::GitRepo repo;
    repo.open_existing(project.root_path);
    const auto result_oid = repo.head_oid();
    if (!result_oid) throw std::runtime_error("restored revision is missing"); // LCOV_EXCL_LINE
    return CardHistoryRestoreResponse{
        true,
        {std::move(card_id), resolved, *result_oid, restored->title, restored->updated_at,
         restored->deleted_at}};
  });
}

} // namespace holder::api
