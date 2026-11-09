#include "ai/AiMessageRepo.h"
#include "ai/AiThreadRepo.h"
#include "capi/Context.h"
#include "capi/StorageProviders.h"
#include "card/CardRepo.h"
#include "card/CardPaths.h"
#include "card/LinkMutation.h"
#include "core_test_helpers.h"
#if __has_include(<catch2/matchers/catch_matchers_string.hpp>)
#include <catch2/matchers/catch_matchers_string.hpp>
#endif
#include "sqlite_read_race_test_helpers.h"
#include "card/CardStore.h"
#include "git/GitOps.h"
#include "git/EcdsaDerSigningCredentialProvider.h"
#include "project/ProjectRepo.h"
#include "resource/ResourceRepo.h"
#include "resource/LocationRepo.h"
#include "card/LinkRepo.h"
#include "platform/Tx.h"

#include <holder/holder_api.h>
#include <memory>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <set>

namespace {
struct Host {
  std::filesystem::path dir = holder::test::make_temp_dir();
  holder::test::EnvGuard keystore{"HOLDER_TEST_KEYSTORE_DIR", (dir / "keystore").string()};
  holder::platform::Db db = holder::test::open_db_with_schema(dir / "holder.db");
  holder::index::FtsIndexer fts{db};
  holder::git::RealGitOps git;
  holder_context context{db, fts, &git};
  std::unique_ptr<holder_arena, decltype(&holder_arena_free)> arena{
      holder_arena_new(),
      holder_arena_free
  };
  holder_error* error = nullptr;

  Host() { context.projects_root = dir / "projects"; }
  ~Host() { holder_error_destroy(error); }

  std::string project() {
    holder_new_project request{};
    request.name = "Maintenance";
    request.privacy_mode = HOLDER_NEW_PROJECT_PRIVACY_MODE_PLAIN;
    const holder_project* result = nullptr;
    REQUIRE(holder_create_project(&context, &request, arena.get(), &result, &error) == HOLDER_OK);
    return result->project_id;
  }
  std::string card(const std::string& project, const char* title, const char* parent = nullptr) {
    holder_new_card request{};
    request.title = title;
    request.parent_card_id = parent;
    const holder_card* result = nullptr;
    REQUIRE(
        holder_create_card(&context, project.c_str(), &request, arena.get(), &result, &error) ==
        HOLDER_OK
    );
    return result->card_id;
  }
  void message(const std::string& project, const std::string& id, const std::string& body) {
    holder::model::AiThread thread;
    thread.thread_id = "thread-" + id;
    thread.project_id = project;
    thread.title = "Conversation";
    thread.created_at = thread.updated_at = 10;
    holder::ai::AiThreadRepo(db).create(thread);
    holder::model::AiMessage message;
    message.message_id = id;
    message.thread_id = thread.thread_id;
    message.role = "user";
    message.source = "manual";
    message.content = body;
    message.created_at = 11;
    holder::ai::AiMessageRepo(db, &fts).append(message);
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
    "Card creation rejects a cyclic existing parent chain without writing a new card",
    "[capi][cyclic-create-parent]"
) {
  Host host;
  const auto project = host.project();
  const auto first = host.card(project, "First");
  const auto second = host.card(project, "Second", first.c_str());
  host.db.exec("UPDATE cards SET parent_card_id='" + second + "' WHERE card_id='" + first + "';");
  const auto record = holder::project::ProjectRepo(host.db).get(project);
  REQUIRE(record.has_value());
  holder::git::GitRepo git;
  git.open_existing(record->root_path);
  const auto head = git.head_oid();
  holder_new_card request{};
  request.title = "Cannot create";
  request.parent_card_id = first.c_str();
  const holder_card* card = nullptr;
  host.failed(
      holder_create_card(
          &host.context,
          project.c_str(),
          &request,
          host.arena.get(),
          &card,
          &host.error
      ),
      HOLDER_ERROR_INVALID_ARGUMENT
  );
  CHECK(card == nullptr);
  CHECK(holder::card::CardRepo(host.db).list_all(project).size() == 2);
  CHECK(git.head_oid() == head);
}

TEST_CASE("Card moves revalidate source and parent changes before writing", "[capi][broad-move-race]") {
  Host host;
  const auto project = host.project();
  const auto source = host.card(project, "Source");
  const auto target = host.card(project, "Target");
  const auto row = holder::project::ProjectRepo(host.db).get(project);
  REQUIRE(row.has_value());
  holder::git::GitRepo git;
  git.open_existing(row->root_path);
  const auto head = git.head_oid();
  int operation = 0;
  int expected = HOLDER_ERROR_NOT_FOUND;
  std::string mutation;
  SECTION("source is trashed after the initial lookup") {
    mutation = "UPDATE cards SET deleted_at=10 WHERE card_id='" + source + "';";
  }
  SECTION("a parent cycle appears after the initial check") {
    mutation = "UPDATE cards SET parent_card_id='" + source + "' WHERE card_id='" + target + "';";
    expected = HOLDER_ERROR_INVALID_MOVE;
  }
  SECTION("an update's parent disappears after validation") {
    operation = 1;
    mutation = "UPDATE cards SET deleted_at=10 WHERE card_id='" + target + "';";
  }
  holder::test::SqliteReadRace race(host.db.handle(), "FROM cards WHERE card_id", source, 2, mutation);
  const holder_card* result = nullptr;
  if (operation == 0) {
    holder_card_move move{};
    move.intent = HOLDER_CARD_MOVE_INTENT_INTO;
    move.target_card_id = target.c_str();
    host.failed(holder_move_card(&host.context, project.c_str(), source.c_str(), &move,
        host.arena.get(), &result, &host.error), expected);
  } else {
    holder_card_update update{};
    update.parent_card_id = {HOLDER_SET, target.c_str()};
    host.failed(holder_update_card(&host.context, project.c_str(), source.c_str(), &update,
        host.arena.get(), &result, &host.error), expected);
  }
  REQUIRE(race.fired);
  REQUIRE(race.result == SQLITE_OK);
  CHECK(result == nullptr);
  CHECK(git.head_oid() == head);
  CHECK_FALSE(holder::card::CardRepo(host.db).get(source)->parent_card_id.has_value());
}

TEST_CASE("Permanent deletion rechecks a card or message's project ownership",
          "[capi][broad-purge-race]") {
  Host host;
  const auto project = host.project();
  const auto other = host.project();
  SECTION("trashed card changes projects before the locked read") {
    const auto id = host.card(project, "Keep card");
    holder::card::CardStore store(host.db, &host.fts, nullptr, &host.git);
    store.trash(id, 10);
    const auto row = holder::project::ProjectRepo(host.db).get(project);
    holder::git::GitRepo git;
    git.open_existing(row->root_path);
    const auto head = git.head_oid();
    holder::test::SqliteReadRace race(host.db.handle(), "FROM cards WHERE card_id", id, 2,
        "UPDATE cards SET project_id='" + other + "' WHERE card_id='" + id + "';");
    REQUIRE_THROWS_WITH(store.hard_delete(id, project),
        Catch::Matchers::ContainsSubstring("card not found in project"));
    REQUIRE(race.fired);
    REQUIRE(race.result == SQLITE_OK);
    const auto saved = store.get(id);
    REQUIRE(saved.has_value());
    CHECK(saved->project_id == other);
    CHECK(saved->deleted_at == 10);
    CHECK(git.head_oid() == head);
    CHECK(std::filesystem::is_regular_file(std::filesystem::path(row->root_path) / holder::core::card_trash_rel_path(id)));
  }
  SECTION("trashed message changes to another project's thread") {
    host.message(project, "message-first", "Keep message");
    host.message(other, "message-second", "Other message");
    holder::ai::AiMessageRepo messages(host.db, &host.fts);
    messages.trash("message-first", 10);
    holder::test::SqliteReadRace race(host.db.handle(), "FROM ai_messages WHERE message_id", "message-first", 2,
        "UPDATE ai_messages SET thread_id='thread-message-second' WHERE message_id='message-first';");
    REQUIRE_THROWS_WITH(messages.remove_trashed(project, "message-first"),
        Catch::Matchers::ContainsSubstring("ai message not found in project"));
    REQUIRE(race.fired);
    REQUIRE(race.result == SQLITE_OK);
    const auto saved = messages.get("message-first");
    REQUIRE(saved.has_value());
    CHECK(saved->thread_id == "thread-message-second");
    CHECK(saved->deleted_at == 10);
    CHECK(saved->content == "Keep message");
  }
}

TEST_CASE("Generated API aliases preserve their documented behavior", "[capi][broad-api-aliases]") {
  Host host;
  const auto project = host.project();
  const auto parent = host.card(project, "Parent");
  const auto child = host.card(project, "Child", parent.c_str());
  const holder_project_encryption_check* safety = nullptr;
  REQUIRE(holder_check_project_encryption(&host.context, project.c_str(), host.arena.get(),
      &safety, &host.error) == HOLDER_OK);
  CHECK(safety->privacy_mode == HOLDER_PROJECT_ENCRYPTION_CHECK_PRIVACY_MODE_PLAIN);
  holder_list_cards_parent scope{};
  const holder_card_list_item_page* page = nullptr;
  REQUIRE(holder_list_cards(&host.context, project.c_str(), scope, nullptr,
      HOLDER_LIST_CARDS_ORDER_CARD_ID_ASC, nullptr, nullptr, false, false,
      host.arena.get(), &page, &host.error) == HOLDER_OK);
  REQUIRE(page->items.count == 2);
  CHECK(std::string(page->items.items[0].card_id) < std::string(page->items.items[1].card_id));
  const holder_card* trashed = nullptr;
  REQUIRE(holder_trash_card(&host.context, project.c_str(), parent.c_str(),
      HOLDER_TRASH_CARD_CHILDREN_PROMOTE, host.arena.get(), &trashed, &host.error) == HOLDER_OK);
  const auto promoted = holder::card::CardRepo(host.db).get(child);
  REQUIRE(promoted.has_value());
  CHECK_FALSE(promoted->parent_card_id.has_value());
  CHECK_FALSE(promoted->deleted_at.has_value());
  const holder_trash_empty_result* emptied = nullptr;
  REQUIRE(holder_empty_trash(&host.context, project.c_str(), HOLDER_EMPTY_TRASH_TYPE_AI_MESSAGE,
      host.arena.get(), &emptied, &host.error) == HOLDER_OK);
  CHECK(holder::card::CardRepo(host.db).get(parent).has_value());
  REQUIRE(holder_empty_trash(&host.context, project.c_str(), HOLDER_EMPTY_TRASH_TYPE_DEFAULT,
      host.arena.get(), &emptied, &host.error) == HOLDER_OK);
  CHECK_FALSE(holder::card::CardRepo(host.db).get(parent).has_value());
  CHECK(holder::card::CardRepo(host.db).get(child).has_value());
}

TEST_CASE("Generated C API rejects unsupported input enums without changing cards",
          "[capi][batch-enum-validation]") {
  Host host;
  const auto project = host.project();
  const auto card_id = host.card(project, "Keep me");
  const auto project_row = holder::project::ProjectRepo(host.db).get(project);
  REQUIRE(project_row.has_value());
  holder::git::GitRepo git;
  git.open_existing(project_row->root_path);
  const auto head = git.head_oid();
  for (int operation = 0; operation < 5; ++operation) {
    INFO("invalid enum operation " << operation);
    const holder_card_list_item_page* cards = nullptr;
    const holder_card* card = nullptr;
    const holder_project_history_page* history = nullptr;
    const holder_card_history_comparison* comparison = nullptr;
    const holder_trash_empty_result* emptied = nullptr;
    int status;
    if (operation == 0) {
      holder_list_cards_parent parent{};
      parent.kind = HOLDER_LIST_CARDS_PARENT_MAX_ENUM;
      status = holder_list_cards(&host.context, project.c_str(), parent, nullptr,
          HOLDER_LIST_CARDS_ORDER_DEFAULT, nullptr, nullptr, false, false, host.arena.get(), &cards, &host.error);
    } else if (operation == 1) {
      status = holder_trash_card(&host.context, project.c_str(), card_id.c_str(),
          HOLDER_TRASH_CARD_CHILDREN_MAX_ENUM, host.arena.get(), &card, &host.error);
    } else if (operation == 2) {
      status = holder_list_project_history(&host.context, project.c_str(), nullptr, nullptr,
          HOLDER_LIST_PROJECT_HISTORY_KIND_MAX_ENUM, host.arena.get(), &history, &host.error);
    } else if (operation == 3) {
      status = holder_compare_card_history(&host.context, project.c_str(), card_id.c_str(),
          head->c_str(), nullptr, HOLDER_COMPARE_CARD_HISTORY_MODE_MAX_ENUM,
          host.arena.get(), &comparison, &host.error);
    } else {
      status = holder_empty_trash(&host.context, project.c_str(), HOLDER_EMPTY_TRASH_TYPE_MAX_ENUM,
          host.arena.get(), &emptied, &host.error);
    }
    host.failed(status, HOLDER_ERROR_INVALID_ARGUMENT);
    CHECK(cards == nullptr);
    CHECK(card == nullptr);
    CHECK(history == nullptr);
    CHECK(comparison == nullptr);
    CHECK(emptied == nullptr);
    CHECK(git.head_oid() == head);
    const auto current = holder::card::CardRepo(host.db).get(card_id);
    REQUIRE(current.has_value());
    CHECK_FALSE(current->deleted_at.has_value());
    CHECK(current->title == "Keep me");
  }
}

TEST_CASE(
    "Generated card moves reject invalid targets and damaged sources without changing hierarchy",
    "[capi][move-errors]"
) {
  Host host;
  const auto project = host.project();
  const auto root = host.card(project, "Root");
  const auto source = host.card(project, "Source", root.c_str());
  const auto child = host.card(project, "Child", source.c_str());
  const auto sibling = host.card(project, "Sibling", root.c_str());
  const auto other_project = host.project();
  const auto foreign = host.card(other_project, "Foreign");
  const auto project_record = holder::project::ProjectRepo(host.db).get(project);
  REQUIRE(project_record.has_value());
  holder::git::GitRepo git;
  git.open_existing(project_record->root_path);
  holder_card_move move{};
  move.intent = HOLDER_CARD_MOVE_INTENT_INTO;
  std::string target = sibling;
  std::string moved = source;
  int expected = HOLDER_ERROR_INVALID_MOVE;
  bool missing_file = false;
  bool bad_path = false;
  SECTION("before itself") {
    move.intent = HOLDER_CARD_MOVE_INTENT_BEFORE;
    target = source;
  }
  SECTION("after itself") {
    move.intent = HOLDER_CARD_MOVE_INTENT_AFTER;
    target = source;
  }
  SECTION("into its descendant") { target = child; }
  SECTION("missing target") {
    target = "00000000-0000-4000-8000-000000000001";
    expected = HOLDER_ERROR_NOT_FOUND;
  }
  SECTION("foreign target") {
    target = foreign;
    expected = HOLDER_ERROR_NOT_FOUND;
  }
  SECTION("trashed target") {
    const holder_card* trashed = nullptr;
    REQUIRE(holder_trash_card(&host.context, project.c_str(), sibling.c_str(),
        HOLDER_TRASH_CARD_CHILDREN_TRASH_SUBTREE, host.arena.get(), &trashed, &host.error) == HOLDER_OK);
    expected = HOLDER_ERROR_NOT_FOUND;
  }
  SECTION("root cannot move up a level") {
    moved = root;
    move.intent = HOLDER_CARD_MOVE_INTENT_UP_LEVEL;
  }
  SECTION("missing source content") {
    missing_file = true;
    expected = HOLDER_ERROR_RUNTIME;
  }
  SECTION("source path does not match its identity") {
    bad_path = true;
    expected = HOLDER_ERROR_RUNTIME;
    host.db.exec("UPDATE cards SET rel_path='cards/wrong.md' WHERE card_id='" + source + "';");
  }
  const auto path = std::filesystem::path(project_record->root_path) / holder::core::card_rel_path(source);
  std::ifstream input(path, std::ios::binary);
  REQUIRE(input.is_open());
  const std::string content{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
  input.close();
  if (missing_file) std::filesystem::rename(path, path.string() + ".saved");
  const auto before = holder::card::CardRepo(host.db).list_all(project);
  const auto head = git.head_oid();
  REQUIRE(head.has_value());
  move.target_card_id = target.c_str();
  const holder_card* result = nullptr;
  host.failed(holder_move_card(&host.context, project.c_str(), moved.c_str(), &move,
      host.arena.get(), &result, &host.error), expected);
  CHECK(result == nullptr);
  CHECK(git.head_oid() == head);
  const auto after = holder::card::CardRepo(host.db).list_all(project);
  REQUIRE(after.size() == before.size());
  for (const auto& old : before) {
    const auto current = holder::card::CardRepo(host.db).get(old.card_id);
    REQUIRE(current.has_value());
    CHECK(current->parent_card_id == old.parent_card_id);
    CHECK(current->sort_key == old.sort_key);
    CHECK(current->updated_at == old.updated_at);
    CHECK(current->deleted_at == old.deleted_at);
    CHECK(current->rel_path == old.rel_path);
  }
  if (missing_file) std::filesystem::rename(path.string() + ".saved", path);
  if (bad_path) host.db.exec("UPDATE cards SET rel_path='" + holder::core::card_rel_path(source) +
                           "' WHERE card_id='" + source + "';");
  std::ifstream remaining(path, std::ios::binary);
  REQUIRE(remaining.is_open());
  CHECK(std::string(std::istreambuf_iterator<char>(remaining), std::istreambuf_iterator<char>()) == content);
}

TEST_CASE(
    "Generated Home creation is idempotent and returns an optional project",
    "[capi][maintenance]"
) {
  Host host;
  const holder_home_project* first = nullptr;
  REQUIRE(
      holder_ensure_home_project(&host.context, host.arena.get(), &first, &host.error) == HOLDER_OK
  );
  REQUIRE(first->project != nullptr);
  REQUIRE(std::string(first->project->name) == "Home");
  REQUIRE(first->project->privacy_mode == HOLDER_PROJECT_PRIVACY_MODE_ENCRYPTED_GIT);
  const std::string id = first->project->project_id;
  const auto cards = holder::card::CardRepo(host.db).list_all(id);
  REQUIRE(cards.size() == 1);
  REQUIRE(cards[0].title == "Welcome to Holder");
  const holder_home_project* second = nullptr;
  REQUIRE(
      holder_ensure_home_project(&host.context, host.arena.get(), &second, &host.error) == HOLDER_OK
  );
  REQUIRE(second->project == nullptr);
  REQUIRE(holder::project::ProjectRepo(host.db).list().size() == 1);
  REQUIRE(std::string(first->project->project_id) == id);
}

TEST_CASE(
    "Generated breadcrumbs recover after a planned hierarchy repair is applied",
    "[capi][maintenance]"
) {
  Host host;
  const auto project = host.project();
  const auto parent = host.card(project, "Parent");
  const auto child = host.card(project, "Child", parent.c_str());
  const holder_card_breadcrumb_list* path = nullptr;
  REQUIRE(
      holder_get_card_breadcrumbs(
          &host.context,
          project.c_str(),
          child.c_str(),
          host.arena.get(),
          &path,
          &host.error
      ) == HOLDER_OK
  );
  REQUIRE(path->count == 3);
  REQUIRE(path->items[0].kind == HOLDER_CARD_BREADCRUMB_KIND_PROJECT);
  REQUIRE(std::string(path->items[0].id) == project);
  REQUIRE(path->items[1].kind == HOLDER_CARD_BREADCRUMB_KIND_CARD);
  REQUIRE(std::string(path->items[1].id) == parent);
  REQUIRE(std::string(path->items[2].title) == "Child");
  holder::card::CardRepo cards(host.db);
  cards.soft_delete(parent, 50, 50);
  host.failed(
      holder_get_card_breadcrumbs(
          &host.context,
          project.c_str(),
          child.c_str(),
          host.arena.get(),
          &path,
          &host.error
      ),
      HOLDER_ERROR_HIERARCHY_INVALID
  );
  holder_hierarchy_repair_request request{};
  const holder_hierarchy_repair* plan = nullptr;
  REQUIRE(
      holder_repair_project_hierarchy(
          &host.context,
          project.c_str(),
          &request,
          host.arena.get(),
          &plan,
          &host.error
      ) == HOLDER_OK
  );
  REQUIRE_FALSE(plan->applied);
  REQUIRE(plan->changed_card_count == 1);
  REQUIRE(plan->moved.count == 1);
  REQUIRE(std::string(plan->moved.items[0].card_id) == child);
  REQUIRE(plan->moved.items[0].parent_card_id == nullptr);
  REQUIRE(plan->cycle_card_ids.count == 0);
  REQUIRE(cards.get(child)->parent_card_id == parent);
  const bool apply = true;
  request.apply = &apply;
  REQUIRE(
      holder_repair_project_hierarchy(
          &host.context,
          project.c_str(),
          &request,
          host.arena.get(),
          &plan,
          &host.error
      ) == HOLDER_OK
  );
  REQUIRE(plan->applied);
  REQUIRE_FALSE(cards.get(child)->parent_card_id);
  REQUIRE(
      holder_get_card_breadcrumbs(
          &host.context,
          project.c_str(),
          child.c_str(),
          host.arena.get(),
          &path,
          &host.error
      ) == HOLDER_OK
  );
  REQUIRE(path->count == 2);
  REQUIRE(std::string(path->items[1].id) == child);
  REQUIRE(
      holder_repair_project_hierarchy(
          &host.context,
          project.c_str(),
          &request,
          host.arena.get(),
          &plan,
          &host.error
      ) == HOLDER_OK
  );
  REQUIRE_FALSE(plan->applied);
  REQUIRE(plan->moved.count == 0);
}

TEST_CASE(
    "Generated message search scopes matches, paginates and rejects malformed queries",
    "[capi][maintenance]"
) {
  Host host;
  const auto project = host.project();
  const auto other = host.project();
  host.message(project, "message-1", "A sapphire message");
  host.message(other, "message-2", "Another sapphire message");
  const holder_search_message_list* matches = nullptr;
  REQUIRE(
      holder_search_ai_messages(
          &host.context,
          project.c_str(),
          "sapphire",
          nullptr,
          0,
          host.arena.get(),
          &matches,
          &host.error
      ) == HOLDER_OK
  );
  REQUIRE(matches->count == 1);
  REQUIRE(std::string(matches->items[0].message_id) == "message-1");
  REQUIRE(matches->items[0].created_at == 11);
  REQUIRE(std::string(matches->items[0].snippet).find("[sapphire]") != std::string::npos);
  REQUIRE(
      holder_search_ai_messages(
          &host.context,
          project.c_str(),
          "sapphire",
          nullptr,
          1,
          host.arena.get(),
          &matches,
          &host.error
      ) == HOLDER_OK
  );
  REQUIRE(matches->count == 0);
  host.failed(
      holder_search_ai_messages(
          &host.context,
          project.c_str(),
          "\"unbalanced",
          nullptr,
          0,
          host.arena.get(),
          &matches,
          &host.error
      ),
      HOLDER_ERROR_INVALID_ARGUMENT
  );
  holder::ai::AiMessageRepo(host.db, &host.fts).trash("message-1", 50);
  REQUIRE(
      holder_search_ai_messages(
          &host.context,
          project.c_str(),
          "sapphire",
          nullptr,
          0,
          host.arena.get(),
          &matches,
          &host.error
      ) == HOLDER_OK
  );
  REQUIRE(matches->count == 0);
}

TEST_CASE(
    "Generated Trash deletion distinguishes live items and empties only the requested kind and project",
    "[capi][maintenance]"
) {
  Host host;
  const auto project = host.project();
  const auto other = host.project();
  const auto card = host.card(project, "Discarded");
  const auto other_card = host.card(other, "Keep in other Trash");
  host.message(project, "message-1", "Discard this message");
  holder::ai::AiMessageRepo messages(host.db, &host.fts);
  const holder_trash_delete_result* deleted = nullptr;
  host.failed(
      holder_delete_trash_item(
          &host.context,
          HOLDER_DELETE_TRASH_ITEM_TYPE_CARD,
          card.c_str(),
          host.arena.get(),
          &deleted,
          &host.error
      ),
      HOLDER_ERROR_NOT_TRASHED
  );
  host.failed(
      holder_delete_trash_item(
          &host.context,
          HOLDER_DELETE_TRASH_ITEM_TYPE_AI_MESSAGE,
          "message-1",
          host.arena.get(),
          &deleted,
          &host.error
      ),
      HOLDER_ERROR_NOT_TRASHED
  );
  messages.trash("message-1", 50);
  REQUIRE(
      holder_delete_trash_item(
          &host.context,
          HOLDER_DELETE_TRASH_ITEM_TYPE_AI_MESSAGE,
          "message-1",
          host.arena.get(),
          &deleted,
          &host.error
      ) == HOLDER_OK
  );
  REQUIRE(deleted->card_id == nullptr);
  REQUIRE(std::string(deleted->message_id) == "message-1");
  REQUIRE_FALSE(messages.get("message-1"));
  host.message(project, "message-3", "Keep until empty all");
  messages.trash("message-3", 50);
  const holder_card* ignored = nullptr;
  REQUIRE(
      holder_trash_card(
          &host.context,
          project.c_str(),
          card.c_str(),
          HOLDER_TRASH_CARD_CHILDREN_DEFAULT,
          host.arena.get(),
          &ignored,
          &host.error
      ) == HOLDER_OK
  );
  REQUIRE(
      holder_trash_card(
          &host.context,
          other.c_str(),
          other_card.c_str(),
          HOLDER_TRASH_CARD_CHILDREN_DEFAULT,
          host.arena.get(),
          &ignored,
          &host.error
      ) == HOLDER_OK
  );
  const holder_trash_empty_result* emptied = nullptr;
  REQUIRE(
      holder_empty_trash(
          &host.context,
          project.c_str(),
          HOLDER_EMPTY_TRASH_TYPE_CARD,
          host.arena.get(),
          &emptied,
          &host.error
      ) == HOLDER_OK
  );
  REQUIRE(std::string(emptied->project_id) == project);
  REQUIRE_FALSE(holder::card::CardRepo(host.db).get(card));
  REQUIRE(holder::card::CardRepo(host.db).get(other_card)->deleted_at.has_value());
  REQUIRE(messages.get("message-3")->deleted_at.has_value());
  REQUIRE(
      holder_empty_trash(
          &host.context,
          project.c_str(),
          HOLDER_EMPTY_TRASH_TYPE_ALL,
          host.arena.get(),
          &emptied,
          &host.error
      ) == HOLDER_OK
  );
  REQUIRE_FALSE(messages.get("message-3"));
  REQUIRE(
      holder_empty_trash(
          &host.context,
          project.c_str(),
          HOLDER_EMPTY_TRASH_TYPE_ALL,
          host.arena.get(),
          &emptied,
          &host.error
      ) == HOLDER_OK
  );
  REQUIRE(
      holder_delete_trash_item(
          &host.context,
          HOLDER_DELETE_TRASH_ITEM_TYPE_CARD,
          other_card.c_str(),
          host.arena.get(),
          &deleted,
          &host.error
      ) == HOLDER_OK
  );
  REQUIRE(std::string(deleted->card_id) == other_card);
  REQUIRE(deleted->message_id == nullptr);
  REQUIRE_FALSE(holder::card::CardRepo(host.db).get(other_card));
}

TEST_CASE(
    "Generated SSH signer callbacks preserve binary challenges and reject invalid results",
    "[capi][signer]"
) {
  struct Signer {
    int mode = 0;
    int calls = 0;
    int destroys = 0;
    std::vector<unsigned char> challenge;
  } signer;
  Host host;
  SECTION("valid DER signature") { signer.mode = 0; }
  SECTION("empty signature") { signer.mode = 1; }
  SECTION("null bytes with nonzero length") { signer.mode = 2; }
  SECTION("error with message") { signer.mode = 3; }
  SECTION("error without message") { signer.mode = 4; }
  const holder_ssh_signer_callbacks callbacks{
      +[](void* data,
          const unsigned char* bytes,
          size_t length,
          unsigned char** result,
          size_t* result_length,
          char** error) -> int {
        auto& state = *static_cast<Signer*>(data);
        ++state.calls;
        state.challenge.assign(bytes, bytes + length);
        if (state.mode == 3) {
          *error = static_cast<char*>(std::malloc(16));
          std::memcpy(*error, "signing refused", 16);
          return 1;
        }
        if (state.mode == 4) return 1;
        if (state.mode == 1) return 0;
        if (state.mode == 2) {
          *result_length = 8;
          return 0;
        }
        const unsigned char der[] = {0x30, 6, 2, 1, 1, 2, 1, 2};
        *result = static_cast<unsigned char*>(std::malloc(sizeof(der)));
        std::memcpy(*result, der, sizeof(der));
        *result_length = sizeof(der);
        return 0;
      }
  };
  const unsigned char public_key[] = {1, 2, 3};
  REQUIRE(
      holder_register_ssh_signer(
          &host.context,
          "git",
          public_key,
          sizeof(public_key),
          &callbacks,
          &signer,
          +[](void* data) {
            ++static_cast<Signer*>(data)->destroys;
          },
          &host.error
      ) == HOLDER_OK
  );
  using Provider = holder::git::EcdsaDerSigningCredentialProvider;
  auto* provider = dynamic_cast<Provider*>(host.context.credential_provider.get());
  REQUIRE(provider != nullptr);
  void* payload = provider;
  const unsigned char challenge[] = {0, 1, 255, 0, 17};
  unsigned char* signature = nullptr;
  size_t length = 0;
  const auto status = Provider::sign_trampoline_for_tests(
      &signature,
      &length,
      challenge,
      sizeof(challenge),
      &payload
  );
  const std::unique_ptr<unsigned char, decltype(&std::free)> owned_signature(signature, std::free);
  REQUIRE(signer.calls == 1);
  REQUIRE(signer.challenge == std::vector<unsigned char>{0, 1, 255, 0, 17});
  if (signer.mode == 0) {
    REQUIRE(status == 0);
    // SSH mpint(r=1), mpint(s=2), each prefixed by its four-byte length.
    REQUIRE(
        std::vector<unsigned char>(signature, signature + length) ==
        std::vector<unsigned char>{0, 0, 0, 1, 1, 0, 0, 0, 1, 2}
    );
  } else {
    REQUIRE(status != 0);
    REQUIRE(signature == nullptr);
  }
  REQUIRE(signer.destroys == 0);
  host.context.credential_provider.reset();
  REQUIRE(signer.destroys == 1);
}

TEST_CASE("Link target validation distinguishes local, external, missing and trashed targets", "[links][validation]") {
  Host host;
  const auto project = host.project();
  const auto other = host.project();
  const auto card = host.card(project, "Target");
  const auto foreign = host.card(other, "Foreign");
  auto validate = [&](const std::string& id,
                      const std::string& type,
                      std::optional<std::string> target_project = std::nullopt) {
    holder::card::validate_link_target(host.db, project, id, type, target_project);
  };
  REQUIRE_NOTHROW(validate(card, "card"));
  REQUIRE_NOTHROW(validate(card, "card", project));
  REQUIRE_NOTHROW(validate(foreign, "card", other));
  REQUIRE_NOTHROW(validate("offline-card", "card", other));
  REQUIRE_THROWS_AS(validate("", "card"), std::invalid_argument);
  REQUIRE_THROWS_AS(validate(card, "card", ""), std::invalid_argument);
  REQUIRE_THROWS_AS(validate("missing", "card"), std::invalid_argument);
  REQUIRE_THROWS_AS(validate(foreign, "card"), std::invalid_argument);
  REQUIRE_THROWS_AS(validate(card, "resource", other), std::invalid_argument);
  REQUIRE_THROWS_AS(validate("missing", "resource"), std::invalid_argument);
  REQUIRE_THROWS_AS(validate("missing", "ai_thread"), std::invalid_argument);
  REQUIRE_THROWS_AS(validate("missing", "ai_message"), std::invalid_argument);
  REQUIRE_THROWS_AS(validate(card, "unsupported"), std::invalid_argument);
  holder::resource::ResourceRepo resources(host.db);
  resources.add({"resource-local", project, "document", "Local", {}, 10, 10});
  resources.add({"resource-foreign", other, "document", "Foreign", {}, 10, 10});
  REQUIRE_NOTHROW(validate("resource-local", "resource"));
  REQUIRE_THROWS_AS(validate("resource-foreign", "resource"), std::invalid_argument);
  holder::card::CardRepo(host.db).soft_delete(card, 50, 50);
  REQUIRE_THROWS_AS(validate(card, "card"), std::invalid_argument);
  host.message(project, "local-message", "Local body");
  host.message(other, "foreign-message", "Foreign body");
  REQUIRE_NOTHROW(validate("thread-local-message", "ai_thread"));
  REQUIRE_NOTHROW(validate("local-message", "ai_message"));
  REQUIRE_THROWS_AS(validate("thread-foreign-message", "ai_thread"), std::invalid_argument);
  REQUIRE_THROWS_AS(validate("foreign-message", "ai_message"), std::invalid_argument);
  holder::ai::AiMessageRepo(host.db, &host.fts).trash("local-message", 50);
  REQUIRE_THROWS_AS(validate("local-message", "ai_message"), std::invalid_argument);
}

TEST_CASE(
    "Generated nested hierarchy repair returns hidden children to their live ancestor",
    "[capi][remaining-maintenance]"
) {
  Host host;
  const auto project = host.project();
  const auto ancestor = host.card(project, "Ancestor");
  const auto parent = host.card(project, "Hidden parent", ancestor.c_str());
  const auto child = host.card(project, "Hidden child", parent.c_str());
  holder::card::CardRepo cards(host.db);
  cards.soft_delete(parent, 50, 50);
  holder_hierarchy_repair_request request{};
  const holder_hierarchy_repair* repair = nullptr;
  REQUIRE(
      holder_repair_project_hierarchy(
          &host.context,
          project.c_str(),
          &request,
          host.arena.get(),
          &repair,
          &host.error
      ) == HOLDER_OK
  );
  REQUIRE_FALSE(repair->applied);
  REQUIRE(repair->moved.count == 1);
  REQUIRE(std::string(repair->moved.items[0].card_id) == child);
  REQUIRE(repair->moved.items[0].parent_card_id != nullptr);
  REQUIRE(std::string(repair->moved.items[0].parent_card_id) == ancestor);
  REQUIRE(cards.get(child)->parent_card_id == parent);
  const bool apply = true;
  request.apply = &apply;
  REQUIRE(
      holder_repair_project_hierarchy(
          &host.context,
          project.c_str(),
          &request,
          host.arena.get(),
          &repair,
          &host.error
      ) == HOLDER_OK
  );
  REQUIRE(repair->applied);
  REQUIRE(cards.get(child)->parent_card_id == ancestor);
  const holder_card_breadcrumb_list* path = nullptr;
  REQUIRE(
      holder_get_card_breadcrumbs(
          &host.context,
          project.c_str(),
          child.c_str(),
          host.arena.get(),
          &path,
          &host.error
      ) == HOLDER_OK
  );
  REQUIRE(path->count == 3);
  REQUIRE(std::string(path->items[1].id) == ancestor);
  REQUIRE(std::string(path->items[2].id) == child);
}

TEST_CASE(
    "Generated rebuild reports corrupt durable cards and preserves the existing projection",
    "[capi][remaining-maintenance]"
) {
  Host host;
  const auto project = host.project();
  const auto card = host.card(project, "Durable original");
  const auto root = holder::project::ProjectRepo(host.db).get(project)->root_path;
  const auto path = std::filesystem::path(root) / holder::core::card_rel_path(card);
  std::ifstream input(path, std::ios::binary);
  REQUIRE(input.is_open());
  const std::string original{
      std::istreambuf_iterator<char>(input),
      std::istreambuf_iterator<char>()
  };
  input.close();
  std::string corrupt;
  SECTION("invalid card identity") {
    corrupt = original;
    const auto id_at = corrupt.find(card);
    REQUIRE(id_at != std::string::npos);
    corrupt.replace(id_at, card.size(), "not-a-valid-card-id");
  }
  SECTION("invalid milestone date shape") {
    corrupt = "---\ncard_id: " + card + "\nproject_id: " + project +
              "\ntitle: Durable original\nmilestones:\n  - milestone_id: broken\n"
              "    dates: not-a-list\n---\nOriginal body\n";
  }
  SECTION("invalid milestone date shape with CRLF") {
    corrupt = "---\r\ncard_id: " + card + "\r\nproject_id: " + project +
              "\r\ntitle: Durable original\r\nmilestones:\r\n  - milestone_id: broken\r\n"
              "    dates: not-a-list\r\n---\r\nOriginal body\r\n";
  }
  {
    std::ofstream damaged(path, std::ios::binary);
    damaged << corrupt;
  }
  holder_rebuild_request request{project.c_str()};
  const holder_rebuild_result* result = nullptr;
  host.failed(
      holder_rebuild_project(&host.context, &request, host.arena.get(), &result, &host.error),
      HOLDER_ERROR_REBUILD_FAILED
  );
  REQUIRE(result == nullptr);
  REQUIRE(holder::card::CardRepo(host.db).get(card)->title == "Durable original");
  {
    std::ofstream restored(path, std::ios::binary);
    restored << original;
  }
  REQUIRE(
      holder_rebuild_project(&host.context, &request, host.arena.get(), &result, &host.error) ==
      HOLDER_OK
  );
  REQUIRE(result->cards == 1);
  REQUIRE(std::string(result->project_id) == project);
  const holder_card* read = nullptr;
  REQUIRE(
      holder_get_card(
          &host.context,
          project.c_str(),
          card.c_str(),
          host.arena.get(),
          &read,
          &host.error
      ) == HOLDER_OK
  );
  REQUIRE(std::string(read->title) == "Durable original");
}

TEST_CASE(
    "Generated message Trash returns metadata and enforces project scope on deletion",
    "[capi][remaining-maintenance]"
) {
  Host host;
  const auto project = host.project();
  const auto other = host.project();
  host.message(project, "trashed-message", "Discarded body");
  host.message(project, "live-message", "Keep this body");
  holder::ai::AiMessageRepo messages(host.db, &host.fts);
  messages.trash("trashed-message", 50);
  const holder_trash_item_list* items = nullptr;
  REQUIRE(
      holder_list_trash(
          &host.context,
          project.c_str(),
          "ai_message",
          host.arena.get(),
          &items,
          &host.error
      ) == HOLDER_OK
  );
  REQUIRE(items->count == 1);
  const auto& item = items->items[0];
  REQUIRE(item.type == HOLDER_TRASH_ITEM_TYPE_AI_MESSAGE);
  REQUIRE(std::string(item.project_id) == project);
  REQUIRE(item.card_id == nullptr);
  REQUIRE(std::string(item.message_id) == "trashed-message");
  REQUIRE(std::string(item.thread_id) == "thread-trashed-message");
  REQUIRE(std::string(item.role) == "user");
  REQUIRE(item.deleted_at == 50);
  REQUIRE(item.created_at != nullptr);
  REQUIRE(*item.created_at == 11);
  const holder_trash_delete_result* result = nullptr;
  host.failed(
      holder_delete_project_trash_item(
          &host.context,
          other.c_str(),
          HOLDER_DELETE_PROJECT_TRASH_ITEM_TYPE_AI_MESSAGE,
          "trashed-message",
          host.arena.get(),
          &result,
          &host.error
      ),
      HOLDER_ERROR_NOT_FOUND
  );
  host.failed(
      holder_delete_trash_item(
          &host.context,
          HOLDER_DELETE_TRASH_ITEM_TYPE_AI_MESSAGE,
          "missing-message",
          host.arena.get(),
          &result,
          &host.error
      ),
      HOLDER_ERROR_NOT_FOUND
  );
  REQUIRE(messages.get("trashed-message").has_value());
  REQUIRE(
      holder_delete_project_trash_item(
          &host.context,
          project.c_str(),
          HOLDER_DELETE_PROJECT_TRASH_ITEM_TYPE_AI_MESSAGE,
          "trashed-message",
          host.arena.get(),
          &result,
          &host.error
      ) == HOLDER_OK
  );
  REQUIRE(std::string(result->message_id) == "trashed-message");
  REQUIRE(result->card_id == nullptr);
  REQUIRE_FALSE(messages.get("trashed-message"));
  REQUIRE(messages.get("live-message").has_value());
}

TEST_CASE(
    "Generated card references separate live, trashed and ambiguous exact titles",
    "[capi][remaining-maintenance]"
) {
  Host host;
  const auto project = host.project();
  const auto live = host.card(project, "Same title");
  const auto trash = host.card(project, "Same title");
  const holder_card* ignored = nullptr;
  REQUIRE(
      holder_trash_card(
          &host.context,
          project.c_str(),
          trash.c_str(),
          HOLDER_TRASH_CARD_CHILDREN_DEFAULT,
          host.arena.get(),
          &ignored,
          &host.error
      ) == HOLDER_OK
  );
  holder_card_reference_resolve_request request{
      project.c_str(),
      "Same title",
      HOLDER_CARD_REFERENCE_RESOLVE_REQUEST_SCOPE_EITHER
  };
  const holder_card_reference_resolve_data* result = nullptr;
  REQUIRE(
      holder_resolve_card_reference(
          &host.context,
          &request,
          host.arena.get(),
          &result,
          &host.error
      ) == HOLDER_OK
  );
  REQUIRE(result->status == HOLDER_CARD_REFERENCE_RESOLVE_DATA_STATUS_AMBIGUOUS);
  REQUIRE(result->candidates != nullptr);
  REQUIRE(result->candidates->count == 2);
  REQUIRE(result->card == nullptr);
  request.scope = HOLDER_CARD_REFERENCE_RESOLVE_REQUEST_SCOPE_TRASHED;
  REQUIRE(
      holder_resolve_card_reference(
          &host.context,
          &request,
          host.arena.get(),
          &result,
          &host.error
      ) == HOLDER_OK
  );
  REQUIRE(result->status == HOLDER_CARD_REFERENCE_RESOLVE_DATA_STATUS_RESOLVED);
  REQUIRE(std::string(result->card->card_id) == trash);
  REQUIRE(result->card->deleted_at != nullptr);
  request.scope = HOLDER_CARD_REFERENCE_RESOLVE_REQUEST_SCOPE_LIVE;
  REQUIRE(
      holder_resolve_card_reference(
          &host.context,
          &request,
          host.arena.get(),
          &result,
          &host.error
      ) == HOLDER_OK
  );
  REQUIRE(std::string(result->card->card_id) == live);
  REQUIRE(result->card->deleted_at == nullptr);
}

TEST_CASE("Generated storage callbacks preserve typed download, lookup and removal failures", "[capi][storage-failures]") {
  Host host;
  struct Failure {
    int code = 0;
    bool message = false;
    static int fail(void* data, int* code, char** error) {
      const auto& state = *static_cast<Failure*>(data);
      *code = state.code;
      if (state.message) {
        constexpr char text[] = "provider failure";
        *error = static_cast<char*>(std::malloc(sizeof(text)));
        std::memcpy(*error, text, sizeof(text));
      }
      return 1;
    }
  };
  // Registration owns this heap state; it remains valid until the next provider
  // replacement, even when a REQUIRE aborts the test early.
  auto owned = std::make_unique<Failure>();
  auto* state = owned.get();
  SECTION("callback supplies a message") { state->message = true; }
  SECTION("callback supplies no message") { state->message = false; }
  const holder_storage_provider_callbacks callbacks{
      +[](void*, const char*, const char*, int64_t, const char*, int*, char**) {
        return 0;
      },
      +[](void* data, const char*, const char*, int* code, char** error) {
        return Failure::fail(data, code, error);
      },
      +[](void* data, const char*, int*, int* code, char** error) {
        return Failure::fail(data, code, error);
      },
      +[](void* data, const char*, int* code, char** error) {
        return Failure::fail(data, code, error);
      }
  };
  REQUIRE(
      holder_register_storage_provider(
          HOLDER_REGISTER_STORAGE_PROVIDER_KIND_S3_COMPATIBLE,
          &callbacks,
          owned.release(),
          +[](void* data) {
            delete static_cast<Failure*>(data);
          },
          &host.error
      ) == HOLDER_OK
  );
  const auto provider = holder::capi::storage_provider(host.context, "s3_compatible");
  using Code = holder::resource::StorageErrorCode;
  const std::pair<int, Code> cases[] = {
      {0, Code::Unavailable},
      {1, Code::Authentication},
      {2, Code::Permission},
      {3, Code::Capacity},
      {4, Code::Integrity},
      {5, Code::Conflict},
      {6, Code::InvalidConfiguration},
      {7, Code::Transient},
      {-1, Code::Unavailable},
      {99, Code::Unavailable}
  };
  for (const auto& [code, expected] : cases) {
    state->code = code;
    for (int operation = 0; operation < 3; ++operation) {
      bool caught = false;
      try {
        if (operation == 0) provider->get("object", host.dir / "destination");
        if (operation == 1) (void)provider->exists("object");
        if (operation == 2) provider->remove("object");
      } catch (const holder::resource::StorageError& error) {
        caught = true;
        REQUIRE(error.code() == expected);
        const std::string fallback = operation == 0   ? "storage get failed"
                                     : operation == 1 ? "storage exists failed"
                                                      : "storage remove failed";
        REQUIRE(std::string(error.what()) == (state->message ? "provider failure" : fallback));
      }
      REQUIRE(caught);
    }
  }
  REQUIRE_FALSE(std::filesystem::exists(host.dir / "destination"));
}

TEST_CASE(
    "Scoped message purge refuses live messages and other projects",
    "[privacy][message-purge-guards]"
) {
  Host host;
  const auto project = host.project();
  const auto other = host.project();
  host.message(project, "guarded-message", "Preserve this message");
  holder::ai::AiMessageRepo messages(host.db, &host.fts);
  REQUIRE_THROWS_AS(messages.remove_trashed(other, "guarded-message"), std::runtime_error);
  REQUIRE_THROWS_AS(messages.remove_trashed(project, "guarded-message"), std::runtime_error);
  REQUIRE(messages.get("guarded-message")->content == "Preserve this message");
  REQUIRE_FALSE(messages.get("guarded-message")->deleted_at);
  REQUIRE(host.fts.search_messages(project, "Preserve", 10, 0).size() == 1);
  messages.trash("guarded-message", 50);
  REQUIRE_THROWS_AS(messages.remove_trashed(other, "guarded-message"), std::runtime_error);
  REQUIRE(messages.get("guarded-message")->deleted_at == 50);
  REQUIRE_NOTHROW(messages.remove_trashed(project, "guarded-message"));
  REQUIRE_FALSE(messages.get("guarded-message"));
}

TEST_CASE(
    "Generated resources expose encrypted placement metadata and reject unknown encodings",
    "[capi][resource-boundaries]"
) {
  Host host;
  const auto project = host.project();
  holder::resource::LocationRepo(host.db).put(
      {"location-1", project, "Files", "local_directory", {}, 10, 20}
  );
  holder::model::ResourceBundle bundle;
  bundle.resource = {"resource-1", project, "document", "Notes", {}, 10, 20};
  bundle.assets.push_back(
      {"asset-1",
       "resource-1",
       "notes.txt",
       "text/plain",
       4,
       std::string(64, 'a'),
       10,
       20,
       {{"placement-1",
         "asset-1",
         "location-1",
         "object/notes",
         "holder_asset_v1",
         64,
         std::string(64, 'b'),
         10}}}
  );
  holder::resource::ResourceRepo(host.db).put_bundle(bundle);
  const holder_resource* resource = nullptr;
  REQUIRE(
      holder_get_resource(
          &host.context,
          project.c_str(),
          "resource-1",
          host.arena.get(),
          &resource,
          &host.error
      ) == HOLDER_OK
  );
  REQUIRE(resource->assets.count == 1);
  const auto& asset = resource->assets.items[0];
  REQUIRE(std::string(asset.original_filename) == "notes.txt");
  REQUIRE(asset.byte_size == 4);
  REQUIRE(asset.placements.count == 1);
  REQUIRE(asset.placements.items[0].encoding == HOLDER_ASSET_PLACEMENT_ENCODING_HOLDER_ASSET_V1);
  REQUIRE(asset.placements.items[0].stored_byte_size == 64);
  host.db.exec("UPDATE asset_placements SET encoding='unsupported';");
  host.failed(
      holder_get_resource(
          &host.context,
          project.c_str(),
          "resource-1",
          host.arena.get(),
          &resource,
          &host.error
      ),
      HOLDER_ERROR_RUNTIME
  );
  REQUIRE(resource == nullptr);
  host.db.exec("UPDATE asset_placements SET encoding='holder_asset_v1';");
  REQUIRE(
      holder_get_resource(
          &host.context,
          project.c_str(),
          "resource-1",
          host.arena.get(),
          &resource,
          &host.error
      ) == HOLDER_OK
  );
  REQUIRE(
      resource->assets.items[0].placements.items[0].encoding ==
      HOLDER_ASSET_PLACEMENT_ENCODING_HOLDER_ASSET_V1
  );
}

TEST_CASE(
    "Generated card resource listings cross the repository page boundary without truncation",
    "[capi][resource-boundaries]"
) {
  Host host;
  const auto project = host.project();
  const auto card = host.card(project, "Many attachments");
  int count = 1000;
  SECTION("exactly one full page") { count = 1000; }
  SECTION("one attachment beyond a full page") { count = 1001; }
  holder::resource::ResourceRepo resources(host.db);
  std::vector<holder::model::CardLink> links;
  {
    holder::platform::Tx transaction(host.db);
    for (int i = 0; i < count; ++i) {
      const auto id = "resource-" + std::to_string(i);
      resources.add({id, project, "document", "Attachment " + std::to_string(i), {}, i + 1, i + 1});
      links.push_back(
          {project, card, id, "resource", "attachment", std::nullopt, i + 1, std::nullopt}
      );
    }
    holder::card::LinkRepo(host.db).upsert_links(project, card, links);
    transaction.commit();
  }
  const holder_resource_list* result = nullptr;
  REQUIRE(
      holder_list_card_resources(
          &host.context,
          project.c_str(),
          card.c_str(),
          host.arena.get(),
          &result,
          &host.error
      ) == HOLDER_OK
  );
  REQUIRE(result->count == static_cast<size_t>(count));
  std::set<std::string> ids;
  for (size_t i = 0; i < result->count; ++i) {
    REQUIRE(std::string(result->items[i].project_id) == project);
    REQUIRE(ids.insert(result->items[i].resource_id).second);
  }
  REQUIRE(ids.contains("resource-0"));
  REQUIRE(ids.contains("resource-" + std::to_string(count - 1)));
}

TEST_CASE(
    "Generated links round trip AI and resource targets and combine available backlink sources",
    "[capi][typed-link-roundtrip]"
) {
  Host host;
  const auto project = host.project();
  const auto card = host.card(project, "Linked card");
  host.message(project, "linked-message", "Conversation body");
  holder::resource::ResourceRepo(host.db).add(
      {"linked-resource", project, "document", "Notes", {}, 10, 10}
  );
  struct Target {
    const char* id;
    holder_new_link_target_type create;
    holder_link_target_type returned;
    holder_remove_card_link_target_type remove;
  };
  const Target targets[] = {
      {"linked-message",
       HOLDER_NEW_LINK_TARGET_TYPE_AI_MESSAGE,
       HOLDER_LINK_TARGET_TYPE_AI_MESSAGE,
       HOLDER_REMOVE_CARD_LINK_TARGET_TYPE_AI_MESSAGE},
      {"thread-linked-message",
       HOLDER_NEW_LINK_TARGET_TYPE_AI_THREAD,
       HOLDER_LINK_TARGET_TYPE_AI_THREAD,
       HOLDER_REMOVE_CARD_LINK_TARGET_TYPE_AI_THREAD},
      {"linked-resource",
       HOLDER_NEW_LINK_TARGET_TYPE_RESOURCE,
       HOLDER_LINK_TARGET_TYPE_RESOURCE,
       HOLDER_REMOVE_CARD_LINK_TARGET_TYPE_RESOURCE}
  };
  for (const auto& target : targets) {
    holder_new_link request{};
    request.target_id = target.id;
    request.target_type = target.create;
    request.kind = "ref";
    const holder_link* result = nullptr;
    REQUIRE(
        holder_add_card_link(
            &host.context,
            project.c_str(),
            card.c_str(),
            &request,
            host.arena.get(),
            &result,
            &host.error
        ) == HOLDER_OK
    );
    REQUIRE(result->target_type == target.returned);
    REQUIRE(std::string(result->target_id) == target.id);
    REQUIRE(result->source_type == HOLDER_LINK_SOURCE_TYPE_CARD);
  }
  // One incoming AI-message link in this project and one card link in another
  // available project must both be represented with their source types intact.
  holder::card::LinkRepo(host.db).upsert_links(
      project,
      "linked-message",
      {{project, "linked-message", card, "card", "ref", std::nullopt, 10, std::nullopt}}
  );
  holder::ai::AiMessageRepo(host.db, &host.fts).update_links("linked-message");
  const auto other = host.project();
  const auto other_card = host.card(other, "External source");
  holder_new_link incoming{};
  incoming.target_id = card.c_str();
  incoming.target_project_id = project.c_str();
  incoming.target_type = HOLDER_NEW_LINK_TARGET_TYPE_CARD;
  const holder_link* result = nullptr;
  REQUIRE(
      holder_add_card_link(
          &host.context,
          other.c_str(),
          other_card.c_str(),
          &incoming,
          host.arena.get(),
          &result,
          &host.error
      ) == HOLDER_OK
  );
  const holder_link_list* backlinks = nullptr;
  REQUIRE(
      holder_list_card_backlinks(
          &host.context,
          project.c_str(),
          card.c_str(),
          HOLDER_LIST_CARD_BACKLINKS_SCOPE_ALL_AVAILABLE,
          false,
          host.arena.get(),
          &backlinks,
          &host.error
      ) == HOLDER_OK
  );
  REQUIRE(backlinks->count == 2);
  std::set<holder_link_source_type> types;
  std::set<std::string> sources;
  for (size_t i = 0; i < backlinks->count; ++i) {
    types.insert(backlinks->items[i].source_type);
    sources.insert(backlinks->items[i].source_id);
  }
  REQUIRE(
      types == std::set<holder_link_source_type>{
                   HOLDER_LINK_SOURCE_TYPE_CARD,
                   HOLDER_LINK_SOURCE_TYPE_AI_MESSAGE
               }
  );
  REQUIRE(sources == std::set<std::string>{"linked-message", other_card});
  size_t remaining = 3;
  for (const auto& target : targets) {
    const holder_link_list* links = nullptr;
    REQUIRE(
        holder_remove_card_link(
            &host.context,
            project.c_str(),
            card.c_str(),
            target.remove,
            target.id,
            "ref",
            nullptr,
            host.arena.get(),
            &links,
            &host.error
        ) == HOLDER_OK
    );
    --remaining;
    REQUIRE(links->count == remaining);
    for (size_t i = 0; i < links->count; ++i)
      REQUIRE(std::string(links->items[i].target_id) != target.id);
  }
}
