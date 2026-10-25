#pragma once

#include "api/ApiService.h"
#include "api/RigAnnotations.h"

namespace holder::api {

struct RIG_OPEN_RECORD AliveResponse {
  bool ok RIG_BOOL_VALUE(true);
};

RIG_ROUTE(GET, "/alive")
RIG_PUBLIC
RIG_SUMMARY("Liveness check returning JSON, unauthenticated")
RIG_RESPONSE_DESCRIPTION("OK")
inline AliveResponse check_alive() { return {true}; }

} // namespace holder::api
