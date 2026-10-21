#include "api/ProjectUpdate.h"

#include "git/GitOps.h"
#include "platform/Paths.h"
#include "platform/ProjectRegistry.h"
#include "privacy/ProjectPrivacy.h"
#include "project/ProjectManifest.h"
#include "project/ProjectRepo.h"

#include <utility>

namespace holder::api {
namespace {

std::string privacy_mode_name(ProjectUpdateRequestPrivacyMode mode) {
  switch (mode) {
  case ProjectUpdateRequestPrivacyMode::encrypted_git: return "encrypted_git";
  case ProjectUpdateRequestPrivacyMode::plain: return "plain";
  }
  throw std::invalid_argument("Invalid privacy mode.");
}

} // namespace

ProjectUpdateResponse update_project(holder::platform::Db& db,
                                     holder::git::GitOps* git_ops,
                                     const std::function<std::string()>& uuid_v4,
                                     std::string project_id,
                                     ProjectUpdateRequest request) {
  const bool has_name = request.name && request.name->has_value();
  const bool has_root = request.root_path && request.root_path->has_value();
  const bool has_remote = request.git_remote_url.has_value();
  const bool has_provider = request.git_provider.has_value();
  const bool has_privacy = request.privacy_mode && request.privacy_mode->has_value();
  const bool has_key = request.project_key_id.has_value();
  if (!has_name && !has_root && !has_remote && !has_provider && !has_privacy && !has_key)
    throw std::invalid_argument("No fields to update.");

  holder::project::ProjectRepo repo(db);
  const auto project = repo.get(project_id);
  if (!project) throw ProjectUpdateNotFound();

  const std::string repo_root = has_root ? **request.root_path : project->root_path;
  static holder::git::RealGitOps real_git;
  auto& git = git_ops ? *git_ops : real_git;
  try {
    auto operation = git.lock_operation(repo_root);
    // Read the remote under the same repository lock as the mutation.
    const auto locked_project = repo.get(project_id);
    if (!locked_project) throw ProjectUpdateNotFound();

    if (has_name) repo.update_name(project_id, **request.name, request.updated_at);
    if (has_root) repo.update_root_path(project_id, repo_root, request.updated_at);
    if (has_provider)
      repo.update_git_provider(project_id, *request.git_provider, request.updated_at);
    if (has_privacy)
      repo.update_privacy_mode(project_id, privacy_mode_name(**request.privacy_mode),
                               request.updated_at);
    if (has_key)
      repo.update_project_key_id(project_id, *request.project_key_id, request.updated_at);

    std::optional<bool> git_remote_changed;
    if (has_remote) {
      const auto& remote = *request.git_remote_url;
      git.open_or_init(repo_root);
      if (remote) git.set_remote("origin", *remote);
      else git.remove_remote("origin");
      git_remote_changed = locked_project->git_remote_url != remote;
      // Persist the remote only after Git accepts it.
      repo.update_git_remote(project_id, remote, request.updated_at);
    }

    const std::string effective_privacy = has_privacy
        ? privacy_mode_name(**request.privacy_mode) : project->privacy_mode;
    const std::optional<std::string> effective_key = has_key
        ? *request.project_key_id : project->project_key_id;
    if (effective_privacy == "encrypted_git") {
      holder::privacy::ensure_encrypted_project_ready(
          git, repo, project_id, repo_root, effective_key, request.updated_at, uuid_v4);
    }
    const auto updated_project = repo.get(project_id).value();
    holder::project::write_project_manifest(git, updated_project);
    git.commit("Update project metadata");
    holder::core::ProjectRegistry(holder::core::Paths::resolve("holder").project_registry_path())
        .remember(repo.list());
    return {true, {std::move(project_id), git_remote_changed}};
  } catch (const holder::privacy::PrivacyError& error) {
    switch (error.code()) {
    case holder::privacy::PrivacyErrorCode::KeyringUnavailable:
      throw ProjectUpdateUnavailable(error.code(), error.what());
    case holder::privacy::PrivacyErrorCode::KeyMaterialMissing:
    case holder::privacy::PrivacyErrorCode::CryptMetadataMissing:
      throw ProjectUpdateInternal(error.code(), error.what());
    default:
      throw ProjectUpdateBadRequest(error.code(), error.what());
    }
  }
}

} // namespace holder::api
