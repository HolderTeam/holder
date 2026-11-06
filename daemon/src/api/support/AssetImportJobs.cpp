#include "api/support/AssetImportJobs.h"

#include <mutex>
#include <unordered_map>

namespace holder::api::support {
namespace {

std::mutex import_jobs_mutex;
std::unordered_map<std::string, ImportJob> import_jobs;

} // namespace

void put_import_job(const ImportJob& job) {
  std::lock_guard<std::mutex> lock(import_jobs_mutex);
  import_jobs[job.job_id] = job;
}

void update_import_job(const std::string& job_id,
                       const std::string& status,
                       const std::string& error) {
  std::lock_guard<std::mutex> lock(import_jobs_mutex);
  auto found = import_jobs.find(job_id);
  if (found == import_jobs.end()) return;
  found->second.status = status;
  found->second.error = error;
}

void complete_import_job(const std::string& job_id,
                         const std::string& resource_id,
                         const std::string& asset_id,
                         bool duplicate_reused,
                         bool link_created) {
  std::lock_guard<std::mutex> lock(import_jobs_mutex);
  auto found = import_jobs.find(job_id);
  if (found == import_jobs.end()) return;
  found->second.status = "completed";
  found->second.resource_id = resource_id;
  found->second.asset_id = asset_id;
  found->second.duplicate_reused = duplicate_reused;
  found->second.link_created = link_created;
}

std::optional<ImportJob> find_import_job(const std::string& job_id) {
  std::lock_guard<std::mutex> lock(import_jobs_mutex);
  const auto found = import_jobs.find(job_id);
  if (found == import_jobs.end()) return std::nullopt;
  return found->second;
}

} // namespace holder::api::support
