#pragma once

#include "api/HttpError.h"
#include "api/RigAnnotations.h"
#include "api/support/StorageErrors.h"

#include <exception>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>

namespace holder::git { class GitOps; }
namespace holder::platform { class Db; }
namespace holder::privacy { class SecretStore; }

namespace holder::api {

struct AssetContentNotFound : std::runtime_error {
  using std::runtime_error::runtime_error;
};

// Generated dispatch looks the asset up and recovers it from storage; the daemon then
// writes the bytes, with the route's support::StreamServer.
RIG_ROUTE(GET, "/resources/{resource_id}/assets/{asset_id}/content")
RIG_SUMMARY("Stream verified asset content")
RIG_DESCRIPTION("Authenticated binary download. Content-Type preserves the asset media type and Content-Disposition contains a sanitized quoted original filename. Successful content is arbitrary bytes, not a JSON envelope; failures use ErrorResponse. Without placement_id the daemon selects a stored placement.")
RIG_RESPONSE_DESCRIPTION("Recovered plaintext asset")
RIG_HOST_RESPONSE("application/octet-stream")
RIG_FORMAT("binary")
RIG_RESPONSE_HEADER("Content-Type", "Stored asset media type.")
RIG_RESPONSE_HEADER("Content-Disposition", "Attachment disposition with sanitized quoted original filename.")
RIG_ERROR_RESPONSE(409, ErrorResponse, "Storage binding required or storage conflict", support::StorageConflict, support::StorageBindingRequired)
RIG_ERROR_RESPONSE(422, ErrorResponse, "Asset integrity verification failed", support::StorageIntegrityFailed)
RIG_ERROR_RESPONSE(502, ErrorResponse, "Storage authentication or permission failure", support::StorageGatewayFailure)
RIG_ERROR_RESPONSE(503, ErrorResponse, "Storage unavailable or transient failure", support::StorageUnavailable)
RIG_ERROR_RESPONSE(507, ErrorResponse, "Storage capacity exceeded", support::StorageCapacityExceeded)
RIG_ERROR_RESPONSE(404, ErrorResponse, "Resource, asset, or placement not found", AssetContentNotFound)
RIG_ERROR_RESPONSE(400, ErrorResponse, "Invalid configuration, missing key, or unclassified retrieval/integrity failure", support::StorageBadRequest, std::exception)
rig::host_value get_resources_resource_id_assets_asset_id_content(
    holder::platform::Db& db RIG_CONTEXT,
    holder::privacy::SecretStore* secret_store RIG_CONTEXT,
    holder::git::GitOps* git_ops RIG_CONTEXT,
    const std::function<std::string()>& uuid_v4 RIG_CONTEXT,
    std::string resource_id,
    std::string asset_id,
    std::optional<std::string> placement_id RIG_QUERY);

} // namespace holder::api
