// The contract's database rebuild and search reindex operations.
#include "api/Contract.h"

#include "capi/Context.h"
#include "index/Reindexer.h"
#include "project/ProjectRepo.h"
#include "project/Rebuilder.h"

#include <cstdint>
#include <exception>
#include <utility>

namespace holder::contract {
namespace {

std::int32_t response_count(std::size_t count) {
  if (!std::in_range<std::int32_t>(count)) throw RebuildFailed("rebuild count exceeds API range");
  return static_cast<std::int32_t>(count);
}

} // namespace

RebuildResult rebuild_project(holder_context& context, const RebuildRequest& request) {
  const auto project = holder::project::ProjectRepo(context.db).get(request.project_id);
  if (!project) throw NotFound("Project not found.");
  try {
    const auto stats = holder::store::Rebuilder(context.db, &context.fts).rebuild_project(*project);
    return {
        request.project_id,
        response_count(stats.cards),
        response_count(stats.ai_messages),
        response_count(stats.ai_threads),
        response_count(stats.links)
    };
  } catch (const RebuildFailed&) {
    throw;
  } catch (const std::exception& error) {
    throw RebuildFailed(error.what());
  }
}

ReindexResult reindex_search(holder_context& context) {
  holder::index::Reindexer(context.db).run();
  return {"Reindex complete."};
}

} // namespace holder::contract
