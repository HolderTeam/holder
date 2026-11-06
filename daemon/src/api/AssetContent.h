#pragma once

#include "api/HttpError.h"
#include "api/support/StorageErrors.h"
#include "api/support/StreamServer.h"

#include <exception>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace holder::git { class GitOps; }
namespace holder::platform { class Db; }
namespace holder::privacy { class SecretStore; }

namespace holder::api {

struct AssetContentNotFound : std::runtime_error {
  using std::runtime_error::runtime_error;
};

// Recover the asset and return a callback that writes its bytes to the HTTP socket.
std::pair<int, support::StreamServer> get_resources_resource_id_assets_asset_id_content(
    holder::platform::Db& db,
    holder::privacy::SecretStore* secret_store,
    holder::git::GitOps* git_ops,
    const std::function<std::string()>& uuid_v4,
    std::string resource_id,
    std::string asset_id,
    std::optional<std::string> placement_id);

} // namespace holder::api
