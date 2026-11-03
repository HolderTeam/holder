#if __has_include(<catch2/catch_test_macros.hpp>)
#include <catch2/catch_test_macros.hpp>
#else
#include <catch2/catch.hpp>
#endif

#include "core_test_helpers.h"

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
    REQUIRE(holder_context_open(data_dir.string().c_str(), schema.c_str(), &context, &error) == HOLDER_OK);
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

TEST_CASE("Generated C API storage locations create, list, update and delete portable declarations",
          "[capi][locations]") {
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
  REQUIRE(holder_create_storage_location(f.context, &request, f.arena, &location, &f.error) == HOLDER_OK);
  REQUIRE(std::string(location->location_id) == "location-1234");
  REQUIRE(std::string(location->name) == "Family Assets");
  REQUIRE(location->created_at == created);
  REQUIRE_FALSE(location->bound);

  const holder_storage_location_list* list = nullptr;
  REQUIRE(holder_list_storage_locations(f.context, f.project_id.c_str(), f.arena, &list, &f.error) == HOLDER_OK);
  REQUIRE(list->data.count == 1);
  REQUIRE(std::string(list->data.items[0].location_id) == "location-1234");
  REQUIRE(holder_get_storage_location(f.context, "location-1234", f.arena, &location, &f.error) == HOLDER_OK);

  holder_storage_location_update update{};
  update.name = "Archive";
  REQUIRE(holder_update_storage_location(f.context, "location-1234", &update, f.arena, &location, &f.error) ==
          HOLDER_OK);
  REQUIRE(std::string(location->name) == "Archive");
  REQUIRE(std::filesystem::is_directory(f.project_root / ".git"));

  const holder_location_delete_result* removed = nullptr;
  REQUIRE(holder_delete_storage_location(f.context, "location-1234", f.arena, &removed, &f.error) == HOLDER_OK);
  REQUIRE(std::string(removed->location_id) == "location-1234");
  REQUIRE(holder_list_storage_locations(f.context, f.project_id.c_str(), f.arena, &list, &f.error) == HOLDER_OK);
  REQUIRE(list->data.count == 0);
  REQUIRE(f.failed(holder_get_storage_location(f.context, "location-1234", f.arena, &location, &f.error)) ==
          HOLDER_ERROR_NOT_FOUND);
}

TEST_CASE("Generated C API storage locations validate project and provider", "[capi][locations]") {
  Fixture f;
  holder_new_storage_location request{};
  request.location_id = "location-1234";
  request.project_id = "missing-project";
  request.name = "Missing";
  request.provider = HOLDER_NEW_STORAGE_LOCATION_PROVIDER_LOCAL_DIRECTORY;
  const holder_project_storage_location* location = nullptr;
  REQUIRE(f.failed(holder_create_storage_location(f.context, &request, f.arena, &location, &f.error)) ==
          HOLDER_ERROR_NOT_FOUND);
  request.project_id = f.project_id.c_str();
  request.provider = HOLDER_NEW_STORAGE_LOCATION_PROVIDER_MAX_ENUM;
  REQUIRE(f.failed(holder_create_storage_location(f.context, &request, f.arena, &location, &f.error)) ==
          HOLDER_ERROR_INVALID_ARGUMENT);
}
