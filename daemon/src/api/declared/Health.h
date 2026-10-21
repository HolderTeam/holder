#pragma once

#include "api/ApiService.h"
#include "api/HttpError.h"
#include "api/RigAnnotations.h"
#include "api/declared/Schemas.h"


namespace holder::api {

// Declared for the contract; the existing route handlers still write these responses.

RIG_ROUTE(GET, "/health")
RIG_SUMMARY("Health check")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
HealthResponse get_health();

} // namespace holder::api
