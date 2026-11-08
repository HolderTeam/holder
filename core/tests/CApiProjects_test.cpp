#if __has_include(<catch2/catch_test_macros.hpp>)
#include <catch2/catch_test_macros.hpp>
#else
#include <catch2/catch.hpp>
#endif

#include "core_test_helpers.h"
#include "capi/Context.h"

#include <holder/holder_api.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

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

  Fixture() {
    const auto schema = schema_sql();
    REQUIRE(holder_context_open(data_dir.string().c_str(), schema.c_str(), &context, &error) == HOLDER_OK);
  }
  ~Fixture() {
    holder_error_destroy(error);
    holder_arena_free(arena);
    holder_context_destroy(context);
  }

  const holder_project* create(const char* name) {
    holder_new_project request{};
    request.name = name;
    request.privacy_mode = HOLDER_NEW_PROJECT_PRIVACY_MODE_PLAIN;
    const holder_project* project = nullptr;
    REQUIRE(holder_create_project(context, &request, arena, &project, &error) == HOLDER_OK);
    return project;
  }

  const holder_project_list_item_list* list() {
    const holder_project_list_item_list* projects = nullptr;
    REQUIRE(holder_list_projects(context, nullptr, nullptr, nullptr, HOLDER_LIST_PROJECTS_ORDER_NONE,
                                 nullptr, 0, false, arena, &projects, &error) == HOLDER_OK);
    return projects;
  }
};

} // namespace

TEST_CASE("The generated C API creates, lists, renames and removes a plain project", "[capi][projects]") {
  Fixture f;
  REQUIRE(f.list()->count == 0);

  const holder_project* project = f.create("Home");
  const std::string id = project->project_id;
  const std::filesystem::path root = project->root_path;
  REQUIRE(std::string(project->name) == "Home");
  REQUIRE(project->privacy_mode == HOLDER_PROJECT_PRIVACY_MODE_PLAIN);
  REQUIRE(id.size() == 36);
  REQUIRE(id[14] == '4');
  REQUIRE(root.filename() == "home");
  REQUIRE(root.parent_path().filename() == "projects");
  REQUIRE(std::filesystem::is_directory(root / ".git"));
  REQUIRE(std::filesystem::is_regular_file(root / ".holder" / "privacy.json"));
  REQUIRE(std::filesystem::is_regular_file(root / ".holder" / "project.json"));

  const holder_project* fetched = nullptr;
  REQUIRE(holder_get_project(f.context, id.c_str(), f.arena, &fetched, &f.error) == HOLDER_OK);
  REQUIRE(std::string(fetched->name) == "Home");
  REQUIRE(f.list()->count == 1);

  holder_new_card card_request{};
  card_request.title = "Welcome";
  const holder_card* card = nullptr;
  REQUIRE(holder_create_card(f.context, id.c_str(), &card_request, f.arena, &card, &f.error) == HOLDER_OK);
  const std::string card_id = card->card_id;

  holder_project_update change{};
  change.name = "Personal";
  REQUIRE(holder_update_project(f.context, id.c_str(), &change, f.arena, &fetched, &f.error) == HOLDER_OK);
  REQUIRE(std::string(fetched->name) == "Personal");
  REQUIRE(std::string(f.list()->items[0].name) == "Personal");

  REQUIRE(holder_delete_project(f.context, id.c_str(), f.arena, &fetched, &f.error) == HOLDER_OK);
  REQUIRE(f.list()->count == 0);
  REQUIRE(holder_get_card(f.context, id.c_str(), card_id.c_str(), f.arena, &card, &f.error) ==
          HOLDER_ERROR_NOT_FOUND);
}

TEST_CASE("The generated C API validates project requests", "[capi][projects]") {
  Fixture f;
  holder_new_project request{};
  request.name = "Home";
  request.privacy_mode = HOLDER_NEW_PROJECT_PRIVACY_MODE_PLAIN;
  const holder_project* project = nullptr;
  REQUIRE(holder_create_project(nullptr, &request, f.arena, &project, &f.error) == HOLDER_ERROR_INVALID_ARGUMENT);
  REQUIRE(project == nullptr);
  holder_error_destroy(f.error);
  f.error = nullptr;

  request.name = "";
  REQUIRE(holder_create_project(f.context, &request, f.arena, &project, &f.error) == HOLDER_ERROR_INVALID_ARGUMENT);
  REQUIRE(std::string(holder_error_message(f.error)).find("name") != std::string::npos);
  holder_error_destroy(f.error);
  f.error = nullptr;

  holder_project_update change{};
  change.name = "Personal";
  REQUIRE(holder_update_project(f.context, "missing-project", &change, f.arena, &project, &f.error) ==
          HOLDER_ERROR_NOT_FOUND);
  holder_error_destroy(f.error);
  f.error = nullptr;
  REQUIRE(holder_delete_project(f.context, "missing-project", f.arena, &project, &f.error) ==
          HOLDER_ERROR_NOT_FOUND);
}

TEST_CASE("The generated C API reports sync outcomes without a remote", "[capi][git]") {
  Fixture f;
  const std::string id = f.create("Offline")->project_id;
  const holder_sync_state* state = nullptr;
  REQUIRE(holder_get_project_sync_state(f.context, id.c_str(), f.arena, &state, &f.error) == HOLDER_OK);
  REQUIRE(state->last_push_status == nullptr);

  const holder_sync_result* result = nullptr;
  holder_push_options options{};
  REQUIRE(holder_push_project(f.context, id.c_str(), &options, f.arena, &result, &f.error) == HOLDER_OK);
  REQUIRE(result->push.status == HOLDER_PUSH_PHASE_STATUS_REMOTE_UNSET);
  REQUIRE(holder_pull_project(f.context, id.c_str(), f.arena, &result, &f.error) == HOLDER_OK);
  REQUIRE(result->pull.status == HOLDER_PULL_PHASE_STATUS_REMOTE_UNSET);

  const holder_due_sync_result* due = nullptr;
  REQUIRE(holder_sync_project_if_due(f.context, id.c_str(), 60, 60, f.arena, &due, &f.error) == HOLDER_OK);
  REQUIRE_FALSE(due->push.attempted);
  REQUIRE_FALSE(due->pull.attempted);
}

TEST_CASE("The generated C API accepts an explicit project root", "[capi][projects]") {
  Fixture f;
  const auto root = f.data_dir / "custom-root";
  const std::string root_text = root.string();
  holder_new_project request{};
  request.name = "Custom";
  request.root_path = root_text.c_str();
  request.privacy_mode = HOLDER_NEW_PROJECT_PRIVACY_MODE_PLAIN;
  const holder_project* project = nullptr;
  REQUIRE(holder_create_project(f.context, &request, f.arena, &project, &f.error) == HOLDER_OK);
  REQUIRE(std::filesystem::equivalent(project->root_path, root));
  REQUIRE(std::filesystem::is_directory(root / ".git"));
}

TEST_CASE("The generated C API exports and restores an encrypted project's recovery token", "[capi][privacy]") {
  const auto keystore_dir = holder::test::make_temp_dir();
  holder::test::EnvGuard keystore("HOLDER_TEST_KEYSTORE_DIR", keystore_dir.string());
  Fixture f;
  holder_new_project request{};
  request.name = "Secret";
  request.privacy_mode = HOLDER_NEW_PROJECT_PRIVACY_MODE_ENCRYPTED_GIT;
  const holder_project* project = nullptr;
  REQUIRE(holder_create_project(f.context, &request, f.arena, &project, &f.error) == HOLDER_OK);
  const std::string id = project->project_id;

  const holder_project_encryption_check* safety = nullptr;
  REQUIRE(holder_check_project_encryption(f.context, id.c_str(), f.arena, &safety, &f.error) == HOLDER_OK);
  REQUIRE(safety->check.ok);
  holder_recovery_token_export_request export_request{"1234"};
  const holder_project_recovery_token_export_result* exported = nullptr;
  REQUIRE(holder_export_project_recovery_token(f.context, id.c_str(), &export_request, f.arena,
                                               &exported, &f.error) == HOLDER_OK);
  const std::string token = exported->recovery_token;
  REQUIRE_FALSE(token.empty());

  holder_recovery_token_import_request wrong_pin{"0000", token.c_str()};
  const holder_recovery_token_global_import_result* imported = nullptr;
  REQUIRE(holder_import_recovery_token_global(f.context, &wrong_pin, f.arena, &imported, &f.error) ==
          HOLDER_ERROR_RECOVERY_TOKEN_INVALID);
  holder_error_destroy(f.error);
  f.error = nullptr;

  holder_recovery_token_import_request valid{"1234", token.c_str()};
  REQUIRE(holder_import_recovery_token_global(f.context, &valid, f.arena, &imported, &f.error) == HOLDER_OK);
  REQUIRE(std::string(imported->project_id) == id);
  REQUIRE_FALSE(imported->project_created);

  const auto other_dir = holder::test::make_temp_dir();
  const auto schema = schema_sql();
  holder_context* other = nullptr;
  REQUIRE(holder_context_open(other_dir.string().c_str(), schema.c_str(), &other, &f.error) == HOLDER_OK);
  REQUIRE(holder_import_recovery_token_global(other, &valid, f.arena, &imported, &f.error) == HOLDER_OK);
  REQUIRE(std::string(imported->project_id) == id);
  REQUIRE(imported->project_created);
  REQUIRE_FALSE(imported->remote_hint_present);
  holder_context_destroy(other);
}

TEST_CASE("Generated project ordering is applied before pagination", "[capi][project-order]") {
  Fixture f;
  const std::string a = f.create("Alpha")->project_id;
  const std::string m = f.create("Middle")->project_id;
  const std::string z = f.create("Zulu")->project_id;
  // Deliberately disagreeing creation/update/name orders, without sleeps or clock assumptions.
  f.context->db.exec(
      "UPDATE projects SET created_at=10, updated_at=30 WHERE project_id='" + a + "';"
  );
  f.context->db.exec(
      "UPDATE projects SET created_at=20, updated_at=10 WHERE project_id='" + m + "';"
  );
  f.context->db.exec(
      "UPDATE projects SET created_at=30, updated_at=20 WHERE project_id='" + z + "';"
  );
  struct Case {
    holder_list_projects_order order;
    std::vector<std::string> ids;
  };
  const Case cases[] = {
      {HOLDER_LIST_PROJECTS_ORDER_NAME_ASC, {a, m, z}},
      {HOLDER_LIST_PROJECTS_ORDER_NAME_DESC, {z, m, a}},
      {HOLDER_LIST_PROJECTS_ORDER_CREATED_AT_ASC, {a, m, z}},
      {HOLDER_LIST_PROJECTS_ORDER_CREATED_AT_DESC, {z, m, a}},
      {HOLDER_LIST_PROJECTS_ORDER_UPDATED_AT_ASC, {m, z, a}},
      {HOLDER_LIST_PROJECTS_ORDER_UPDATED_AT_DESC, {a, z, m}}
  };
  for (const auto& entry : cases) {
    const holder_project_list_item_list* listed = nullptr;
    REQUIRE(
        holder_list_projects(
            f.context,
            nullptr,
            nullptr,
            nullptr,
            entry.order,
            nullptr,
            0,
            false,
            f.arena,
            &listed,
            &f.error
        ) == HOLDER_OK
    );
    REQUIRE(listed->count == 3);
    for (size_t i = 0; i < listed->count; ++i)
      REQUIRE(std::string(listed->items[i].project_id) == entry.ids[i]);
    const int32_t limit = 1;
    REQUIRE(
        holder_list_projects(
            f.context,
            nullptr,
            nullptr,
            nullptr,
            entry.order,
            &limit,
            1,
            false,
            f.arena,
            &listed,
            &f.error
        ) == HOLDER_OK
    );
    REQUIRE(listed->count == 1);
    REQUIRE(std::string(listed->items[0].project_id) == entry.ids[1]);
  }
}
