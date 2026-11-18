#pragma once

#include "api/AiRuntimeTypes.h"
#include "api/HttpError.h"
#include "api/LocationRead.h"
#include "api/Schemas.h"
#include "api/generated/DaemonTypes.h"
#include "api/support/StorageErrors.h"

#include <functional>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>

namespace holder::platform {
class Db;
}
namespace holder::privacy {
class SecretStore;
}

namespace holder::api {

// Preserve the runtime's existing nested-type spelling; its fields are generated.
using LocationActionResponseData = LocationAction;

struct LocationTestResponseData {
  bool available;
};

struct LocationTestResponse {
  bool ok;
  LocationTestResponseData data;
};

struct PutLocationsPreferredRequest {
  std::string project_id;
  std::string location_id;
};

LocationActionResponse delete_locations_location_id_binding(
    holder::platform::Db& db,
    holder::privacy::SecretStore* secret_store,
    std::string location_id
);

LocationTestResponse post_locations_location_id_test(
    holder::platform::Db& db,
    holder::privacy::SecretStore* secret_store,
    const std::function<std::string()>& uuid_v4,
    const std::string& location_id
);

LocationActionResponse put_locations_preferred(
    holder::platform::Db& db,
    holder::privacy::SecretStore* secret_store,
    PutLocationsPreferredRequest request
);

} // namespace holder::api
