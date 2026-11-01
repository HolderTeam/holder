// Storage declarations from contract/interface/locations.py.
#include "api/Contract.h"

#include "api/CardAccess.h"
#include "api/CoreErrors.h"
#include "capi/Context.h"

#include "identity/Uuid.h"
#include "model/ConflictError.h"
#include "project/ProjectRepo.h"
#include "resource/LocationBindingStore.h"
#include "resource/LocationRepo.h"
#include "resource/LocationStore.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>

namespace holder::contract {
namespace {

ProjectStorageLocationProvider provider(const std::string& value) {
  if (value == "local_directory") return ProjectStorageLocationProvider::LocalDirectory;
  if (value == "s3_compatible") return ProjectStorageLocationProvider::S3Compatible;
  if (value == "google-drive") return ProjectStorageLocationProvider::GoogleDrive;
  throw Error("unknown storage provider: " + value);
}

std::string provider(NewStorageLocationProvider value) {
  switch (value) {
  case NewStorageLocationProvider::LocalDirectory: return "local_directory";
  case NewStorageLocationProvider::S3Compatible: return "s3_compatible";
  case NewStorageLocationProvider::GoogleDrive: return "google-drive";
  }
  throw InvalidArgument("unknown storage provider");
}

std::unique_ptr<holder::resource::LocationBindingStore> binding_store(holder_context& context) {
  if (!context.secret_store) return nullptr;
  return std::make_unique<holder::resource::LocationBindingStore>(*context.secret_store);
}

ProjectStorageLocation item(const holder::model::Location& location,
                     const holder::resource::LocationBindingStore* bindings) {
  auto preview = bindings ? bindings->preview(location.project_id, location.location_id) : std::nullopt;
  return {location.location_id, location.project_id, location.name, provider(location.provider),
          location.configuration, preview.has_value(), std::move(preview),
          location.created_at, location.updated_at};
}

holder::model::Location require_location(holder_context& context, const std::string& id) {
  auto location = holder::resource::LocationRepo(context.db).get(id);
  if (!location) throw NotFound("location not found");
  return std::move(*location);
}

void save(holder_context& context, const holder::model::Location& location) {
  if (context.git)
    holder::resource::LocationStore(context.db, nullptr, context.git).put(location);
  else
    holder::resource::LocationRepo(context.db).put(location);
}

} // namespace

StorageLocationList list_storage_locations(holder_context& context, std::string project_id) {
  return with_contract_errors([&] {
    const auto bindings = binding_store(context);
    StorageLocationList out{{}, bindings ? bindings->preferred(project_id) : std::nullopt};
    for (const auto& location : holder::resource::LocationRepo(context.db).list(project_id))
      out.data.push_back(item(location, bindings.get()));
    return out;
  });
}

ProjectStorageLocation get_storage_location(holder_context& context, std::string location_id) {
  return with_contract_errors([&] {
    const auto location = require_location(context, location_id);
    const auto bindings = binding_store(context);
    return item(location, bindings.get());
  });
}

ProjectStorageLocation create_storage_location(holder_context& context, const NewStorageLocation& request) {
  return with_contract_errors([&] {
    holder::model::Location location;
    location.location_id = request.location_id.value_or("");
    if (location.location_id.empty()) location.location_id = holder::identity::uuid_v4();
    location.project_id = request.project_id;
    location.name = request.name;
    location.provider = provider(request.provider);
    location.configuration = request.configuration.value_or(std::map<std::string, std::string>{});
    location.created_at = request.created_at.value_or(now_epoch_seconds());
    location.updated_at = request.updated_at.value_or(location.created_at);
    if (!holder::project::ProjectRepo(context.db).get(location.project_id))
      throw NotFound("Project not found.");
    save(context, location);
    const auto bindings = binding_store(context);
    return item(location, bindings.get());
  });
}

ProjectStorageLocation update_storage_location(holder_context& context, std::string location_id,
                                               const StorageLocationUpdate& request) {
  return with_contract_errors([&] {
    auto location = require_location(context, location_id);
    if (request.name) location.name = *request.name;
    if (request.configuration) location.configuration = *request.configuration;
    location.updated_at = request.updated_at.value_or(now_epoch_seconds());
    save(context, location);
    const auto bindings = binding_store(context);
    return item(location, bindings.get());
  });
}

LocationDeleteResult delete_storage_location(holder_context& context, std::string location_id) {
  return with_contract_errors([&] {
    const auto location = require_location(context, location_id);
    if (context.git) {
      holder::resource::LocationStore(context.db, nullptr, context.git).remove(location_id);
    } else {
      holder::resource::LocationRepo repo(context.db);
      if (repo.is_in_use(location_id))
        throw holder::model::ConflictError("storage location is in use by an asset placement");
      repo.remove(location_id);
    }
    if (const auto bindings = binding_store(context)) {
      if (bindings->preferred(location.project_id) == location_id)
        bindings->clear_preferred(location.project_id);
      bindings->unbind(location.project_id, location_id);
    }
    return LocationDeleteResult{std::move(location_id)};
  });
}

} // namespace holder::contract
