#include "app/Bootstrap.h"

#include "platform/Paths.h"
#include "project/ProjectRegistry.h"
#include "project/DefaultProject.h"
#include "project/ProjectRepo.h"
#include "project/StartupRecovery.h"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <spdlog/spdlog.h>

#include <filesystem>

namespace holder::app {

std::string generate_uuid_v4() {
  boost::uuids::random_generator gen;
  return boost::uuids::to_string(gen());
}

void recover_existing_projects(
    holder::platform::Db& db,
    holder::index::FtsIndexer* fts,
    const holder::core::Paths& paths,
    bool require_durable_manifest
) {
  if (!holder::project::ProjectRepo(db).list().empty()) return;
  const auto roots = holder::core::ProjectRegistry(paths.project_registry_path())
                         .discover_roots(holder::core::default_projects_root());
  holder::project::recover_project_roots(
      db,
      fts,
      roots,
      generate_uuid_v4,
      require_durable_manifest
  );
}

std::optional<holder::model::Project> bootstrap_default_home_project(
    holder::platform::Db& db,
    holder::index::FtsIndexer* fts
) {
  const auto home = holder::project::ensure_default_project(
      db, generate_uuid_v4, holder::core::default_projects_root(), fts
  );
  if (home.has_value()) {
    spdlog::info("Bootstrapped default Home project ({})", home->project_id);
  }
  return home;
}

} // namespace holder::app
