#pragma once

#include "api/HttpError.h"
#include "api/RigAnnotations.h"

namespace holder::api {

struct RIG_API("Holder API", "0.1.0")
    RIG_SERVER("/")
    RIG_SERVER("http://127.0.0.1:11499")
    RIG_BEARER_AUTH("bearerAuth", "token")
    RIG_UNAUTHORIZED(ErrorResponse, "Unauthorized")
    RIG_INTERNAL_ERROR(ErrorResponse, "Internal server error")
    RIG_VALIDATION_ERROR(ErrorResponse, "Bad request")
    RIG_METHOD_NOT_ALLOWED(ErrorResponse, "Method not allowed") ApiService {};

} // namespace holder::api
