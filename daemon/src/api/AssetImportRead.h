#pragma once

#include "api/RigAnnotations.h"
#include "api/declared/Schemas.h"

#include <stdexcept>
#include <string>

namespace holder::api {

struct AssetImportJobNotFound : std::runtime_error {
  AssetImportJobNotFound() : std::runtime_error("Import job not found.") {}
};

RIG_ROUTE(GET, "/imports/{job_id}")
RIG_SUMMARY("Poll an asset import job")
RIG_DESCRIPTION("Jobs are held in memory, so a job queued before the daemon restarted is not found.")
RIG_RESPONSE_DESCRIPTION("Current job state")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Job not found", AssetImportJobNotFound)
AssetImportJobResponse get_import_job(std::string job_id);

} // namespace holder::api
