// The contract's project operations (contract/interface/projects.py). The generated glue checks
// their arguments first; these do the work.
#include "api/Contract.h"

#include "api/CardAccess.h"
#include "api/CoreErrors.h"

#include "capi/Context.h"
#include "card/CardRepo.h"
#include "git/GitOps.h"
#include "identity/Uuid.h"
#include "platform/Tx.h"
#include "project/DefaultProject.h"
#include "project/ProjectManifest.h"
#include "project/ProjectRegistry.h"
#include "project/ProjectRepo.h"
#include "project/ProjectStore.h"
#include "project/ProjectSyncRepo.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace holder::contract {
namespace {

ProjectPrivacyMode privacy_mode(const std::string& name) {
  if (name == "encrypted_git") return ProjectPrivacyMode::EncryptedGit;
  if (name == "plain") return ProjectPrivacyMode::Plain;
  throw Error("a project has an unknown privacy mode: " + name);
}

Project to_project(const holder::model::Project& project) {
  return {project.project_id,   project.name,         project.root_path,
          project.git_remote_url, project.git_provider, privacy_mode(project.privacy_mode),
          project.project_key_id, project.created_at,   project.updated_at};
}

holder::model::Project find(holder_context& context, const std::string& project_id) {
  auto project = holder::project::ProjectRepo(context.db).get(project_id);
  if (!project) throw NotFound("project not found: " + project_id);
  return std::move(*project);
}

std::string lowered(std::string_view text) {
  std::string out;
  for (const char c : text) out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
  return out;
}

bool is_home(const std::string& name) { return lowered(name) == lowered(HOME_PROJECT_NAME); }

std::vector<ProjectListItem> list(holder_context& context, const std::optional<std::string>& name,
                                  std::optional<long long> updated_after, std::optional<long long> updated_before,
                                  std::optional<ListProjectsOrder> order, std::int32_t limit, std::int32_t offset,
                                  bool include_card_counts) {
  auto projects = holder::project::ProjectRepo(context.db).list();
  std::erase_if(projects, [&](const holder::model::Project& project) {
    return (name && project.name.find(*name) == std::string::npos) ||
           (updated_after && project.updated_at < *updated_after) ||
           (updated_before && project.updated_at > *updated_before);
  });
  using Order = ListProjectsOrder;
  const auto by = order.value_or(Order::UpdatedAtDesc);
  std::ranges::stable_sort(projects, [by](const auto& a, const auto& b) {
    switch (by) {
    case Order::UpdatedAtAsc: return a.updated_at < b.updated_at;
    case Order::CreatedAtDesc: return a.created_at > b.created_at;
    case Order::CreatedAtAsc: return a.created_at < b.created_at;
    case Order::NameDesc: return a.name > b.name;
    case Order::NameAsc: return a.name < b.name;
    case Order::UpdatedAtDesc: break;
    }
    return a.updated_at > b.updated_at;
  });
  // Without an order, Home comes first.
  if (!order)
    std::ranges::stable_partition(projects, [](const auto& project) { return is_home(project.name); });

  holder::card::CardRepo cards(context.db);
  std::vector<ProjectListItem> out;
  const auto start = std::min(projects.size(), static_cast<std::size_t>(offset));
  const auto end = std::min(projects.size(), start + static_cast<std::size_t>(limit));
  for (auto i = start; i < end; ++i) {
    ProjectListItem item{to_project(projects[i]), std::nullopt, std::nullopt};
    if (include_card_counts) {
      item.card_count = cards.count_all_not_deleted(projects[i].project_id);
      item.root_card_count = cards.count_roots_not_deleted(projects[i].project_id);
    }
    out.push_back(std::move(item));
  }
  return out;
}

const char* privacy_name(NewProjectPrivacyMode mode) {
  return mode == NewProjectPrivacyMode::Plain ? "plain" : "encrypted_git";
}

Project create(holder_context& context, const NewProject& request) {
  // A relative directory would be read against wherever the host happens to run.
  if (request.root_path && !std::filesystem::path(*request.root_path).is_absolute())
    throw InvalidArgument("root_path must be an absolute path");
  holder::model::Project project;
  project.project_id = request.project_id.value_or("");
  project.name = request.name;
  project.root_path = request.root_path.value_or("");
  project.git_remote_url = request.git_remote_url;
  project.git_provider = request.git_provider;
  project.privacy_mode = privacy_name(request.privacy_mode);
  project.created_at = request.created_at.value_or(now_epoch_seconds());
  project.updated_at = request.updated_at.value_or(project.created_at);
  if (!project.project_id.empty() && holder::project::ProjectRepo(context.db).get(project.project_id))
    throw Conflict("project ID already in use: " + project.project_id);
  const bool own_root = !project.root_path.empty();
  auto created = holder::project::ProjectStore(context.db, context.git)
                     .create(std::move(project), holder::identity::uuid_v4, context.projects_root);
  if (!context.registry_path.empty()) {
    holder::core::ProjectRegistry(context.registry_path).restore(created);
  } else if (own_root && !context.data_dir.empty()) {
    // Without a registry, a rebuild from the managed projects alone would lose this one.
    std::error_code ignored;
    std::filesystem::remove(holder::capi::rebuild_readiness_path(context.data_dir), ignored);
  }
  return to_project(created);
}

Project update(holder_context& context, const std::string& project_id, const ProjectUpdate& request) {
  auto project = find(context, project_id);
  std::optional<holder::git::RealGitOps> own_git;
  auto& git = context.git != nullptr ? *context.git : own_git.emplace();
  auto operation = git.lock_operation(project.root_path);
  // Read again under the repository's lock.
  project = find(context, project_id);
  const auto name = request.name.value_or(project.name);
  const auto remote = request.git_remote_url.value_or(project.git_remote_url);
  const auto provider = request.git_provider.value_or(project.git_provider);
  if (name == project.name && remote == project.git_remote_url && provider == project.git_provider)
    return to_project(project);

  holder::project::ProjectRepo repo(context.db);
  const auto now = now_epoch_seconds();
  git.open_or_init(project.root_path);
  if (remote != project.git_remote_url) {
    // The remote is stored only once Git has it.
    if (remote) {
      try {
        git.set_remote("origin", *remote);
      } catch (const std::runtime_error& error) {
        throw InvalidArgument(std::string("Git does not accept the remote: ") + error.what());
      }
    } else {
      git.remove_remote("origin");
    }
    repo.update_git_remote(project_id, remote, now);
  }
  if (name != project.name) repo.update_name(project_id, name, now);
  if (provider != project.git_provider) repo.update_git_provider(project_id, provider, now);
  const auto updated = find(context, project_id);
  holder::project::write_project_manifest(git, updated);
  git.commit("Update project metadata");
  if (!context.registry_path.empty())
    holder::core::ProjectRegistry(context.registry_path).remember(repo.list());
  return to_project(updated);
}

Project remove(holder_context& context, const std::string& project_id) {
  const auto project = find(context, project_id);
  holder::platform::Tx transaction(context.db);
  holder::project::ProjectRepo(context.db).remove(project_id);
  holder::project::ProjectSyncRepo(context.db).remove(project_id);
  // The removal is remembered, so that looking for projects does not bring it back.
  std::optional<holder::core::ProjectRegistry> registry;
  if (!context.registry_path.empty()) registry.emplace(context.registry_path).forget(project);
  try {
    transaction.commit();
  } catch (...) {
    if (registry) registry->restore(project);
    throw;
  }
  return to_project(project);
}

} // namespace

std::vector<ProjectListItem> list_projects(holder_context& context, std::optional<std::string> name,
                                           std::optional<long long> updated_after,
                                           std::optional<long long> updated_before,
                                           std::optional<ListProjectsOrder> order, std::int32_t limit,
                                           std::int32_t offset, bool include_card_counts) {
  return with_contract_errors(
      [&] { return list(context, name, updated_after, updated_before, order, limit, offset, include_card_counts); });
}

HomeProject ensure_home_project(holder_context& context) {
  return with_contract_errors([&] {
    const auto created = holder::project::ensure_default_project(context.db, holder::identity::uuid_v4,
                                                                 context.projects_root, &context.fts, context.git);
    if (!created) return HomeProject{};
    if (!context.registry_path.empty()) holder::core::ProjectRegistry(context.registry_path).restore(*created);
    return HomeProject{to_project(*created)};
  });
}

Project get_project(holder_context& context, std::string project_id) {
  return with_contract_errors([&] { return to_project(find(context, project_id)); });
}

Project create_project(holder_context& context, const NewProject& project) {
  return with_contract_errors([&] { return create(context, project); });
}

Project update_project(holder_context& context, std::string project_id, const ProjectUpdate& request) {
  return with_contract_errors([&] { return update(context, project_id, request); });
}

Project delete_project(holder_context& context, std::string project_id) {
  return with_contract_errors([&] { return remove(context, project_id); });
}

} // namespace holder::contract
