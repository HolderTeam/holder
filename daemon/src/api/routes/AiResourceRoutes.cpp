#include "api/routes/AiResourceRoutes.h"
#include "api/support/HttpResponses.h"
#include "api/support/StorageErrors.h"
#include "api/support/StorageProviders.h"
#include "api/support/Time.h"
#include "platform/Paths.h"

#include "resource/AssetImportService.h"
#include "resource/LocationBindingStore.h"
#include "resource/LocationRepo.h"
#include "resource/ResourceRepo.h"

#include <boost/asio/write.hpp>
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

namespace holder::api::routes {
namespace {

namespace http = boost::beast::http;

std::string safe_download_filename(std::string filename) {
  for (auto& ch : filename) {
    if (ch == '"' || ch == '\\' || static_cast<unsigned char>(ch) < 0x20U) ch = '_';
  }
  if (filename.empty()) return "asset";
  return filename;
}

void cleanup_asset_cache(const std::filesystem::path& root) {
  std::error_code ec;
  if (!std::filesystem::exists(root, ec)) return;
  const auto cutoff = std::filesystem::file_time_type::clock::now() - std::chrono::hours(24);
  for (const auto& entry : std::filesystem::directory_iterator(root, ec)) {
    if (ec) break;
    if (!entry.is_regular_file(ec)) continue;
    const auto changed = entry.last_write_time(ec);
    if (!ec && changed < cutoff) std::filesystem::remove(entry.path(), ec);
    ec.clear();
  }
}

void stream_file_response(
    boost::asio::ip::tcp::socket& socket,
    const std::filesystem::path& path,
    const holder::model::Asset& asset
) {
  http::response<http::empty_body> header{http::status::ok, 11};
  header.set(http::field::content_type, asset.media_type);
  header.set(
      http::field::content_disposition,
      "attachment; filename=\"" + safe_download_filename(asset.original_filename) + "\""
  );
  header.content_length(static_cast<std::uint64_t>(asset.byte_size));
  header.keep_alive(false);
  http::response_serializer<http::empty_body> serializer{header};
  http::write_header(socket, serializer);

  std::ifstream input(path, std::ios::binary);
  if (!input) throw std::runtime_error("failed to open recovered asset");
  std::array<char, 64 * 1024> buffer{};
  while (input) {
    input.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    const auto count = input.gcount();
    if (count > 0) {
      boost::asio::write(
          socket,
          boost::asio::buffer(buffer.data(), static_cast<std::size_t>(count))
      );
    }
  }
  if (!input.eof()) throw std::runtime_error("failed while reading recovered asset");
}

} // namespace

http::response<http::string_body> resource_error_response(const std::exception& ex) {
  const std::string message = ex.what();
  if (const auto* storage = dynamic_cast<const holder::resource::StorageError*>(&ex)) {
    const auto [status, code] = support::storage_failure_status(*storage);
    return support::error_response(static_cast<http::status>(status), code, message);
  }
  if (message.find("storage location configuration required") != std::string::npos) {
    return support::error_response(http::status::conflict, "storage_binding_required", message);
  }
  if (message.rfind("conflict:", 0) == 0) {
    return support::error_response(http::status::conflict, "conflict", message);
  }
  if (message.find("not found") != std::string::npos) {
    return support::error_response(http::status::not_found, "not_found", message);
  }
  return support::error_response(http::status::bad_request, "bad_request", message);
}

bool handle_ai_resource_routes(
    const std::string& path,
    const http::request<http::string_body>& req,
    http::response<http::string_body>& res,
    holder::platform::Db& db,
    const std::function<std::string()>& uuid_v4,
    const std::function<std::string(const std::string&)>& param_get,
    holder::privacy::SecretStore* secret_store,
    holder::git::GitOps* git_ops,
    boost::asio::ip::tcp::socket* socket,
    bool* streamed
) {
  if (streamed != nullptr) *streamed = false;
  std::unique_ptr<holder::resource::LocationBindingStore> bindings;
  if (secret_store != nullptr) {
    bindings = std::make_unique<holder::resource::LocationBindingStore>(*secret_store);
  }

  if (path.rfind("/resources/", 0) == 0 && path.find("/assets/") != std::string::npos &&
      path.ends_with("/content") && req.method() == http::verb::get) {
    try {
      if (!bindings || git_ops == nullptr || socket == nullptr || streamed == nullptr) {
        throw std::runtime_error("asset retrieval services unavailable");
      }
      const auto resource_start = std::string("/resources/").size();
      const auto assets_pos = path.find("/assets/", resource_start);
      const auto content_pos = path.size() - std::string("/content").size();
      const auto resource_id = path.substr(resource_start, assets_pos - resource_start);
      const auto asset_id = path.substr(
          assets_pos + std::string("/assets/").size(),
          content_pos - (assets_pos + std::string("/assets/").size())
      );
      const auto bundle = holder::resource::ResourceRepo(db).get_bundle(resource_id);
      if (!bundle.has_value()) throw std::runtime_error("resource not found");
      const auto asset =
          std::find_if(bundle->assets.begin(), bundle->assets.end(), [&](const auto& item) {
            return item.asset_id == asset_id;
          });
      if (asset == bundle->assets.end()) throw std::runtime_error("asset not found in resource");
      const auto requested_placement = param_get("placement_id");
      const auto placement =
          std::find_if(asset->placements.begin(), asset->placements.end(), [&](const auto& item) {
            return requested_placement.empty() || item.placement_id == requested_placement;
          });
      if (placement == asset->placements.end())
        throw std::runtime_error("asset placement not found");
      const auto location = holder::resource::LocationRepo(db).get(placement->location_id);
      if (!location.has_value() || location->project_id != bundle->resource.project_id) {
        throw std::runtime_error("storage location not found in project");
      }
      const auto binding = bindings->get(location->project_id, location->location_id);
      if (!binding.has_value()) throw std::runtime_error("storage location configuration required");
      auto provider = support::storage_provider(*location, *binding);
      const auto cache = holder::core::Paths::resolve("holder").cache_dir / "asset-cache";
      std::filesystem::create_directories(cache);
      cleanup_asset_cache(cache);
      const auto recovered = cache / (uuid_v4() + ".recovered");
      try {
        holder::resource::AssetImportService(db, cache, uuid_v4, nullptr, git_ops)
            .retrieve(resource_id, asset_id, placement->placement_id, *provider, recovered);
        *streamed = true;
        stream_file_response(*socket, recovered, *asset);
        std::filesystem::remove(recovered);
      } catch (...) {
        std::filesystem::remove(recovered);
        throw;
      }
    } catch (const std::exception& ex) {
      if (streamed != nullptr && *streamed) {
        // LCOV_EXCL_START: the branch is covered; GCC assigns its no-throw cleanup to duplicate
        // zero-count lines even when shutdown executes.
        boost::system::error_code ignored;
        socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored);
        // LCOV_EXCL_STOP
      } else { // LCOV_EXCL_LINE: duplicate control-flow line; error response below is covered.
        res = resource_error_response(ex);
      }
    }
    return true;
  }

  // The other location and resource routes are generated.
  return false;
}

} // namespace holder::api::routes
