#include "api/AssetImportRead.h"

#include "api/support/AssetImportJobs.h"

#include <optional>
#include <string>

namespace holder::api {
namespace {

std::optional<std::string> unless_empty(const std::string& value) {
  return value.empty() ? std::nullopt : std::optional<std::string>(value);
}

} // namespace

AssetImportJobResponse get_import_job(std::string job_id) {
  const auto job = support::find_import_job(job_id);
  if (!job) throw AssetImportJobNotFound{};
  return {true,
          {job->job_id,
           job->status,
           unless_empty(job->resource_id),
           unless_empty(job->asset_id),
           job->duplicate_reused,
           job->link_created,
           unless_empty(job->error)}};
}

} // namespace holder::api
