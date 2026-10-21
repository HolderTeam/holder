#pragma once

#include "api/RigAnnotations.h"
#include "api/declared/Schemas.h"

#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace holder::platform { class Db; }
namespace holder::privacy { class SecretStore; }
namespace holder::resource { class LocationBindingStore; }
namespace holder::model { struct Location; }

namespace holder::api {

// A location as every location response reports it; unbound without a binding store.
StorageLocation location_item(const holder::model::Location& location,
                              const holder::resource::LocationBindingStore* bindings);

struct RIG_OPEN_RECORD StorageLocationListResponse {
  bool ok RIG_BOOL_VALUE(true);
  std::vector<StorageLocation> data;
  std::optional<std::string> preferred_location_id RIG_NULLABLE;
};

struct LocationNotFound : std::runtime_error {
  LocationNotFound() : std::runtime_error("location not found") {}
};

RIG_ROUTE(GET, "/locations")
RIG_SUMMARY("List project storage locations")
RIG_RESPONSE_DESCRIPTION("OK")
StorageLocationListResponse get_locations(holder::platform::Db& db RIG_CONTEXT,
                                          holder::privacy::SecretStore* secret_store RIG_CONTEXT,
                                          std::string project_id RIG_QUERY RIG_MIN_LENGTH(1));

RIG_ROUTE(GET, "/locations/{location_id}")
RIG_SUMMARY("Get a storage location")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found", LocationNotFound)
StorageLocationResponse get_location(holder::platform::Db& db RIG_CONTEXT,
                                     holder::privacy::SecretStore* secret_store RIG_CONTEXT,
                                     std::string location_id);

} // namespace holder::api
