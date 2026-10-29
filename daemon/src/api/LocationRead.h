#pragma once

#include "api/RigAnnotations.h"
#include "api/Schemas.h"

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
  explicit LocationNotFound(const std::string& message = "location not found")
      : std::runtime_error(message) {}
};

StorageLocationListResponse get_locations(holder::platform::Db& db RIG_CONTEXT,
                                          holder::privacy::SecretStore* secret_store RIG_CONTEXT,
                                          std::string project_id RIG_QUERY RIG_MIN_LENGTH(1));

} // namespace holder::api
