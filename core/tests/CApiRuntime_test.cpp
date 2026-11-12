#if __has_include(<catch2/catch_test_macros.hpp>)
#include <catch2/catch_test_macros.hpp>
#else
#include <catch2/catch.hpp>
#endif

#include "core_test_helpers.h"
#include "platform/Db.h"
#include "platform/Migrations.h"

#include <git2.h>
#include <holder/holder_api.h>

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace {

std::string schema_sql() {
  std::ifstream in(SCHEMA_SQL_PATH);
  REQUIRE(in.is_open());
  return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

std::pair<std::string, std::string> seed_project_and_card(
    holder_context* context,
    holder_arena* arena,
    holder_error** error,
    const char* content
) {
  holder_new_project project_request{};
  project_request.name = "Rebuild me";
  project_request.privacy_mode = HOLDER_NEW_PROJECT_PRIVACY_MODE_PLAIN;
  const holder_project* project = nullptr;
  REQUIRE(holder_create_project(context, &project_request, arena, &project, error) == HOLDER_OK);
  const std::string project_id = project->project_id;

  holder_new_card card_request{};
  card_request.title = "Durable card";
  card_request.content = content;
  const holder_card* card = nullptr;
  REQUIRE(
      holder_create_card(context, project_id.c_str(), &card_request, arena, &card, error) ==
      HOLDER_OK
  );
  return {project_id, card->card_id};
}

} // namespace

TEST_CASE(
    "Context open preserves durable data when recovery lacks schema SQL",
    "[capi][batch-runtime-failures]"
) {
  const auto data_dir = holder::test::make_temp_dir();
  const auto schema = schema_sql();
  holder_context* context = nullptr;
  holder_error* error = nullptr;
  holder_arena* arena = holder_arena_new();
  REQUIRE(
      holder_context_open(data_dir.string().c_str(), schema.c_str(), &context, &error) == HOLDER_OK
  );
  const auto [project_id, card_id] =
      seed_project_and_card(context, arena, &error, "Recover these bytes");
  holder_context_destroy(context);
  context = nullptr;
  const auto database = data_dir / "server/holder.db";
  std::filesystem::rename(database, database.string() + ".saved");
  REQUIRE(
      holder_context_open(data_dir.string().c_str(), nullptr, &context, &error) ==
      HOLDER_ERROR_RUNTIME
  );
  REQUIRE(error != nullptr);
  CHECK(
      std::string(holder_error_message(error)).find("schema SQL is required") != std::string::npos
  );
  CHECK(context == nullptr);
  CHECK_FALSE(std::filesystem::exists(database));
  CHECK(std::filesystem::is_regular_file(database.string() + ".saved"));
  holder_error_destroy(error);
  error = nullptr;
  REQUIRE(
      holder_context_open(data_dir.string().c_str(), schema.c_str(), &context, &error) == HOLDER_OK
  );
  const holder_card* card = nullptr;
  REQUIRE(
      holder_get_card(context, project_id.c_str(), card_id.c_str(), arena, &card, &error) ==
      HOLDER_OK
  );
  CHECK(std::string(card->content) == "Recover these bytes");
  holder_context_destroy(context);
  holder_arena_free(arena);
}

TEST_CASE(
    "Context open reports an unusable database path without removing it",
    "[capi][batch-runtime-failures]"
) {
  const auto data_dir = holder::test::make_temp_dir();
  const auto database = data_dir / "server/holder.db";
  std::filesystem::create_directories(database);
  {
    std::ofstream marker(database / "keep.txt");
    marker << "keep me";
  }
  holder_context* context = nullptr;
  holder_error* error = nullptr;
  const auto schema = schema_sql();
  REQUIRE(
      holder_context_open(data_dir.string().c_str(), schema.c_str(), &context, &error) ==
      HOLDER_ERROR_RUNTIME
  );
  REQUIRE(error != nullptr);
  CHECK(
      std::string(holder_error_message(error)).find("database I/O failure:") != std::string::npos
  );
  CHECK(context == nullptr);
  CHECK(std::filesystem::is_regular_file(database / "keep.txt"));
  holder_error_destroy(error);
}

TEST_CASE("The generated C API exposes the core version", "[capi][runtime]") {
  holder_arena* arena = holder_arena_new();
  const holder_core_version* version = nullptr;
  holder_error* error = nullptr;
  REQUIRE(holder_version(arena, &version, &error) == HOLDER_OK);
  REQUIRE(std::string(version->version) == EXPECTED_HOLDER_VERSION_STRING);
  REQUIRE(version->major == EXPECTED_HOLDER_VERSION_MAJOR);
  REQUIRE(version->minor == EXPECTED_HOLDER_VERSION_MINOR);
  REQUIRE(version->patch == EXPECTED_HOLDER_VERSION_PATCH);
  holder_arena_free(arena);
}

TEST_CASE("The generated C API configures libgit2's process-wide home", "[capi][git]") {
  holder_error* error = nullptr;
  REQUIRE(holder_configure_git_home(nullptr, &error) == HOLDER_ERROR_INVALID_ARGUMENT);
  REQUIRE(error != nullptr);
  holder_error_destroy(error);

  error = nullptr;
  REQUIRE(holder_configure_git_home("", &error) == HOLDER_ERROR_INVALID_ARGUMENT);
  REQUIRE(error != nullptr);
  holder_error_destroy(error);

  error = nullptr;
  const auto dir = holder::test::make_temp_dir();
  REQUIRE(holder_configure_git_home(dir.string().c_str(), &error) == HOLDER_OK);
  REQUIRE(error == nullptr);

  git_buf actual = GIT_BUF_INIT;
  REQUIRE(git_libgit2_opts(GIT_OPT_GET_HOMEDIR, &actual) == 0);
  const std::filesystem::path actual_path(actual.ptr);
  git_buf_dispose(&actual);
  REQUIRE(std::filesystem::equivalent(actual_path, dir));
}

TEST_CASE("The generated C API opens a context with no projects", "[capi][runtime]") {
  const auto data_dir = holder::test::make_temp_dir();
  const auto schema = schema_sql();
  holder_context* context = nullptr;
  holder_error* error = nullptr;
  holder_arena* arena = holder_arena_new();
  REQUIRE(
      holder_context_open(data_dir.string().c_str(), schema.c_str(), &context, &error) == HOLDER_OK
  );
  const holder_project_list_item_list* projects = nullptr;
  REQUIRE(
      holder_list_projects(
          context,
          nullptr,
          nullptr,
          nullptr,
          HOLDER_LIST_PROJECTS_ORDER_NONE,
          nullptr,
          0,
          false,
          arena,
          &projects,
          &error
      ) == HOLDER_OK
  );
  REQUIRE(projects->count == 0);
  holder_arena_free(arena);
  holder_context_destroy(context);
}

TEST_CASE(
    "The generated C API rebuilds its database from durable project files",
    "[capi][rebuild]"
) {
  const auto data_dir = holder::test::make_temp_dir();
  const auto schema = schema_sql();
  holder_context* context = nullptr;
  holder_error* error = nullptr;
  holder_arena* arena = holder_arena_new();
  REQUIRE(
      holder_context_open(data_dir.string().c_str(), schema.c_str(), &context, &error) == HOLDER_OK
  );
  const auto [project_id, card_id] = seed_project_and_card(context, arena, &error, "Body survives");
  holder_context_destroy(context);

  const holder_database_rebuild_report* report = nullptr;
  REQUIRE(
      holder_database_rebuild(
          data_dir.string().c_str(),
          schema.c_str(),
          true,
          arena,
          &report,
          &error
      ) == HOLDER_OK
  );
  REQUIRE(report->dry_run);
  REQUIRE(report->projects == 1);
  REQUIRE(report->cards == 1);
  REQUIRE(report->backup_path == nullptr);

  REQUIRE(std::filesystem::remove(data_dir / "server" / "holder.db"));
  REQUIRE(
      holder_database_rebuild(
          data_dir.string().c_str(),
          schema.c_str(),
          false,
          arena,
          &report,
          &error
      ) == HOLDER_OK
  );
  REQUIRE(std::string(report->previous_health) == "missing");
  REQUIRE(report->projects == 1);
  REQUIRE(report->cards == 1);
  const auto temporary = data_dir / "server" / "holder.db.rebuild.tmp";
  REQUIRE_FALSE(std::filesystem::exists(temporary));
  REQUIRE_FALSE(std::filesystem::exists(temporary.string() + "-wal"));
  REQUIRE_FALSE(std::filesystem::exists(temporary.string() + "-shm"));

  REQUIRE(
      holder_context_open(data_dir.string().c_str(), schema.c_str(), &context, &error) == HOLDER_OK
  );
  const holder_card* card = nullptr;
  REQUIRE(
      holder_get_card(context, project_id.c_str(), card_id.c_str(), arena, &card, &error) ==
      HOLDER_OK
  );
  REQUIRE(std::string(card->content) == "Body survives");
  holder_context_destroy(context);

  {
    std::ofstream damaged(data_dir / "server" / "holder.db", std::ios::binary | std::ios::trunc);
    damaged << "not a sqlite database";
  }
  REQUIRE(
      holder_context_open(data_dir.string().c_str(), schema.c_str(), &context, &error) == HOLDER_OK
  );
  REQUIRE(
      holder_get_card(context, project_id.c_str(), card_id.c_str(), arena, &card, &error) ==
      HOLDER_OK
  );
  REQUIRE(std::string(card->content) == "Body survives");
  holder_context_destroy(context);
  holder_arena_free(arena);
}

TEST_CASE("The generated C API rebuilds an older database schema", "[capi][rebuild]") {
  const auto data_dir = holder::test::make_temp_dir();
  const auto schema = schema_sql();
  holder_context* context = nullptr;
  holder_error* error = nullptr;
  holder_arena* arena = holder_arena_new();
  REQUIRE(
      holder_context_open(data_dir.string().c_str(), schema.c_str(), &context, &error) == HOLDER_OK
  );
  const auto [project_id, card_id] =
      seed_project_and_card(context, arena, &error, "Body survives an app update");
  holder_context_destroy(context);

  {
    holder::platform::Db db;
    db.open(data_dir / "server" / "holder.db");
    db.exec(
        "UPDATE schema_version SET version = " +
        std::to_string(holder::platform::Migrations::latest_schema_version - 1) + ";"
    );
  }

  REQUIRE(
      holder_context_open(data_dir.string().c_str(), schema.c_str(), &context, &error) == HOLDER_OK
  );
  const holder_card* card = nullptr;
  REQUIRE(
      holder_get_card(context, project_id.c_str(), card_id.c_str(), arena, &card, &error) ==
      HOLDER_OK
  );
  REQUIRE(std::string(card->content) == "Body survives an app update");
  holder_context_destroy(context);

  holder::platform::Db reopened;
  reopened.open(data_dir / "server" / "holder.db");
  REQUIRE(
      holder::platform::Migrations::read_schema_version(reopened) ==
      holder::platform::Migrations::latest_schema_version
  );
  holder_arena_free(arena);
}

TEST_CASE(
    "Generated C API contexts on one data directory work on separate threads",
    "[capi][runtime]"
) {
  const auto data_dir = holder::test::make_temp_dir();
  const auto schema = schema_sql();
  holder_context* writer = nullptr;
  holder_context* reader = nullptr;
  holder_error* error = nullptr;
  holder_arena* setup_arena = holder_arena_new();
  REQUIRE(
      holder_context_open(data_dir.string().c_str(), schema.c_str(), &writer, &error) == HOLDER_OK
  );

  holder_new_project project_request{};
  project_request.name = "Home";
  project_request.privacy_mode = HOLDER_NEW_PROJECT_PRIVACY_MODE_PLAIN;
  const holder_project* project = nullptr;
  REQUIRE(
      holder_create_project(writer, &project_request, setup_arena, &project, &error) == HOLDER_OK
  );
  const std::string project_id = project->project_id;
  std::string moved_id;
  for (const char* title : {"A", "B", "C"}) {
    holder_new_card request{};
    request.title = title;
    const holder_card* card = nullptr;
    REQUIRE(
        holder_create_card(writer, project_id.c_str(), &request, setup_arena, &card, &error) ==
        HOLDER_OK
    );
    moved_id = card->card_id;
  }
  REQUIRE(
      holder_context_open(data_dir.string().c_str(), schema.c_str(), &reader, &error) == HOLDER_OK
  );

  std::atomic<bool> moving{true};
  std::atomic<int> failures{0};
  std::atomic<int> reads{0};
  std::thread mover([&]() {
    holder_arena* arena = holder_arena_new();
    for (int i = 0; i < 20; ++i) {
      holder_card_move move{};
      move.intent = i % 2 == 0 ? HOLDER_CARD_MOVE_INTENT_TO_END : HOLDER_CARD_MOVE_INTENT_TO_START;
      const holder_card* card = nullptr;
      holder_error* move_error = nullptr;
      if (holder_move_card(
              writer,
              project_id.c_str(),
              moved_id.c_str(),
              &move,
              arena,
              &card,
              &move_error
          ) != HOLDER_OK) {
        ++failures;
        holder_error_destroy(move_error);
      }
    }
    holder_arena_free(arena);
    moving = false;
  });
  std::thread lister([&]() {
    holder_arena* arena = holder_arena_new();
    while (moving || reads == 0) {
      const holder_card_list_item_page* page = nullptr;
      holder_error* list_error = nullptr;
      if (holder_list_cards(
              reader,
              project_id.c_str(),
              {},
              nullptr,
              HOLDER_LIST_CARDS_ORDER_DEFAULT,
              nullptr,
              nullptr,
              false,
              false,
              arena,
              &page,
              &list_error
          ) != HOLDER_OK ||
          page->items.count != 3) {
        ++failures;
        holder_error_destroy(list_error);
      }
      ++reads;
    }
    holder_arena_free(arena);
  });
  mover.join();
  lister.join();
  REQUIRE(failures == 0);

  const holder_card_list_item_page* page = nullptr;
  REQUIRE(
      holder_list_cards(
          reader,
          project_id.c_str(),
          {},
          nullptr,
          HOLDER_LIST_CARDS_ORDER_DEFAULT,
          nullptr,
          nullptr,
          false,
          false,
          setup_arena,
          &page,
          &error
      ) == HOLDER_OK
  );
  double moved_key = 0;
  double lowest_other_key = std::numeric_limits<double>::max();
  for (size_t i = 0; i < page->items.count; ++i) {
    const auto& card = page->items.items[i];
    if (std::string(card.card_id) == moved_id)
      moved_key = card.sort_key;
    else
      lowest_other_key = std::min(lowest_other_key, card.sort_key);
  }
  REQUIRE(moved_key < lowest_other_key);
  holder_arena_free(setup_arena);
  holder_context_destroy(reader);
  holder_context_destroy(writer);
}

TEST_CASE("The generated C API reports database failures as runtime errors", "[capi][runtime]") {
  const auto data_dir = holder::test::make_temp_dir();
  const auto schema = schema_sql();
  holder_context* context = nullptr;
  holder_error* error = nullptr;
  holder_arena* arena = holder_arena_new();
  REQUIRE(
      holder_context_open(data_dir.string().c_str(), schema.c_str(), &context, &error) == HOLDER_OK
  );
  {
    holder::platform::Db db;
    db.open(data_dir / "server" / "holder.db");
    db.exec("DROP TABLE projects;");
  }
  const holder_project_list_item_list* projects = nullptr;
  REQUIRE(
      holder_list_projects(
          context,
          nullptr,
          nullptr,
          nullptr,
          HOLDER_LIST_PROJECTS_ORDER_NONE,
          nullptr,
          0,
          false,
          arena,
          &projects,
          &error
      ) == HOLDER_ERROR_RUNTIME
  );
  REQUIRE(error != nullptr);
  REQUIRE(projects == nullptr);
  holder_error_destroy(error);
  holder_arena_free(arena);
  holder_context_destroy(context);
}

TEST_CASE(
    "The generated C API validates runtime arguments and reports malformed schema SQL",
    "[capi][runtime]"
) {
  holder_error* error = nullptr;
  holder_context* context = nullptr;
  REQUIRE(holder_context_open(nullptr, nullptr, nullptr, &error) == HOLDER_ERROR_INVALID_ARGUMENT);
  REQUIRE(std::string(holder_error_message(error)).find("out_context") != std::string::npos);
  holder_error_destroy(error);
  error = nullptr;
  REQUIRE(std::string(holder_error_message(nullptr)).empty());

  const auto data_dir = holder::test::make_temp_dir();
  REQUIRE(
      holder_context_open("", schema_sql().c_str(), &context, &error) ==
      HOLDER_ERROR_INVALID_ARGUMENT
  );
  REQUIRE(context == nullptr);
  REQUIRE(std::string(holder_error_message(error)).find("data_dir") != std::string::npos);
  holder_error_destroy(error);
  error = nullptr;
  REQUIRE(
      holder_context_open(data_dir.string().c_str(), "not valid SQL", &context, &error) ==
      HOLDER_ERROR_RUNTIME
  );
  REQUIRE(context == nullptr);
  holder_error_destroy(error);
  error = nullptr;

  holder_arena* arena = holder_arena_new();
  const holder_database_rebuild_report* report = nullptr;
  const auto schema = schema_sql();
  REQUIRE(
      holder_database_rebuild(
          data_dir.string().c_str(),
          schema.c_str(),
          true,
          arena,
          nullptr,
          &error
      ) == HOLDER_ERROR_INVALID_ARGUMENT
  );
  holder_error_destroy(error);
  error = nullptr;
  REQUIRE(
      holder_database_rebuild("", schema.c_str(), true, arena, &report, &error) ==
      HOLDER_ERROR_INVALID_ARGUMENT
  );
  holder_error_destroy(error);
  error = nullptr;
  REQUIRE(
      holder_database_rebuild(data_dir.string().c_str(), "", true, arena, &report, &error) ==
      HOLDER_ERROR_INVALID_ARGUMENT
  );
  holder_error_destroy(error);
  error = nullptr;
  REQUIRE(
      holder_database_rebuild(
          data_dir.string().c_str(),
          "not valid SQL",
          true,
          arena,
          &report,
          &error
      ) == HOLDER_ERROR_RUNTIME
  );
  REQUIRE(report == nullptr);
  holder_error_destroy(error);
  holder_arena_free(arena);
}
