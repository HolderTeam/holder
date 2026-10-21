#pragma once

#include "api/ApiService.h"
#include "api/HttpError.h"
#include "api/RigAnnotations.h"
#include "api/declared/Schemas.h"

#include <optional>
#include <string>

namespace holder::api {

// Declared for the contract; the existing route handlers still write these responses.

RIG_ROUTE(GET, "/resources/{resource_id}/assets/{asset_id}/content")
RIG_SUMMARY("Stream verified asset content")
RIG_DESCRIPTION("Authenticated binary download. Content-Type preserves the asset media type and Content-Disposition contains a sanitized quoted original filename. Successful content is arbitrary bytes, not a JSON envelope; failures use ErrorResponse. Without placement_id the daemon selects a stored placement.")
RIG_RESPONSE_DESCRIPTION("Recovered plaintext asset")
RIG_HOST_RESPONSE("application/octet-stream")
// Keep the binary format: the pinned Python generator turns it into a File response.
RIG_FORMAT("binary")
RIG_RESPONSE_HEADER("Content-Type", "Stored asset media type.")
RIG_RESPONSE_HEADER("Content-Disposition", "Attachment disposition with sanitized quoted original filename.")
RIG_ERROR_RESPONSE(409, ErrorResponse, "Storage binding required or storage conflict")
RIG_ERROR_RESPONSE(422, ErrorResponse, "Asset integrity verification failed")
RIG_ERROR_RESPONSE(502, ErrorResponse, "Storage authentication or permission failure")
RIG_ERROR_RESPONSE(503, ErrorResponse, "Storage unavailable or transient failure")
RIG_ERROR_RESPONSE(507, ErrorResponse, "Storage capacity exceeded")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Invalid configuration, missing key, or unclassified retrieval/integrity failure")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Resource, asset, or placement not found")
std::string get_resources_resource_id_assets_asset_id_content(
    std::string resource_id,
    std::string asset_id,
    std::optional<std::string> placement_id RIG_QUERY);

} // namespace holder::api
