#pragma once

#include "api/Schemas.h"

#include <stdexcept>
#include <string>

namespace holder::api {

struct AssetImportJobNotFound : std::runtime_error {
  AssetImportJobNotFound() : std::runtime_error("Import job not found.") {}
};

AssetImportJobResponse get_import_job(std::string job_id);

} // namespace holder::api
