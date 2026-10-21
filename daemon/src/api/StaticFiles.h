#pragma once

#include "api/ApiService.h"
#include "api/HttpError.h"
#include "api/RigAnnotations.h"

#include <nlohmann/json.hpp>

#include <string>

namespace holder::api {

// routes::handle_static_routes serves these files; the declarations are their contract.

// Both served from the bytes of api/openapi.yaml and api/openapi.json compiled into the daemon,
// so they are always present and always the contract this daemon was built from.
RIG_ROUTE(GET, "/openapi.yaml")
RIG_PUBLIC
RIG_SUMMARY("Download the OpenAPI specification")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/yaml")
std::string download_openapi_yaml();

RIG_ROUTE(GET, "/openapi.json")
RIG_PUBLIC
RIG_SUMMARY("Download the OpenAPI specification as JSON")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
nlohmann::json download_openapi_json();

RIG_ROUTE(GET, "/ai_catalog.yaml")
RIG_PUBLIC
RIG_SUMMARY("Download canonical AI catalog config (models + cloud)")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/yaml")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found")
std::string download_ai_catalog_yaml();

RIG_ROUTE(GET, "/git_providers.yaml")
RIG_PUBLIC
RIG_SUMMARY("Download git provider UI/runtime config")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/yaml")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found")
std::string download_git_providers_yaml();

RIG_ROUTE(GET, "/ai_catalog.json")
RIG_PUBLIC
RIG_SUMMARY("Download canonical AI catalog config as JSON")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found")
nlohmann::json download_ai_catalog_json();

RIG_ROUTE(GET, "/git_providers.json")
RIG_PUBLIC
RIG_SUMMARY("Download git provider UI/runtime config as JSON")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found")
nlohmann::json download_git_providers_json();

} // namespace holder::api
