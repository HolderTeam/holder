#include "api/ProjectCreate.h"

#include "platform/Paths.h"
#include "platform/ProjectRegistry.h"
#include "project/ProjectStore.h"

#include <utility>

namespace holder::api {

ProjectCreateResponse create_project(holder::platform::Db& db,
                                     holder::git::GitOps* git_ops,
                                     const std::function<std::string()>& uuid_v4,
                                     ProjectCreateRequest request) {
  holder::model::Project project;
  project.project_id = request.project_id.value_or("");
  if (project.project_id.empty()) project.project_id = uuid_v4();
  project.name = std::move(request.name);
  project.root_path = request.root_path.value_or("");
  project.git_remote_url = request.git_remote_url.value_or(std::nullopt);
  project.git_provider = request.git_provider.value_or(std::nullopt);
  project.privacy_mode = request.privacy_mode.value_or("encrypted_git");
  project.project_key_id = request.project_key_id.value_or(std::nullopt);
  project.created_at = request.created_at.value_or(0);
  project.updated_at = request.updated_at.value_or(0);

  try {
    holder::project::ProjectStore store(db, git_ops);
    project = store.create(std::move(project), uuid_v4, holder::core::default_projects_root());
    holder::core::ProjectRegistry(holder::core::Paths::resolve("holder").project_registry_path())
        .restore(project);
  } catch (const holder::privacy::PrivacyError& error) {
    switch (error.code()) {
    case holder::privacy::PrivacyErrorCode::KeyringUnavailable:
      throw ProjectCreateUnavailable(error.code(), error.what());
    case holder::privacy::PrivacyErrorCode::KeyMaterialMissing:
    case holder::privacy::PrivacyErrorCode::CryptMetadataMissing:
      throw ProjectCreateInternal(error.code(), error.what());
    default:
      throw ProjectCreateBadRequest(error.code(), error.what());
    }
  }

  return {true, get_project(db, project.project_id).data};
}

} // namespace holder::api
