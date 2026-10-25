#pragma once

#include "api/RigAnnotations.h"

#include <string>

namespace holder::platform { class Db; }

namespace holder::api {

struct RIG_OPEN_RECORD ReindexData {
  std::string message;
};

struct RIG_OPEN_RECORD ReindexResponse {
  bool ok RIG_BOOL_VALUE(true);
  ReindexData data;
};

RIG_ROUTE(POST, "/reindex")
RIG_SUMMARY("Rebuild search indexes from database state")
RIG_RESPONSE_DESCRIPTION("OK")
ReindexResponse post_reindex(holder::platform::Db& db RIG_CONTEXT);

} // namespace holder::api
