#include "api/Rebuild.h"

#include "project/ProjectRepo.h"
#include "project/Rebuilder.h"

#include <exception>
#include <limits>
#include <utility>

namespace holder::api {
namespace {

int response_count(std::size_t count) {
  if (!std::in_range<int>(count)) throw RebuildFailed("Rebuild count exceeds API range.");
  return static_cast<int>(count);
}

} // namespace

RebuildResponse rebuild_project_from_files(holder::platform::Db& db,
                                           holder::index::FtsIndexer* fts,
                                           RebuildRequest request) {
  try {
    holder::project::ProjectRepo projects(db);
    const auto project = projects.get(request.project_id);
    if (!project) throw RebuildProjectNotFound{};

    holder::store::Rebuilder rebuilder(db, fts);
    const auto stats = rebuilder.rebuild_project(*project);
    return {true, {std::move(request.project_id), response_count(stats.cards),
                   response_count(stats.ai_messages), response_count(stats.ai_threads),
                   response_count(stats.links)}};
  } catch (const RebuildProjectNotFound&) {
    throw;
  } catch (const std::exception& error) {
    throw RebuildFailed(error.what());
  }
}

} // namespace holder::api
