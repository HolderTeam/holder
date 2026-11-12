#if __has_include(<catch2/catch_test_macros.hpp>)
#include <catch2/catch_test_macros.hpp>
#else
#include <catch2/catch.hpp>
#endif

#include "capi/Context.h"
#include "core_test_helpers.h"
#include "privacy/SecretStore.h"
#include "resource/LocationBindingStore.h"

#include <holder/holder_api.h>

#include <filesystem>
#include <fstream>
#include <string>

namespace {

std::string schema_sql() {
  std::ifstream in(SCHEMA_SQL_PATH);
  REQUIRE(in.is_open());
  return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

struct Fixture {
  std::filesystem::path data_dir = holder::test::make_temp_dir();
  holder_context* context = nullptr;
  holder_arena* arena = holder_arena_new();
  holder_error* error = nullptr;
  std::string project_id;
  std::filesystem::path project_root;

  Fixture() {
    const auto schema = schema_sql();
    REQUIRE(
        holder_context_open(data_dir.string().c_str(), schema.c_str(), &context, &error) ==
        HOLDER_OK
    );
    holder_new_project request{};
    request.name = "Locations";
    request.privacy_mode = HOLDER_NEW_PROJECT_PRIVACY_MODE_PLAIN;
    const holder_project* project = nullptr;
    REQUIRE(holder_create_project(context, &request, arena, &project, &error) == HOLDER_OK);
    project_id = project->project_id;
    project_root = project->root_path;
  }

  ~Fixture() {
    holder_error_destroy(error);
    holder_arena_free(arena);
    holder_context_destroy(context);
  }

  int failed(int code) {
    const int result = code;
    holder_error_destroy(error);
    error = nullptr;
    return result;
  }
};

} // namespace

TEST_CASE(
    "Location reads reject unsupported stored providers and recover after repair",
    "[capi][batch-location-failures]"
) {
  Fixture f;
  holder_new_storage_location request{};
  request.project_id = f.project_id.c_str();
  request.name = "Keep location";
  request.provider = HOLDER_NEW_STORAGE_LOCATION_PROVIDER_LOCAL_DIRECTORY;
  const holder_project_storage_location* made = nullptr;
  REQUIRE(
      holder_create_storage_location(f.context, &request, f.arena, &made, &f.error) == HOLDER_OK
  );
  const std::string id = made->location_id;
  f.context->db.exec("UPDATE storage_locations SET provider='future-provider';");
  const holder_project_storage_location* location = nullptr;
  const holder_storage_location_list* list = nullptr;
  for (bool listing : {false, true}) {
    const auto status =
        listing ? holder_list_storage_locations(
                      f.context,
                      f.project_id.c_str(),
                      f.arena,
                      &list,
                      &f.error
                  )
                : holder_get_storage_location(f.context, id.c_str(), f.arena, &location, &f.error);
    REQUIRE(status == HOLDER_ERROR_RUNTIME);
    REQUIRE(f.error != nullptr);
    CHECK(
        std::string(holder_error_message(f.error)).find("unknown storage provider") !=
        std::string::npos
    );
    CHECK(location == nullptr);
    CHECK(list == nullptr);
    holder_error_destroy(f.error);
    f.error = nullptr;
  }
  f.context->db.exec("UPDATE storage_locations SET provider='local_directory';");
  REQUIRE(
      holder_get_storage_location(f.context, id.c_str(), f.arena, &location, &f.error) == HOLDER_OK
  );
  CHECK(std::string(location->name) == "Keep location");
  REQUIRE(
      holder_list_storage_locations(f.context, f.project_id.c_str(), f.arena, &list, &f.error) ==
      HOLDER_OK
  );
  CHECK(list->data.count == 1);
}

TEST_CASE(
    "Generated C API storage locations create, list, update and delete portable declarations",
    "[capi][locations]"
) {
  Fixture f;
  const int64_t created = 10;
  holder_new_storage_location request{};
  request.location_id = "location-1234";
  request.project_id = f.project_id.c_str();
  request.name = "Family Assets";
  request.provider = HOLDER_NEW_STORAGE_LOCATION_PROVIDER_LOCAL_DIRECTORY;
  request.created_at = &created;
  request.updated_at = &created;
  const holder_project_storage_location* location = nullptr;
  REQUIRE(
      holder_create_storage_location(f.context, &request, f.arena, &location, &f.error) == HOLDER_OK
  );
  REQUIRE(std::string(location->location_id) == "location-1234");
  REQUIRE(std::string(location->name) == "Family Assets");
  REQUIRE(location->created_at == created);
  REQUIRE_FALSE(location->bound);

  const holder_storage_location_list* list = nullptr;
  REQUIRE(
      holder_list_storage_locations(f.context, f.project_id.c_str(), f.arena, &list, &f.error) ==
      HOLDER_OK
  );
  REQUIRE(list->data.count == 1);
  REQUIRE(std::string(list->data.items[0].location_id) == "location-1234");
  REQUIRE(
      holder_get_storage_location(f.context, "location-1234", f.arena, &location, &f.error) ==
      HOLDER_OK
  );

  holder_storage_location_update update{};
  update.name = "Archive";
  REQUIRE(
      holder_update_storage_location(
          f.context,
          "location-1234",
          &update,
          f.arena,
          &location,
          &f.error
      ) == HOLDER_OK
  );
  REQUIRE(std::string(location->name) == "Archive");
  REQUIRE(std::filesystem::is_directory(f.project_root / ".git"));

  const holder_location_delete_result* removed = nullptr;
  REQUIRE(
      holder_delete_storage_location(f.context, "location-1234", f.arena, &removed, &f.error) ==
      HOLDER_OK
  );
  REQUIRE(std::string(removed->location_id) == "location-1234");
  REQUIRE(
      holder_list_storage_locations(f.context, f.project_id.c_str(), f.arena, &list, &f.error) ==
      HOLDER_OK
  );
  REQUIRE(list->data.count == 0);
  REQUIRE(
      f.failed(holder_get_storage_location(f.context, "location-1234", f.arena, &location, &f.error)
      ) == HOLDER_ERROR_NOT_FOUND
  );
}

TEST_CASE("Generated C API storage locations validate project and provider", "[capi][locations]") {
  Fixture f;
  holder_new_storage_location request{};
  request.location_id = "location-1234";
  request.project_id = "missing-project";
  request.name = "Missing";
  request.provider = HOLDER_NEW_STORAGE_LOCATION_PROVIDER_LOCAL_DIRECTORY;
  const holder_project_storage_location* location = nullptr;
  REQUIRE(
      f.failed(holder_create_storage_location(f.context, &request, f.arena, &location, &f.error)) ==
      HOLDER_ERROR_NOT_FOUND
  );
  request.project_id = f.project_id.c_str();
  request.provider = HOLDER_NEW_STORAGE_LOCATION_PROVIDER_MAX_ENUM;
  REQUIRE(
      f.failed(holder_create_storage_location(f.context, &request, f.arena, &location, &f.error)) ==
      HOLDER_ERROR_INVALID_ARGUMENT
  );
}

TEST_CASE(
    "Generated C API storage locations live in the project repository and survive a rebuild",
    "[capi][locations]"
) {
  Fixture f;
  holder_new_storage_location request{};
  request.location_id = "location-5678";
  request.project_id = f.project_id.c_str();
  request.name = "Family Drive";
  request.provider = HOLDER_NEW_STORAGE_LOCATION_PROVIDER_GOOGLE_DRIVE;
  const holder_project_storage_location* created = nullptr;
  REQUIRE(
      holder_create_storage_location(f.context, &request, f.arena, &created, &f.error) == HOLDER_OK
  );
  const auto file = f.project_root / "locations" / "lo" / "ca" / "location-5678.json";
  REQUIRE(std::filesystem::exists(file));

  // A rebuild reads the repository, so a location written only to the database would be lost.
  holder_rebuild_request rebuild{f.project_id.c_str()};
  const holder_rebuild_result* rebuilt = nullptr;
  REQUIRE(holder_rebuild_project(f.context, &rebuild, f.arena, &rebuilt, &f.error) == HOLDER_OK);
  const holder_project_storage_location* location = nullptr;
  REQUIRE(
      holder_get_storage_location(f.context, "location-5678", f.arena, &location, &f.error) ==
      HOLDER_OK
  );
  REQUIRE(std::string(location->name) == "Family Drive");

  const holder_location_delete_result* deleted = nullptr;
  REQUIRE(
      holder_delete_storage_location(f.context, "location-5678", f.arena, &deleted, &f.error) ==
      HOLDER_OK
  );
  REQUIRE_FALSE(std::filesystem::exists(file));
  REQUIRE(holder_rebuild_project(f.context, &rebuild, f.arena, &rebuilt, &f.error) == HOLDER_OK);
  REQUIRE(
      holder_get_storage_location(f.context, "location-5678", f.arena, &location, &f.error) ==
      HOLDER_ERROR_NOT_FOUND
  );
}

TEST_CASE(
    "Generated location deletion removes only its private binding and clears its preference",
    "[capi][location-bindings]"
) {
  Fixture f;
  auto secrets = holder::privacy::make_encrypted_file_secret_store_for_tests(
      f.data_dir / "private-bindings"
  );
  f.context->secret_store = secrets.get();
  holder::resource::LocationBindingStore bindings(*secrets);
  for (const char* id : {"location-first", "location-other"}) {
    holder_new_storage_location request{};
    request.location_id = id;
    request.project_id = f.project_id.c_str();
    request.name = id;
    request.provider = HOLDER_NEW_STORAGE_LOCATION_PROVIDER_LOCAL_DIRECTORY;
    const holder_project_storage_location* result = nullptr;
    REQUIRE(
        holder_create_storage_location(f.context, &request, f.arena, &result, &f.error) == HOLDER_OK
    );
    holder::resource::LocationBinding binding;
    binding.provider = "local_directory";
    binding.values = {{"path", std::string("/private/") + id}};
    bindings.bind(f.project_id, id, binding, "Safe folder preview", 10);
  }
  bindings.set_preferred(f.project_id, "location-first", 11);
  const holder_project_storage_location* bound = nullptr;
  REQUIRE(
      holder_get_storage_location(f.context, "location-first", f.arena, &bound, &f.error) ==
      HOLDER_OK
  );
  REQUIRE(bound->bound);
  REQUIRE(std::string(bound->binding_preview) == "Safe folder preview");
  REQUIRE(bound->configuration.count == 0);
  const holder_storage_location_list* before = nullptr;
  REQUIRE(
      holder_list_storage_locations(f.context, f.project_id.c_str(), f.arena, &before, &f.error) ==
      HOLDER_OK
  );
  REQUIRE(std::string(before->preferred_location_id) == "location-first");
  const holder_location_delete_result* removed = nullptr;
  REQUIRE(
      holder_delete_storage_location(f.context, "location-first", f.arena, &removed, &f.error) ==
      HOLDER_OK
  );
  REQUIRE(std::string(removed->location_id) == "location-first");
  REQUIRE_FALSE(bindings.get(f.project_id, "location-first"));
  REQUIRE_FALSE(bindings.preview(f.project_id, "location-first"));
  REQUIRE_FALSE(bindings.preferred(f.project_id));
  REQUIRE(bindings.get(f.project_id, "location-other").has_value());
  const holder_storage_location_list* after = nullptr;
  REQUIRE(
      holder_list_storage_locations(f.context, f.project_id.c_str(), f.arena, &after, &f.error) ==
      HOLDER_OK
  );
  REQUIRE(after->data.count == 1);
  REQUIRE(std::string(after->data.items[0].location_id) == "location-other");
  REQUIRE(after->data.items[0].bound);
  REQUIRE(after->preferred_location_id == nullptr);
  f.context->secret_store = nullptr;
}
