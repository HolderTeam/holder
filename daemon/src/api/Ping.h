#pragma once

#include "api/AiRuntimeTypes.h"
#include "api/ApiService.h"

#include <string>

namespace holder::api {

inline std::string get_ping() { return "pong"; }

} // namespace holder::api
