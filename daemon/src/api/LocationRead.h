#pragma once

#include "api/AiRuntimeTypes.h"
#include "api/Contract.h"
#include "api/Schemas.h"

#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace holder::platform { class Db; }
namespace holder::privacy { class SecretStore; }

namespace holder::api {

struct StorageLocationListResponse {
  bool ok;
  std::vector<holder::contract::ProjectStorageLocation> data;
  std::optional<std::string> preferred_location_id;
};

struct LocationNotFound : std::runtime_error {
  explicit LocationNotFound(const std::string& message = "location not found")
      : std::runtime_error(message) {}
};

StorageLocationListResponse get_locations(holder::platform::Db& db,
                                          holder::privacy::SecretStore* secret_store,
                                          std::string project_id);

} // namespace holder::api
