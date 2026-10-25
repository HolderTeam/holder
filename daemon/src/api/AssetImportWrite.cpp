#include "api/AssetImportWrite.h"

#include "api/support/AssetImportJobs.h"
#include "api/support/StorageProviders.h"
#include "api/support/Time.h"
#include "core/ActivityTracker.h"
#include "platform/Paths.h"
#include "resource/AssetImportService.h"
#include "resource/LocationBindingStore.h"
#include "resource/LocationRepo.h"

#include <filesystem>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

namespace holder::api {
namespace {

using support::ImportJob;
using support::update_import_job;

std::mutex import_threads_mutex;
std::vector<std::thread> import_threads;

// Runs one queued import to completion on its own database connection, recording each stage.
void run_import(const std::string& job_id,
                const holder::resource::AssetImportRequest& request,
                const holder::model::Location& location,
                const holder::resource::LocationBinding& binding,
                const std::filesystem::path& db_path,
                const std::filesystem::path& cache,
                const std::function<std::string()>& uuid_v4,
                holder::git::GitOps* git_ops) {
  try {
    holder::platform::Db job_db;
    job_db.open(db_path);
    auto provider = support::storage_provider(location, binding);
    holder::resource::AssetImportService importer(
        job_db, cache, uuid_v4, nullptr, git_ops, [&job_id](holder::resource::AssetImportStage stage) {
          switch (stage) {
          case holder::resource::AssetImportStage::Staging:
            update_import_job(job_id, "staging");
            break;
          case holder::resource::AssetImportStage::Storing:
            update_import_job(job_id, "storing");
            break;
          case holder::resource::AssetImportStage::Committing:
            update_import_job(job_id, "committing");
            break;
          }
        });
    const auto result = importer.import_file(request, *provider);
    support::complete_import_job(job_id, result.resource_id, result.asset_id,
                                 result.duplicate_reused, result.link_created);
  } catch (const std::exception& error) {
    update_import_job(job_id, "failed", error.what());
  } catch (...) { // LCOV_EXCL_LINE: defensive boundary for non-standard provider throws.
    update_import_job(job_id, "failed", "unknown asset import error"); // LCOV_EXCL_LINE
  } // LCOV_EXCL_LINE
}

} // namespace

void wait_for_asset_import_jobs() {
  std::vector<std::thread> threads;
  {
    std::lock_guard<std::mutex> lock(import_threads_mutex);
    threads.swap(import_threads);
  }
  for (auto& thread : threads) {
    if (thread.joinable()) thread.join();
  }
}

AssetImportJobResponse post_imports(holder::platform::Db& db,
                                    holder::privacy::SecretStore* secret_store,
                                    holder::git::GitOps* git_ops,
                                    const std::function<std::string()>& uuid_v4,
                                    AssetImportRequest body) {
  if (secret_store == nullptr) throw SecretStoreUnavailable{};
  if (git_ops == nullptr) throw AssetImportUnavailable{};
  holder::resource::AssetImportRequest request;
  request.project_id = std::move(body.project_id);
  request.card_id = std::move(body.card_id);
  request.location_id = std::move(body.location_id);
  request.source_file = std::move(body.source_path);
  request.now = support::now_epoch_seconds();
  if (!std::filesystem::is_regular_file(request.source_file))
    throw std::invalid_argument("asset source must be a readable regular file");
  const auto location = holder::resource::LocationRepo(db).get(request.location_id);
  if (!location || location->project_id != request.project_id)
    throw AssetImportLocationNotFound{};
  const auto binding =
      holder::resource::LocationBindingStore(*secret_store).get(request.project_id,
                                                                request.location_id);
  if (!binding) throw support::StorageBindingRequired("storage location configuration required");

  // Declarations and credentials are resolved before the job is queued. The job keeps neither
  // the HTTP body nor the local source path in durable project state.
  const auto job_id = uuid_v4();
  const ImportJob queued{.job_id = job_id,
                         .status = "queued",
                         .resource_id = {},
                         .asset_id = {},
                         .duplicate_reused = false,
                         .link_created = false,
                         .error = {}};
  support::put_import_job(queued);
  // The import outlives the request that queued it; it is work until it finishes.
  std::thread import_thread(
      [job_id, request, location = *location, binding = *binding, db_path = db.path(),
       cache = holder::core::Paths::resolve("holder").cache_dir / "asset-staging", uuid_v4,
       git_ops, activity_scope = holder::core::activity().begin()] {
        run_import(job_id, request, location, binding, db_path, cache, uuid_v4, git_ops);
      });
  {
    std::lock_guard<std::mutex> lock(import_threads_mutex);
    import_threads.push_back(std::move(import_thread));
  }
  return {true, {job_id, queued.status, std::nullopt, std::nullopt, false, false, std::nullopt}};
}

} // namespace holder::api
