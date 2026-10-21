#pragma once

#include "api/ApiService.h"
#include "api/HttpError.h"
#include "api/RigAnnotations.h"
#include "api/declared/Schemas.h"


namespace holder::api {

// Declared for the contract; the existing route handlers still write these responses.

RIG_ROUTE(POST, "/reindex")
RIG_SUMMARY("Rebuild search indexes from database state")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request")
ReindexResponse post_reindex();

} // namespace holder::api
