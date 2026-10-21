#include "api/History.h"

#include "api/generated/RuntimeJson.h"

#include "git/GitRepo.h"
#include "git/RevisionReferenceResolver.h"
#include "history/CardHistory.h"
#include "history/ProjectHistory.h"
#include "privacy/PrivacyError.h"
#include "project/ProjectRepo.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
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

std::optional<holder::history::ProjectHistoryObjectKind> history_kind(
    const std::optional<std::string>& kind
) {
  using Kind = holder::history::ProjectHistoryObjectKind;
  if (!kind) return std::nullopt;
  if (*kind == "card") return Kind::Card;
  if (*kind == "resource") return Kind::Resource;
  if (*kind == "location") return Kind::Location;
  if (*kind == "ai_data") return Kind::AiData;
  if (*kind == "project_settings") return Kind::ProjectSettings;
  return Kind::Unknown;
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

ProjectHistoryActivity activity_item(const holder::history::ProjectHistoryActivity& activity) {
  ProjectHistoryActivity item{
      activity.oid,
      activity.parent_oids,
      author(activity.author_name, activity.author_email),
      activity.authored_at,
      activity.committed_at,
      activity.message,
      {},
      activity.is_merge,
  };
  for (const auto& object : activity.affected_objects) {
    ProjectHistoryAffectedObject affected{
        holder::history::project_history_object_kind_name(object.kind), {}, {}};
    for (const auto& path : object.items) {
      // paths stays for existing consumers; items adds historical display details.
      affected.paths.push_back(path.path);
      affected.items.push_back({path.path, path.title, path.detail});
    }
    item.affected_objects.push_back(std::move(affected));
  }
  return item;
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

ProjectHistoryPageResponse get_project_history(holder::platform::Db& db,
                                               std::string project_id,
                                               int limit,
                                               std::optional<std::string> cursor,
                                               std::optional<std::string> kind) {
  return guarded([&] {
    const auto project = project_or_throw(db, project_id);
    const auto page = holder::history::ProjectHistoryService().list(
        project, static_cast<std::size_t>(limit), history_cursor(cursor), history_kind(kind));
    ProjectHistoryPageResponse response{
        true, {page.head_oid, {}, page.next_cursor, page.scan_limited}};
    for (const auto& activity : page.activities) {
      response.data.activities.push_back(activity_item(activity));
    }
    return bounded(std::move(response));
  });
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

} // namespace holder::api
