// Asset files from contract/interface/resources: importing a local file into a project, and writing
// an asset's bytes back out to one, through this device's storage providers.
#include "api/Contract.h"

#include "api/CardAccess.h"
#include "api/CoreErrors.h"

#include "capi/Context.h"
#include "capi/StorageProviders.h"
#include "identity/Uuid.h"
#include "project/ProjectRepo.h"
#include "resource/AssetImportService.h"
#include "resource/LocationRepo.h"
#include "resource/ResourceRepo.h"
#include "resource/StorageProvider.h"

#include <algorithm>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <system_error>

namespace holder::contract {
namespace {

holder::resource::AssetImportService service(holder_context& context) {
  return {context.db, context.data_dir / "server" / "asset-staging", holder::identity::uuid_v4, nullptr,
          context.git};
}

// Runs body, turning a failed integrity check into DataCorrupted and a bad source into an invalid argument.
template <class Body>
decltype(auto) with_asset_errors(Body&& body) {
  return with_contract_errors([&]() -> decltype(auto) {
    try {
      return std::forward<Body>(body)();
    } catch (const holder::resource::StorageError& error) {
      if (error.code() == holder::resource::StorageErrorCode::Integrity) throw DataCorrupted(error.what());
      throw;
    } catch (const std::invalid_argument& error) {
      throw InvalidArgument(error.what());
    }
  });
}

AssetImport import_file(holder_context& context, const AssetImportRequest& request) {
  if (request.project_id.empty() || request.card_id.empty() || request.location_id.empty() ||
      request.source_path.empty())
    throw InvalidArgument("project_id, card_id, location_id and source_path must not be empty");
  if (!holder::project::ProjectRepo(context.db).get(request.project_id))
    throw NotFound("project not found: " + request.project_id);
  auto store = card_store(context);
  live_card(store, request.project_id, request.card_id);
  const auto location = holder::resource::LocationRepo(context.db).get(request.location_id);
  if (!location || location->project_id != request.project_id)
    throw NotFound("storage location not found in project: " + request.location_id);
  std::error_code error;
  if (!std::filesystem::is_regular_file(request.source_path, error))
    throw InvalidArgument("the source must be a readable regular file");

  holder::resource::AssetImportRequest import;
  import.project_id = request.project_id;
  import.card_id = request.card_id;
  import.location_id = request.location_id;
  import.source_file = request.source_path;
  import.now = now_epoch_seconds();
  const auto provider = holder::capi::storage_provider(context, location->provider);
  const auto result = service(context).import_file(import, *provider);
  return {result.resource_id, result.asset_id, result.duplicate_reused, result.link_created};
}

AssetFile retrieve(holder_context& context, const std::string& resource_id, const std::string& asset_id,
                   const std::string& placement_id, const std::string& destination_path) {
  const auto bundle = holder::resource::ResourceRepo(context.db).get_bundle(resource_id);
  if (!bundle) throw NotFound("resource not found: " + resource_id);
  const auto asset = std::find_if(bundle->assets.begin(), bundle->assets.end(),
                                  [&](const auto& item) { return item.asset_id == asset_id; });
  if (asset == bundle->assets.end()) throw NotFound("asset not found in resource: " + asset_id);
  const auto placement = std::find_if(asset->placements.begin(), asset->placements.end(),
                                      [&](const auto& item) { return item.placement_id == placement_id; });
  if (placement == asset->placements.end()) throw NotFound("placement not found in asset: " + placement_id);
  const auto location = holder::resource::LocationRepo(context.db).get(placement->location_id);
  if (!location || location->project_id != bundle->resource.project_id)
    throw NotFound("storage location not found in project: " + placement->location_id);

  const auto provider = holder::capi::storage_provider(context, location->provider);
  const std::filesystem::path destination(destination_path);
  try {
    service(context).retrieve(resource_id, asset_id, placement_id, *provider, destination);
  } catch (...) {
    std::error_code ignored;
    std::filesystem::remove(destination, ignored);
    throw;
  }
  return {destination_path, asset->original_filename, asset->media_type, asset->byte_size};
}

} // namespace

AssetImport import_asset_file(holder_context& context, const AssetImportRequest& request) {
  return with_asset_errors([&] { return import_file(context, request); });
}

AssetFile retrieve_asset_file(holder_context& context, std::string resource_id, std::string asset_id,
                              std::string placement_id, std::string destination_path) {
  return with_asset_errors(
      [&] { return retrieve(context, resource_id, asset_id, placement_id, destination_path); });
}

} // namespace holder::contract
