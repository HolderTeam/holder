#include "api/ProjectRead.h"

#include "card/CardRepo.h"
#include "model/Project.h"
#include "model/ProjectSyncState.h"
#include "project/ProjectRepo.h"
#include "project/ProjectSyncRepo.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace holder::api {
namespace {

bool is_home_project_name(const std::string& name) {
  std::string lowered;
  lowered.reserve(name.size());
  for (const char character : name) {
    lowered.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(character))));
  }
  return lowered == "home";
}

ProjectSync project_sync_response(const std::optional<holder::model::ProjectSyncState>& value) {
  const auto& sync = value.value_or(holder::model::ProjectSyncState{});
  return {
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
      sync.updated_at > 0 ? std::optional<long long>{sync.updated_at} : std::nullopt,
  };
}

Project project_response(const holder::model::Project& project,
                         const std::optional<holder::model::ProjectSyncState>& sync,
                         std::optional<int> card_count = std::nullopt,
                         std::optional<int> root_card_count = std::nullopt) {
  return {
      project.project_id,
      project.name,
      project.root_path,
      std::optional<std::optional<std::string>>{std::in_place, project.git_remote_url},
      std::optional<std::optional<std::string>>{std::in_place, project.git_provider},
      project.privacy_mode,
      std::optional<std::optional<std::string>>{std::in_place, project.project_key_id},
      project.created_at,
      project.updated_at,
      card_count,
      root_card_count,
      project_sync_response(sync),
  };
}

} // namespace

ProjectListResponse get_projects(holder::platform::Db& db,
                                 std::optional<std::string> name,
                                 std::optional<long long> updated_after,
                                 std::optional<long long> updated_before,
                                 std::optional<std::string> order,
                                 std::optional<int> limit,
                                 std::optional<int> offset,
                                 bool count) {
  holder::project::ProjectRepo repo(db);
  holder::project::ProjectSyncRepo sync_repo(db);
  auto projects = repo.list();

  const auto name_filter = name.value_or("");
  std::erase_if(projects, [&](const holder::model::Project& project) {
    return (!name_filter.empty() && project.name.find(name_filter) == std::string::npos) ||
           (updated_after && project.updated_at < *updated_after) ||
           (updated_before && project.updated_at > *updated_before);
  });

  enum class OrderKey : std::uint8_t { UpdatedAt, CreatedAt, Name };
  OrderKey order_key = OrderKey::UpdatedAt;
  bool ascending = false;
  if (order) {
    ascending = order->ends_with("_asc");
    if (order->starts_with("created_at_")) order_key = OrderKey::CreatedAt;
    if (order->starts_with("name_")) order_key = OrderKey::Name;
  }
  std::stable_sort(projects.begin(), projects.end(),
                   [ascending, order_key](const auto& a, const auto& b) {
    switch (order_key) {
    case OrderKey::CreatedAt:
      return ascending ? a.created_at < b.created_at : a.created_at > b.created_at;
    case OrderKey::Name:
      return ascending ? a.name < b.name : a.name > b.name;
    case OrderKey::UpdatedAt:
    default:
      return ascending ? a.updated_at < b.updated_at : a.updated_at > b.updated_at;
    }
  });
  if (!order) {
    std::stable_sort(projects.begin(), projects.end(), [](const auto& a, const auto& b) {
      return is_home_project_name(a.name) && !is_home_project_name(b.name);
    });
  }

  const auto start = std::min(projects.size(), static_cast<std::size_t>(offset.value_or(0)));
  const auto page_size = static_cast<std::size_t>(std::min(limit.value_or(100), 1000));
  const auto end = start + std::min(projects.size() - start, page_size);

  holder::card::CardRepo card_repo(db);
  ProjectListResponse response{true, {}};
  response.data.reserve(end - start);
  for (auto index = start; index < end; ++index) {
    const auto& project = projects[index];
    response.data.push_back(project_response(
        project,
        sync_repo.get(project.project_id),
        count ? std::optional<int>{card_repo.count_all_not_deleted(project.project_id)} : std::nullopt,
        count ? std::optional<int>{card_repo.count_roots_not_deleted(project.project_id)} : std::nullopt));
  }
  return response;
}

ProjectGetResponse get_project(holder::platform::Db& db, std::string project_id) {
  holder::project::ProjectRepo repo(db);
  const auto project = repo.get(project_id);
  if (!project) throw ProjectReadNotFound{};
  holder::project::ProjectSyncRepo sync_repo(db);
  return {true, project_response(*project, sync_repo.get(project_id))};
}

} // namespace holder::api
