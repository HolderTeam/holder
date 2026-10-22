#include "api/LocationRead.h"

#include "resource/LocationBindingStore.h"
#include "resource/LocationRepo.h"

#include <memory>
#include <optional>
#include <string>

namespace holder::api {
namespace {

std::unique_ptr<holder::resource::LocationBindingStore> binding_store(
    holder::privacy::SecretStore* secret_store
) {
  if (secret_store == nullptr) return nullptr;
  return std::make_unique<holder::resource::LocationBindingStore>(*secret_store);
}

} // namespace

StorageLocation location_item(const holder::model::Location& location,
                              const holder::resource::LocationBindingStore* bindings) {
  auto preview = bindings ? bindings->preview(location.project_id, location.location_id)
                          : std::nullopt;
  const bool bound = preview.has_value();
  return {
      location.location_id,
      location.project_id,
      location.name,
      location.provider,
      location.configuration,
      bound,
      std::move(preview),
      location.created_at,
      location.updated_at,
  };
}

StorageLocationListResponse get_locations(holder::platform::Db& db,
                                          holder::privacy::SecretStore* secret_store,
                                          std::string project_id) {
  const auto bindings = binding_store(secret_store);
  StorageLocationListResponse response{
      true, {}, bindings ? bindings->preferred(project_id) : std::nullopt};
  for (const auto& location : holder::resource::LocationRepo(db).list(project_id)) {
    response.data.push_back(location_item(location, bindings.get()));
  }
  return response;
}

StorageLocationResponse get_location(holder::platform::Db& db,
                                     holder::privacy::SecretStore* secret_store,
                                     std::string location_id) {
  const auto location = holder::resource::LocationRepo(db).get(location_id);
  if (!location) throw LocationNotFound{};
  const auto bindings = binding_store(secret_store);
  return {true, location_item(*location, bindings.get())};
}

} // namespace holder::api
