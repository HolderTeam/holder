#include "api/LocationRead.h"

#include "capi/Context.h"
#include "index/FtsIndexer.h"

#include <string>

namespace holder::api {

StorageLocationListResponse get_locations(holder::platform::Db& db,
                                          holder::privacy::SecretStore* secret_store,
                                          std::string project_id) {
  // The listing is core's contract operation; the daemon adds its response envelope.
  holder::index::FtsIndexer fts(db);
  holder_context core(db, fts);
  core.secret_store = secret_store;
  auto listed = holder::contract::list_storage_locations(core, std::move(project_id));
  return {true, std::move(listed.data), std::move(listed.preferred_location_id)};
}

} // namespace holder::api
