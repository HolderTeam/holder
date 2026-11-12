#pragma once

#include "api/AiRuntimeTypes.h"
#include "api/ApiService.h"

namespace holder::api {

struct AliveResponse {
  bool ok;
};

inline AliveResponse check_alive() { return {true}; }

} // namespace holder::api
