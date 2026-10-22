#pragma once

#include "api/ApiService.h"
#include "api/HttpError.h"
#include "api/RigAnnotations.h"

#include <nlohmann/json.hpp>

#include <stdexcept>
#include <string>

namespace holder::api {

struct StaticFileNotFound : std::runtime_error {
  using std::runtime_error::runtime_error;
};

// Both served from the bytes of api/openapi.yaml and api/openapi.json compiled into the daemon,
// so they are always present and always the contract this daemon was built from.
RIG_ROUTE(GET, "/openapi.yaml")
RIG_PUBLIC
RIG_SUMMARY("Download the OpenAPI specification")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_RESPONSE_MEDIA_TYPE("application/yaml")
std::string download_openapi_yaml();

RIG_ROUTE(GET, "/openapi.json")
RIG_PUBLIC
RIG_SUMMARY("Download the OpenAPI specification as JSON")
RIG_RESPONSE_DESCRIPTION("OK")
nlohmann::json download_openapi_json();

// The catalogs are found at run time: see support::find_ai_catalog_path and
// support::find_git_providers_path.
RIG_ROUTE(GET, "/ai_catalog.yaml")
RIG_PUBLIC
RIG_SUMMARY("Download canonical AI catalog config (models + cloud)")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_RESPONSE_MEDIA_TYPE("application/yaml")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found", StaticFileNotFound)
std::string download_ai_catalog_yaml();

RIG_ROUTE(GET, "/git_providers.yaml")
RIG_PUBLIC
RIG_SUMMARY("Download git provider UI/runtime config")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_RESPONSE_MEDIA_TYPE("application/yaml")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found", StaticFileNotFound)
std::string download_git_providers_yaml();

RIG_ROUTE(GET, "/ai_catalog.json")
RIG_PUBLIC
RIG_SUMMARY("Download canonical AI catalog config as JSON")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found", StaticFileNotFound)
nlohmann::json download_ai_catalog_json();

RIG_ROUTE(GET, "/git_providers.json")
RIG_PUBLIC
RIG_SUMMARY("Download git provider UI/runtime config as JSON")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found", StaticFileNotFound)
nlohmann::json download_git_providers_json();

// Swagger UI, served from the daemon's documentation assets. Each file has its own media
// type, so the route's support::StreamServer writes the response.
RIG_ROUTE(GET, "/docs")
RIG_PUBLIC
RIG_SUMMARY("Browse the API documentation")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("text/html")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Documentation assets not installed", StaticFileNotFound)
rig::host_value get_docs();

RIG_ROUTE(GET, "/docs/{file}")
RIG_PUBLIC
RIG_SUMMARY("Download one of the API documentation's files")
RIG_RESPONSE_DESCRIPTION("OK, with the file's media type")
RIG_HOST_RESPONSE("*/*")
RIG_FORMAT("binary")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found", StaticFileNotFound)
rig::host_value get_docs_file(std::string file);

} // namespace holder::api
