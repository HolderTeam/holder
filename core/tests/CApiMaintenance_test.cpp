#include "ai/AiMessageRepo.h"
#include "ai/AiThreadRepo.h"
#include "capi/Context.h"
#include "card/CardRepo.h"
#include "core_test_helpers.h"
#include "git/GitOps.h"
#include "project/ProjectRepo.h"

#include <holder/holder_api.h>
#include <memory>
#include <string>

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
