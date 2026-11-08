#include "core_test_helpers.h"

#include <holder/holder_api.h>

#include <fstream>
#include <set>
#include <string>

namespace {

struct HistoryFixture {
  std::filesystem::path dir = holder::test::make_temp_dir();
  holder_context* context = nullptr;
  holder_arena* arena = holder_arena_new();
  holder_error* error = nullptr;
  std::string project;
  std::string card;

  HistoryFixture() {
    std::ifstream in(SCHEMA_SQL_PATH);
    REQUIRE(in.is_open());
    const std::string schema{std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
    REQUIRE(
        holder_context_open(dir.string().c_str(), schema.c_str(), &context, &error) == HOLDER_OK
    );
    holder_new_project request{};
    request.name = "History coverage";
    request.privacy_mode = HOLDER_NEW_PROJECT_PRIVACY_MODE_PLAIN;
    const holder_project* made = nullptr;
    REQUIRE(holder_create_project(context, &request, arena, &made, &error) == HOLDER_OK);
    project = made->project_id;
    holder_new_card new_card{};
    new_card.title = "Original title";
    new_card.content = "Original body\n";
    const holder_card* created = nullptr;
    REQUIRE(
        holder_create_card(context, project.c_str(), &new_card, arena, &created, &error) ==
        HOLDER_OK
    );
    card = created->card_id;
  }

  ~HistoryFixture() {
    holder_error_destroy(error);
    holder_arena_free(arena);
    holder_context_destroy(context);
  }

  const holder_card_history_page* history() {
    const holder_card_history_page* page = nullptr;
    REQUIRE(
        holder_list_card_history(
            context,
            project.c_str(),
            card.c_str(),
            nullptr,
            nullptr,
            arena,
            &page,
            &error
        ) == HOLDER_OK
    );
    REQUIRE(page != nullptr);
    return page;
  }

  void edit() {
    holder_card_update update{};
    update.title = "Revised title";
    update.content = "Revised body\n";
    const holder_card* result = nullptr;
    REQUIRE(
        holder_update_card(
            context,
            project.c_str(),
            card.c_str(),
            &update,
            arena,
            &result,
            &error
        ) == HOLDER_OK
    );
  }

  void failed(int status, int expected) {
    REQUIRE(status == expected);
    REQUIRE(error != nullptr);
    REQUIRE(std::string(holder_error_message(error)).size() > 0);
    holder_error_destroy(error);
    error = nullptr;
  }
};

} // namespace

TEST_CASE(
    "Generated card history preserves snapshots, diff line numbers, and restored content",
    "[capi][history]"
) {
  HistoryFixture f;
  const auto* initial = f.history();
  REQUIRE(initial->entries.count == 1);
  const auto& entry = initial->entries.items[0];
  const std::string original = entry.last_oid;
  REQUIRE(original.size() == 40);
  REQUIRE(entry.commit_count == 1);
  REQUIRE(entry.saves.count == 1);
  REQUIRE(std::string(entry.saves.items[0].oid) == original);
  REQUIRE(entry.author.name != nullptr);
  REQUIRE(entry.author.email != nullptr);
  REQUIRE(entry.summary != nullptr);
  REQUIRE(entry.saves.items[0].message != nullptr);

  const holder_card_history_comparison* creation = nullptr;
  REQUIRE(
      holder_compare_card_history(
          f.context,
          f.project.c_str(),
          f.card.c_str(),
          original.c_str(),
          nullptr,
          HOLDER_COMPARE_CARD_HISTORY_MODE_CHANGE,
          f.arena,
          &creation,
          &f.error
      ) == HOLDER_OK
  );
  REQUIRE_FALSE(creation->before.exists);
  REQUIRE(creation->after.exists);
  REQUIRE(std::string(creation->after.body) == "Original body\n");

  f.edit();
  const auto* edited = f.history();
  const std::string revised = edited->entries.items[0].last_oid;
  REQUIRE(revised != original);
  const holder_card_history_snapshot* snapshot = nullptr;
  const auto abbreviation = original.substr(0, 8);
  REQUIRE(
      holder_get_card_history_snapshot(
          f.context,
          f.project.c_str(),
          f.card.c_str(),
          abbreviation.c_str(),
          f.arena,
          &snapshot,
          &f.error
      ) == HOLDER_OK
  );
  REQUIRE(std::string(snapshot->card_id) == f.card);
  REQUIRE(snapshot->snapshot.exists);
  REQUIRE(std::string(snapshot->snapshot.oid) == original);
  REQUIRE(std::string(snapshot->snapshot.title) == "Original title");
  REQUIRE(std::string(snapshot->snapshot.body) == "Original body\n");

  const holder_card_history_comparison* diff = nullptr;
  REQUIRE(
      holder_compare_card_history(
          f.context,
          f.project.c_str(),
          f.card.c_str(),
          revised.c_str(),
          original.c_str(),
          HOLDER_COMPARE_CARD_HISTORY_MODE_SINCE,
          f.arena,
          &diff,
          &f.error
      ) == HOLDER_OK
  );
  REQUIRE(std::string(diff->before.oid) == original);
  REQUIRE(std::string(diff->after.oid) == revised);
  REQUIRE(std::string(diff->before.title) == "Original title");
  REQUIRE(std::string(diff->after.title) == "Revised title");
  REQUIRE_FALSE(diff->truncated);
  bool added = false;
  bool removed = false;
  for (size_t i = 0; i < diff->lines.count; ++i) {
    const auto& line = diff->lines.items[i];
    if (std::string(line.origin) == "+" &&
        std::string(line.text).find("Revised body") != std::string::npos) {
      REQUIRE(line.old_line == nullptr);
      REQUIRE(line.new_line != nullptr);
      added = true;
    }
    if (std::string(line.origin) == "-" &&
        std::string(line.text).find("Original body") != std::string::npos) {
      REQUIRE(line.old_line != nullptr);
      REQUIRE(line.new_line == nullptr);
      removed = true;
    }
  }
  REQUIRE(added);
  REQUIRE(removed);

  const holder_card_history_restore_result* restored = nullptr;
  REQUIRE(
      holder_restore_card_history_snapshot(
          f.context,
          f.project.c_str(),
          f.card.c_str(),
          original.c_str(),
          f.arena,
          &restored,
          &f.error
      ) == HOLDER_OK
  );
  REQUIRE(std::string(restored->card_id) == f.card);
  REQUIRE(std::string(restored->restored_from_oid) == original);
  REQUIRE(std::string(restored->result_oid) != revised);
  REQUIRE(std::string(restored->title) == "Original title");
  REQUIRE(restored->deleted_at == nullptr);
  const holder_card* current = nullptr;
  REQUIRE(
      holder_get_card(f.context, f.project.c_str(), f.card.c_str(), f.arena, &current, &f.error) ==
      HOLDER_OK
  );
  REQUIRE(std::string(current->content) == "Original body\n");
  REQUIRE(std::string(current->title) == "Original title");
  // Results from earlier calls remain valid until the caller frees its arena.
  REQUIRE(std::string(snapshot->snapshot.body) == "Original body\n");
}

TEST_CASE(
    "Generated project history pages without duplicates and includes historical path details",
    "[capi][history]"
) {
  HistoryFixture f;
  f.edit();
  const int32_t limit = 1;
  std::string cursor;
  std::set<std::string> commits;
  bool card_path = false;
  for (int page_number = 0; page_number < 10; ++page_number) {
    const holder_project_history_page* page = nullptr;
    REQUIRE(
        holder_list_project_history(
            f.context,
            f.project.c_str(),
            &limit,
            cursor.empty() ? nullptr : cursor.c_str(),
            HOLDER_LIST_PROJECT_HISTORY_KIND_NONE,
            f.arena,
            &page,
            &f.error
        ) == HOLDER_OK
    );
    REQUIRE(page != nullptr);
    REQUIRE(page->head_oid != nullptr);
    REQUIRE(page->activities.count <= 1);
    REQUIRE_FALSE(page->scan_limited);
    for (size_t i = 0; i < page->activities.count; ++i) {
      const auto& activity = page->activities.items[i];
      REQUIRE(commits.insert(activity.oid).second);
      REQUIRE(activity.author.name != nullptr);
      REQUIRE(activity.message != nullptr);
      for (size_t j = 0; j < activity.affected_objects.count; ++j) {
        const auto& object = activity.affected_objects.items[j];
        REQUIRE(object.paths.count == object.items.count);
        for (size_t k = 0; k < object.items.count; ++k) {
          REQUIRE(std::string(object.paths.items[k]) == object.items.items[k].path);
          if (object.kind == HOLDER_HISTORY_AFFECTED_OBJECT_KIND_CARD) {
            REQUIRE(object.items.items[k].title != nullptr);
            card_path = true;
          }
        }
      }
    }
    if (!page->next_cursor) break;
    REQUIRE(page->next_cursor != cursor);
    cursor = page->next_cursor;
    REQUIRE(page_number < 9);
  }
  REQUIRE(commits.size() >= 3);
  REQUIRE(card_path);
}

TEST_CASE(
    "Generated history reports invalid revisions, cursors, limits and missing projects",
    "[capi][history]"
) {
  HistoryFixture f;
  const std::string oid = f.history()->entries.items[0].last_oid;
  const holder_card_history_comparison* diff = nullptr;
  f.failed(
      holder_compare_card_history(
          f.context,
          f.project.c_str(),
          f.card.c_str(),
          oid.c_str(),
          nullptr,
          HOLDER_COMPARE_CARD_HISTORY_MODE_SINCE,
          f.arena,
          &diff,
          &f.error
      ),
      HOLDER_ERROR_INVALID_ARGUMENT
  );
  const holder_card_history_snapshot* snapshot = nullptr;
  f.failed(
      holder_get_card_history_snapshot(
          f.context,
          f.project.c_str(),
          f.card.c_str(),
          "bad-revision",
          f.arena,
          &snapshot,
          &f.error
      ),
      HOLDER_ERROR_INVALID_ARGUMENT
  );
  const std::string absent(40, '0');
  f.failed(
      holder_get_card_history_snapshot(
          f.context,
          f.project.c_str(),
          f.card.c_str(),
          absent.c_str(),
          f.arena,
          &snapshot,
          &f.error
      ),
      HOLDER_ERROR_NOT_FOUND
  );
  const holder_card_history_page* cards = nullptr;
  f.failed(
      holder_list_card_history(
          f.context,
          f.project.c_str(),
          f.card.c_str(),
          nullptr,
          "bad-cursor",
          f.arena,
          &cards,
          &f.error
      ),
      HOLDER_ERROR_INVALID_ARGUMENT
  );
  const holder_project_history_page* projects = nullptr;
  const int32_t zero = 0;
  f.failed(
      holder_list_project_history(
          f.context,
          f.project.c_str(),
          &zero,
          nullptr,
          HOLDER_LIST_PROJECT_HISTORY_KIND_NONE,
          f.arena,
          &projects,
          &f.error
      ),
      HOLDER_ERROR_INVALID_ARGUMENT
  );
  f.failed(
      holder_list_project_history(
          f.context,
          "00000000-0000-4000-8000-000000000001",
          nullptr,
          nullptr,
          HOLDER_LIST_PROJECT_HISTORY_KIND_NONE,
          f.arena,
          &projects,
          &f.error
      ),
      HOLDER_ERROR_NOT_FOUND
  );
}
