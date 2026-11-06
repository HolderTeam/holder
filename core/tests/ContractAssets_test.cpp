// Importing local files into a project and writing assets back out, through the built-in local
// directory provider, on a context a host makes from its own services.
#if __has_include(<catch2/catch_test_macros.hpp>)
#include <catch2/catch_test_macros.hpp>
#else
#include <catch2/catch.hpp>
#endif

#include "api/Contract.h"
#include "capi/Context.h"
#include "core_test_helpers.h"
#include "git/GitOps.h"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

namespace {

namespace contract = holder::contract;

struct Host {
  std::filesystem::path dir = holder::test::make_temp_dir();
  holder::platform::Db db = holder::test::open_db_with_schema(dir / "holder.db");
  holder::index::FtsIndexer fts{db};
  holder::git::RealGitOps git;
  holder_context context{db, fts, &git};
  std::string project_id;
  std::string card_id;
  std::string location_id;

  Host() {
    context.data_dir = dir / "data";
    context.projects_root = dir / "projects";
    contract::NewProject project;
    project.name = "Photos";
    project.privacy_mode = contract::NewProjectPrivacyMode::Plain;
    project_id = contract::create_project(context, project).project_id;
    contract::NewCard card;
    card.title = "Trip";
    card_id = contract::create_card(context, project_id, card).card_id;
    contract::NewStorageLocation location;
    location.project_id = project_id;
    location.name = "Local";
    location.provider = contract::NewStorageLocationProvider::LocalDirectory;
    location_id = contract::create_storage_location(context, location).location_id;
  }

  std::string file(const std::string& name, const std::string& content) {
    const auto path = dir / "source" / name;
    std::filesystem::create_directories(path.parent_path());
    std::ofstream(path, std::ios::binary) << content;
    return path.string();
  }

  contract::AssetImport import(const std::string& source, std::string card = "") {
    return contract::import_asset_file(
        context, {project_id, card.empty() ? card_id : card, location_id, source});
  }

  std::string placement(const contract::AssetImport& imported) {
    return contract::get_resource(context, project_id, imported.resource_id).assets.at(0).placements.at(0).placement_id;
  }
};

std::string read(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

} // namespace

TEST_CASE("An imported file is attached to its card and written back out unchanged", "[assets]") {
  Host host;
  const auto imported = host.import(host.file("photo.jpg", "jpeg bytes"));
  REQUIRE_FALSE(imported.duplicate_reused);
  REQUIRE(imported.link_created);
  const auto links = contract::list_card_links(host.context, host.project_id, host.card_id, false);
  REQUIRE(links.size() == 1);
  REQUIRE(links[0].target_id == imported.resource_id);

  // The same bytes again reuse the resource, and the card already links to it.
  const auto again = host.import(host.file("copy.jpg", "jpeg bytes"));
  REQUIRE(again.duplicate_reused);
  REQUIRE_FALSE(again.link_created);
  REQUIRE(again.resource_id == imported.resource_id);

  const auto destination = (host.dir / "out" / "photo.jpg").string();
  std::filesystem::create_directories(host.dir / "out");
  const auto file = contract::retrieve_asset_file(host.context, imported.resource_id, imported.asset_id,
                                                  host.placement(imported), destination);
  REQUIRE(file.path == destination);
  REQUIRE(file.original_filename == "photo.jpg");
  REQUIRE(file.byte_size == 10);
  REQUIRE(read(destination) == "jpeg bytes");
}

TEST_CASE("Asset file operations report what is missing or wrong", "[assets]") {
  Host host;
  const auto source = host.file("photo.jpg", "jpeg bytes");
  REQUIRE_THROWS_AS(contract::import_asset_file(host.context, {"missing", host.card_id, host.location_id, source}),
                    contract::NotFound);
  REQUIRE_THROWS_AS(host.import(source, "missing-card"), contract::NotFound);
  REQUIRE_THROWS_AS(contract::import_asset_file(host.context, {host.project_id, host.card_id, "missing", source}),
                    contract::NotFound);
  REQUIRE_THROWS_AS(host.import((host.dir / "absent").string()), contract::InvalidArgument);
  REQUIRE_THROWS_AS(host.import((host.dir / "source").string()), contract::InvalidArgument);
  REQUIRE_THROWS_AS(contract::import_asset_file(host.context, {host.project_id, "", host.location_id, source}),
                    contract::InvalidArgument);

  const auto imported = host.import(source);
  const auto placement = host.placement(imported);
  const auto destination = (host.dir / "out.jpg").string();
  REQUIRE_THROWS_AS(contract::retrieve_asset_file(host.context, "missing", imported.asset_id, placement, destination),
                    contract::NotFound);
  REQUIRE_THROWS_AS(
      contract::retrieve_asset_file(host.context, imported.resource_id, "missing", placement, destination),
      contract::NotFound);
  REQUIRE_THROWS_AS(
      contract::retrieve_asset_file(host.context, imported.resource_id, imported.asset_id, "missing", destination),
      contract::NotFound);
}

TEST_CASE("Retrieving bytes that fail their check is data corrupted and leaves no file", "[assets]") {
  Host host;
  const auto imported = host.import(host.file("photo.jpg", "jpeg bytes"));
  // Change the stored copy behind Holder's back.
  for (const auto& entry : std::filesystem::recursive_directory_iterator(host.context.data_dir / "resource-store"))
    if (entry.is_regular_file()) std::ofstream(entry.path(), std::ios::binary) << "tampered!!";
  const auto destination = (host.dir / "out.jpg").string();
  REQUIRE_THROWS_AS(contract::retrieve_asset_file(host.context, imported.resource_id, imported.asset_id,
                                                  host.placement(imported), destination),
                    contract::DataCorrupted);
  REQUIRE_FALSE(std::filesystem::exists(destination));
}
