#pragma once

#include "api/ApiService.h"
#include "api/HttpError.h"
#include "api/RigAnnotations.h"

#include <string>

namespace holder::api {

// routes::handle_static_routes serves these files; the declarations are their contract.

RIG_ROUTE(GET, "/openapi.yaml")
RIG_PUBLIC
RIG_SUMMARY("Download the OpenAPI specification")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/yaml")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found")
std::string download_openapi_yaml();

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

} // namespace holder::api
