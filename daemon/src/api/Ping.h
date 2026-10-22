#pragma once

#include "api/ApiService.h"
#include "api/RigAnnotations.h"

#include <string>

namespace holder::api {

RIG_ROUTE(GET, "/ping")
RIG_PUBLIC
RIG_SUMMARY("Liveness check, unauthenticated")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_STRING_VALUES("pong")
RIG_RESPONSE_MEDIA_TYPE("text/plain")
inline std::string get_ping() { return "pong"; }

} // namespace holder::api
