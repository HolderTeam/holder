#include "api/AssetContent.h"

#include "api/support/StorageProviders.h"
#include "api/support/StreamServer.h"
#include "platform/Paths.h"
#include "resource/AssetImportService.h"
#include "resource/LocationBindingStore.h"
#include "resource/LocationRepo.h"
#include "resource/ResourceRepo.h"

#include <boost/asio/write.hpp>
#include <boost/beast/http.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>

namespace holder::api {
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

std::pair<int, support::StreamServer> get_resources_resource_id_assets_asset_id_content(
    holder::platform::Db& db,
    holder::privacy::SecretStore* secret_store,
    holder::git::GitOps* git_ops,
    const std::function<std::string()>& uuid_v4,
    std::string resource_id,
    std::string asset_id,
    std::optional<std::string> placement_id
) {
  if (secret_store == nullptr || git_ops == nullptr) {
    throw std::runtime_error("asset retrieval services unavailable");
  }
  const auto bundle = holder::resource::ResourceRepo(db).get_bundle(resource_id);
  if (!bundle.has_value()) throw AssetContentNotFound("resource not found");
  const auto asset =
      std::find_if(bundle->assets.begin(), bundle->assets.end(), [&](const auto& item) {
        return item.asset_id == asset_id;
      });
  if (asset == bundle->assets.end()) throw AssetContentNotFound("asset not found in resource");
  const auto requested_placement = placement_id.value_or("");
  const auto placement =
      std::find_if(asset->placements.begin(), asset->placements.end(), [&](const auto& item) {
        return requested_placement.empty() || item.placement_id == requested_placement;
      });
  if (placement == asset->placements.end()) {
    throw AssetContentNotFound("asset placement not found");
  }
  const auto location = holder::resource::LocationRepo(db).get(placement->location_id);
  if (!location.has_value() || location->project_id != bundle->resource.project_id) {
    throw AssetContentNotFound("storage location not found in project");
  }
  const auto binding = holder::resource::LocationBindingStore(*secret_store)
                           .get(location->project_id, location->location_id);
  if (!binding.has_value()) {
    throw support::StorageBindingRequired("storage location configuration required");
  }
  auto provider = support::storage_provider(*location, *binding);
  const auto cache = holder::core::Paths::resolve("holder").cache_dir / "asset-cache";
  std::filesystem::create_directories(cache);
  cleanup_asset_cache(cache);
  const auto recovered = cache / (uuid_v4() + ".recovered");
  try {
    holder::resource::AssetImportService(db, cache, uuid_v4, nullptr, git_ops)
        .retrieve(resource_id, asset_id, placement->placement_id, *provider, recovered);
  } catch (const holder::resource::StorageError& error) {
    std::filesystem::remove(recovered);
    support::throw_storage_failure(error);
  } catch (...) {
    std::filesystem::remove(recovered);
    throw;
  }
  // Once the headers are written there is no other response to send: a failure closes it.
  return {
      200,
      support::StreamServer([recovered, asset = *asset](
                                boost::asio::ip::tcp::socket& socket,
                                http::response<http::string_body>&
                            ) {
        try {
          stream_file_response(socket, recovered, asset);
        } catch (const std::exception&) {
          boost::system::error_code ignored;
          socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored);
        }
        std::filesystem::remove(recovered);
        return true;
      })
  };
}

} // namespace holder::api
