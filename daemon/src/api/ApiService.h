#pragma once

#include "api/HttpError.h"
#include "api/RigAnnotations.h"

namespace holder::api {

struct RIG_API("Holder API", "0.1.0")
    RIG_SERVER("/")
    RIG_SERVER("http://127.0.0.1:11499")
    RIG_BEARER_AUTH("bearerAuth", "token")
    RIG_UNAUTHORIZED(ErrorResponse, "Unauthorized")
    RIG_VALIDATION_ERROR(ErrorResponse, "Bad request") ApiService {};

} // namespace holder::api
