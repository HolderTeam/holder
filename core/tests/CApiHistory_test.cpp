#include "core_test_helpers.h"
#include "git/GitRepo.h"
#include "git_revision_test_helpers.h"
#include "capi/Context.h"
#include "card/CardPaths.h"
#include "card/CardRepo.h"
#include "project/ProjectRepo.h"

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

  explicit HistoryFixture(bool encrypted = false) {
    std::ifstream in(SCHEMA_SQL_PATH);
    REQUIRE(in.is_open());
    const std::string schema{std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
    REQUIRE(
        holder_context_open(dir.string().c_str(), schema.c_str(), &context, &error) == HOLDER_OK
    );
    holder_new_project request{};
    request.name = "History coverage";
    request.privacy_mode = encrypted ? HOLDER_NEW_PROJECT_PRIVACY_MODE_ENCRYPTED_GIT
                                    : HOLDER_NEW_PROJECT_PRIVACY_MODE_PLAIN;
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

TEST_CASE("The default history comparison mode compares the supplied revisions",
          "[capi][broad-history-default]") {
  HistoryFixture f;
  const std::string before = f.history()->entries.items[0].last_oid;
  f.edit();
  const std::string after = f.history()->entries.items[0].last_oid;
  const holder_card_history_comparison* result = nullptr;
  REQUIRE(holder_compare_card_history(f.context, f.project.c_str(), f.card.c_str(),
      after.c_str(), before.c_str(), HOLDER_COMPARE_CARD_HISTORY_MODE_DEFAULT,
      f.arena, &result, &f.error) == HOLDER_OK);
  REQUIRE(result != nullptr);
  CHECK(std::string(result->before.body) == "Original body\n");
  CHECK(std::string(result->after.body) == "Revised body\n");
}

TEST_CASE(
    "Generated encrypted history reports key and envelope failures without mutating saved data",
    "[capi][history-privacy-errors]"
) {
  const auto key_dir = holder::test::make_temp_dir() / "keys";
  holder::test::EnvGuard keystore("HOLDER_TEST_KEYSTORE_DIR", key_dir.string());
  HistoryFixture f(true);
  const auto project = holder::project::ProjectRepo(f.context->db).get(f.project);
  REQUIRE(project.has_value());
  REQUIRE(project->project_key_id.has_value());
  const auto key_path = key_dir / (*project->project_key_id + ".key");
  REQUIRE(std::filesystem::is_regular_file(key_path));
  holder::git::GitRepo repo;
  repo.open_existing(project->root_path);
  const auto valid_revision = repo.head_oid();
  REQUIRE(valid_revision.has_value());
  std::string revision = *valid_revision;
  int expected = HOLDER_ERROR_HISTORY_KEY_UNAVAILABLE;
  int failure = 0;
  SECTION("project key ID is missing") {
    f.context->db.exec("UPDATE projects SET project_key_id=NULL;");
    failure = 0;
  }
  SECTION("project key material is missing") {
    std::filesystem::rename(key_path, key_path.string() + ".saved");
    failure = 1;
  }
  SECTION("encrypted envelope is damaged") {
    repo.write_file(holder::core::card_rel_path(f.card), "Plaintext must not become history\n");
    repo.stage_path(holder::core::card_rel_path(f.card));
    repo.commit("Damaged encrypted history");
    revision = *repo.head_oid();
    expected = HOLDER_ERROR_HISTORY_UNAVAILABLE;
    failure = 2;
  }
  const auto head = repo.head_oid();
  const auto path = std::filesystem::path(project->root_path) / holder::core::card_rel_path(f.card);
  const auto read = [&] {
    std::ifstream input(path, std::ios::binary);
    REQUIRE(input.is_open());
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
  };
  const auto bytes = read();
  const auto before = holder::card::CardRepo(f.context->db).get(f.card);
  REQUIRE(before.has_value());
  for (int operation = 0; operation < 4; ++operation) {
    INFO("history operation " << operation);
    const holder_card_history_page* page = nullptr;
    const holder_card_history_snapshot* snapshot = nullptr;
    const holder_card_history_comparison* comparison = nullptr;
    const holder_card_history_restore_result* restored = nullptr;
    int status;
    if (operation == 0)
      status = holder_list_card_history(f.context, f.project.c_str(), f.card.c_str(), nullptr,
                                        nullptr, f.arena, &page, &f.error);
    else if (operation == 1)
      status = holder_get_card_history_snapshot(f.context, f.project.c_str(), f.card.c_str(),
                                                revision.c_str(), f.arena, &snapshot, &f.error);
    else if (operation == 2)
      status = holder_compare_card_history(f.context, f.project.c_str(), f.card.c_str(),
          revision.c_str(), nullptr, HOLDER_COMPARE_CARD_HISTORY_MODE_CHANGE,
          f.arena, &comparison, &f.error);
    else
      status = holder_restore_card_history_snapshot(f.context, f.project.c_str(), f.card.c_str(),
          revision.c_str(), f.arena, &restored, &f.error);
    f.failed(status, expected);
    CHECK(page == nullptr);
    CHECK(snapshot == nullptr);
    CHECK(comparison == nullptr);
    CHECK(restored == nullptr);
    CHECK(repo.head_oid() == head);
    CHECK(read() == bytes);
    const auto after = holder::card::CardRepo(f.context->db).get(f.card);
    REQUIRE(after.has_value());
    CHECK(after->title == before->title);
    CHECK(after->updated_at == before->updated_at);
    CHECK(after->deleted_at == before->deleted_at);
  }
  if (failure == 0)
    f.context->db.exec("UPDATE projects SET project_key_id='" + *project->project_key_id + "';");
  if (failure == 1) std::filesystem::rename(key_path.string() + ".saved", key_path);
  const holder_card_history_snapshot* recovered = nullptr;
  REQUIRE(holder_get_card_history_snapshot(f.context, f.project.c_str(), f.card.c_str(),
      valid_revision->c_str(), f.arena, &recovered, &f.error) == HOLDER_OK);
  REQUIRE(recovered != nullptr);
  CHECK(std::string(recovered->snapshot.body) == "Original body\n");
}

TEST_CASE(
    "Generated history rejects ambiguous revisions without changing the saved card",
    "[capi][history-errors][ambiguous-revision]"
) {
  HistoryFixture f;
  const holder_project* project = nullptr;
  REQUIRE(holder_get_project(f.context, f.project.c_str(), f.arena, &project, &f.error) == HOLDER_OK);
  holder::git::GitRepo repo;
  repo.open_existing(project->root_path);
  const auto head = repo.head_oid();
  REQUIRE(head.has_value());
  const auto [first, second] = holder::test::write_ambiguous_commit_prefix(project->root_path);
  const auto prefix = first.substr(0, 8);
  REQUIRE(second.substr(0, 8) == prefix);
  const holder_card_history_snapshot* snapshot = nullptr;
  const holder_card_history_comparison* comparison = nullptr;
  const holder_card_history_restore_result* restored = nullptr;
  int status = HOLDER_OK;
  SECTION("snapshot") {
    status = holder_get_card_history_snapshot(f.context, f.project.c_str(), f.card.c_str(),
                                              prefix.c_str(), f.arena, &snapshot, &f.error);
  }
  SECTION("comparison target") {
    status = holder_compare_card_history(f.context, f.project.c_str(), f.card.c_str(),
        prefix.c_str(), nullptr, HOLDER_COMPARE_CARD_HISTORY_MODE_CHANGE,
        f.arena, &comparison, &f.error);
  }
  SECTION("comparison source") {
    status = holder_compare_card_history(f.context, f.project.c_str(), f.card.c_str(),
        head->c_str(), prefix.c_str(), HOLDER_COMPARE_CARD_HISTORY_MODE_SINCE,
        f.arena, &comparison, &f.error);
  }
  SECTION("restore") {
    status = holder_restore_card_history_snapshot(f.context, f.project.c_str(), f.card.c_str(),
        prefix.c_str(), f.arena, &restored, &f.error);
  }
  REQUIRE(f.error != nullptr);
  CHECK(std::string(holder_error_message(f.error)).find("ambiguous") != std::string::npos);
  f.failed(status, HOLDER_ERROR_REVISION_AMBIGUOUS);
  CHECK(snapshot == nullptr);
  CHECK(comparison == nullptr);
  CHECK(restored == nullptr);
  CHECK(repo.head_oid() == head);
  const holder_card* current = nullptr;
  REQUIRE(holder_get_card(f.context, f.project.c_str(), f.card.c_str(), f.arena, &current, &f.error) == HOLDER_OK);
  CHECK(std::string(current->title) == "Original title");
  CHECK(std::string(current->content) == "Original body\n");
  // A full object ID still resolves even when its shorter prefix is ambiguous.
  REQUIRE(holder_get_card_history_snapshot(f.context, f.project.c_str(), f.card.c_str(),
      first.c_str(), f.arena, &snapshot, &f.error) == HOLDER_OK);
  REQUIRE(snapshot != nullptr);
  CHECK_FALSE(snapshot->snapshot.exists);
  REQUIRE(holder_get_card_history_snapshot(f.context, f.project.c_str(), f.card.c_str(),
      head->c_str(), f.arena, &snapshot, &f.error) == HOLDER_OK);
  REQUIRE(snapshot != nullptr);
  CHECK(std::string(snapshot->snapshot.body) == "Original body\n");
}

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

TEST_CASE(
    "Generated history refuses oversized snapshots without changing the saved card",
    "[capi][history-errors]"
) {
  HistoryFixture f;
  const std::string body(2 * 1024 * 1024, 'x');
  holder_card_update update{};
  update.content = body.c_str();
  const holder_card* card = nullptr;
  REQUIRE(
      holder_update_card(
          f.context,
          f.project.c_str(),
          f.card.c_str(),
          &update,
          f.arena,
          &card,
          &f.error
      ) == HOLDER_OK
  );
  const std::string revision = f.history()->entries.items[0].last_oid;
  const holder_card_history_snapshot* snapshot = nullptr;
  f.failed(
      holder_get_card_history_snapshot(
          f.context,
          f.project.c_str(),
          f.card.c_str(),
          revision.c_str(),
          f.arena,
          &snapshot,
          &f.error
      ),
      HOLDER_ERROR_HISTORY_RESPONSE_TOO_LARGE
  );
  REQUIRE(snapshot == nullptr);
  REQUIRE(
      holder_get_card(f.context, f.project.c_str(), f.card.c_str(), f.arena, &card, &f.error) ==
      HOLDER_OK
  );
  REQUIRE(std::string(card->content) == body);
}

TEST_CASE(
    "Generated history reports missing repositories without recreating them",
    "[capi][history-errors]"
) {
  HistoryFixture f;
  const std::string revision = f.history()->entries.items[0].last_oid;
  const holder_project* project = nullptr;
  REQUIRE(
      holder_get_project(f.context, f.project.c_str(), f.arena, &project, &f.error) == HOLDER_OK
  );
  const auto git_dir = std::filesystem::path(project->root_path) / ".git";
  const auto moved = std::filesystem::path(project->root_path) / ".git-hidden";
  std::filesystem::rename(git_dir, moved);
  const holder_project_history_page* history = nullptr;
  f.failed(
      holder_list_project_history(
          f.context,
          f.project.c_str(),
          nullptr,
          nullptr,
          HOLDER_LIST_PROJECT_HISTORY_KIND_NONE,
          f.arena,
          &history,
          &f.error
      ),
      HOLDER_ERROR_HISTORY_UNAVAILABLE
  );
  const holder_card_history_snapshot* snapshot = nullptr;
  f.failed(
      holder_get_card_history_snapshot(
          f.context,
          f.project.c_str(),
          f.card.c_str(),
          revision.c_str(),
          f.arena,
          &snapshot,
          &f.error
      ),
      HOLDER_ERROR_HISTORY_UNAVAILABLE
  );
  REQUIRE_FALSE(std::filesystem::exists(git_dir));
  REQUIRE(std::filesystem::is_directory(moved));
}

TEST_CASE(
    "Generated project history filters each object kind even without display metadata",
    "[capi][history-filters]"
) {
  HistoryFixture f;
  const holder_project* project = nullptr;
  REQUIRE(
      holder_get_project(f.context, f.project.c_str(), f.arena, &project, &f.error) == HOLDER_OK
  );
  holder::git::GitRepo repo;
  repo.open_existing(project->root_path);
  // Historical files can predate the current format. Their paths must still be
  // listed and filtered when optional display metadata cannot be decoded.
  for (const auto* path :
       {"resources/ab/cd/abcd-resource.json",
        "locations/ab/cd/abcd-location.json",
        "ai_threads/ab/cd/abcd-thread.json",
        "unrecognised.txt"}) {
    repo.write_file(path, "{}\n");
    repo.stage_path(path);
    repo.commit(std::string("Add historical object ") + path);
  }
  struct Kind {
    holder_list_project_history_kind filter;
    holder_history_affected_object_kind result;
  };
  const Kind kinds[] = {
      {HOLDER_LIST_PROJECT_HISTORY_KIND_CARD, HOLDER_HISTORY_AFFECTED_OBJECT_KIND_CARD},
      {HOLDER_LIST_PROJECT_HISTORY_KIND_RESOURCE, HOLDER_HISTORY_AFFECTED_OBJECT_KIND_RESOURCE},
      {HOLDER_LIST_PROJECT_HISTORY_KIND_LOCATION, HOLDER_HISTORY_AFFECTED_OBJECT_KIND_LOCATION},
      {HOLDER_LIST_PROJECT_HISTORY_KIND_AI_DATA, HOLDER_HISTORY_AFFECTED_OBJECT_KIND_AI_DATA},
      {HOLDER_LIST_PROJECT_HISTORY_KIND_PROJECT_SETTINGS,
       HOLDER_HISTORY_AFFECTED_OBJECT_KIND_PROJECT_SETTINGS},
      {HOLDER_LIST_PROJECT_HISTORY_KIND_UNKNOWN, HOLDER_HISTORY_AFFECTED_OBJECT_KIND_UNKNOWN}
  };
  for (const auto& kind : kinds) {
    const holder_project_history_page* page = nullptr;
    REQUIRE(
        holder_list_project_history(
            f.context,
            f.project.c_str(),
            nullptr,
            nullptr,
            kind.filter,
            f.arena,
            &page,
            &f.error
        ) == HOLDER_OK
    );
    REQUIRE(page->activities.count > 0);
    for (size_t i = 0; i < page->activities.count; ++i) {
      const auto& objects = page->activities.items[i].affected_objects;
      REQUIRE(objects.count > 0);
      for (size_t j = 0; j < objects.count; ++j) {
        REQUIRE(objects.items[j].kind == kind.result);
        REQUIRE(objects.items[j].items.count > 0);
        REQUIRE(objects.items[j].paths.count == objects.items[j].items.count);
      }
    }
  }
}
