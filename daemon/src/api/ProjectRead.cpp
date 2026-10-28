#include "api/ProjectRead.h"

#include "card/CardRepo.h"
#include "model/Project.h"
#include "model/ProjectSyncState.h"
#include "privacy/ProjectPrivacy.h"
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

} // namespace

ProjectGitSyncStatusResponse get_project_git_sync_status(holder::platform::Db& db,
                                                         std::string project_id) {
  if (!holder::project::ProjectRepo(db).get(project_id)) throw ProjectReadNotFound{};
  auto sync = project_sync_response(holder::project::ProjectSyncRepo(db).get(project_id));
  return {true, {std::move(project_id), std::move(sync)}};
}

ProjectEncryptionCheckResponse check_project_encryption(holder::platform::Db& db,
                                                        std::string project_id) {
  const auto project = holder::project::ProjectRepo(db).get(project_id);
  if (!project) throw ProjectReadNotFound{};
  ProjectEncryptionCheckResponse response{true, {std::move(project_id), project->privacy_mode, {}}};
  auto& check = response.data.check;
  if (project->privacy_mode != "encrypted_git") {
    check = {true, 0, 0, {}, "Project is plain mode; privacy check not required."};
    return response;
  }
  auto result = holder::privacy::run_encryption_safety_check(project->root_path);
  check = {
      result.ok,
      static_cast<int>(result.checked_files),
      static_cast<int>(result.unsafe_paths.size()),
      std::move(result.unsafe_paths),
      std::move(result.message),
  };
  return response;
}

} // namespace holder::api
