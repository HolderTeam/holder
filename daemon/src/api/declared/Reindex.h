#pragma once

#include "api/ApiService.h"
#include "api/HttpError.h"
#include "api/RigAnnotations.h"

#include <string>

namespace holder::api {

// Declared for the contract; the existing route handlers still write these responses.

struct RIG_OPEN_RECORD ReindexData {
  std::string message;
};

struct RIG_OPEN_RECORD ReindexResponse {
  bool ok RIG_BOOL_VALUE(true);
  ReindexData data;
};

RIG_ROUTE(POST, "/reindex")
RIG_SUMMARY("Rebuild search indexes from database state")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request")
ReindexResponse post_reindex();

} // namespace holder::api
