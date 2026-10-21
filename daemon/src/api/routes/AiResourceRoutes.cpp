#include "core/ActivityTracker.h"
#include "api/routes/AiResourceRoutes.h"
#include "api/ResourceRead.h"
#include "api/generated/RuntimeJson.h"
#include "api/routes/GoogleDriveOAuthRoutes.h"
#include "api/support/AssetImportJobs.h"
#include "api/support/HttpResponses.h"
#include "api/support/StorageErrors.h"
#include "api/support/StorageProviders.h"
#include "api/support/Time.h"
#include "platform/Paths.h"

#include "resource/AssetEnvelope.h"
#include "resource/AssetImportService.h"
#include "resource/LocationBindingStore.h"
#include "resource/LocationRepo.h"
#include "resource/ResourceRepo.h"
#include "resource/ResourceStore.h"

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
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace holder::api::routes {
namespace {

namespace http = boost::beast::http;

using support::ImportJob;
using support::update_import_job;

std::mutex import_threads_mutex;
std::vector<std::thread> import_threads;

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

nlohmann::json import_job_json(const ImportJob& job) {
  return {
      // LCOV_EXCL_LINE: initializer-list cleanup is reported as an unexecuted duplicate.
      {"job_id", job.job_id},
      {"status", job.status},
      {"resource_id",
       job.resource_id.empty() ? nlohmann::json(nullptr) : nlohmann::json(job.resource_id)},
      {"asset_id", job.asset_id.empty() ? nlohmann::json(nullptr) : nlohmann::json(job.asset_id)},
      {"duplicate_reused", job.duplicate_reused},
      {"link_created", job.link_created},
      {"error", job.error.empty() ? nlohmann::json(nullptr) : nlohmann::json(job.error)},
  };
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

void wait_for_asset_import_jobs() {
  std::vector<std::thread> threads;
  {
    std::lock_guard<std::mutex> lock(import_threads_mutex);
    threads.swap(import_threads);
  }
  for (auto& thread : threads) {
    if (thread.joinable()) thread.join();
  }
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

  if (path == "/resources" && req.method() == http::verb::post) {
    try {
      const auto body = nlohmann::json::parse(req.body());
      if (!body.contains("project_id") || !body.contains("type") || !body.contains("label")) {
        res = support::error_response(
            http::status::bad_request,
            "bad_request",
            "Missing required fields."
        );
        return true;
      }
      holder::model::ResourceBundle bundle;
      bundle.resource.resource_id = body.value("resource_id", std::string());
      if (bundle.resource.resource_id.empty()) bundle.resource.resource_id = uuid_v4();
      bundle.resource.project_id = body.at("project_id").get<std::string>();
      bundle.resource.type = body.at("type").get<std::string>();
      bundle.resource.label = body.at("label").get<std::string>();
      if (body.contains("metadata")) {
        bundle.resource.metadata = body.at("metadata").get<holder::model::ResourceMetadata>();
      }
      bundle.resource.created_at = body.value("created_at", support::now_epoch_seconds());
      bundle.resource.updated_at = body.value("updated_at", bundle.resource.created_at);
      if (git_ops != nullptr) {
        holder::resource::ResourceStore(db, nullptr, git_ops).put(bundle);
      } else {
        holder::resource::ResourceRepo(db).add(bundle.resource);
      }
      res = support::json_response(
          http::status::created,
          {{"ok", true}, {"data", rig_json::encode(holder::api::resource_item(bundle))}}
      );
    } catch (const std::exception& ex) {
      res = resource_error_response(ex);
    }
    return true;
  }

  if (path == "/imports" && req.method() == http::verb::post) {
    try {
      if (!bindings || git_ops == nullptr) {
        throw std::runtime_error("asset import services unavailable");
      }
      const auto body = nlohmann::json::parse(req.body());
      holder::resource::AssetImportRequest request;
      request.project_id = body.at("project_id").get<std::string>();
      request.card_id = body.at("card_id").get<std::string>();
      request.location_id = body.at("location_id").get<std::string>();
      request.source_file = body.at("source_path").get<std::string>();
      request.now = support::now_epoch_seconds();
      if (!std::filesystem::is_regular_file(request.source_file)) {
        throw std::invalid_argument("asset source must be a readable regular file");
      }
      const auto location = holder::resource::LocationRepo(db).get(request.location_id);
      if (!location.has_value() || location->project_id != request.project_id) {
        throw std::runtime_error("storage location not found in project");
      }
      const auto binding = bindings->get(request.project_id, request.location_id);
      if (!binding.has_value()) throw std::runtime_error("storage location configuration required");
      // Resolve declarations and credentials before handing work to the background job. The job
      // retains neither the HTTP body nor the local source path in durable project state.
      const auto location_copy = *location;
      const auto binding_copy = *binding;
      const auto db_path = db.path();
      const auto cache = holder::core::Paths::resolve("holder").cache_dir / "asset-staging";
      const auto job_id = uuid_v4();
      support::put_import_job(ImportJob{
          .job_id = job_id,
          .status = "queued",
          .resource_id = {},
          .asset_id = {},
          .duplicate_reused = false,
          .link_created = false,
          .error = {},
      });
      // The import outlives the request that queued it; it is work until it finishes.
      std::thread import_thread(
          [job_id,
           request,
           location_copy,
           binding_copy,
           db_path,
           cache,
           uuid_v4,
           git_ops,
           activity_scope = holder::core::activity().begin()]() mutable {
            try {
              holder::platform::Db job_db;
              job_db.open(db_path);
              auto provider = support::storage_provider(location_copy, binding_copy);
              holder::resource::AssetImportService importer(
                  job_db,
                  cache,
                  uuid_v4,
                  nullptr,
                  git_ops,
                  [job_id](holder::resource::AssetImportStage stage) {
                    switch (stage) {
                    case holder::resource::AssetImportStage::Staging:
                      update_import_job(job_id, "staging");
                      break;
                    case holder::resource::AssetImportStage::Storing:
                      update_import_job(job_id, "storing");
                      break;
                    case holder::resource::AssetImportStage::Committing:
                      update_import_job(job_id, "committing");
                      break;
                    }
                  }
              );
              const auto result = importer.import_file(request, *provider);
              support::complete_import_job(
                  job_id,
                  result.resource_id,
                  result.asset_id,
                  result.duplicate_reused,
                  result.link_created
              );
            } catch (const std::exception& ex) {
              update_import_job(job_id, "failed", ex.what());
            } catch (...) { // LCOV_EXCL_LINE: defensive boundary for non-standard provider throws.
              update_import_job(job_id, "failed", "unknown asset import error"); // LCOV_EXCL_LINE
            } // LCOV_EXCL_LINE
          }
      );
      {
        std::lock_guard<std::mutex> lock(import_threads_mutex);
        import_threads.push_back(std::move(import_thread));
      }
      res = support::json_response(
          http::status::accepted,
          {{"ok", true},
           {"data",
            import_job_json(ImportJob{
                .job_id = job_id,
                .status = "queued",
                .resource_id = {},
                .asset_id = {},
                .duplicate_reused = false,
                .link_created = false,
                .error = {},
            })}}
      );
    } catch (const std::exception& ex) {
      res = resource_error_response(ex);
    }
    return true;
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

  if (path.rfind("/locations/", 0) == 0) {
    const auto suffix = path.substr(std::string("/locations/").size());
    const auto separator = suffix.find('/');
    const auto location_id = suffix.substr(0, separator);
    const auto action = separator == std::string::npos ? std::string()
                                                       : suffix.substr(separator + 1);
    if (location_id.empty()) {
      res = support::error_response(http::status::not_found, "not_found", "Route not found.");
      return true;
    }

    if (action == "oauth/google-drive/authorize" && req.method() == http::verb::post) {
      return handle_google_drive_oauth_authorize_route(location_id, req, res, db);
    }

    res = support::error_response(http::status::not_found, "not_found", "Route not found.");
    return true;
  }

  if (path.rfind("/resources/", 0) == 0) {
    const auto resource_id = path.substr(std::string("/resources/").size());
    if (resource_id.empty() || resource_id.find('/') != std::string::npos) {
      res = support::error_response(http::status::not_found, "not_found", "Route not found.");
      return true;
    }
    if (req.method() == http::verb::patch) {
      try {
        holder::resource::ResourceRepo repo(db);
        auto bundle = repo.get_bundle(resource_id);
        if (!bundle.has_value()) throw std::runtime_error("Resource not found.");
        const auto body = nlohmann::json::parse(req.body());
        if (body.contains("type")) bundle->resource.type = body.at("type").get<std::string>();
        if (body.contains("label")) bundle->resource.label = body.at("label").get<std::string>();
        if (body.contains("metadata")) {
          const auto patch = body.at("metadata").get<holder::model::ResourceMetadata>();
          for (const auto& [property, values] : patch) {
            if (values.empty()) {
              bundle->resource.metadata.erase(property);
            } else {
              bundle->resource.metadata[property] = values;
            }
          }
        }
        bundle->resource.updated_at = body.value("updated_at", support::now_epoch_seconds());
        if (git_ops != nullptr) {
          holder::resource::ResourceStore(db, nullptr, git_ops).put(*bundle);
        } else {
          repo.put_bundle(*bundle);
        }
        res = support::json_response(
            http::status::ok,
            {{"ok", true}, {"data", rig_json::encode(holder::api::resource_item(*bundle))}}
        );
      } catch (const std::exception& ex) {
        res = resource_error_response(ex);
      }
      return true;
    }
    if (req.method() == http::verb::delete_) {
      try {
        if (git_ops != nullptr) {
          holder::resource::ResourceStore(db, nullptr, git_ops).remove(resource_id);
        } else {
          holder::resource::ResourceRepo(db).remove(resource_id);
        }
        res = support::json_response(
            http::status::ok,
            {{"ok", true}, {"data", {{"resource_id", resource_id}}}}
        );
      } catch (const std::exception& ex) {
        res = resource_error_response(ex);
      }
      return true;
    }
    res = support::error_response(http::status::not_found, "not_found", "Route not found.");
    return true;
  }

  return false;
}

} // namespace holder::api::routes
