#include "api/ProjectDelete.h"

#include "platform/Paths.h"
#include "platform/ProjectRegistry.h"
#include "platform/Tx.h"
#include "project/ProjectRepo.h"
#include "project/ProjectSyncRepo.h"

#include <utility>

namespace holder::api {

ProjectDeleteResponse delete_project(holder::platform::Db& db, std::string project_id) {
  holder::platform::Tx transaction(db);
  holder::project::ProjectRepo repo(db);
  holder::project::ProjectSyncRepo sync_repo(db);
  const auto project = repo.get(project_id);
  if (!project) throw ProjectDeleteNotFound();

  repo.remove(project_id);
  sync_repo.remove(project_id);
  holder::core::ProjectRegistry registry(
      holder::core::Paths::resolve("holder").project_registry_path());
  registry.forget(*project);
  try {
    transaction.commit();
  } catch (...) {
    registry.restore(*project);
    throw;
  }
  return {true, {std::move(project_id)}};
}

} // namespace holder::api
