#pragma once

#include "api/ApiService.h"
#include "api/HttpError.h"
#include "api/RigAnnotations.h"
#include "api/declared/Schemas.h"

#include <string>

namespace holder::api {

// Declared for the contract; the existing route handlers still write these responses.

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
