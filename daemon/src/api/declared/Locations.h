#pragma once

#include "api/ApiService.h"
#include "api/HttpError.h"
#include "api/RigAnnotations.h"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace holder::api {

// Declared for the contract; the existing route handlers still write these responses.

struct RIG_OPEN_RECORD StorageLocation {
  std::string location_id;
  std::string project_id;
  std::string name;
  std::string provider RIG_STRING_VALUES("local_directory", "s3_compatible");
  std::map<std::string, std::string> configuration RIG_FIELD_DESCRIPTION("Portable non-secret provider configuration.");
  bool bound;
  std::optional<std::optional<std::string>> binding_preview RIG_FIELD_DESCRIPTION("Safe display text; credentials are never returned.") RIG_OMIT_EMPTY RIG_NULLABLE;
  long long created_at;
  long long updated_at;
};

struct RIG_OPEN_RECORD StorageLocationListResponse {
  bool ok RIG_BOOL_VALUE(true);
  std::vector<StorageLocation> data;
  std::optional<std::string> preferred_location_id RIG_NULLABLE;
};

struct RIG_OPEN_RECORD StorageLocationResponse {
  bool ok RIG_BOOL_VALUE(true);
  StorageLocation data;
};

struct RIG_OPEN_RECORD StorageLocationRequest {
  std::optional<std::string> location_id RIG_OMIT_EMPTY;
  std::string project_id;
  std::string name;
  std::string provider RIG_STRING_VALUES("local_directory", "s3_compatible");
  std::optional<std::map<std::string, std::string>> configuration RIG_OMIT_EMPTY;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD PatchLocationsLocationIdRequest {
  std::optional<std::string> name RIG_OMIT_EMPTY;
  std::optional<std::map<std::string, std::string>> configuration RIG_OMIT_EMPTY;
};

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
  std::optional<std::string> preview RIG_OMIT_EMPTY;
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

RIG_ROUTE(GET, "/locations")
RIG_SUMMARY("List project storage locations")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
StorageLocationListResponse get_locations(std::string project_id RIG_QUERY);

RIG_ROUTE(POST, "/locations")
RIG_SUMMARY("Create a storage location declaration")
RIG_RESPONSE_DESCRIPTION("Created")
RIG_HOST_RESPONSE("application/json")
RIG_SUCCESS_STATUS(201)
StorageLocationResponse post_locations(StorageLocationRequest request);

RIG_ROUTE(GET, "/locations/{location_id}")
RIG_SUMMARY("Get a storage location")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
StorageLocationResponse get_locations_location_id(std::string location_id);

RIG_ROUTE(PATCH, "/locations/{location_id}")
RIG_SUMMARY("Update a storage location declaration")
RIG_RESPONSE_DESCRIPTION("Updated")
RIG_HOST_RESPONSE("application/json")
StorageLocationResponse patch_locations_location_id(
    std::string location_id,
    PatchLocationsLocationIdRequest request);

RIG_ROUTE(DELETE, "/locations/{location_id}")
RIG_SUMMARY("Delete an unused storage location declaration")
RIG_RESPONSE_DESCRIPTION("Deleted")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(409, ErrorResponse, "A live placement still uses the location")
LocationActionResponse delete_locations_location_id(std::string location_id);

RIG_ROUTE(PUT, "/locations/{location_id}/binding")
RIG_SUMMARY("Bind private device credentials or path")
RIG_RESPONSE_DESCRIPTION("Bound; secret values are not returned")
RIG_HOST_RESPONSE("application/json")
LocationActionResponse put_locations_location_id_binding(
    std::string location_id,
    LocationBindingRequest request);

RIG_ROUTE(DELETE, "/locations/{location_id}/binding")
RIG_SUMMARY("Remove this device's private binding")
RIG_RESPONSE_DESCRIPTION("Unbound")
RIG_HOST_RESPONSE("application/json")
LocationActionResponse delete_locations_location_id_binding(std::string location_id);

RIG_ROUTE(POST, "/locations/{location_id}/test")
RIG_SUMMARY("Upload, verify, and remove a storage probe")
RIG_RESPONSE_DESCRIPTION("Location available")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Missing binding or storage failure")
LocationTestResponse post_locations_location_id_test(std::string location_id);

RIG_ROUTE(PUT, "/locations/preferred")
RIG_SUMMARY("Set this device's preferred project storage location")
RIG_RESPONSE_DESCRIPTION("Preferred location saved in the device keyring")
RIG_HOST_RESPONSE("application/json")
LocationActionResponse put_locations_preferred(PutLocationsPreferredRequest request);

} // namespace holder::api
