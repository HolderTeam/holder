#pragma once

#include "api/HttpError.h"
#include "api/LocationRead.h"
#include "api/RigAnnotations.h"
#include "api/Schemas.h"
#include "api/support/StorageErrors.h"

#include <functional>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>

namespace holder::platform { class Db; }
namespace holder::privacy { class SecretStore; }

namespace holder::api {

struct RIG_OPEN_RECORD RIG_INLINE_RECORD LocationActionResponseData {
  std::string location_id;
  std::optional<bool> bound RIG_OMIT_EMPTY;
  std::optional<std::optional<std::string>> binding_preview RIG_OMIT_EMPTY RIG_NULLABLE;
};

struct RIG_OPEN_RECORD LocationActionResponse {
  bool ok RIG_BOOL_VALUE(true);
  LocationActionResponseData data;
};

struct RIG_OPEN_RECORD LocationBindingRequest {
  std::map<std::string, std::string> values RIG_WRITE_ONLY;
  std::optional<std::string> preview RIG_FIELD_DESCRIPTION("Safe display text for the binding; defaults to Configured.") RIG_OMIT_EMPTY;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD LocationTestResponseData {
  bool available;
};

struct RIG_OPEN_RECORD LocationTestResponse {
  bool ok RIG_BOOL_VALUE(true);
  LocationTestResponseData data;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD PutLocationsPreferredRequest {
  std::string project_id;
  std::string location_id;
};

LocationActionResponse put_locations_location_id_binding(
    holder::platform::Db& db RIG_CONTEXT,
    holder::privacy::SecretStore* secret_store RIG_CONTEXT,
    std::string location_id,
    LocationBindingRequest request);

LocationActionResponse delete_locations_location_id_binding(
    holder::platform::Db& db RIG_CONTEXT,
    holder::privacy::SecretStore* secret_store RIG_CONTEXT,
    std::string location_id);

LocationTestResponse post_locations_location_id_test(
    holder::platform::Db& db RIG_CONTEXT,
    holder::privacy::SecretStore* secret_store RIG_CONTEXT,
    const std::function<std::string()>& uuid_v4 RIG_CONTEXT,
    std::string location_id);

LocationActionResponse put_locations_preferred(holder::platform::Db& db RIG_CONTEXT,
                                               holder::privacy::SecretStore* secret_store RIG_CONTEXT,
                                               PutLocationsPreferredRequest request);

} // namespace holder::api
