#include "api/LocationWrite.h"

#include "api/support/StorageProviders.h"
#include "api/support/Time.h"
#include "platform/Paths.h"
#include "resource/AssetEnvelope.h"
#include "resource/LocationBindingStore.h"
#include "resource/LocationRepo.h"

#include <filesystem>
#include <fstream>
#include <memory>
#include <utility>

namespace holder::api {
namespace {

holder::resource::LocationBindingStore require_bindings(holder::privacy::SecretStore* secret_store) {
  if (secret_store == nullptr) throw SecretStoreUnavailable{};
  return holder::resource::LocationBindingStore(*secret_store);
}

holder::model::Location find_location(holder::platform::Db& db, const std::string& location_id) {
  auto location = holder::resource::LocationRepo(db).get(location_id);
  if (!location) throw LocationNotFound{};
  return std::move(*location);
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
