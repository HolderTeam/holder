#include "api/LocationWrite.h"

#include "api/support/StorageProviders.h"
#include "api/support/Time.h"
#include "platform/Paths.h"
#include "project/ProjectRepo.h"
#include "resource/AssetEnvelope.h"
#include "resource/LocationBindingStore.h"
#include "resource/LocationRepo.h"
#include "resource/LocationStore.h"

#include <filesystem>
#include <fstream>
#include <memory>
#include <utility>

namespace holder::api {
namespace {

std::unique_ptr<holder::resource::LocationBindingStore> binding_store(
    holder::privacy::SecretStore* secret_store
) {
  if (secret_store == nullptr) return nullptr;
  return std::make_unique<holder::resource::LocationBindingStore>(*secret_store);
}

holder::resource::LocationBindingStore require_bindings(holder::privacy::SecretStore* secret_store) {
  if (secret_store == nullptr) throw SecretStoreUnavailable{};
  return holder::resource::LocationBindingStore(*secret_store);
}

holder::model::Location find_location(holder::platform::Db& db, const std::string& location_id) {
  auto location = holder::resource::LocationRepo(db).get(location_id);
  if (!location) throw LocationNotFound{};
  return std::move(*location);
}

// Declarations are committed to the project's Git repository when the daemon has one.
void save_location(holder::platform::Db& db,
                   holder::git::GitOps* git_ops,
                   const holder::model::Location& location) {
  if (git_ops != nullptr)
    holder::resource::LocationStore(db, nullptr, git_ops).put(location);
  else
    holder::resource::LocationRepo(db).put(location);
}

// Writes a probe object, checks it is there and removes it, removing it on failure too.
void probe(holder::resource::StorageProvider& provider,
           const holder::model::Location& location,
           const std::function<std::string()>& uuid_v4) {
  const auto probe_dir = holder::core::Paths::resolve("holder").cache_dir / "asset-probes";
  std::filesystem::create_directories(probe_dir);
  const auto probe_file = probe_dir / (uuid_v4() + ".probe");
  std::ofstream(probe_file, std::ios::binary) << "Holder storage probe\n";
  const auto digest = holder::resource::digest_file(probe_file);
  const auto object_key = support::location_object_key(location, ".holder-probes/" + uuid_v4());
  try {
    // A PUT can reach the provider even if the client then sees an error, so every failure
    // attempts a best-effort removal.
    provider.put(object_key, probe_file, digest.byte_size, digest.sha256);
    if (!provider.exists(object_key)) {
      throw holder::resource::StorageError(holder::resource::StorageErrorCode::Integrity,
                                           "storage probe not found");
    }
    provider.remove(object_key);
  } catch (...) {
    try {
      provider.remove(object_key);
    } catch (...) {
    }
    std::error_code ignored;
    std::filesystem::remove(probe_file, ignored);
    throw;
  }
  std::error_code ignored;
  std::filesystem::remove(probe_file, ignored);
}

} // namespace

StorageLocationResponse post_locations(holder::platform::Db& db,
                                       holder::privacy::SecretStore* secret_store,
                                       holder::git::GitOps* git_ops,
                                       const std::function<std::string()>& uuid_v4,
                                       StorageLocationRequest request) {
  holder::model::Location location;
  location.location_id = request.location_id.value_or("");
  if (location.location_id.empty()) location.location_id = uuid_v4();
  location.project_id = std::move(request.project_id);
  location.name = std::move(request.name);
  location.provider = std::move(request.provider);
  location.configuration = request.configuration.value_or(std::map<std::string, std::string>{});
  location.created_at = request.created_at.value_or(support::now_epoch_seconds());
  location.updated_at = request.updated_at.value_or(location.created_at);
  if (!holder::project::ProjectRepo(db).get(location.project_id))
    throw std::invalid_argument("Project not found.");
  save_location(db, git_ops, location);
  const auto bindings = binding_store(secret_store);
  return {true, location_item(location, bindings.get())};
}

StorageLocationResponse patch_locations_location_id(holder::platform::Db& db,
                                                    holder::privacy::SecretStore* secret_store,
                                                    holder::git::GitOps* git_ops,
                                                    std::string location_id,
                                                    PatchLocationsLocationIdRequest request) {
  auto location = find_location(db, location_id);
  if (request.name) location.name = std::move(*request.name);
  if (request.configuration) location.configuration = std::move(*request.configuration);
  location.updated_at = request.updated_at.value_or(support::now_epoch_seconds());
  save_location(db, git_ops, location);
  const auto bindings = binding_store(secret_store);
  return {true, location_item(location, bindings.get())};
}

LocationActionResponse delete_locations_location_id(holder::platform::Db& db,
                                                    holder::privacy::SecretStore* secret_store,
                                                    holder::git::GitOps* git_ops,
                                                    std::string location_id) {
  const auto location = find_location(db, location_id);
  if (git_ops != nullptr) {
    holder::resource::LocationStore(db, nullptr, git_ops).remove(location_id);
  } else {
    holder::resource::LocationRepo repo(db);
    if (repo.is_in_use(location_id))
      throw holder::model::ConflictError("storage location is in use by an asset placement");
    repo.remove(location_id);
  }
  if (const auto bindings = binding_store(secret_store)) {
    if (bindings->preferred(location.project_id) == location_id)
      bindings->clear_preferred(location.project_id);
    bindings->unbind(location.project_id, location_id);
  }
  return {true, {std::move(location_id), std::nullopt, std::nullopt}};
}

LocationActionResponse put_locations_location_id_binding(holder::platform::Db& db,
                                                         holder::privacy::SecretStore* secret_store,
                                                         std::string location_id,
                                                         LocationBindingRequest request) {
  auto bindings = require_bindings(secret_store);
  const auto location = find_location(db, location_id);
  holder::resource::LocationBinding binding;
  binding.provider = location.provider;
  binding.values = std::move(request.values);
  const auto preview = request.preview.value_or("Configured");
  bindings.bind(location.project_id, location_id, binding, preview, support::now_epoch_seconds());
  return {true, {std::move(location_id), true, std::optional<std::string>(preview)}};
}

LocationActionResponse delete_locations_location_id_binding(
    holder::platform::Db& db,
    holder::privacy::SecretStore* secret_store,
    std::string location_id) {
  auto bindings = require_bindings(secret_store);
  const auto location = find_location(db, location_id);
  bindings.unbind(location.project_id, location_id);
  if (bindings.preferred(location.project_id) == location_id)
    bindings.clear_preferred(location.project_id);
  return {true, {std::move(location_id), std::nullopt, std::nullopt}};
}

LocationTestResponse post_locations_location_id_test(holder::platform::Db& db,
                                                     holder::privacy::SecretStore* secret_store,
                                                     const std::function<std::string()>& uuid_v4,
                                                     std::string location_id) {
  auto bindings = require_bindings(secret_store);
  const auto location = find_location(db, location_id);
  const auto binding = bindings.get(location.project_id, location_id);
  if (!binding) throw support::StorageBindingRequired("storage location configuration required");
  try {
    probe(*support::storage_provider(location, *binding), location, uuid_v4);
  } catch (const holder::resource::StorageError& error) {
    support::throw_storage_failure(error);
  }
  return {true, {true}};
}

LocationActionResponse put_locations_preferred(holder::platform::Db& db,
                                               holder::privacy::SecretStore* secret_store,
                                               PutLocationsPreferredRequest request) {
  auto bindings = require_bindings(secret_store);
  const auto location = holder::resource::LocationRepo(db).get(request.location_id);
  if (!location || location->project_id != request.project_id)
    throw LocationNotFound("location not found in project");
  bindings.set_preferred(request.project_id, request.location_id, support::now_epoch_seconds());
  return {true, {std::move(request.location_id), std::nullopt, std::nullopt}};
}

} // namespace holder::api
