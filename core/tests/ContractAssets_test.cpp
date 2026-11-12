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
#include "resource/LocalDirectoryProvider.h"
#include "resource/ResourceRepo.h"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

namespace {

namespace contract = holder::contract;

class RejectingProvider final : public contract::StorageProvider {
 public:
  explicit RejectingProvider(const std::filesystem::path& root) : files(root) {}
  holder::resource::LocalDirectoryProvider files;
  bool reject_put = false;
  bool reject_get = false;
  void put(const std::string& key, const std::string& source, long long size, const std::string& hash) override {
    if (reject_put) throw std::invalid_argument("provider rejected upload configuration");
    files.put(key, source, size, hash);
  }
  void get(const std::string& key, const std::string& destination) override {
    if (reject_get) throw std::invalid_argument("provider rejected download configuration");
    files.get(key, destination);
  }
  bool exists(const std::string& key) override { return files.exists(key); }
  void remove(const std::string& key) override { files.remove(key); }
};

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

TEST_CASE("Host provider argument failures preserve asset metadata and allow retry",
          "[assets][provider-argument-failures]") {
  Host host;
  const auto backend = std::make_shared<RejectingProvider>(host.dir / "provider");
  contract::register_storage_provider(contract::RegisterStorageProviderKind::S3Compatible, backend);
  contract::NewStorageLocation location;
  location.project_id = host.project_id;
  location.name = "Host storage";
  location.provider = contract::NewStorageLocationProvider::S3Compatible;
  host.location_id = contract::create_storage_location(host.context, location).location_id;
  const auto source = host.file("notes.txt", "source bytes remain intact");
  const auto project = contract::get_project(host.context, host.project_id);
  holder::git::GitRepo git;
  git.open_existing(project.root_path);
  bool downloading = false;
  SECTION("upload rejects provider configuration") {}
  SECTION("download rejects provider configuration") { downloading = true; }
  if (!downloading) {
    const auto head = git.head_oid();
    backend->reject_put = true;
    REQUIRE_THROWS_AS(host.import(source), contract::InvalidArgument);
    CHECK(holder::resource::ResourceRepo(host.db).list(host.project_id).empty());
    CHECK(contract::list_card_links(host.context, host.project_id, host.card_id, false).empty());
    CHECK(git.head_oid() == head);
    CHECK(read(source) == "source bytes remain intact");
    backend->reject_put = false;
  }
  const auto imported = host.import(source);
  const auto placement = host.placement(imported);
  const auto destination = (host.dir / "download.txt").string();
  if (downloading) {
    const auto head = git.head_oid();
    std::ofstream(destination, std::ios::binary) << "keep existing destination";
    backend->reject_get = true;
    REQUIRE_THROWS_AS(contract::retrieve_asset_file(host.context, imported.resource_id,
        imported.asset_id, placement, destination), contract::InvalidArgument);
    CHECK(read(destination) == "keep existing destination");
    CHECK(holder::resource::ResourceRepo(host.db).list(host.project_id).size() == 1);
    CHECK(git.head_oid() == head);
    backend->reject_get = false;
  }
  const auto retrieved = contract::retrieve_asset_file(host.context, imported.resource_id,
      imported.asset_id, placement, destination);
  CHECK(retrieved.path == destination);
  CHECK(read(destination) == "source bytes remain intact");
  CHECK(read(source) == "source bytes remain intact");
}

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

TEST_CASE("Failed integrity checks preserve existing destinations and leave no partial new file", "[assets]") {
  Host host;
  const auto imported = host.import(host.file("photo.jpg", "jpeg bytes"));
  const auto plaintext_hash = contract::get_resource(host.context, host.project_id, imported.resource_id)
                                  .assets.at(0).plaintext_sha256;
  SECTION("stored bytes fail their integrity check before decoding") {
    for (const auto& entry : std::filesystem::recursive_directory_iterator(host.context.data_dir / "resource-store"))
      if (entry.is_regular_file()) std::ofstream(entry.path(), std::ios::binary) << "tampered!!";
  }
  SECTION("decoded bytes fail their plaintext check after writing") {
    host.db.exec("UPDATE assets SET plaintext_sha256='" + std::string(64, '0') + "';");
  }
  for (bool existing : {false, true}) {
    const auto destination = (host.dir / (existing ? "existing.jpg" : "new.jpg")).string();
    if (existing) std::ofstream(destination, std::ios::binary) << "existing user bytes";
    REQUIRE_THROWS_AS(contract::retrieve_asset_file(host.context, imported.resource_id, imported.asset_id,
                                                  host.placement(imported), destination),
                    contract::DataCorrupted);
    if (existing) CHECK(read(destination) == "existing user bytes");
    else CHECK_FALSE(std::filesystem::exists(destination));
    for (const auto& entry : std::filesystem::directory_iterator(host.dir))
      CHECK(entry.path().filename().string().find(".holder-retrieve-") == std::string::npos);
  }
  // Repair either form of corruption and retry both paths, including replacement
  // of the existing user file now that validated bytes are available.
  for (const auto& entry : std::filesystem::recursive_directory_iterator(host.context.data_dir / "resource-store"))
    if (entry.is_regular_file()) std::ofstream(entry.path(), std::ios::binary) << "jpeg bytes";
  host.db.exec("UPDATE assets SET plaintext_sha256='" + plaintext_hash + "';");
  for (const auto* name : {"existing.jpg", "new.jpg"}) {
    const auto destination = (host.dir / name).string();
    contract::retrieve_asset_file(host.context, imported.resource_id, imported.asset_id,
        host.placement(imported), destination);
    CHECK(read(destination) == "jpeg bytes");
  }
}

TEST_CASE("A failed asset publish preserves a destination directory and cleans temporary files",
          "[assets][retrieve-publish-failure]") {
  Host host;
  const auto imported = host.import(host.file("notes.txt", "notes"));
  const auto destination = host.dir / "existing-directory";
  std::filesystem::create_directories(destination);
  std::ofstream(destination / "keep.txt") << "keep directory contents";
  REQUIRE_THROWS_AS(contract::retrieve_asset_file(host.context, imported.resource_id,
      imported.asset_id, host.placement(imported), destination.string()), std::filesystem::filesystem_error);
  CHECK(read((destination / "keep.txt").string()) == "keep directory contents");
  for (const auto& entry : std::filesystem::directory_iterator(host.dir))
    CHECK(entry.path().filename().string().find(".holder-retrieve-") == std::string::npos);
  const auto output = (host.dir / "valid.txt").string();
  contract::retrieve_asset_file(host.context, imported.resource_id, imported.asset_id,
      host.placement(imported), output);
  CHECK(read(output) == "notes");
}
