#pragma once

#include "api/ApiService.h"
#include "api/AiRuntimeTypes.h"

#include <string>

namespace holder::api {

inline std::string get_ping() { return "pong"; }

} // namespace holder::api
