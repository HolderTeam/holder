#pragma once

#include <optional>
#include <string>

namespace holder::api::support {

// An asset import queued by POST /imports, held in memory for this process so it can be polled.
struct ImportJob {
  std::string job_id;
  std::string status = "queued";
  std::string resource_id;
  std::string asset_id;
  bool duplicate_reused = false;
  bool link_created = false;
  std::string error;
};

void put_import_job(const ImportJob& job);
void update_import_job(const std::string& job_id,
                       const std::string& status,
                       const std::string& error = {});
void complete_import_job(const std::string& job_id,
                         const std::string& resource_id,
                         const std::string& asset_id,
                         bool duplicate_reused,
                         bool link_created);
std::optional<ImportJob> find_import_job(const std::string& job_id);

} // namespace holder::api::support
