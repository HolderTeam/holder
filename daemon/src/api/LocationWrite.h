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

RIG_ROUTE(PUT, "/locations/{location_id}/binding")
RIG_SUMMARY("Bind private device credentials or path")
RIG_RESPONSE_DESCRIPTION("Bound; secret values are not returned")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request", std::invalid_argument)
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found", LocationNotFound)
RIG_ERROR_RESPONSE(503, ErrorResponse, "The daemon has no secret store", SecretStoreUnavailable)
LocationActionResponse put_locations_location_id_binding(
    holder::platform::Db& db RIG_CONTEXT,
    holder::privacy::SecretStore* secret_store RIG_CONTEXT,
    std::string location_id,
    LocationBindingRequest request);

RIG_ROUTE(DELETE, "/locations/{location_id}/binding")
RIG_SUMMARY("Remove this device's private binding")
RIG_DESCRIPTION("Also clears the preferred location if it was this one.")
RIG_RESPONSE_DESCRIPTION("Unbound")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found", LocationNotFound)
RIG_ERROR_RESPONSE(503, ErrorResponse, "The daemon has no secret store", SecretStoreUnavailable)
LocationActionResponse delete_locations_location_id_binding(
    holder::platform::Db& db RIG_CONTEXT,
    holder::privacy::SecretStore* secret_store RIG_CONTEXT,
    std::string location_id);

RIG_ROUTE(POST, "/locations/{location_id}/test")
RIG_SUMMARY("Upload, verify, and remove a storage probe")
RIG_RESPONSE_DESCRIPTION("Location available")
RIG_ERROR_RESPONSE(400, ErrorResponse, "The declaration or binding cannot make a provider", support::StorageBadRequest)
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found", LocationNotFound)
RIG_ERROR_RESPONSE(409, ErrorResponse, "The location has no binding on this device, or the storage reports a conflict", support::StorageBindingRequired, support::StorageConflict)
RIG_ERROR_RESPONSE(422, ErrorResponse, "The probe failed verification", support::StorageIntegrityFailed)
RIG_ERROR_RESPONSE(502, ErrorResponse, "The storage rejected the credentials or permissions", support::StorageGatewayFailure)
RIG_ERROR_RESPONSE(503, ErrorResponse, "The storage or the daemon's secret store is unavailable", support::StorageUnavailable, SecretStoreUnavailable)
RIG_ERROR_RESPONSE(507, ErrorResponse, "The storage is full", support::StorageCapacityExceeded)
LocationTestResponse post_locations_location_id_test(
    holder::platform::Db& db RIG_CONTEXT,
    holder::privacy::SecretStore* secret_store RIG_CONTEXT,
    const std::function<std::string()>& uuid_v4 RIG_CONTEXT,
    std::string location_id);

RIG_ROUTE(PUT, "/locations/preferred")
RIG_SUMMARY("Set this device's preferred project storage location")
RIG_RESPONSE_DESCRIPTION("Preferred location saved in the device keyring")
RIG_ERROR_RESPONSE(404, ErrorResponse, "No such location in the project", LocationNotFound)
RIG_ERROR_RESPONSE(503, ErrorResponse, "The daemon has no secret store", SecretStoreUnavailable)
LocationActionResponse put_locations_preferred(holder::platform::Db& db RIG_CONTEXT,
                                               holder::privacy::SecretStore* secret_store RIG_CONTEXT,
                                               PutLocationsPreferredRequest request);

} // namespace holder::api
