// Project activity history served through the shared contract.
#include "api/Contract.h"

#include "capi/Context.h"
#include "history/ProjectHistory.h"
#include "privacy/PrivacyError.h"
#include "project/ProjectRepo.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <exception>
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

} // namespace holder::contract
