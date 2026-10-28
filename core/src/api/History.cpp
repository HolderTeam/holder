// Project activity history served through the shared contract.
#include "api/Contract.h"
#include "api/CardAccess.h"

#include "capi/Context.h"
#include "git/GitRepo.h"
#include "git/RepoLocks.h"
#include "git/RevisionReferenceResolver.h"
#include "history/CardHistory.h"
#include "history/ProjectHistory.h"
#include "privacy/PrivacyError.h"
#include "project/ProjectRepo.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <exception>
#include <mutex>
#include <optional>
#include <string>
#include <utility>

namespace holder::contract {
namespace {

constexpr std::size_t max_history_response_bytes = 2 * 1024 * 1024;

holder::history::ProjectHistoryObjectKind to_core(ListProjectHistoryKind kind) {
  using Kind = holder::history::ProjectHistoryObjectKind;
  switch (kind) {
  case ListProjectHistoryKind::Card: return Kind::Card;
  case ListProjectHistoryKind::Resource: return Kind::Resource;
  case ListProjectHistoryKind::Location: return Kind::Location;
  case ListProjectHistoryKind::AiData: return Kind::AiData;
  case ListProjectHistoryKind::ProjectSettings: return Kind::ProjectSettings;
  case ListProjectHistoryKind::Unknown: return Kind::Unknown;
  }
  throw InvalidArgument("unknown history object kind");
}

HistoryAffectedObjectKind to_contract(holder::history::ProjectHistoryObjectKind kind) {
  using Kind = holder::history::ProjectHistoryObjectKind;
  switch (kind) {
  case Kind::Card: return HistoryAffectedObjectKind::Card;
  case Kind::Resource: return HistoryAffectedObjectKind::Resource;
  case Kind::Location: return HistoryAffectedObjectKind::Location;
  case Kind::AiData: return HistoryAffectedObjectKind::AiData;
  case Kind::ProjectSettings: return HistoryAffectedObjectKind::ProjectSettings;
  case Kind::Unknown: return HistoryAffectedObjectKind::Unknown;
  }
  throw HistoryUnavailable("unknown history object kind");
}

const char* kind_name(HistoryAffectedObjectKind kind) {
  switch (kind) {
  case HistoryAffectedObjectKind::Card: return "card";
  case HistoryAffectedObjectKind::Resource: return "resource";
  case HistoryAffectedObjectKind::Location: return "location";
  case HistoryAffectedObjectKind::AiData: return "ai_data";
  case HistoryAffectedObjectKind::ProjectSettings: return "project_settings";
  case HistoryAffectedObjectKind::Unknown: return "unknown";
  }
  throw HistoryUnavailable("unknown history object kind");
}

void check_cursor(const std::optional<std::string>& cursor) {
  if (!cursor || cursor->empty()) return;
  if (cursor->size() != 40 || !std::ranges::all_of(*cursor, [](unsigned char ch) { return std::isxdigit(ch) != 0; }))
    throw InvalidArgument("cursor must be a full commit OID");
}

ProjectHistoryPage read(holder_context& context, const std::string& project_id, std::int32_t limit,
                        const std::optional<std::string>& cursor, std::optional<ListProjectHistoryKind> kind) {
  const auto project = holder::project::ProjectRepo(context.db).get(project_id);
  if (!project) throw NotFound("Project not found.");
  check_cursor(cursor);
  const auto page = holder::history::ProjectHistoryService().list(
      *project, static_cast<std::size_t>(limit), cursor && !cursor->empty() ? cursor : std::nullopt,
      kind ? std::optional(to_core(*kind)) : std::nullopt);
  ProjectHistoryPage out{page.head_oid, {}, page.next_cursor, page.scan_limited};
  for (const auto& activity : page.activities) {
    ProjectActivity item{activity.oid, activity.parent_oids,
                         {activity.author_name, activity.author_email}, activity.authored_at,
                         activity.committed_at, activity.message, {}, activity.is_merge};
    for (const auto& object : activity.affected_objects) {
      HistoryAffectedObject affected{to_contract(object.kind), {}, {}};
      for (const auto& path : object.items) {
        affected.paths.push_back(path.path);
        affected.items.push_back({path.path, path.title, path.detail});
      }
      item.affected_objects.push_back(std::move(affected));
    }
    out.activities.push_back(std::move(item));
  }
  return out;
}

// Apply the same 2 MiB bound to the complete HTTP response as the old route.
std::size_t response_size(const ProjectHistoryPage& page) {
  const auto nullable = [](const auto& value) -> nlohmann::json {
    return value ? nlohmann::json(*value) : nlohmann::json(nullptr);
  };
  nlohmann::json activities = nlohmann::json::array();
  for (const auto& activity : page.activities) {
    nlohmann::json affected = nlohmann::json::array();
    for (const auto& object : activity.affected_objects) {
      nlohmann::json items = nlohmann::json::array();
      for (const auto& path : object.items) {
        nlohmann::json item = {{"path", path.path}};
        if (path.title) item["title"] = *path.title;
        if (path.detail) item["detail"] = *path.detail;
        items.push_back(std::move(item));
      }
      affected.push_back({{"kind", kind_name(object.kind)},
                          {"paths", object.paths}, {"items", std::move(items)}});
    }
    activities.push_back({{"oid", activity.oid}, {"parent_oids", activity.parent_oids},
                          {"author", {{"name", activity.author.name}, {"email", activity.author.email}}},
                          {"authored_at", activity.authored_at}, {"committed_at", activity.committed_at},
                          {"message", activity.message}, {"affected_objects", std::move(affected)},
                          {"is_merge", activity.is_merge}});
  }
  return nlohmann::json({{"ok", true}, {"data", {{"head_oid", nullable(page.head_oid)},
                                                  {"activities", std::move(activities)},
                                                  {"next_cursor", nullable(page.next_cursor)},
                                                  {"scan_limited", page.scan_limited}}}}).dump().size();
}

} // namespace

ProjectHistoryPage list_project_history(holder_context& context, std::string project_id, std::int32_t limit,
                                        std::optional<std::string> cursor, std::optional<ListProjectHistoryKind> kind) {
  try {
    auto page = read(context, project_id, limit, cursor, kind);
    if (response_size(page) > max_history_response_bytes)
      throw HistoryResponseTooLarge("History response exceeds the 2 MiB limit.");
    return page;
  } catch (const Error&) {
    throw;
  } catch (const std::invalid_argument& error) {
    throw InvalidArgument(error.what());
  } catch (const holder::privacy::PrivacyError& error) {
    using Code = holder::privacy::PrivacyErrorCode;
    if (error.code() == Code::KeyMaterialMissing || error.code() == Code::KeyringUnavailable)
      throw HistoryKeyUnavailable(error.what());
    throw HistoryUnavailable(error.what());
  } catch (const std::exception& error) {
    throw HistoryUnavailable(error.what());
  }
}

namespace {

template <class Operation>
auto card_history_guard(Operation operation) {
  try {
    return operation();
  } catch (const Error&) {
    throw;
  } catch (const std::invalid_argument& error) {
    throw InvalidArgument(error.what());
  } catch (const holder::privacy::PrivacyError& error) {
    using Code = holder::privacy::PrivacyErrorCode;
    if (error.code() == Code::KeyMaterialMissing || error.code() == Code::KeyringUnavailable)
      throw HistoryKeyUnavailable(error.what());
    throw HistoryUnavailable(error.what());
  } catch (const std::exception& error) {
    throw HistoryUnavailable(error.what());
  }
}

holder::model::Project history_project(holder_context& context, const std::string& id) {
  auto project = holder::project::ProjectRepo(context.db).get(id);
  if (!project) throw NotFound("Project not found.");
  return std::move(*project);
}

std::string history_revision(const holder::model::Project& project, const std::string& reference,
                             const std::string& parameter) {
  if (reference.size() < 8 || reference.size() > 40 ||
      !std::ranges::all_of(reference, [](unsigned char ch) { return std::isxdigit(ch) != 0; }))
    throw InvalidArgument(parameter + " must be an 8-40 character hexadecimal revision reference");
  holder::git::GitRepo repo;
  repo.open_existing(project.root_path);
  const auto result = holder::git::RevisionReferenceResolver(repo).resolve(reference);
  switch (result.status) {
  case holder::git::RevisionReferenceStatus::Resolved: return *result.oid;
  case holder::git::RevisionReferenceStatus::Ambiguous:
    throw RevisionAmbiguous(parameter + " revision reference is ambiguous: " + reference);
  case holder::git::RevisionReferenceStatus::NotFound:
    throw NotFound(parameter + " revision was not found: " + reference);
  }
  throw HistoryUnavailable("unknown revision reference status");
}

CardHistoryVersion history_version(const holder::history::CardVersion& value) {
  return {value.exists, value.oid, value.title, value.body};
}

nlohmann::json history_json(const CardHistoryVersion& value) {
  return {{"exists", value.exists}, {"oid", value.oid}, {"title", value.title}, {"body", value.body}};
}

void bound_card_history(const nlohmann::json& data) {
  if (nlohmann::json({{"ok", true}, {"data", data}}).dump().size() > max_history_response_bytes)
    throw HistoryResponseTooLarge("History response exceeds the 2 MiB limit.");
}

} // namespace

CardHistoryPage list_card_history(holder_context& context, std::string project_id, std::string card_id,
                                  std::int32_t limit, std::optional<std::string> cursor) {
  return card_history_guard([&] {
    const auto project = history_project(context, project_id);
    check_cursor(cursor);
    const auto page = holder::history::CardHistoryService().list(
        project, card_id, static_cast<std::size_t>(limit), cursor && !cursor->empty() ? cursor : std::nullopt);
    CardHistoryPage out{page.head_oid, {}, page.next_cursor, page.scan_limited};
    nlohmann::json entries = nlohmann::json::array();
    for (const auto& entry : page.entries) {
      CardHistoryEntry item{entry.first_oid, entry.last_oid, entry.parent_oids, entry.visible_parent_oids,
                            {entry.author_name, entry.author_email}, entry.started_at, entry.ended_at,
                            entry.kind, entry.summary, static_cast<std::int32_t>(entry.commit_count),
                            entry.is_merge, {}};
      nlohmann::json saves = nlohmann::json::array();
      for (const auto& save : entry.saves) {
        item.saves.push_back({save.oid, save.parent_oids, save.authored_at, save.committed_at, save.message});
        saves.push_back({{"oid", save.oid}, {"parent_oids", save.parent_oids},
                         {"authored_at", save.authored_at}, {"committed_at", save.committed_at},
                         {"message", save.message}});
      }
      entries.push_back({{"first_oid", item.first_oid}, {"last_oid", item.last_oid},
                         {"parent_oids", item.parent_oids}, {"visible_parent_oids", item.visible_parent_oids},
                         {"author", {{"name", item.author.name}, {"email", item.author.email}}},
                         {"started_at", item.started_at}, {"ended_at", item.ended_at}, {"kind", item.kind},
                         {"summary", item.summary}, {"commit_count", item.commit_count},
                         {"is_merge", item.is_merge}, {"saves", std::move(saves)}});
      out.entries.push_back(std::move(item));
    }
    bound_card_history({{"head_oid", page.head_oid ? nlohmann::json(*page.head_oid) : nlohmann::json(nullptr)},
                        {"entries", std::move(entries)},
                        {"next_cursor", page.next_cursor ? nlohmann::json(*page.next_cursor) : nlohmann::json(nullptr)},
                        {"scan_limited", page.scan_limited}});
    return out;
  });
}

CardHistoryComparison compare_card_history(holder_context& context, std::string project_id,
                                           std::string card_id, std::string to_oid,
                                           std::optional<std::string> from_oid, CompareCardHistoryMode mode) {
  return card_history_guard([&] {
    const auto project = history_project(context, project_id);
    holder::history::CardHistoryService service;
    holder::history::CardHistoryComparison comparison;
    if (mode == CompareCardHistoryMode::Change) {
      const auto resolved = history_revision(project, to_oid, "to_oid");
      auto change = service.compare_change(project, card_id, resolved);
      if (!change.comparison) throw HistoryUnavailable("resolved history change has no comparison");
      comparison = std::move(*change.comparison);
    } else {
      if (!from_oid) throw InvalidArgument("from_oid and to_oid are required in since mode");
      comparison = service.compare(project, card_id, history_revision(project, *from_oid, "from_oid"),
                                   history_revision(project, to_oid, "to_oid"));
    }
    CardHistoryComparison out{history_version(comparison.from), history_version(comparison.to),
                              comparison.summary, {}, comparison.truncated};
    nlohmann::json lines = nlohmann::json::array();
    for (const auto& line : comparison.lines) {
      const auto origin = std::string(1, line.origin);
      const auto old_line = line.old_line < 0 ? std::optional<long long>{} : std::optional<long long>{line.old_line};
      const auto new_line = line.new_line < 0 ? std::optional<long long>{} : std::optional<long long>{line.new_line};
      out.lines.push_back({origin, line.text, old_line, new_line});
      lines.push_back({{"origin", origin}, {"text", line.text},
                       {"old_line", old_line ? nlohmann::json(*old_line) : nlohmann::json(nullptr)},
                       {"new_line", new_line ? nlohmann::json(*new_line) : nlohmann::json(nullptr)}});
    }
    bound_card_history({{"before", history_json(out.before)}, {"after", history_json(out.after)},
                        {"summary", out.summary}, {"lines", std::move(lines)}, {"truncated", out.truncated}});
    return out;
  });
}

CardHistorySnapshot get_card_history_snapshot(holder_context& context, std::string project_id,
                                              std::string card_id, std::string oid) {
  return card_history_guard([&] {
    const auto project = history_project(context, project_id);
    const auto resolved = history_revision(project, oid, "oid");
    const auto snapshot = holder::history::CardHistoryService().compare(project, card_id, std::nullopt, resolved).to;
    auto out = CardHistorySnapshot{card_id, history_version(snapshot)};
    bound_card_history({{"card_id", out.card_id}, {"snapshot", history_json(out.snapshot)}});
    return out;
  });
}

CardHistoryRestoreResult restore_card_history_snapshot(holder_context& context, std::string project_id,
                                                       std::string card_id, std::string oid) {
  return card_history_guard([&] {
    const auto project = history_project(context, project_id);
    const auto repo_mutex = holder::git::repo_mutex_for(project.root_path);
    const std::lock_guard<std::recursive_mutex> guard(*repo_mutex);
    const auto resolved = history_revision(project, oid, "oid");
    auto store = card_store(context);
    if (!card_in(store, project_id, card_id)) throw NotFound("Card not found.");
    store.restore_version(card_id, resolved, now_epoch_seconds());
    const auto restored = store.get(card_id);
    if (!restored) throw HistoryUnavailable("restored card is missing");
    holder::git::GitRepo repo;
    repo.open_existing(project.root_path);
    const auto result_oid = repo.head_oid();
    if (!result_oid) throw HistoryUnavailable("restored revision is missing");
    return CardHistoryRestoreResult{std::move(card_id), resolved, *result_oid, restored->title,
                                    restored->updated_at, restored->deleted_at};
  });
}

} // namespace holder::contract
