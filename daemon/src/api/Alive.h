#pragma once

#include "api/ApiService.h"
#include "api/AiRuntimeTypes.h"

namespace holder::api {

struct AliveResponse {
  bool ok;
};

inline AliveResponse check_alive() { return {true}; }

} // namespace holder::api
