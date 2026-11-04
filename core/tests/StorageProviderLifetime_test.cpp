#include "capi_asset_helpers.h"
#include "core_test_helpers.h"
#include "model/Location.h"
#include "platform/Db.h"
#include "resource/LocalDirectoryProvider.h"
#include "resource/LocationStore.h"

#include <holder/holder_api.h>
#include <nlohmann/json.hpp>

#include <atomic>
#include <chrono>
#include <functional>
#include <future>
#include <memory>
#include <string>
#include <vector>

namespace {

struct ProviderProbe {
  explicit ProviderProbe(const std::filesystem::path& root)
      : files(root) {}

  holder::resource::LocalDirectoryProvider files;
  std::function<void()> on_put;
  std::function<void()> on_get;
  std::function<void()> on_destroy;
  std::atomic<int> destroyed{0};
  int replacement_result = -1;
  bool destroyed_in_callback = false;
};

using ProbeOwner = std::shared_ptr<ProviderProbe>;

int register_probe(const char* name, const ProbeOwner& probe) {
  holder_error* error = nullptr;
  const holder_storage_provider_callbacks callbacks{
      [](void* data,
         const char* key,
         const char* file,
         int64_t size,
         const char* hash,
         int*,
         char**) {
        const auto state = *static_cast<ProbeOwner*>(data);
        state->files.put(key, file, size, hash);
        if (state->on_put) state->on_put();
        state->destroyed_in_callback = state->destroyed != 0;
        return 0;
      },
      [](void* data, const char* key, const char* file, int*, char**) {
        const auto state = *static_cast<ProbeOwner*>(data);
        state->files.get(key, file);
        if (state->on_get) state->on_get();
        state->destroyed_in_callback = state->destroyed != 0;
        return 0;
      },
      [](void* data, const char* key, int* exists, int*, char**) {
        const auto state = *static_cast<ProbeOwner*>(data);
        *exists = state->files.exists(key);
        return 0;
      },
      [](void* data, const char* key, int*, char**) {
        const auto state = *static_cast<ProbeOwner*>(data);
        state->files.remove(key);
        return 0;
      }
  };
  const int result = holder_register_storage_provider(
      name,
      &callbacks,
      new ProbeOwner(probe),
      [](void* data) {
        const std::unique_ptr<ProbeOwner> owner(static_cast<ProbeOwner*>(data));
        ++(*owner)->destroyed;
        if ((*owner)->on_destroy) (*owner)->on_destroy();
      },
      &error
  );
  holder_error_destroy(error);
  return result;
}

struct AssetFixture {
  std::filesystem::path root = holder::test::make_temp_dir();
  std::unique_ptr<holder_context, decltype(&holder_context_destroy)> context{
      nullptr,
      holder_context_destroy
  };
  std::string project_id;
  std::string card_id;

  explicit AssetFixture(const char* provider) {
    std::ifstream schema_file(SCHEMA_SQL_PATH);
    const std::string schema((std::istreambuf_iterator<char>(schema_file)), {});
    holder_context* raw_context = nullptr;
    holder_error* error = nullptr;
    const int result =
        holder_context_open(root.string().c_str(), schema.c_str(), &raw_context, &error);
    context.reset(raw_context);
    holder_error_destroy(error);
    REQUIRE(result == HOLDER_OK);
    const std::unique_ptr<holder_arena, decltype(&holder_arena_free)> arena(holder_arena_new(), holder_arena_free);
    holder_new_project project_request{};
    project_request.name = "Lifetime";
    project_request.privacy_mode = HOLDER_NEW_PROJECT_PRIVACY_MODE_PLAIN;
    const holder_project* project = nullptr;
    REQUIRE(holder_create_project(context.get(), &project_request, arena.get(), &project, &error) == HOLDER_OK);
    project_id = project->project_id;
    holder_new_card card_request{};
    card_request.title = "Asset";
    const holder_card* card = nullptr;
    REQUIRE(holder_create_card(context.get(), project_id.c_str(), &card_request, arena.get(), &card, &error) ==
            HOLDER_OK);
    card_id = card->card_id;
    holder::platform::Db db;
    db.open(root / "server" / "holder.db");
    holder::model::Location location;
    location.location_id = "lifetime-location";
    location.project_id = project_id;
    location.name = "Lifetime";
    location.provider = provider;
    location.created_at = 1;
    location.updated_at = 1;
    holder::resource::LocationStore(db).put(location);
    holder_error_destroy(error);
  }

  std::string placement_id(const std::string& resource_id) const {
    const std::unique_ptr<holder_arena, decltype(&holder_arena_free)> arena(holder_arena_new(), holder_arena_free);
    holder_error* error = nullptr;
    const holder_resource* resource = nullptr;
    const int rc = holder_get_resource(context.get(), project_id.c_str(), resource_id.c_str(),
                                       arena.get(), &resource, &error);
    INFO((error ? holder_error_message(error) : "no C API error"));
    REQUIRE(rc == HOLDER_OK);
    REQUIRE(resource->assets.count == 1);
    REQUIRE(resource->assets.items[0].placements.count == 1);
    const std::string id = resource->assets.items[0].placements.items[0].placement_id;
    holder_error_destroy(error);
    return id;
  }

  nlohmann::json import() {
    const auto source = (root / "input.txt").string();
    std::ofstream(source) << "provider lifetime bytes";
    nlohmann::json result;
    holder_error* error = nullptr;
    const int rc = holder::test::import_asset_file(
        context.get(), project_id.c_str(), card_id.c_str(), "lifetime-location", source.c_str(), &result, &error);
    const std::string message = error != nullptr ? holder_error_message(error) : "";
    holder_error_destroy(error);
    INFO(message);
    REQUIRE(rc == HOLDER_OK);
    return result;
  }
};

} // namespace

TEST_CASE("C API asset operations retain providers replaced inside callbacks", "[capi][resource]") {
  const char* name = "lifetime-operation";
  AssetFixture fixture(name);
  auto original = std::make_shared<ProviderProbe>(fixture.root / "objects");
  auto replacement = std::make_shared<ProviderProbe>(fixture.root / "objects");
  REQUIRE(register_probe(name, original) == HOLDER_OK);
  const auto replace = [weak = std::weak_ptr<ProviderProbe>(original), replacement, name] {
    if (auto state = weak.lock()) state->replacement_result = register_probe(name, replacement);
  };

  SECTION("import retains the original through put and existence verification") {
    original->on_put = replace;
    fixture.import();
  }
  SECTION("retrieve retains the original until the get callback returns") {
    const auto imported = fixture.import();
    const auto resource_id = imported.at("resource_id").get<std::string>();
    const auto asset_id = imported.at("asset_id").get<std::string>();
    const auto placement_id = fixture.placement_id(resource_id);
    original->on_get = replace;
    holder_error* error = nullptr;
    const auto destination = (fixture.root / "download.txt").string();
    const int result = holder::test::retrieve_asset_file(
        fixture.context.get(),
        resource_id.c_str(),
        asset_id.c_str(),
        placement_id.c_str(),
        destination.c_str(),
        &error
    );
    holder_error_destroy(error);
    REQUIRE(result == HOLDER_OK);
    std::ifstream downloaded(destination);
    REQUIRE(
        std::string((std::istreambuf_iterator<char>(downloaded)), {}) == "provider lifetime bytes"
    );
  }
  REQUIRE(original->replacement_result == HOLDER_OK);
  REQUIRE_FALSE(original->destroyed_in_callback);
  REQUIRE(original->destroyed.load() == 1);
  REQUIRE(replacement->destroyed.load() == 0);
}

TEST_CASE("C API storage provider cleanup can register another provider", "[capi][resource]") {
  const auto root = holder::test::make_temp_dir();
  auto original = std::make_shared<ProviderProbe>(root);
  auto replacement = std::make_shared<ProviderProbe>(root);
  auto nested = std::make_shared<ProviderProbe>(root);
  original->on_destroy = [weak = std::weak_ptr<ProviderProbe>(original), nested] {
    if (auto state = weak.lock())
      state->replacement_result = register_probe("lifetime-nested", nested);
  };
  REQUIRE(register_probe("lifetime-cleanup", original) == HOLDER_OK);
  REQUIRE(register_probe("lifetime-cleanup", replacement) == HOLDER_OK);
  REQUIRE(original->destroyed.load() == 1);
  REQUIRE(original->replacement_result == HOLDER_OK);
}

TEST_CASE(
    "C API retains an active provider during concurrent replacements",
    "[capi][resource][concurrency][stress]"
) {
  using namespace std::chrono_literals;
  const char* name = "lifetime-concurrent";
  AssetFixture fixture(name);
  auto original = std::make_shared<ProviderProbe>(fixture.root / "objects");
  REQUIRE(register_probe(name, original) == HOLDER_OK);
  std::vector<ProbeOwner> replacements;
  for (int i = 0; i < 128; ++i)
    replacements.push_back(std::make_shared<ProviderProbe>(fixture.root / "objects"));

  std::promise<void> entered;
  auto entered_future = entered.get_future();
  std::promise<void> replaced;
  auto replaced_future = replaced.get_future();
  bool callback_synchronized = false;
  bool retained_during_replacement = false;
  const auto during_callback = [&] {
    entered.set_value();
    callback_synchronized = replaced_future.wait_for(10s) == std::future_status::ready;
    retained_during_replacement = original->destroyed.load() == 0;
  };
  std::string resource_id, asset_id, placement_id;
  SECTION("import") { original->on_put = during_callback; }
  SECTION("retrieve") {
    const auto imported = fixture.import();
    resource_id = imported.at("resource_id").get<std::string>();
    asset_id = imported.at("asset_id").get<std::string>();
    placement_id = fixture.placement_id(resource_id);
    original->on_get = during_callback;
  }
  bool registrations_ok = true;
  auto worker = std::async(std::launch::async, [&] {
    if (entered_future.wait_for(10s) == std::future_status::ready) {
      for (const auto& replacement : replacements)
        if (register_probe(name, replacement) != HOLDER_OK) registrations_ok = false;
    } else {
      registrations_ok = false;
    }
    replaced.set_value();
  });
  if (resource_id.empty()) {
    fixture.import();
  } else {
    holder_error* error = nullptr;
    const auto destination = (fixture.root / "concurrent-download.txt").string();
    const auto result = holder::test::retrieve_asset_file(
        fixture.context.get(),
        resource_id.c_str(),
        asset_id.c_str(),
        placement_id.c_str(),
        destination.c_str(),
        &error
    );
    holder_error_destroy(error);
    REQUIRE(result == HOLDER_OK);
    std::ifstream downloaded(destination);
    REQUIRE(
        std::string((std::istreambuf_iterator<char>(downloaded)), {}) == "provider lifetime bytes"
    );
  }
  worker.get();
  REQUIRE(registrations_ok);
  REQUIRE(callback_synchronized);
  REQUIRE(retained_during_replacement);
  REQUIRE_FALSE(original->destroyed_in_callback);
  REQUIRE(original->destroyed.load() == 1);
  for (std::size_t i = 0; i + 1 < replacements.size(); ++i)
    REQUIRE(replacements[i]->destroyed.load() == 1);
  REQUIRE(replacements.back()->destroyed.load() == 0);
}
