#pragma once

#include "api/ApiService.h"
#include "api/HttpError.h"
#include "api/RigAnnotations.h"

#include <string>

namespace holder::api {

// Declared for the contract; the existing route handlers still write these responses.

struct RIG_OPEN_RECORD RIG_INLINE_RECORD ResourceDeleteResponseData {
  std::string resource_id;
};

struct RIG_OPEN_RECORD ResourceDeleteResponse {
  bool ok RIG_BOOL_VALUE(true);
  ResourceDeleteResponseData data;
};

RIG_ROUTE(DELETE, "/resources/{resource_id}")
RIG_SUMMARY("Delete a resource and remove its Card links")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
ResourceDeleteResponse delete_resources_resource_id(std::string resource_id);

} // namespace holder::api
