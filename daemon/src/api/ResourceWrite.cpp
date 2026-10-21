#include "api/ResourceWrite.h"

#include "api/support/Time.h"
#include "project/ProjectRepo.h"
#include "resource/ResourceRepo.h"
#include "resource/ResourceStore.h"

#include <utility>

namespace holder::api {
namespace {

holder::model::ResourceBundle find_resource(holder::platform::Db& db,
                                            const std::string& resource_id) {
  auto bundle = holder::resource::ResourceRepo(db).get_bundle(resource_id);
  if (!bundle) throw ResourceNotFound{};
  return std::move(*bundle);
}

// Resources are committed to the project's Git repository when the daemon has one.
void save_resource(holder::platform::Db& db,
                   holder::git::GitOps* git_ops,
                   const holder::model::ResourceBundle& bundle) {
  if (git_ops != nullptr)
    holder::resource::ResourceStore(db, nullptr, git_ops).put(bundle);
  else
    holder::resource::ResourceRepo(db).put_bundle(bundle);
}

} // namespace

ResourceResponse post_resources(holder::platform::Db& db,
                                holder::git::GitOps* git_ops,
                                const std::function<std::string()>& uuid_v4,
                                ResourceCreateRequest request) {
  holder::model::ResourceBundle bundle;
  auto& resource = bundle.resource;
  resource.resource_id = request.resource_id.value_or("");
  if (resource.resource_id.empty()) resource.resource_id = uuid_v4();
  resource.project_id = std::move(request.project_id);
  resource.type = std::move(request.type);
  resource.label = std::move(request.label);
  if (request.metadata) resource.metadata = std::move(request.metadata->value);
  resource.created_at = request.created_at.value_or(support::now_epoch_seconds());
  resource.updated_at = request.updated_at.value_or(resource.created_at);

  if (!holder::project::ProjectRepo(db).get(resource.project_id))
    throw std::invalid_argument("Project not found.");
  if (holder::resource::ResourceRepo(db).get_bundle(resource.resource_id))
    throw holder::model::ConflictError("resource_id already exists");
  save_resource(db, git_ops, bundle);
  return {true, resource_item(bundle)};
}

ResourceResponse patch_resources_resource_id(holder::platform::Db& db,
                                             holder::git::GitOps* git_ops,
                                             std::string resource_id,
                                             ResourceUpdateRequest request) {
  auto bundle = find_resource(db, resource_id);
  auto& resource = bundle.resource;
  if (request.type) resource.type = std::move(*request.type);
  if (request.label) resource.label = std::move(*request.label);
  if (request.metadata) {
    for (auto& [property, values] : request.metadata->value) {
      if (values.empty())
        resource.metadata.erase(property);
      else
        resource.metadata[property] = std::move(values);
    }
  }
  resource.updated_at = request.updated_at.value_or(support::now_epoch_seconds());
  save_resource(db, git_ops, bundle);
  return {true, resource_with_references(db, bundle)};
}

ResourceDeleteResponse delete_resources_resource_id(holder::platform::Db& db,
                                                    holder::git::GitOps* git_ops,
                                                    std::string resource_id) {
  find_resource(db, resource_id);
  if (git_ops != nullptr)
    holder::resource::ResourceStore(db, nullptr, git_ops).remove(resource_id);
  else
    holder::resource::ResourceRepo(db).remove(resource_id);
  return {true, {std::move(resource_id)}};
}

} // namespace holder::api
