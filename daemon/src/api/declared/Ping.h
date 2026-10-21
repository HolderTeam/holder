#pragma once

#include "api/ApiService.h"
#include "api/HttpError.h"
#include "api/RigAnnotations.h"
#include "api/declared/Schemas.h"

#include <string>

namespace holder::api {

// Declared for the contract; the existing route handlers still write these responses.

RIG_ROUTE(GET, "/ping")
RIG_PUBLIC
RIG_SUMMARY("Liveness check, unauthenticated")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_STRING_VALUES("pong")
RIG_HOST_RESPONSE("text/plain")
std::string get_ping();

} // namespace holder::api
