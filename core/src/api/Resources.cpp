// The contract's resource operations (contract/interface/resources.py). The generated glue checks
// their arguments first; these do the work.
#include "api/Contract.h"

#include "api/CardAccess.h"
#include "api/CoreErrors.h"

#include "identity/Uuid.h"
#include "project/ProjectRepo.h"
#include "resource/ResourceRepo.h"
#include "resource/ResourceStore.h"

#include <algorithm>
#include <cctype>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace holder::contract {
namespace {

// A card's link to a resource it has attached has this kind.
constexpr const char* ATTACHMENT = "attachment";

AssetPlacementEncoding encoding(const std::string& name) {
  if (name == "plain") return AssetPlacementEncoding::Plain;
  if (name == "holder_asset_v1") return AssetPlacementEncoding::HolderAssetV1;
  throw Error("an asset copy has an unknown encoding: " + name);
}

Resource to_resource(const holder::model::ResourceBundle& bundle) {
  const auto& resource = bundle.resource;
  Resource out{resource.resource_id, resource.project_id, resource.type,       resource.label,
               resource.metadata,    {},                  resource.created_at, resource.updated_at};
  for (const auto& asset : bundle.assets) {
    ResourceAsset value{asset.asset_id,         asset.original_filename, asset.media_type, asset.byte_size,
                        asset.plaintext_sha256, {},                      asset.created_at, asset.updated_at};
    for (const auto& placement : asset.placements)
      value.placements.push_back({placement.placement_id, placement.location_id, encoding(placement.encoding),
                                  placement.stored_byte_size, placement.created_at});
    out.assets.push_back(std::move(value));
  }
  return out;
}

holder::resource::ResourceStore resource_store(holder_context& context) {
  return holder::resource::ResourceStore(context.db, nullptr, context.git);
}

void require_project(holder_context& context, const std::string& project_id) {
  if (!holder::project::ProjectRepo(context.db).get(project_id)) throw NotFound("project not found: " + project_id);
}

// The resource with its files, when it is in the project: a resource in another project is not found.
holder::model::ResourceBundle bundle_in(holder_context& context, const std::string& project_id,
                                        const std::string& resource_id) {
  auto bundle = holder::resource::ResourceRepo(context.db).get_bundle(resource_id);
  if (!bundle || bundle->resource.project_id != project_id) throw NotFound("resource not found: " + resource_id);
  return std::move(*bundle);
}

std::vector<Resource> to_resources(holder_context& context, const std::vector<holder::model::Resource>& resources) {
  holder::resource::ResourceRepo repo(context.db);
  std::vector<Resource> out;
  for (const auto& resource : resources) {
    if (const auto bundle = repo.get_bundle(resource.resource_id)) out.push_back(to_resource(*bundle));
  }
  return out;
}

std::vector<Resource> list(holder_context& context, const std::string& project_id) {
  require_project(context, project_id);
  return to_resources(context, holder::resource::ResourceRepo(context.db).list(project_id));
}

// An ID names the resource's file, so it holds nothing a path could read differently.
void check_id(const std::string& resource_id) {
  const auto allowed = [](unsigned char c) { return std::isalnum(c) || c == '-' || c == '_'; };
  if (!std::all_of(resource_id.begin(), resource_id.end(), allowed))
    throw InvalidArgument("a resource ID holds only ASCII letters, digits, - and _");
}

// The contract states in words that a metadata name is not empty.
void check_names(const std::optional<holder::model::ResourceMetadata>& metadata) {
  if (metadata && metadata->contains("")) throw InvalidArgument("a metadata name must not be empty");
}

Resource create(holder_context& context, const std::string& project_id, const NewResource& request) {
  if (request.resource_id) check_id(*request.resource_id);
  check_names(request.metadata);
  require_project(context, project_id);
  holder::model::ResourceBundle bundle;
  auto& resource = bundle.resource;
  resource.resource_id = request.resource_id.value_or(holder::identity::uuid_v4());
  resource.project_id = project_id;
  resource.type = request.type;
  resource.label = request.label;
  resource.metadata = request.metadata.value_or(holder::model::ResourceMetadata{});
  std::erase_if(resource.metadata, [](const auto& entry) { return entry.second.empty(); });
  resource.created_at = request.created_at.value_or(now_epoch_seconds());
  resource.updated_at = request.updated_at.value_or(resource.created_at);
  // Resource IDs are unique across projects.
  if (holder::resource::ResourceRepo(context.db).get(resource.resource_id))
    throw Conflict("resource ID already in use: " + resource.resource_id);
  resource_store(context).put(bundle);
  return to_resource(bundle);
}

Resource update(holder_context& context, const std::string& project_id, const std::string& resource_id,
                const ResourceUpdate& request) {
  check_names(request.metadata);
  auto bundle = bundle_in(context, project_id, resource_id);
  const auto before = bundle.resource;
  auto& resource = bundle.resource;
  if (request.type) resource.type = *request.type;
  if (request.label) resource.label = *request.label;
  if (request.metadata) {
    for (const auto& [name, values] : *request.metadata) {
      if (values.empty())
        resource.metadata.erase(name);
      else
        resource.metadata[name] = values;
    }
  }
  // An update that changes nothing writes nothing, unless it says when it was made.
  if (!request.updated_at && resource.type == before.type && resource.label == before.label &&
      resource.metadata == before.metadata)
    return to_resource(bundle);
  resource.updated_at = request.updated_at.value_or(now_epoch_seconds());
  resource_store(context).put(bundle);
  return to_resource(bundle);
}

Resource remove(holder_context& context, const std::string& project_id, const std::string& resource_id) {
  const auto bundle = bundle_in(context, project_id, resource_id);
  resource_store(context).remove(resource_id);
  return to_resource(bundle);
}

std::vector<Resource> card_resources(holder_context& context, const std::string& project_id,
                                     const std::string& card_id) {
  auto store = card_store(context);
  live_card(store, project_id, card_id);
  // The repository reads at most 1000 at a time.
  constexpr int PAGE = 1000;
  holder::resource::ResourceRepo repo(context.db);
  std::vector<holder::model::Resource> resources;
  for (int offset = 0;; offset += PAGE) {
    const auto page = repo.list_for_card(project_id, card_id, PAGE, offset);
    resources.insert(resources.end(), page.begin(), page.end());
    if (page.size() < PAGE) break;
  }
  return to_resources(context, resources);
}

} // namespace

std::vector<Resource> list_resources(holder_context& context, std::string project_id) {
  return with_contract_errors([&] { return list(context, project_id); });
}

Resource get_resource(holder_context& context, std::string project_id, std::string resource_id) {
  return with_contract_errors([&] { return to_resource(bundle_in(context, project_id, resource_id)); });
}

Resource create_resource(holder_context& context, std::string project_id, const NewResource& resource) {
  return with_contract_errors([&] { return create(context, project_id, resource); });
}

Resource update_resource(holder_context& context, std::string project_id, std::string resource_id,
                         const ResourceUpdate& request) {
  return with_contract_errors([&] { return update(context, project_id, resource_id, request); });
}

Resource delete_resource(holder_context& context, std::string project_id, std::string resource_id) {
  return with_contract_errors([&] { return remove(context, project_id, resource_id); });
}

std::vector<Link> list_resource_backlinks(holder_context& context, std::string project_id, std::string resource_id,
                                          bool include_deleted) {
  return with_contract_errors([&] {
    bundle_in(context, project_id, resource_id);
    return local_backlinks(context, project_id, "resource", resource_id, include_deleted);
  });
}

std::vector<Resource> list_card_resources(holder_context& context, std::string project_id, std::string card_id) {
  return with_contract_errors([&] { return card_resources(context, project_id, card_id); });
}

std::vector<Resource> attach_card_resource(holder_context& context, std::string project_id, std::string card_id,
                                           std::string resource_id) {
  NewLink link;
  link.target_type = NewLinkTargetType::Resource;
  link.target_id = resource_id;
  link.kind = ATTACHMENT;
  add_card_link(context, project_id, card_id, link);
  return list_card_resources(context, project_id, card_id);
}

std::vector<Resource> detach_card_resource(holder_context& context, std::string project_id, std::string card_id,
                                           std::string resource_id) {
  remove_card_link(context, project_id, card_id, RemoveCardLinkTargetType::Resource, resource_id, ATTACHMENT,
                   std::nullopt);
  return list_card_resources(context, project_id, card_id);
}

} // namespace holder::contract
