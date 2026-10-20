#pragma once

#include "api/RigAnnotations.h"

namespace holder::api {

struct RIG_API("Holder API", "0.1.0")
    RIG_SERVER("/")
    RIG_SERVER("http://127.0.0.1:11499")
    RIG_BEARER_AUTH("bearerAuth", "token") ApiService {};

} // namespace holder::api
