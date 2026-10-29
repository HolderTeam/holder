#pragma once

#include "api/ApiService.h"
#include "api/RigAnnotations.h"

namespace holder::api {

struct RIG_OPEN_RECORD AliveResponse {
  bool ok RIG_BOOL_VALUE(true);
};

inline AliveResponse check_alive() { return {true}; }

} // namespace holder::api
