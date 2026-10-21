#pragma once

#include "api/ApiService.h"
#include "api/HttpError.h"
#include "api/RigAnnotations.h"
#include "api/declared/Schemas.h"

#include <string>

namespace holder::api {

// Declared for the contract; the existing route handlers still write these responses.

RIG_ROUTE(DELETE, "/resources/{resource_id}")
RIG_SUMMARY("Delete a resource and remove its Card links")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
ResourceDeleteResponse delete_resources_resource_id(std::string resource_id);

} // namespace holder::api
