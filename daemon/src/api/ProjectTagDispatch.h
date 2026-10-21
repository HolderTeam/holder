#pragma once

#include "api/Router.h"

namespace holder::platform { class Db; }

namespace holder::api {

bool dispatch_project_tags(const Router::Request& req, Router::Response& res,
                           holder::platform::Db& db);

} // namespace holder::api
