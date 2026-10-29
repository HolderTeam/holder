#pragma once

#include "api/ApiService.h"
#include "api/HttpError.h"
#include "api/AiRuntimeTypes.h"

#include <nlohmann/json.hpp>

#include <stdexcept>
#include <string>

namespace holder::api {

struct StaticFileNotFound : std::runtime_error {
  using std::runtime_error::runtime_error;
};

// Both served from the bytes of api/openapi.yaml and api/openapi.json compiled into the daemon,
// so they are always present and always the contract this daemon was built from.
std::string download_openapi_yaml();

nlohmann::json download_openapi_json();

// The catalogs are found at run time: see support::find_ai_catalog_path and
// support::find_git_providers_path.
std::string download_ai_catalog_yaml();

std::string download_git_providers_yaml();

nlohmann::json download_ai_catalog_json();

nlohmann::json download_git_providers_json();

// Swagger UI, served from the daemon's documentation assets. Each file has its own media
// type, so the route's support::StreamServer writes the response.
api_runtime::host_value get_docs();

api_runtime::host_value get_docs_file(std::string file);

} // namespace holder::api
