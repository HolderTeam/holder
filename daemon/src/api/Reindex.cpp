#include "api/Reindex.h"

#include "index/Reindexer.h"

namespace holder::api {

ReindexResponse post_reindex(holder::platform::Db& db) {
  holder::index::Reindexer(db).run();
  return {true, {"Reindex complete."}};
}

} // namespace holder::api
