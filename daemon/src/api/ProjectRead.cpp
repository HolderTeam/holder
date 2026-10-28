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
