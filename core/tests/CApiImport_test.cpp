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

struct ImportFixture {
  std::filesystem::path source_dir = holder::test::make_temp_dir();
  std::filesystem::path target_dir = holder::test::make_temp_dir();
  holder_context* target = nullptr;
  holder_arena* arena = holder_arena_new();
  holder_error* error = nullptr;
  std::string project_id;
  std::string card_id;
  std::filesystem::path source_root;
  std::filesystem::path root;

  ImportFixture() {
    const auto schema = schema_sql();
    holder_context* source = nullptr;
    REQUIRE(holder_context_open(source_dir.string().c_str(), schema.c_str(), &source, &error) == HOLDER_OK);
    holder_new_project project_request{};
    project_request.name = "Import fixture";
    project_request.privacy_mode = HOLDER_NEW_PROJECT_PRIVACY_MODE_PLAIN;
    const holder_project* project = nullptr;
    REQUIRE(holder_create_project(source, &project_request, arena, &project, &error) == HOLDER_OK);
    project_id = project->project_id;
    source_root = project->root_path;
    holder_new_card card_request{};
    card_request.title = "Durable";
    card_request.content = "Body #science";
    const holder_card* card = nullptr;
    REQUIRE(holder_create_card(source, project_id.c_str(), &card_request, arena, &card, &error) == HOLDER_OK);
    card_id = card->card_id;
    holder_context_destroy(source);

    REQUIRE(holder_context_open(target_dir.string().c_str(), schema.c_str(), &target, &error) == HOLDER_OK);
    root = target_dir / "projects" / "copy";
    std::filesystem::create_directories(root.parent_path());
    std::filesystem::copy(source_root, root, std::filesystem::copy_options::recursive);
  }

  ~ImportFixture() {
    holder_context_destroy(target);
    holder_error_destroy(error);
    holder_arena_free(arena);
  }

  size_t project_count() {
    const holder_project_list_item_list* projects = nullptr;
    REQUIRE(holder_list_projects(target, nullptr, nullptr, nullptr, HOLDER_LIST_PROJECTS_ORDER_NONE,
                                 nullptr, 0, false, arena, &projects, &error) == HOLDER_OK);
    return projects->count;
  }
};

} // namespace

TEST_CASE("Generated C API imports a durable project and recovers it from files", "[capi][project-import]") {
  ImportFixture f;
  const holder_project* imported = nullptr;
  REQUIRE(holder_import_project(f.target, f.root.string().c_str(), f.arena, &imported, &f.error) == HOLDER_OK);
  REQUIRE(std::string(imported->project_id) == f.project_id);
  REQUIRE(std::string(imported->root_path) == std::filesystem::canonical(f.root).string());
  const holder_card* card = nullptr;
  REQUIRE(holder_get_card(f.target, f.project_id.c_str(), f.card_id.c_str(), f.arena, &card, &f.error) == HOLDER_OK);
  REQUIRE(std::string(card->content) == "Body #science");
  REQUIRE(card->tags.count == 1);
  REQUIRE(std::string(card->tags.items[0]) == "science");
  REQUIRE(f.project_count() == 1);

  REQUIRE(holder_import_project(f.target, f.root.string().c_str(), f.arena, &imported, &f.error) != HOLDER_OK);
  holder_error_destroy(f.error);
  f.error = nullptr;
  holder_context_destroy(f.target);
  f.target = nullptr;
  REQUIRE(std::filesystem::remove(f.target_dir / "server" / "holder.db"));
  const auto schema = schema_sql();
  REQUIRE(holder_context_open(f.target_dir.string().c_str(), schema.c_str(), &f.target, &f.error) == HOLDER_OK);
  REQUIRE(holder_get_card(f.target, f.project_id.c_str(), f.card_id.c_str(), f.arena, &card, &f.error) == HOLDER_OK);
  REQUIRE(std::string(card->content) == "Body #science");
}

TEST_CASE("Generated C API rejects unsafe or invalid project imports", "[capi][project-import]") {
  ImportFixture f;
  const holder_project* imported = nullptr;
  REQUIRE(holder_import_project(nullptr, f.root.string().c_str(), f.arena, &imported, &f.error) ==
          HOLDER_ERROR_INVALID_ARGUMENT);
  holder_error_destroy(f.error);
  f.error = nullptr;
  REQUIRE(holder_import_project(f.target, f.source_root.string().c_str(), f.arena, &imported, &f.error) ==
          HOLDER_ERROR_INVALID_ARGUMENT);
  holder_error_destroy(f.error);
  f.error = nullptr;
  REQUIRE(f.project_count() == 0);

  const auto invalid = f.root / "cards" / "invalid.md";
  std::ofstream(invalid) << "Broken card";
  REQUIRE(holder_import_project(f.target, f.root.string().c_str(), f.arena, &imported, &f.error) != HOLDER_OK);
  holder_error_destroy(f.error);
  f.error = nullptr;
  REQUIRE(f.project_count() == 0);
  REQUIRE(std::filesystem::remove(invalid));

  const auto escape = f.root / "escape";
  std::error_code ec;
  std::filesystem::create_symlink(f.source_root / ".holder/project.json", escape, ec);
  if (!ec) {
    REQUIRE(holder_import_project(f.target, f.root.string().c_str(), f.arena, &imported, &f.error) ==
            HOLDER_ERROR_INVALID_ARGUMENT);
    holder_error_destroy(f.error);
    f.error = nullptr;
    REQUIRE(std::filesystem::remove(escape));
  }

  const auto hardlink = f.root / "hardlink";
  std::filesystem::create_hard_link(f.root / ".holder" / "project.json", hardlink, ec);
  if (!ec) {
    REQUIRE(holder_import_project(f.target, f.root.string().c_str(), f.arena, &imported, &f.error) ==
            HOLDER_ERROR_INVALID_ARGUMENT);
    holder_error_destroy(f.error);
    f.error = nullptr;
    REQUIRE(std::filesystem::remove(hardlink));
  }

  const auto privacy_path = f.root / ".holder" / "privacy.json";
  std::ifstream privacy_file(privacy_path);
  REQUIRE(privacy_file.is_open());
  const std::string original_privacy(std::istreambuf_iterator<char>{privacy_file}, std::istreambuf_iterator<char>{});
  privacy_file.close();
  std::ofstream(privacy_path) << R"({"mode":"encrypted_git"})";
  REQUIRE(holder_import_project(f.target, f.root.string().c_str(), f.arena, &imported, &f.error) ==
          HOLDER_ERROR_INVALID_ARGUMENT);
  holder_error_destroy(f.error);
  f.error = nullptr;
  std::ofstream(privacy_path) << original_privacy;

  REQUIRE(holder_import_project(f.target, f.root.string().c_str(), f.arena, &imported, &f.error) == HOLDER_OK);
}
