// The contract's Card operations through the generated C API (holder_api.h), against a real data
// directory: cards, their tags, their links and their milestones, and resources.
#if __has_include(<catch2/catch_test_macros.hpp>)
#include <catch2/catch_test_macros.hpp>
#else
#include <catch2/catch.hpp>
#endif

#include "core_test_helpers.h"
#include "card/CardPaths.h"
#include "api/CardCursor.h"

#include <holder/holder_api.h>

#include <fstream>
#include <algorithm>
#include <limits>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <vector>

namespace {

std::string read_schema_sql() {
  std::ifstream in(SCHEMA_SQL_PATH);
  REQUIRE(in.is_open());
  return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

// A context on a fresh data directory with one plain project, freed with its arena.
struct Fixture {
  std::filesystem::path data_dir = holder::test::make_temp_dir();
  holder_context* context = nullptr;
  holder_arena* arena = holder_arena_new();
  holder_error* error = nullptr;
  std::string project_id;

  Fixture() {
    const auto schema = read_schema_sql();
    REQUIRE(holder_context_open(data_dir.string().c_str(), schema.c_str(), &context, &error) == HOLDER_OK);
    project_id = new_project("Cards");
  }
  ~Fixture() {
    holder_error_destroy(error);
    holder_arena_free(arena);
    holder_context_destroy(context);
  }

  const holder_card* create(const char* title, const char* content = nullptr, const char* parent = nullptr) {
    holder_new_card card{};
    card.title = title;
    card.content = content;
    card.parent_card_id = parent;
    const holder_card* out = nullptr;
    REQUIRE(holder_create_card(context, project_id.c_str(), &card, arena, &out, &error) == HOLDER_OK);
    return out;
  }

  std::string new_project(const char* name) {
    holder_new_project request{};
    request.name = name;
    request.privacy_mode = HOLDER_NEW_PROJECT_PRIVACY_MODE_PLAIN;
    const holder_project* project = nullptr;
    REQUIRE(holder_create_project(context, &request, arena, &project, &error) == HOLDER_OK);
    return project->project_id;
  }

  void trash(const char* card_id) {
    const holder_card* card = nullptr;
    REQUIRE(holder_trash_card(context, project_id.c_str(), card_id, HOLDER_TRASH_CARD_CHILDREN_DEFAULT,
                              arena, &card, &error) == HOLDER_OK);
  }

  // The status of a failing call, freeing its error.
  int failed(int status) {
    REQUIRE(status != HOLDER_OK);
    REQUIRE(error != nullptr);
    holder_error_destroy(error);
    error = nullptr;
    return status;
  }

  const holder_card_list_item_page* list(holder_list_cards_parent parent, const int32_t* limit = nullptr,
                                         const char* cursor = nullptr, bool content = false, bool counts = false,
                                         holder_list_cards_order order = HOLDER_LIST_CARDS_ORDER_DEFAULT) {
    const holder_card_list_item_page* page = nullptr;
    REQUIRE(holder_list_cards(context, project_id.c_str(), parent, nullptr, order, limit, cursor, content, counts,
                              arena, &page, &error) == HOLDER_OK);
    return page;
  }
};

} // namespace

TEST_CASE("holder_create_card returns the full card, with its tags", "[capi][contract]") {
  Fixture f;
  const auto* card = f.create("Thesis", "Intro #draft");
  REQUIRE(std::string(card->title) == "Thesis");
  REQUIRE(std::string(card->content) == "Intro #draft");
  REQUIRE(std::string(card->project_id) == f.project_id);
  REQUIRE(std::string(card->rel_path).find(card->card_id) != std::string::npos);
  REQUIRE(card->parent_card_id == nullptr);
  REQUIRE(card->deleted_at == nullptr);
  REQUIRE(card->updated_at == card->created_at);
  REQUIRE(card->tags.count == 1);
  REQUIRE(std::string(card->tags.items[0]) == "draft");
  REQUIRE(card->tag_occurrences.count == 1);
  REQUIRE(card->tag_occurrences.items[0].byte_start == 6);
  REQUIRE(card->tag_occurrences.items[0].byte_end == 12);

  const holder_card* fetched = nullptr;
  REQUIRE(holder_get_card(f.context, f.project_id.c_str(), card->card_id, f.arena, &fetched, &f.error) == HOLDER_OK);
  REQUIRE(std::string(fetched->card_id) == card->card_id);
  REQUIRE(std::string(fetched->content) == "Intro #draft");
}

TEST_CASE("holder_create_card honours the optional fields", "[capi][contract]") {
  Fixture f;
  const auto* parent = f.create("Parent");
  holder_new_card card{};
  card.title = "Imported";
  card.parent_card_id = parent->card_id;
  const double sort_key = 7.5;
  const int64_t created_at = 1700000000;
  const int64_t updated_at = 1700000100;
  card.sort_key = &sort_key;
  card.created_at = &created_at;
  card.updated_at = &updated_at;
  const holder_card* out = nullptr;
  REQUIRE(holder_create_card(f.context, f.project_id.c_str(), &card, f.arena, &out, &f.error) == HOLDER_OK);
  REQUIRE(std::string(out->parent_card_id) == parent->card_id);
  REQUIRE(out->sort_key == 7.5);
  REQUIRE(out->created_at == created_at);
  REQUIRE(out->updated_at == updated_at);
  REQUIRE(std::string(out->content).empty());
}

TEST_CASE("holder_create_card keeps timestamps at or before the epoch", "[capi][contract]") {
  Fixture f;
  holder_new_card card{};
  card.title = "Old";
  const int64_t epoch = 0;
  const int64_t before = -86400;
  card.created_at = &before;
  card.updated_at = &epoch;
  const holder_card* out = nullptr;
  REQUIRE(holder_create_card(f.context, f.project_id.c_str(), &card, f.arena, &out, &f.error) == HOLDER_OK);
  REQUIRE(out->created_at == before);
  REQUIRE(out->updated_at == epoch);
}

TEST_CASE("holder_create_card rejects numbers that are not finite and text that is not UTF-8", "[capi][contract]") {
  Fixture f;
  holder_new_card card{};
  card.title = "Fine";
  const holder_card* out = nullptr;
  for (const double bad : {std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity()}) {
    card.sort_key = &bad;
    REQUIRE(f.failed(holder_create_card(f.context, f.project_id.c_str(), &card, f.arena, &out, &f.error)) ==
            HOLDER_ERROR_INVALID_ARGUMENT);
  }
  card.sort_key = nullptr;
  for (const char* bad : {"\xff", "\xc0\x80", "\xed\xa0\x80", "a\xe2\x82"}) {
    card.title = bad;
    REQUIRE(f.failed(holder_create_card(f.context, f.project_id.c_str(), &card, f.arena, &out, &f.error)) ==
            HOLDER_ERROR_INVALID_ARGUMENT);
  }
  card.title = "caf\xc3\xa9 \xf0\x9f\x93\x9d";
  REQUIRE(holder_create_card(f.context, f.project_id.c_str(), &card, f.arena, &out, &f.error) == HOLDER_OK);
  REQUIRE(std::string(out->title) == card.title);
}

TEST_CASE("holder_create_card reports missing projects, missing parents and bad arguments", "[capi][contract]") {
  Fixture f;
  holder_new_card card{};
  card.title = "Lost";
  const holder_card* out = nullptr;
  REQUIRE(f.failed(holder_create_card(f.context, "no-such-project", &card, f.arena, &out, &f.error)) ==
          HOLDER_ERROR_NOT_FOUND);
  REQUIRE(out == nullptr);

  card.parent_card_id = "no-such-card";
  REQUIRE(f.failed(holder_create_card(f.context, f.project_id.c_str(), &card, f.arena, &out, &f.error)) ==
          HOLDER_ERROR_NOT_FOUND);

  card.parent_card_id = nullptr;
  card.title = "";
  REQUIRE(f.failed(holder_create_card(f.context, f.project_id.c_str(), &card, f.arena, &out, &f.error)) ==
          HOLDER_ERROR_INVALID_ARGUMENT);
  REQUIRE(f.failed(holder_create_card(f.context, f.project_id.c_str(), nullptr, f.arena, &out, &f.error)) ==
          HOLDER_ERROR_INVALID_ARGUMENT);
  card.title = "Fine";
  REQUIRE(f.failed(holder_create_card(f.context, f.project_id.c_str(), &card, nullptr, &out, &f.error)) ==
          HOLDER_ERROR_INVALID_ARGUMENT);
}

TEST_CASE("holder_get_card does not find trashed or unknown cards, or cards in another project", "[capi][contract]") {
  Fixture f;
  const auto* card = f.create("Binned");
  f.trash(card->card_id);
  const holder_card* out = nullptr;
  REQUIRE(f.failed(holder_get_card(f.context, f.project_id.c_str(), card->card_id, f.arena, &out, &f.error)) ==
          HOLDER_ERROR_NOT_FOUND);
  REQUIRE(f.failed(holder_get_card(f.context, f.project_id.c_str(), "no-such-card", f.arena, &out, &f.error)) ==
          HOLDER_ERROR_NOT_FOUND);
  REQUIRE(f.failed(holder_get_card(f.context, f.project_id.c_str(), "", f.arena, &out, &f.error)) ==
          HOLDER_ERROR_INVALID_ARGUMENT);

  const auto* live = f.create("Elsewhere");
  const auto other = f.new_project("Other");
  REQUIRE(f.failed(holder_get_card(f.context, other.c_str(), live->card_id, f.arena, &out, &f.error)) ==
          HOLDER_ERROR_NOT_FOUND);
  REQUIRE(f.failed(holder_get_card(f.context, "no-such-project", live->card_id, f.arena, &out, &f.error)) ==
          HOLDER_ERROR_NOT_FOUND);
}

TEST_CASE("holder_get_card reports a missing durable content file", "[capi][contract]") {
  Fixture f;
  const holder_card* card = f.create("Missing file", "Durable body");
  const std::string id = card->card_id;
  const std::string path = card->rel_path;
  const holder_project* project = nullptr;
  REQUIRE(holder_get_project(f.context, f.project_id.c_str(), f.arena, &project, &f.error) == HOLDER_OK);
  REQUIRE(std::filesystem::remove(std::filesystem::path(project->root_path) / path));
  REQUIRE(f.failed(holder_get_card(f.context, f.project_id.c_str(), id.c_str(), f.arena, &card, &f.error)) ==
          HOLDER_ERROR_NOT_FOUND);
}

TEST_CASE("holder_resolve_card_reference reports full IDs, prefixes, titles and ambiguity", "[capi][contract]") {
  Fixture f;
  const std::string first = f.create("Shared title")->card_id;
  holder_card_reference_resolve_request request{};
  request.project_id = f.project_id.c_str();
  request.scope = HOLDER_CARD_REFERENCE_RESOLVE_REQUEST_SCOPE_LIVE;
  const holder_card_reference_resolve_data* result = nullptr;

  request.reference = first.c_str();
  REQUIRE(holder_resolve_card_reference(f.context, &request, f.arena, &result, &f.error) == HOLDER_OK);
  REQUIRE(result->status == HOLDER_CARD_REFERENCE_RESOLVE_DATA_STATUS_RESOLVED);
  REQUIRE(std::string(result->match_kind) == "full_id");
  REQUIRE(std::string(result->card->card_id) == first);

  const std::string prefix = first.substr(0, 12);
  request.reference = prefix.c_str();
  REQUIRE(holder_resolve_card_reference(f.context, &request, f.arena, &result, &f.error) == HOLDER_OK);
  REQUIRE(result->status == HOLDER_CARD_REFERENCE_RESOLVE_DATA_STATUS_RESOLVED);
  REQUIRE(std::string(result->match_kind) == "id_prefix");

  request.reference = "Shared title";
  REQUIRE(holder_resolve_card_reference(f.context, &request, f.arena, &result, &f.error) == HOLDER_OK);
  REQUIRE(result->status == HOLDER_CARD_REFERENCE_RESOLVE_DATA_STATUS_RESOLVED);
  REQUIRE(std::string(result->match_kind) == "exact_title");

  f.create("Shared title");
  REQUIRE(holder_resolve_card_reference(f.context, &request, f.arena, &result, &f.error) == HOLDER_OK);
  REQUIRE(result->status == HOLDER_CARD_REFERENCE_RESOLVE_DATA_STATUS_AMBIGUOUS);
  REQUIRE(result->candidates != nullptr);
  REQUIRE(result->candidates->count == 2);

  request.reference = "No such card";
  REQUIRE(holder_resolve_card_reference(f.context, &request, f.arena, &result, &f.error) == HOLDER_OK);
  REQUIRE(result->status == HOLDER_CARD_REFERENCE_RESOLVE_DATA_STATUS_NOT_FOUND);
  request.reference = "";
  REQUIRE(f.failed(holder_resolve_card_reference(f.context, &request, f.arena, &result, &f.error)) ==
          HOLDER_ERROR_INVALID_ARGUMENT);
}

TEST_CASE("holder_get_card_metadata reads only a live card in the requested project", "[capi][contract]") {
  Fixture f;
  const auto* created = f.create("Café", "A large body #tag");
  const std::string id = created->card_id;
  const holder_card_metadata* metadata = nullptr;
  REQUIRE(holder_get_card_metadata(f.context, f.project_id.c_str(), id.c_str(), f.arena, &metadata, &f.error) ==
          HOLDER_OK);
  REQUIRE(std::string(metadata->title) == "Café");
  REQUIRE(std::string(metadata->project_id) == f.project_id);
  REQUIRE(metadata->deleted_at == nullptr);

  REQUIRE(f.failed(holder_get_card_metadata(f.context, "other-project", id.c_str(), f.arena, &metadata, &f.error)) ==
          HOLDER_ERROR_NOT_FOUND);
  f.trash(id.c_str());
  REQUIRE(f.failed(holder_get_card_metadata(f.context, f.project_id.c_str(), id.c_str(), f.arena, &metadata,
                                             &f.error)) == HOLDER_ERROR_NOT_FOUND);
}

TEST_CASE("holder_get_card_any_state reads trashed metadata only in its project", "[capi][contract]") {
  Fixture f;
  const auto* created = f.create("Binned", "Durable body");
  const std::string id = created->card_id;
  f.trash(id.c_str());
  const holder_card_metadata* metadata = nullptr;
  REQUIRE(holder_get_card_any_state(f.context, f.project_id.c_str(), id.c_str(), f.arena, &metadata, &f.error) ==
          HOLDER_OK);
  REQUIRE(std::string(metadata->title) == "Binned");
  REQUIRE(metadata->deleted_at != nullptr);
  REQUIRE(f.failed(holder_get_card_any_state(f.context, "another-project", id.c_str(), f.arena, &metadata,
                                              &f.error)) == HOLDER_ERROR_NOT_FOUND);
}

TEST_CASE("holder_update_card and holder_move_card change a card and reject cycles", "[capi][contract]") {
  Fixture f;
  const std::string parent_id = f.create("Parent")->card_id;
  const std::string child_id = f.create("Child", "Old body")->card_id;
  holder_card_update update{};
  update.title = "Renamed";
  update.content = "New body";
  const holder_card* card = nullptr;
  REQUIRE(holder_update_card(f.context, f.project_id.c_str(), child_id.c_str(), &update, f.arena, &card,
                             &f.error) == HOLDER_OK);
  REQUIRE(std::string(card->title) == "Renamed");
  REQUIRE(std::string(card->content) == "New body");

  holder_card_move move{};
  move.intent = HOLDER_CARD_MOVE_INTENT_INTO;
  move.target_card_id = parent_id.c_str();
  REQUIRE(holder_move_card(f.context, f.project_id.c_str(), child_id.c_str(), &move, f.arena, &card,
                           &f.error) == HOLDER_OK);
  REQUIRE(std::string(card->parent_card_id) == parent_id);

  move.target_card_id = child_id.c_str();
  REQUIRE(f.failed(holder_move_card(f.context, f.project_id.c_str(), parent_id.c_str(), &move, f.arena,
                                    &card, &f.error)) == HOLDER_ERROR_INVALID_MOVE);
  move.target_card_id = "missing-card";
  REQUIRE(f.failed(holder_move_card(f.context, f.project_id.c_str(), child_id.c_str(), &move, f.arena,
                                    &card, &f.error)) == HOLDER_ERROR_NOT_FOUND);
  move.intent = HOLDER_CARD_MOVE_INTENT_MAX_ENUM;
  REQUIRE(f.failed(holder_move_card(f.context, f.project_id.c_str(), child_id.c_str(), &move, f.arena,
                                    &card, &f.error)) == HOLDER_ERROR_INVALID_ARGUMENT);
}

TEST_CASE("holder_trash_card, holder_restore_card and Trash removal preserve card content", "[capi][contract]") {
  Fixture f;
  const std::string id = f.create("Binned", "Durable body")->card_id;
  f.trash(id.c_str());
  const holder_card* card = nullptr;
  REQUIRE(f.failed(holder_get_card(f.context, f.project_id.c_str(), id.c_str(), f.arena, &card, &f.error)) ==
          HOLDER_ERROR_NOT_FOUND);
  const holder_trash_item_list* trash = nullptr;
  REQUIRE(holder_list_trash(f.context, f.project_id.c_str(), "card", f.arena, &trash, &f.error) == HOLDER_OK);
  REQUIRE(trash->count == 1);

  REQUIRE(holder_restore_card(f.context, f.project_id.c_str(), id.c_str(), f.arena, &card, &f.error) == HOLDER_OK);
  REQUIRE(std::string(card->content) == "Durable body");
  f.trash(id.c_str());
  const holder_trash_delete_result* removed = nullptr;
  REQUIRE(holder_delete_project_trash_item(f.context, f.project_id.c_str(),
                                           HOLDER_DELETE_PROJECT_TRASH_ITEM_TYPE_CARD, id.c_str(), f.arena,
                                           &removed, &f.error) == HOLDER_OK);
  REQUIRE(holder_list_trash(f.context, f.project_id.c_str(), "card", f.arena, &trash, &f.error) == HOLDER_OK);
  REQUIRE(trash->count == 0);
}

TEST_CASE("holder_restore_card reports a missing durable trash file", "[capi][contract]") {
  Fixture f;
  const std::string id = f.create("Doomed", "content")->card_id;
  const holder_project* project = nullptr;
  REQUIRE(holder_get_project(f.context, f.project_id.c_str(), f.arena, &project, &f.error) == HOLDER_OK);
  const std::filesystem::path root = project->root_path;
  f.trash(id.c_str());
  REQUIRE(std::filesystem::remove(root / holder::core::card_trash_rel_path(id)));
  const holder_card* card = nullptr;
  REQUIRE(f.failed(holder_restore_card(f.context, f.project_id.c_str(), id.c_str(), f.arena, &card,
                                       &f.error)) == HOLDER_ERROR_RUNTIME);
}

TEST_CASE("holder_trash_card promotes children and restore keeps their placement", "[capi][contract]") {
  Fixture f;
  const std::string parent_id = f.create("Parent")->card_id;
  const std::string child_id = f.create("Child", nullptr, parent_id.c_str())->card_id;
  f.trash(parent_id.c_str());
  const holder_card* card = nullptr;
  REQUIRE(holder_get_card(f.context, f.project_id.c_str(), child_id.c_str(), f.arena, &card, &f.error) == HOLDER_OK);
  REQUIRE(card->parent_card_id == nullptr);
  REQUIRE(holder_restore_card(f.context, f.project_id.c_str(), parent_id.c_str(), f.arena, &card, &f.error) ==
          HOLDER_OK);
  REQUIRE(holder_get_card(f.context, f.project_id.c_str(), child_id.c_str(), f.arena, &card, &f.error) == HOLDER_OK);
  REQUIRE(card->parent_card_id == nullptr);
}

TEST_CASE("holder_list_cards filters by level and adds content and counts on request", "[capi][contract]") {
  Fixture f;
  const auto* root = f.create("Root", "Body");
  f.create("Child 1", nullptr, root->card_id);
  f.create("Child 2", nullptr, root->card_id);

  holder_list_cards_parent all{};
  REQUIRE(f.list(all)->items.count == 3);
  REQUIRE(f.list(all)->next_cursor == nullptr);

  const auto* roots = f.list({HOLDER_LIST_CARDS_PARENT_ROOTS, nullptr}, nullptr, nullptr, true, true);
  REQUIRE(roots->items.count == 1);
  REQUIRE(std::string(roots->items.items[0].content) == "Body");
  REQUIRE(*roots->items.items[0].child_count == 2);

  const auto* children = f.list({HOLDER_LIST_CARDS_PARENT_CARD_ID, root->card_id});
  REQUIRE(children->items.count == 2);
  REQUIRE(children->items.items[0].content == nullptr);
  REQUIRE(children->items.items[0].child_count == nullptr);
}

TEST_CASE("holder_list_cards pages through every card once", "[capi][contract]") {
  Fixture f;
  for (int i = 0; i < 5; ++i) f.create(("Card " + std::to_string(i)).c_str());
  holder_list_cards_parent all{};
  const int32_t two = 2;
  std::set<std::string> seen;
  std::string cursor;
  int pages = 0;
  do {
    const auto* page = f.list(all, &two, cursor.empty() ? nullptr : cursor.c_str(), false, false,
                              HOLDER_LIST_CARDS_ORDER_UPDATED_DESC);
    ++pages;
    for (std::size_t i = 0; i < page->items.count; ++i) seen.insert(page->items.items[i].card_id);
    cursor = page->next_cursor ? page->next_cursor : "";
  } while (!cursor.empty());
  REQUIRE(pages == 3);
  REQUIRE(seen.size() == 5);
}

TEST_CASE("holder_list_cards filters tags without case sensitivity", "[capi][contract]") {
  Fixture f;
  const std::string tagged = f.create("Tagged", "Body #Draft")->card_id;
  f.create("Other", "Body #review");
  const holder_card_list_item_page* page = nullptr;
  REQUIRE(holder_list_cards(f.context, f.project_id.c_str(), {}, "draft", HOLDER_LIST_CARDS_ORDER_DEFAULT,
                            nullptr, nullptr, false, false, f.arena, &page, &f.error) == HOLDER_OK);
  REQUIRE(page->items.count == 1);
  REQUIRE(std::string(page->items.items[0].card_id) == tagged);
}

TEST_CASE("holder_search_cards follows card changes and reindexing", "[capi][contract]") {
  Fixture f;
  const std::string id = f.create("Searchable", "alpha marker")->card_id;
  const holder_search_card_list* results = nullptr;
  REQUIRE(holder_search_cards(f.context, f.project_id.c_str(), "alpha", nullptr, 0, f.arena,
                              &results, &f.error) == HOLDER_OK);
  REQUIRE(results->count == 1);
  REQUIRE(std::string(results->items[0].card_id) == id);

  holder_card_update update{};
  update.content = "beta marker";
  const holder_card* card = nullptr;
  REQUIRE(holder_update_card(f.context, f.project_id.c_str(), id.c_str(), &update, f.arena, &card, &f.error) ==
          HOLDER_OK);
  REQUIRE(holder_search_cards(f.context, f.project_id.c_str(), "beta", nullptr, 0, f.arena,
                              &results, &f.error) == HOLDER_OK);
  REQUIRE(results->count == 1);
  const holder_reindex_result* reindexed = nullptr;
  REQUIRE(holder_reindex_search(f.context, f.arena, &reindexed, &f.error) == HOLDER_OK);
  REQUIRE(holder_search_cards(f.context, f.project_id.c_str(), "Searchable", nullptr, 0, f.arena,
                              &results, &f.error) == HOLDER_OK);
  REQUIRE(results->count == 1);
  f.trash(id.c_str());
  REQUIRE(holder_search_cards(f.context, f.project_id.c_str(), "Searchable", nullptr, 0, f.arena,
                              &results, &f.error) == HOLDER_OK);
  REQUIRE(results->count == 0);
}

TEST_CASE("holder_list_card_history reads committed card changes", "[capi][contract]") {
  Fixture f;
  const std::string id = f.create("History", "First body")->card_id;
  holder_card_update update{};
  update.content = "Second body";
  const holder_card* card = nullptr;
  REQUIRE(holder_update_card(f.context, f.project_id.c_str(), id.c_str(), &update, f.arena, &card, &f.error) ==
          HOLDER_OK);
  const holder_card_history_page* page = nullptr;
  REQUIRE(holder_list_card_history(f.context, f.project_id.c_str(), id.c_str(), nullptr, nullptr, f.arena,
                                   &page, &f.error) == HOLDER_OK);
  REQUIRE(page->entries.count > 0);
  REQUIRE(page->head_oid != nullptr);
}

TEST_CASE("holder_list_cards rejects bad queries", "[capi][contract]") {
  Fixture f;
  f.create("One");
  f.create("Two");
  holder_list_cards_parent all{};
  const int32_t one = 1;
  const auto* first = f.list(all, &one);
  REQUIRE(first->next_cursor != nullptr);
  const std::string cursor = first->next_cursor;
  const holder_card_list_item_page* page = nullptr;

  // A cursor only continues the query it came from.
  REQUIRE(f.failed(holder_list_cards(f.context, f.project_id.c_str(), all, nullptr,
                                     HOLDER_LIST_CARDS_ORDER_SORT_KEY_ASC, &one, cursor.c_str(), false, false,
                                     f.arena, &page, &f.error)) == HOLDER_ERROR_INVALID_ARGUMENT);
  REQUIRE(f.failed(holder_list_cards(f.context, f.project_id.c_str(), all, nullptr, HOLDER_LIST_CARDS_ORDER_DEFAULT,
                                     &one, "zz", false, false, f.arena, &page, &f.error)) ==
          HOLDER_ERROR_INVALID_ARGUMENT);
  const int32_t big = 2000;
  REQUIRE(f.failed(holder_list_cards(f.context, f.project_id.c_str(), all, nullptr, HOLDER_LIST_CARDS_ORDER_DEFAULT,
                                     &big, nullptr, true, false, f.arena, &page, &f.error)) ==
          HOLDER_ERROR_INVALID_ARGUMENT);
  REQUIRE(f.failed(holder_list_cards(f.context, f.project_id.c_str(), all, "#bad", HOLDER_LIST_CARDS_ORDER_DEFAULT,
                                     nullptr, nullptr, false, false, f.arena, &page, &f.error)) ==
          HOLDER_ERROR_INVALID_ARGUMENT);
  REQUIRE(f.failed(holder_list_cards(f.context, "no-such-project", all, nullptr, HOLDER_LIST_CARDS_ORDER_DEFAULT,
                                     nullptr, nullptr, false, false, f.arena, &page, &f.error)) ==
          HOLDER_ERROR_NOT_FOUND);
}

namespace {

bool has_tag(const holder_card* card, const std::string& tag) {
  for (std::size_t i = 0; i < card->tags.count; ++i)
    if (card->tags.items[i] == tag) return true;
  return false;
}

} // namespace

TEST_CASE("holder_add_card_tag and holder_remove_card_tag edit only the trailing tag line", "[capi][contract]") {
  Fixture f;
  const std::string card_id = f.create("Notes", "About #prose here")->card_id;
  const auto* project = f.project_id.c_str();
  const holder_card* card = nullptr;

  REQUIRE(holder_add_card_tag(f.context, project, card_id.c_str(), "Idea", f.arena, &card, &f.error) == HOLDER_OK);
  REQUIRE(has_tag(card, "idea"));
  REQUIRE(std::string(card->content).find("#idea") != std::string::npos);
  const auto updated_at = card->updated_at;
  const std::string content = card->content;

  // A tag the card already carries, on its tag line or in its prose, leaves it as it is.
  for (const char* tag : {"idea", "PROSE"}) {
    REQUIRE(holder_add_card_tag(f.context, project, card_id.c_str(), tag, f.arena, &card, &f.error) == HOLDER_OK);
    REQUIRE(card->content == content);
    REQUIRE(card->updated_at == updated_at);
  }

  REQUIRE(holder_remove_card_tag(f.context, project, card_id.c_str(), "IDEA", f.arena, &card, &f.error) ==
          HOLDER_OK);
  REQUIRE_FALSE(has_tag(card, "idea"));
  REQUIRE(holder_remove_card_tag(f.context, project, card_id.c_str(), "absent", f.arena, &card, &f.error) ==
          HOLDER_OK);
  // Holder does not rewrite prose, so a tag that would stay in it is a conflict, even when the tag
  // line carries it too.
  REQUIRE(f.failed(holder_remove_card_tag(f.context, project, card_id.c_str(), "prose", f.arena, &card,
                                          &f.error)) == HOLDER_ERROR_CONFLICT);
  const std::string both_id = f.create("Both", "About #twice\n\n#twice")->card_id;
  REQUIRE(f.failed(holder_remove_card_tag(f.context, project, both_id.c_str(), "twice", f.arena, &card,
                                          &f.error)) == HOLDER_ERROR_CONFLICT);
  REQUIRE(holder_get_card(f.context, project, both_id.c_str(), f.arena, &card, &f.error) == HOLDER_OK);
  REQUIRE(std::string(card->content) == "About #twice\n\n#twice");

  REQUIRE(f.failed(holder_add_card_tag(f.context, project, card_id.c_str(), "fff", f.arena, &card, &f.error)) ==
          HOLDER_ERROR_INVALID_ARGUMENT);
  REQUIRE(f.failed(holder_add_card_tag(f.context, project, card_id.c_str(), "#idea", f.arena, &card, &f.error)) ==
          HOLDER_ERROR_INVALID_ARGUMENT);
  REQUIRE(f.failed(holder_add_card_tag(f.context, "no-such-project", card_id.c_str(), "idea", f.arena, &card,
                                       &f.error)) == HOLDER_ERROR_NOT_FOUND);
  REQUIRE(f.failed(holder_remove_card_tag(f.context, project, "no-such-card", "idea", f.arena, &card, &f.error)) ==
          HOLDER_ERROR_NOT_FOUND);
}

TEST_CASE("holder_list_project_tags counts live cards, most used first", "[capi][contract]") {
  Fixture f;
  f.create("One", "#beta #alpha");
  f.create("Two", "#beta");
  const auto* trashed = f.create("Three", "#gamma #beta");
  f.trash(trashed->card_id);

  const holder_tag_count_list* tags = nullptr;
  REQUIRE(holder_list_project_tags(f.context, f.project_id.c_str(), f.arena, &tags, &f.error) == HOLDER_OK);
  REQUIRE(tags->count == 2);
  REQUIRE(std::string(tags->items[0].tag) == "beta");
  REQUIRE(tags->items[0].card_count == 2);
  REQUIRE(std::string(tags->items[1].tag) == "alpha");
  REQUIRE(tags->items[1].card_count == 1);

  REQUIRE(f.failed(holder_list_project_tags(f.context, "no-such-project", f.arena, &tags, &f.error)) ==
          HOLDER_ERROR_NOT_FOUND);
}

namespace {

holder_new_link card_link(const char* target, const char* kind = "ref", const char* project = nullptr) {
  holder_new_link link{};
  link.target_type = HOLDER_NEW_LINK_TARGET_TYPE_CARD;
  link.target_id = target;
  link.kind = kind;
  link.target_project_id = project;
  return link;
}

} // namespace

TEST_CASE("a card lists the tags remove_card_tag would take off it", "[capi][contract]") {
  Fixture f;
  const auto* card = f.create("Tagged", "Prose with #mixed and #prose\n\n#mixed #line");
  REQUIRE(card->tags.count == 3);
  REQUIRE(card->removable_tags.count == 1);
  REQUIRE(std::string(card->removable_tags.items[0]) == "line");
}

TEST_CASE("holder_list_storage_locations lists a project's locations, most recently updated first", "[capi][contract]") {
  Fixture f;
  const auto add = [&](const char* id, int64_t updated_at) {
    holder_new_storage_location location{};
    location.location_id = id;
    location.project_id = f.project_id.c_str();
    location.name = id;
    location.provider = HOLDER_NEW_STORAGE_LOCATION_PROVIDER_LOCAL_DIRECTORY;
    location.created_at = &updated_at;
    location.updated_at = &updated_at;
    const holder_project_storage_location* made = nullptr;
    REQUIRE(holder_create_storage_location(f.context, &location, f.arena, &made, &f.error) == HOLDER_OK);
  };
  add("older", 1700000000);
  add("newer", 1700000100);

  const holder_storage_location_list* listed = nullptr;
  REQUIRE(holder_list_storage_locations(f.context, f.project_id.c_str(), f.arena, &listed, &f.error) == HOLDER_OK);
  REQUIRE(listed->data.count == 2);
  REQUIRE(std::string(listed->data.items[0].location_id) == "newer");
  REQUIRE(std::string(listed->data.items[1].location_id) == "older");
  // Without a binding store, nothing is bound on this device and nothing is preferred.
  REQUIRE_FALSE(listed->data.items[0].bound);
  REQUIRE(listed->preferred_location_id == nullptr);

  // A project this device does not have lists none, as the daemon's listing always has.
  REQUIRE(holder_list_storage_locations(f.context, "no-such-project", f.arena, &listed, &f.error) == HOLDER_OK);
  REQUIRE(listed->data.count == 0);
}

TEST_CASE("holder_list_link_kinds lists the catalog with both labels", "[capi][contract]") {
  Fixture f;
  const holder_link_kind_labels_list* kinds = nullptr;
  REQUIRE(holder_list_link_kinds(f.context, f.arena, &kinds, &f.error) == HOLDER_OK);
  REQUIRE(kinds->count == 100);
  auto labels = [&](const std::string& kind) -> std::pair<std::string, std::string> {
    for (std::size_t i = 0; i < kinds->count; ++i)
      if (kinds->items[i].kind == kind) return {kinds->items[i].forward_label, kinds->items[i].reverse_label};
    FAIL("no link kind " << kind);
    return {};
  };
  // ref is a link's default kind.
  REQUIRE(labels("ref") == std::pair<std::string, std::string>{"References", "Referenced by"});
  REQUIRE(labels("depends_on") == std::pair<std::string, std::string>{"Depends on", "Required by"});
}

TEST_CASE("holder_add_card_link makes one link per target and kind, and keeps when it was made", "[capi][contract]") {
  Fixture f;
  const auto* project = f.project_id.c_str();
  const std::string from = f.create("From")->card_id;
  const std::string to = f.create("To")->card_id;
  const holder_link* link = nullptr;

  auto request = card_link(to.c_str());
  request.label = "first";
  const int64_t when = 1700000000;
  request.created_at = &when;
  REQUIRE(holder_add_card_link(f.context, project, from.c_str(), &request, f.arena, &link, &f.error) == HOLDER_OK);
  REQUIRE(std::string(link->source_project_id) == f.project_id);
  REQUIRE(link->source_type == HOLDER_LINK_SOURCE_TYPE_CARD);
  REQUIRE(std::string(link->source_id) == from);
  REQUIRE(std::string(link->target_project_id) == f.project_id);
  REQUIRE(link->target_type == HOLDER_LINK_TARGET_TYPE_CARD);
  REQUIRE(std::string(link->target_id) == to);
  REQUIRE(std::string(link->kind) == "ref");
  REQUIRE(std::string(link->label) == "first");
  REQUIRE(link->created_at == when);

  // Making it again sets its label and keeps when it was made.
  request.label = "second";
  request.created_at = nullptr;
  REQUIRE(holder_add_card_link(f.context, project, from.c_str(), &request, f.arena, &link, &f.error) == HOLDER_OK);
  REQUIRE(std::string(link->label) == "second");
  REQUIRE(link->created_at == when);
  auto other_kind = card_link(to.c_str(), "depends_on");
  REQUIRE(holder_add_card_link(f.context, project, from.c_str(), &other_kind, f.arena, &link, &f.error) ==
          HOLDER_OK);
  REQUIRE(link->label == nullptr);

  const holder_link_list* links = nullptr;
  REQUIRE(holder_list_card_links(f.context, project, from.c_str(), false, f.arena, &links, &f.error) == HOLDER_OK);
  REQUIRE(links->count == 2);

  // A link to a card in the trash is listed only when asked for.
  f.trash(to.c_str());
  REQUIRE(holder_list_card_links(f.context, project, from.c_str(), false, f.arena, &links, &f.error) == HOLDER_OK);
  REQUIRE(links->count == 0);
  REQUIRE(holder_list_card_links(f.context, project, from.c_str(), true, f.arena, &links, &f.error) == HOLDER_OK);
  REQUIRE(links->count == 2);

  // Nor can one be made to it, or to a card or thread that does not exist.
  auto trashed = card_link(to.c_str(), "see_also");
  REQUIRE(f.failed(holder_add_card_link(f.context, project, from.c_str(), &trashed, f.arena, &link, &f.error)) ==
          HOLDER_ERROR_NOT_FOUND);
  auto missing = card_link("no-such-card");
  REQUIRE(f.failed(holder_add_card_link(f.context, project, from.c_str(), &missing, f.arena, &link, &f.error)) ==
          HOLDER_ERROR_NOT_FOUND);
  missing.target_type = HOLDER_NEW_LINK_TARGET_TYPE_AI_THREAD;
  REQUIRE(f.failed(holder_add_card_link(f.context, project, from.c_str(), &missing, f.arena, &link, &f.error)) ==
          HOLDER_ERROR_NOT_FOUND);
  // Nor from a card in another project.
  auto fine = card_link(from.c_str());
  REQUIRE(f.failed(holder_add_card_link(f.context, "no-such-project", from.c_str(), &fine, f.arena, &link,
                                        &f.error)) == HOLDER_ERROR_NOT_FOUND);
}

TEST_CASE("Links to cards in other projects, and the backlinks they make", "[capi][contract]") {
  Fixture f;
  const auto* project = f.project_id.c_str();
  const auto other = f.new_project("Other");
  const std::string from = f.create("From")->card_id;
  const std::string local = f.create("Local")->card_id;
  holder_new_card there{};
  there.title = "There";
  const holder_card* created = nullptr;
  REQUIRE(holder_create_card(f.context, other.c_str(), &there, f.arena, &created, &f.error) == HOLDER_OK);
  const std::string remote = created->card_id;
  const holder_link* link = nullptr;

  auto to_remote = card_link(remote.c_str(), "ref", other.c_str());
  REQUIRE(holder_add_card_link(f.context, project, from.c_str(), &to_remote, f.arena, &link, &f.error) == HOLDER_OK);
  REQUIRE(std::string(link->target_project_id) == other);
  // A card in another project need not be on this device.
  auto unresolved = card_link("elsewhere", "ref", "unknown-project");
  REQUIRE(holder_add_card_link(f.context, project, from.c_str(), &unresolved, f.arena, &link, &f.error) ==
          HOLDER_OK);
  // Only a card can be in another project.
  auto thread = card_link("t1", "ref", other.c_str());
  thread.target_type = HOLDER_NEW_LINK_TARGET_TYPE_AI_THREAD;
  REQUIRE(f.failed(holder_add_card_link(f.context, project, from.c_str(), &thread, f.arena, &link, &f.error)) ==
          HOLDER_ERROR_INVALID_ARGUMENT);
  auto to_local = card_link(local.c_str());
  REQUIRE(holder_add_card_link(f.context, project, from.c_str(), &to_local, f.arena, &link, &f.error) == HOLDER_OK);

  const holder_link_list* links = nullptr;
  REQUIRE(holder_list_card_backlinks(f.context, project, local.c_str(), HOLDER_LIST_CARD_BACKLINKS_SCOPE_DEFAULT,
                                     false, f.arena, &links, &f.error) == HOLDER_OK);
  REQUIRE(links->count == 1);
  REQUIRE(std::string(links->items[0].source_id) == from);
  REQUIRE(links->items[0].source_type == HOLDER_LINK_SOURCE_TYPE_CARD);
  REQUIRE(holder_list_card_backlinks(f.context, other.c_str(), remote.c_str(),
                                     HOLDER_LIST_CARD_BACKLINKS_SCOPE_LOCAL, false, f.arena, &links,
                                     &f.error) == HOLDER_OK);
  REQUIRE(links->count == 0);
  REQUIRE(holder_list_card_backlinks(f.context, other.c_str(), remote.c_str(),
                                     HOLDER_LIST_CARD_BACKLINKS_SCOPE_EXTERNAL, false, f.arena, &links,
                                     &f.error) == HOLDER_OK);
  REQUIRE(links->count == 1);
  REQUIRE(std::string(links->items[0].source_project_id) == f.project_id);
  REQUIRE(std::string(links->items[0].target_project_id) == other);

  // Removing names one link exactly, and removing it again changes nothing.
  for (int i = 0; i < 2; ++i) {
    REQUIRE(holder_remove_card_link(f.context, project, from.c_str(), HOLDER_REMOVE_CARD_LINK_TARGET_TYPE_CARD,
                                    remote.c_str(), "ref", other.c_str(), f.arena, &links, &f.error) == HOLDER_OK);
    REQUIRE(links->count == 2);
  }
  // Without its project, the target is in the card's own.
  REQUIRE(holder_remove_card_link(f.context, project, from.c_str(), HOLDER_REMOVE_CARD_LINK_TARGET_TYPE_CARD,
                                  local.c_str(), nullptr, nullptr, f.arena, &links, &f.error) == HOLDER_OK);
  REQUIRE(links->count == 1);
  REQUIRE(std::string(links->items[0].target_id) == "elsewhere");
  REQUIRE(f.failed(holder_remove_card_link(f.context, other.c_str(), from.c_str(),
                                           HOLDER_REMOVE_CARD_LINK_TARGET_TYPE_CARD, local.c_str(), nullptr,
                                           nullptr, f.arena, &links, &f.error)) == HOLDER_ERROR_NOT_FOUND);
}

namespace {

holder_milestone_date day(const char* role, const char* value) {
  holder_milestone_date date{};
  date.role = role;
  date.date = value;
  return date;
}

holder_milestone_date instant(const char* role, const char* value) {
  holder_milestone_date date{};
  date.role = role;
  date.datetime = value;
  return date;
}

std::vector<std::string> ids(const holder_milestone_list* list) {
  std::vector<std::string> out;
  for (std::size_t i = 0; i < list->count; ++i) out.emplace_back(list->items[i].milestone_id);
  return out;
}

} // namespace

TEST_CASE("Milestones on a card keep their dates as given and change one field at a time", "[capi][contract]") {
  Fixture f;
  const auto* project = f.project_id.c_str();
  const auto* card = f.create("Launch");
  const std::string card_id = card->card_id;
  const holder_milestone* milestone = nullptr;

  const holder_milestone_date dates[] = {day("start", "2026-10-16"), instant("due", "2026-10-17T09:30:00+01:00"),
                                         day("start", "2026-10-18")};
  holder_new_milestone request{};
  request.dates = {dates, 3};
  request.kind = "release";
  REQUIRE(holder_add_card_milestone(f.context, project, card_id.c_str(), &request, f.arena, &milestone, &f.error) ==
          HOLDER_OK);
  const std::string milestone_id = milestone->milestone_id;
  REQUIRE(std::string(milestone->project_id) == f.project_id);
  REQUIRE(std::string(milestone->card_id) == card_id);
  REQUIRE(milestone->dates.count == 3);
  // A repeated role and an instant's own offset are kept.
  REQUIRE(std::string(milestone->dates.items[1].datetime) == "2026-10-17T09:30:00+01:00");
  REQUIRE(milestone->dates.items[1].date == nullptr);
  REQUIRE(std::string(milestone->dates.items[2].date) == "2026-10-18");
  REQUIRE(std::string(milestone->kind) == "release");
  REQUIRE(milestone->description == nullptr);
  const auto created_at = milestone->created_at;

  // An undated milestone.
  holder_new_milestone undated{};
  REQUIRE(holder_add_card_milestone(f.context, project, card_id.c_str(), &undated, f.arena, &milestone, &f.error) ==
          HOLDER_OK);
  REQUIRE(milestone->dates.count == 0);
  const std::string undated_id = milestone->milestone_id;

  const holder_milestone_list* list = nullptr;
  REQUIRE(holder_list_card_milestones(f.context, project, card_id.c_str(), f.arena, &list, &f.error) == HOLDER_OK);
  REQUIRE(list->count == 2);

  // Only the description changes; then nothing does, and nothing is written.
  holder_milestone_update update{};
  update.description = {HOLDER_SET, "Ship it"};
  REQUIRE(holder_update_card_milestone(f.context, project, card_id.c_str(), milestone_id.c_str(), &update, f.arena,
                                       &milestone, &f.error) == HOLDER_OK);
  REQUIRE(std::string(milestone->description) == "Ship it");
  REQUIRE(std::string(milestone->kind) == "release");
  REQUIRE(milestone->dates.count == 3);
  REQUIRE(milestone->created_at == created_at);
  const auto updated_at = milestone->updated_at;
  REQUIRE(holder_update_card_milestone(f.context, project, card_id.c_str(), milestone_id.c_str(), &update, f.arena,
                                       &milestone, &f.error) == HOLDER_OK);
  REQUIRE(milestone->updated_at == updated_at);

  // New dates replace them all; null clears the kind.
  const holder_milestone_date due[] = {day("due", "2026-11-01")};
  const holder_milestone_date_list due_list{due, 1};
  holder_milestone_update replace{};
  replace.dates = &due_list;
  replace.kind = {HOLDER_CLEAR, nullptr};
  REQUIRE(holder_update_card_milestone(f.context, project, card_id.c_str(), milestone_id.c_str(), &replace, f.arena,
                                       &milestone, &f.error) == HOLDER_OK);
  REQUIRE(milestone->dates.count == 1);
  REQUIRE(std::string(milestone->dates.items[0].role) == "due");
  REQUIRE(milestone->kind == nullptr);
  REQUIRE(std::string(milestone->description) == "Ship it");

  // Removing returns what remains, and removing again changes nothing.
  REQUIRE(holder_remove_card_milestone(f.context, project, card_id.c_str(), milestone_id.c_str(), f.arena, &list,
                                       &f.error) == HOLDER_OK);
  REQUIRE(ids(list) == std::vector<std::string>{undated_id});
  REQUIRE(holder_remove_card_milestone(f.context, project, card_id.c_str(), milestone_id.c_str(), f.arena, &list,
                                       &f.error) == HOLDER_OK);
  REQUIRE(ids(list) == std::vector<std::string>{undated_id});
  REQUIRE(f.failed(holder_update_card_milestone(f.context, project, card_id.c_str(), milestone_id.c_str(), &update,
                                                f.arena, &milestone, &f.error)) == HOLDER_ERROR_NOT_FOUND);
}

TEST_CASE("Milestone dates must be real days and instants, and their cards live ones in the project",
          "[capi][contract]") {
  Fixture f;
  const auto* project = f.project_id.c_str();
  const std::string card_id = f.create("Card")->card_id;
  const holder_milestone* milestone = nullptr;
  const auto add = [&](holder_milestone_date date) {
    holder_new_milestone request{};
    request.dates = {&date, 1};
    return holder_add_card_milestone(f.context, project, card_id.c_str(), &request, f.arena, &milestone, &f.error);
  };
  for (const auto& bad : {day("due", "2026-02-30"), day("due", "0000-01-01"), day("due", "2026-1-01x"),
                          instant("due", "2026-10-17T24:00:00Z"), instant("due", "2026-10-17 09:30:00Z"),
                          instant("due", "2026-10-17T09:30:00.5Z")}) {
    REQUIRE(f.failed(add(bad)) == HOLDER_ERROR_INVALID_ARGUMENT);
  }
  holder_milestone_date both = day("due", "2026-10-17");
  both.datetime = "2026-10-17T09:30:00Z";
  REQUIRE(f.failed(add(both)) == HOLDER_ERROR_INVALID_ARGUMENT);
  REQUIRE(f.failed(add(holder_milestone_date{"due", nullptr, nullptr})) == HOLDER_ERROR_INVALID_ARGUMENT);
  holder_new_milestone no_items{};
  no_items.dates = {nullptr, 1};
  REQUIRE(f.failed(holder_add_card_milestone(f.context, project, card_id.c_str(), &no_items, f.arena, &milestone,
                                             &f.error)) == HOLDER_ERROR_INVALID_ARGUMENT);
  REQUIRE(add(instant("due", "2026-10-17T09:30:00Z")) == HOLDER_OK);

  // Not on a card in another project, or in the trash.
  const auto other = f.new_project("Other");
  const holder_milestone_list* list = nullptr;
  REQUIRE(f.failed(holder_list_card_milestones(f.context, other.c_str(), card_id.c_str(), f.arena, &list,
                                               &f.error)) == HOLDER_ERROR_NOT_FOUND);
  const holder_card* trashed = nullptr;
  REQUIRE(holder_trash_card(f.context, project, card_id.c_str(), HOLDER_TRASH_CARD_CHILDREN_DEFAULT, f.arena,
                            &trashed, &f.error) == HOLDER_OK);
  REQUIRE(f.failed(add(day("due", "2026-10-17"))) == HOLDER_ERROR_NOT_FOUND);
  REQUIRE(f.failed(holder_list_card_milestones(f.context, project, card_id.c_str(), f.arena, &list, &f.error)) ==
          HOLDER_ERROR_NOT_FOUND);
}

TEST_CASE("Calendar separates created and updated cards and sorts equal update times",
          "[capi][calendar-updated-cards]") {
  Fixture f;
  constexpr int64_t created = 1790931600; // 2026-10-02 09:00 UTC
  constexpr int64_t updated = 1791194400; // 2026-10-05 10:00 UTC
  std::vector<std::string> edited;
  for (const auto* title : {"First edit", "Second edit", "Unchanged"}) {
    const bool unchanged = std::string(title) == "Unchanged";
    holder_new_card request{};
    request.title = title;
    request.created_at = &created;
    request.updated_at = unchanged ? &created : &updated;
    const holder_card* card = nullptr;
    REQUIRE(holder_create_card(f.context, f.project_id.c_str(), &request, f.arena, &card, &f.error) == HOLDER_OK);
    if (!unchanged) edited.emplace_back(card->card_id);
  }
  std::sort(edited.begin(), edited.end());
  const holder_calendar* calendar = nullptr;
  REQUIRE(holder_get_calendar(f.context, f.project_id.c_str(), "2026-10-05", "2026-10-05", "UTC",
      f.arena, &calendar, &f.error) == HOLDER_OK);
  REQUIRE(calendar != nullptr);
  CHECK(calendar->created_cards.count == 0);
  REQUIRE(calendar->updated_cards.count == 2);
  CHECK(std::string(calendar->updated_cards.items[0].card_id) == edited[0]);
  CHECK(std::string(calendar->updated_cards.items[1].card_id) == edited[1]);
  REQUIRE(holder_get_calendar(f.context, f.project_id.c_str(), "2026-10-02", "2026-10-02", "UTC",
      f.arena, &calendar, &f.error) == HOLDER_OK);
  CHECK(calendar->created_cards.count == 3);
  CHECK(calendar->updated_cards.count == 0);
}

TEST_CASE("A project's milestones by day, by instant and on its calendar", "[capi][contract]") {
  Fixture f;
  const auto* project = f.project_id.c_str();
  const std::string first = f.create("First")->card_id;
  const std::string second = f.create("Second")->card_id;
  const holder_milestone* milestone = nullptr;
  const auto add = [&](const std::string& card_id, std::vector<holder_milestone_date> dates) {
    holder_new_milestone request{};
    request.dates = {dates.data(), dates.size()};
    REQUIRE(holder_add_card_milestone(f.context, project, card_id.c_str(), &request, f.arena, &milestone,
                                      &f.error) == HOLDER_OK);
    return std::string(milestone->milestone_id);
  };
  const auto on_day = add(first, {day("start", "2026-10-16"), day("end", "2026-10-20")});
  const auto at_instant = add(second, {instant("due", "2026-10-17T23:30:00-02:00")});
  add(second, {day("start", "2027-01-01")});

  const holder_milestone_list* list = nullptr;
  REQUIRE(holder_list_milestones_by_day(f.context, project, "2026-10-01", "2026-10-31", nullptr, f.arena, &list,
                                        &f.error) == HOLDER_OK);
  REQUIRE(ids(list) == std::vector<std::string>{on_day});
  REQUIRE(holder_list_milestones_by_day(f.context, project, "2026-10-18", "2026-10-31", "start", f.arena, &list,
                                        &f.error) == HOLDER_OK);
  REQUIRE(list->count == 0);
  REQUIRE(holder_list_milestones_by_day(f.context, project, "2026-10-18", "2026-10-31", "end", f.arena, &list,
                                        &f.error) == HOLDER_OK);
  REQUIRE(ids(list) == std::vector<std::string>{on_day});
  // A range whose start is after its end is empty.
  REQUIRE(holder_list_milestones_by_day(f.context, project, "2026-10-31", "2026-10-01", nullptr, f.arena, &list,
                                        &f.error) == HOLDER_OK);
  REQUIRE(list->count == 0);
  REQUIRE(f.failed(holder_list_milestones_by_day(f.context, project, "2026-02-30", "2026-10-31", nullptr, f.arena,
                                                 &list, &f.error)) == HOLDER_ERROR_INVALID_ARGUMENT);
  REQUIRE(f.failed(holder_list_milestones_by_day(f.context, "no-such-project", "2026-10-01", "2026-10-31", nullptr,
                                                 f.arena, &list, &f.error)) == HOLDER_ERROR_NOT_FOUND);

  // 2026-10-18T01:30:00Z.
  const int64_t at = 1792287000;
  REQUIRE(holder_list_milestones_by_instant(f.context, project, at, at, nullptr, f.arena, &list, &f.error) ==
          HOLDER_OK);
  REQUIRE(ids(list) == std::vector<std::string>{at_instant});
  REQUIRE(holder_list_milestones_by_instant(f.context, project, at, at, "start", f.arena, &list, &f.error) ==
          HOLDER_OK);
  REQUIRE(list->count == 0);
  REQUIRE(holder_list_milestones_by_instant(f.context, project, at + 1, at, nullptr, f.arena, &list, &f.error) ==
          HOLDER_OK);
  REQUIRE(list->count == 0);

  // A window of days in a zone, reaching past today so the cards made by this test are in it.
  const holder_calendar* calendar = nullptr;
  REQUIRE(holder_get_calendar(f.context, project, "2026-10-01", "2099-12-31", "UTC", f.arena, &calendar, &f.error) ==
          HOLDER_OK);
  std::map<std::string, std::string> titles;
  for (std::size_t i = 0; i < calendar->milestones.count; ++i)
    titles[calendar->milestones.items[i].milestone_id] = calendar->milestones.items[i].card_title;
  REQUIRE(titles.size() == 3);
  REQUIRE(titles.at(on_day) == "First");
  REQUIRE(titles.at(at_instant) == "Second");
  REQUIRE(calendar->created_cards.count == 2);
  const auto& made = calendar->created_cards.items;
  REQUIRE(std::tuple(made[0].created_at, std::string(made[0].card_id)) <
          std::tuple(made[1].created_at, std::string(made[1].card_id)));

  // One day's milestones in a zone. The timed milestone, at 01:30 UTC on the 18th, is on the 18th in UTC but still
  // the 17th in Los Angeles. The day milestone's start, the 16th, is on the 16th wherever the caller is.
  const auto on = [&](const char* day, const char* zone) {
    REQUIRE(holder_get_calendar(f.context, project, day, day, zone, f.arena, &calendar, &f.error) == HOLDER_OK);
    std::vector<std::string> out;
    for (std::size_t i = 0; i < calendar->milestones.count; ++i)
      out.emplace_back(calendar->milestones.items[i].milestone_id);
    return out;
  };
  REQUIRE(on("2026-10-18", "UTC") == std::vector<std::string>{at_instant});
  REQUIRE(on("2026-10-18", "America/Los_Angeles").empty());
  REQUIRE(on("2026-10-17", "America/Los_Angeles") == std::vector<std::string>{at_instant});
  REQUIRE(on("2026-10-16", "UTC") == std::vector<std::string>{on_day});
  REQUIRE(on("2026-10-16", "America/Los_Angeles") == std::vector<std::string>{on_day});
  REQUIRE(calendar->created_cards.count == 0);

  // Milestones on a card in the trash are left out.
  const holder_card* trashed = nullptr;
  REQUIRE(holder_trash_card(f.context, project, first.c_str(), HOLDER_TRASH_CARD_CHILDREN_DEFAULT, f.arena, &trashed,
                            &f.error) == HOLDER_OK);
  REQUIRE(holder_get_calendar(f.context, project, "2026-10-01", "2099-12-31", "UTC", f.arena, &calendar, &f.error) ==
          HOLDER_OK);
  REQUIRE(calendar->milestones.count == 2);
  REQUIRE(calendar->created_cards.count == 1);
  REQUIRE(f.failed(holder_get_calendar(f.context, project, "2026-10-01", "October", "UTC", f.arena, &calendar,
                                       &f.error)) == HOLDER_ERROR_INVALID_ARGUMENT);
  REQUIRE(f.failed(holder_get_calendar(f.context, project, "2026-10-01", "2026-10-31", "Mars/Olympus_Mons", f.arena,
                                       &calendar, &f.error)) == HOLDER_ERROR_INVALID_ARGUMENT);
  // Without a zone, the device's own zone.
  REQUIRE(holder_get_calendar(f.context, project, "2026-10-01", "2099-12-31", nullptr, f.arena, &calendar, &f.error) ==
          HOLDER_OK);
  REQUIRE(calendar->milestones.count == 2);
}

namespace {

std::map<std::string, std::vector<std::string>> metadata_of(const holder_resource_metadata& metadata) {
  std::map<std::string, std::vector<std::string>> out;
  for (std::size_t i = 0; i < metadata.count; ++i) {
    auto& values = out[metadata.items[i].key];
    for (std::size_t k = 0; k < metadata.items[i].values.count; ++k)
      values.emplace_back(metadata.items[i].values.items[k]);
  }
  return out;
}

std::vector<std::string> resource_ids(const holder_resource_list* list) {
  std::vector<std::string> out;
  for (std::size_t i = 0; i < list->count; ++i) out.emplace_back(list->items[i].resource_id);
  return out;
}

} // namespace

TEST_CASE("Resources keep their metadata by name, and change one name at a time", "[capi][contract]") {
  Fixture f;
  const auto* project = f.project_id.c_str();
  const char* urls[] = {"https://example.com/a", "https://example.com/b"};
  const char* authors[] = {"Ada"};
  const holder_resource_metadata_entry entries[] = {{"identifier", {urls, 2}}, {"author", {authors, 1}}};
  const holder_resource_metadata metadata{entries, 2};
  holder_new_resource request{};
  request.resource_id = "res-1";
  request.type = "website";
  request.label = "Example";
  request.metadata = &metadata;
  const int64_t made = 1700000000;
  request.created_at = &made;
  const holder_resource* resource = nullptr;
  REQUIRE(holder_create_resource(f.context, project, &request, f.arena, &resource, &f.error) == HOLDER_OK);
  REQUIRE(std::string(resource->resource_id) == "res-1");
  REQUIRE(std::string(resource->project_id) == f.project_id);
  REQUIRE(resource->created_at == made);
  REQUIRE(resource->updated_at == made);
  REQUIRE(resource->assets.count == 0);
  const std::map<std::string, std::vector<std::string>> expected{
      {"author", {"Ada"}}, {"identifier", {"https://example.com/a", "https://example.com/b"}}};
  REQUIRE(metadata_of(resource->metadata) == expected);
  // The same ID again is a Conflict.
  REQUIRE(f.failed(holder_create_resource(f.context, project, &request, f.arena, &resource, &f.error)) ==
          HOLDER_ERROR_CONFLICT);
  // An ID names a file, so it holds nothing a path could read differently.
  for (const char* id : {"../../x", "res/1", "res.1"}) {
    request.resource_id = id;
    REQUIRE(f.failed(holder_create_resource(f.context, project, &request, f.arena, &resource, &f.error)) ==
            HOLDER_ERROR_INVALID_ARGUMENT);
  }
  request.resource_id = "res-1";

  // A name given replaces its values, an empty list removes it, and the rest are kept.
  const char* editors[] = {"Grace"};
  const holder_resource_metadata_entry changes[] = {{"author", {nullptr, 0}}, {"editor", {editors, 1}}};
  const holder_resource_metadata change{changes, 2};
  holder_resource_update update{};
  update.metadata = &change;
  update.label = "Renamed";
  REQUIRE(holder_update_resource(f.context, project, "res-1", &update, f.arena, &resource, &f.error) == HOLDER_OK);
  REQUIRE(std::string(resource->label) == "Renamed");
  REQUIRE(std::string(resource->type) == "website");
  REQUIRE(metadata_of(resource->metadata) ==
          std::map<std::string, std::vector<std::string>>{
              {"editor", {"Grace"}}, {"identifier", {"https://example.com/a", "https://example.com/b"}}});
  REQUIRE(resource->updated_at > made);
  const auto updated_at = resource->updated_at;
  // Changing nothing writes nothing.
  REQUIRE(holder_update_resource(f.context, project, "res-1", &update, f.arena, &resource, &f.error) == HOLDER_OK);
  REQUIRE(resource->updated_at == updated_at);

  // A repeated name is not a map.
  const holder_resource_metadata_entry twice[] = {{"author", {authors, 1}}, {"author", {authors, 1}}};
  const holder_resource_metadata repeated{twice, 2};
  update.metadata = &repeated;
  REQUIRE(f.failed(holder_update_resource(f.context, project, "res-1", &update, f.arena, &resource, &f.error)) ==
          HOLDER_ERROR_INVALID_ARGUMENT);

  const holder_resource_list* list = nullptr;
  REQUIRE(holder_list_resources(f.context, project, f.arena, &list, &f.error) == HOLDER_OK);
  REQUIRE(resource_ids(list) == std::vector<std::string>{"res-1"});
  REQUIRE(holder_get_resource(f.context, project, "res-1", f.arena, &resource, &f.error) == HOLDER_OK);
  REQUIRE(std::string(resource->label) == "Renamed");

  // Not from another project.
  const auto other = f.new_project("Other");
  REQUIRE(f.failed(holder_get_resource(f.context, other.c_str(), "res-1", f.arena, &resource, &f.error)) ==
          HOLDER_ERROR_NOT_FOUND);
  REQUIRE(f.failed(holder_create_resource(f.context, "no-such-project", &request, f.arena, &resource,
                                          &f.error)) == HOLDER_ERROR_NOT_FOUND);
}

TEST_CASE("Attaching resources to cards, their backlinks, and deleting a resource", "[capi][contract]") {
  Fixture f;
  const auto* project = f.project_id.c_str();
  const std::string card = f.create("Card")->card_id;
  const holder_resource* resource = nullptr;
  for (const char* id : {"res-1", "res-2"}) {
    holder_new_resource request{};
    request.resource_id = id;
    request.type = "book";
    request.label = id;
    REQUIRE(holder_create_resource(f.context, project, &request, f.arena, &resource, &f.error) == HOLDER_OK);
  }

  const holder_resource_list* attached = nullptr;
  REQUIRE(holder_attach_card_resource(f.context, project, card.c_str(), "res-1", f.arena, &attached, &f.error) ==
          HOLDER_OK);
  REQUIRE(resource_ids(attached) == std::vector<std::string>{"res-1"});
  const holder_card* got = nullptr;
  REQUIRE(holder_get_card(f.context, project, card.c_str(), f.arena, &got, &f.error) == HOLDER_OK);
  const auto updated_at = got->updated_at;
  // Attaching again changes nothing.
  REQUIRE(holder_attach_card_resource(f.context, project, card.c_str(), "res-1", f.arena, &attached, &f.error) ==
          HOLDER_OK);
  REQUIRE(holder_get_card(f.context, project, card.c_str(), f.arena, &got, &f.error) == HOLDER_OK);
  REQUIRE(got->updated_at == updated_at);
  REQUIRE(holder_attach_card_resource(f.context, project, card.c_str(), "res-2", f.arena, &attached, &f.error) ==
          HOLDER_OK);
  REQUIRE(attached->count == 2);
  REQUIRE(f.failed(holder_attach_card_resource(f.context, project, card.c_str(), "res-9", f.arena, &attached,
                                               &f.error)) == HOLDER_ERROR_NOT_FOUND);

  // Backlinks are links of every kind; an attachment is one.
  const holder_link_list* backlinks = nullptr;
  REQUIRE(holder_list_resource_backlinks(f.context, project, "res-1", false, f.arena, &backlinks, &f.error) ==
          HOLDER_OK);
  REQUIRE(backlinks->count == 1);
  REQUIRE(std::string(backlinks->items[0].source_id) == card);
  REQUIRE(std::string(backlinks->items[0].kind) == "attachment");
  REQUIRE(backlinks->items[0].target_type == HOLDER_LINK_TARGET_TYPE_RESOURCE);

  REQUIRE(holder_detach_card_resource(f.context, project, card.c_str(), "res-1", f.arena, &attached, &f.error) ==
          HOLDER_OK);
  REQUIRE(resource_ids(attached) == std::vector<std::string>{"res-2"});
  REQUIRE(holder_detach_card_resource(f.context, project, card.c_str(), "res-1", f.arena, &attached, &f.error) ==
          HOLDER_OK);
  REQUIRE(attached->count == 1);

  // Deleting a resource removes the links to it, and returns it as it was.
  REQUIRE(holder_delete_resource(f.context, project, "res-2", f.arena, &resource, &f.error) == HOLDER_OK);
  REQUIRE(std::string(resource->resource_id) == "res-2");
  REQUIRE(holder_list_card_resources(f.context, project, card.c_str(), f.arena, &attached, &f.error) == HOLDER_OK);
  REQUIRE(attached->count == 0);
  const holder_link_list* links = nullptr;
  REQUIRE(holder_list_card_links(f.context, project, card.c_str(), true, f.arena, &links, &f.error) == HOLDER_OK);
  REQUIRE(links->count == 0);
  REQUIRE(f.failed(holder_delete_resource(f.context, project, "res-2", f.arena, &resource, &f.error)) ==
          HOLDER_ERROR_NOT_FOUND);
  // A resource that no longer exists can still be detached.
  REQUIRE(holder_detach_card_resource(f.context, project, card.c_str(), "res-2", f.arena, &attached, &f.error) ==
          HOLDER_OK);
}

TEST_CASE(
    "Generated card updates validate a new parent before changing content",
    "[capi][parent-update]"
) {
  Fixture f;
  const std::string parent = f.create("Parent", "Original")->card_id;
  const std::string child = f.create("Child", "Child body", parent.c_str())->card_id;
  const std::string other_project = f.new_project("Other");
  std::string target;
  int expected = HOLDER_ERROR_NOT_FOUND;
  SECTION("self parent") {
    target = parent;
    expected = HOLDER_ERROR_INVALID_MOVE;
  }
  SECTION("descendant parent") {
    target = child;
    expected = HOLDER_ERROR_INVALID_MOVE;
  }
  SECTION("missing parent") { target = "00000000-0000-4000-8000-000000000001"; }
  SECTION("trashed parent") {
    target = f.create("Trashed")->card_id;
    f.trash(target.c_str());
  }
  SECTION("parent in another project") {
    holder_new_card request{};
    request.title = "Elsewhere";
    const holder_card* elsewhere = nullptr;
    REQUIRE(
        holder_create_card(
            f.context,
            other_project.c_str(),
            &request,
            f.arena,
            &elsewhere,
            &f.error
        ) == HOLDER_OK
    );
    target = elsewhere->card_id;
  }
  holder_card_update update{};
  update.title = "Must not be written";
  update.content = "Must not replace original";
  update.parent_card_id = {HOLDER_SET, target.c_str()};
  const holder_card* result = nullptr;
  REQUIRE(
      f.failed(holder_update_card(
          f.context,
          f.project_id.c_str(),
          parent.c_str(),
          &update,
          f.arena,
          &result,
          &f.error
      )) == expected
  );
  REQUIRE(
      holder_get_card(
          f.context,
          f.project_id.c_str(),
          parent.c_str(),
          f.arena,
          &result,
          &f.error
      ) == HOLDER_OK
  );
  REQUIRE(std::string(result->title) == "Parent");
  REQUIRE(std::string(result->content) == "Original");
  REQUIRE(result->parent_card_id == nullptr);
}

TEST_CASE(
    "Generated card updates can reparent and clear a parent without rewriting content",
    "[capi][parent-update]"
) {
  Fixture f;
  const std::string parent = f.create("Parent")->card_id;
  const std::string child = f.create("Child", "Keep this body")->card_id;
  holder_card_update update{};
  update.parent_card_id = {HOLDER_SET, parent.c_str()};
  const holder_card* result = nullptr;
  REQUIRE(
      holder_update_card(
          f.context,
          f.project_id.c_str(),
          child.c_str(),
          &update,
          f.arena,
          &result,
          &f.error
      ) == HOLDER_OK
  );
  REQUIRE(std::string(result->parent_card_id) == parent);
  REQUIRE(std::string(result->content) == "Keep this body");
  update.parent_card_id = {HOLDER_CLEAR, nullptr};
  REQUIRE(
      holder_update_card(
          f.context,
          f.project_id.c_str(),
          child.c_str(),
          &update,
          f.arena,
          &result,
          &f.error
      ) == HOLDER_OK
  );
  REQUIRE(result->parent_card_id == nullptr);
  REQUIRE(std::string(result->content) == "Keep this body");
}

TEST_CASE(
    "Generated card pages reject malformed and out of range cursor data",
    "[capi][cursor-validation]"
) {
  Fixture f;
  f.create("First");
  f.create("Second");
  const holder_list_cards_parent all{HOLDER_LIST_CARDS_PARENT_NONE, nullptr};
  const int32_t limit = 1;
  const auto* first =
      f.list(all, &limit, nullptr, false, false, HOLDER_LIST_CARDS_ORDER_SORT_KEY_ASC);
  REQUIRE(first->next_cursor != nullptr);
  const std::string valid = first->next_cursor;
  std::string decoded;
  for (size_t i = 0; i < valid.size(); i += 2)
    decoded += static_cast<char>(std::stoul(valid.substr(i, 2), nullptr, 16));
  auto data = nlohmann::json::parse(decoded);
  std::string raw;
  SECTION("odd hex length") { raw = "0"; }
  SECTION("invalid hex digits") { raw = "gg"; }
  SECTION("invalid JSON") { raw = "7b"; }
  SECTION("unsupported version") { data["version"] = 2; }
  SECTION("missing card id") { data.erase("card_id"); }
  SECTION("empty card id") { data["card_id"] = ""; }
  SECTION("nonstring card id") { data["card_id"] = 42; }
  SECTION("fractional timestamp") { data["updated_at"] = 1.5; }
  SECTION("unsigned timestamp overflow") {
    data["updated_at"] = std::numeric_limits<unsigned long long>::max();
  }
  SECTION("nonnumeric sort key") { data["sort_key"] = "first"; }
  SECTION("missing sort key") { data.erase("sort_key"); }
  const auto cursor = raw.empty() ? holder::contract::encode_cursor(data) : raw;
  const holder_card_list_item_page* page = nullptr;
  REQUIRE(
      f.failed(holder_list_cards(
          f.context,
          f.project_id.c_str(),
          all,
          nullptr,
          HOLDER_LIST_CARDS_ORDER_SORT_KEY_ASC,
          &limit,
          cursor.c_str(),
          false,
          false,
          f.arena,
          &page,
          &f.error
      )) == HOLDER_ERROR_INVALID_ARGUMENT
  );
  REQUIRE(page == nullptr);
  // A failed request must not damage the original continuation.
  const auto* next =
      f.list(all, &limit, valid.c_str(), false, false, HOLDER_LIST_CARDS_ORDER_SORT_KEY_ASC);
  REQUIRE(next->items.count == 1);
  REQUIRE(std::string(next->items.items[0].card_id) != first->items.items[0].card_id);
}

TEST_CASE(
    "Generated card move intents preserve sibling order and reject impossible moves",
    "[capi][move-intents]"
) {
  Fixture f;
  const std::string a = f.create("A")->card_id;
  const std::string b = f.create("B")->card_id;
  const std::string c = f.create("C")->card_id;
  auto move =
      [&](holder_card_move_intent intent, const std::string& id, const char* target = nullptr) {
        holder_card_move request{};
        request.intent = intent;
        request.target_card_id = target;
        const holder_card* result = nullptr;
        REQUIRE(
            holder_move_card(
                f.context,
                f.project_id.c_str(),
                id.c_str(),
                &request,
                f.arena,
                &result,
                &f.error
            ) == HOLDER_OK
        );
        REQUIRE(std::string(result->card_id) == id);
        return result;
      };
  const holder_list_cards_parent roots{HOLDER_LIST_CARDS_PARENT_ROOTS, nullptr};
  auto order = [&]() {
    const auto* page =
        f.list(roots, nullptr, nullptr, false, false, HOLDER_LIST_CARDS_ORDER_SORT_KEY_ASC);
    std::vector<std::string> ids;
    for (size_t i = 0; i < page->items.count; ++i)
      ids.emplace_back(page->items.items[i].card_id);
    return ids;
  };
  move(HOLDER_CARD_MOVE_INTENT_BEFORE, c, a.c_str());
  REQUIRE(order() == std::vector<std::string>{c, a, b});
  move(HOLDER_CARD_MOVE_INTENT_AFTER, c, b.c_str());
  REQUIRE(order() == std::vector<std::string>{a, b, c});
  move(HOLDER_CARD_MOVE_INTENT_LEFT, c);
  REQUIRE(order() == std::vector<std::string>{a, c, b});
  move(HOLDER_CARD_MOVE_INTENT_RIGHT, c);
  REQUIRE(order() == std::vector<std::string>{a, b, c});
  REQUIRE(std::string(move(HOLDER_CARD_MOVE_INTENT_INTO, c, a.c_str())->parent_card_id) == a);
  REQUIRE(move(HOLDER_CARD_MOVE_INTENT_UP_LEVEL, c)->parent_card_id == nullptr);
  holder_card_move invalid{};
  invalid.intent = HOLDER_CARD_MOVE_INTENT_UP_LEVEL;
  const holder_card* result = nullptr;
  REQUIRE(
      f.failed(holder_move_card(
          f.context,
          f.project_id.c_str(),
          a.c_str(),
          &invalid,
          f.arena,
          &result,
          &f.error
      )) == HOLDER_ERROR_INVALID_MOVE
  );
  invalid.intent = HOLDER_CARD_MOVE_INTENT_INTO;
  REQUIRE(
      f.failed(holder_move_card(
          f.context,
          f.project_id.c_str(),
          a.c_str(),
          &invalid,
          f.arena,
          &result,
          &f.error
      )) == HOLDER_ERROR_INVALID_ARGUMENT
  );
}

TEST_CASE("Generated C API rejects unknown input enums before mutating cards or projects", "[capi][enum-validation]") {
  Fixture f;
  const std::string id = f.create("Original", "Keep this body")->card_id;
  int status = HOLDER_OK;
  SECTION("partial update change tag") {
    holder_card_update request{};
    request.title = "Must not be saved";
    request.parent_card_id.change = static_cast<holder_change>(3);
    const holder_card* result = nullptr;
    status = holder_update_card(
        f.context,
        f.project_id.c_str(),
        id.c_str(),
        &request,
        f.arena,
        &result,
        &f.error
    );
    REQUIRE(result == nullptr);
  }
  SECTION("card reference scope") {
    holder_card_reference_resolve_request request{
        f.project_id.c_str(),
        id.c_str(),
        HOLDER_CARD_REFERENCE_RESOLVE_REQUEST_SCOPE_MAX_ENUM
    };
    const holder_card_reference_resolve_data* result = nullptr;
    status = holder_resolve_card_reference(f.context, &request, f.arena, &result, &f.error);
    REQUIRE(result == nullptr);
  }
  SECTION("parent filter tag") {
    const holder_list_cards_parent parent{HOLDER_LIST_CARDS_PARENT_MAX_ENUM, nullptr};
    const holder_card_list_item_page* result = nullptr;
    status = holder_list_cards(
        f.context,
        f.project_id.c_str(),
        parent,
        nullptr,
        HOLDER_LIST_CARDS_ORDER_DEFAULT,
        nullptr,
        nullptr,
        false,
        false,
        f.arena,
        &result,
        &f.error
    );
  }
  SECTION("card order") {
    const holder_list_cards_parent parent{HOLDER_LIST_CARDS_PARENT_NONE, nullptr};
    const holder_card_list_item_page* result = nullptr;
    status = holder_list_cards(
        f.context,
        f.project_id.c_str(),
        parent,
        nullptr,
        HOLDER_LIST_CARDS_ORDER_MAX_ENUM,
        nullptr,
        nullptr,
        false,
        false,
        f.arena,
        &result,
        &f.error
    );
  }
  SECTION("trash child policy") {
    const holder_card* result = nullptr;
    status = holder_trash_card(
        f.context,
        f.project_id.c_str(),
        id.c_str(),
        HOLDER_TRASH_CARD_CHILDREN_MAX_ENUM,
        f.arena,
        &result,
        &f.error
    );
  }
  SECTION("backlink scope") {
    const holder_link_list* result = nullptr;
    status = holder_list_card_backlinks(
        f.context,
        f.project_id.c_str(),
        id.c_str(),
        HOLDER_LIST_CARD_BACKLINKS_SCOPE_MAX_ENUM,
        false,
        f.arena,
        &result,
        &f.error
    );
  }
  SECTION("new link target type") {
    holder_new_link request{};
    request.target_id = id.c_str();
    request.target_type = HOLDER_NEW_LINK_TARGET_TYPE_MAX_ENUM;
    const holder_link* result = nullptr;
    status = holder_add_card_link(
        f.context,
        f.project_id.c_str(),
        id.c_str(),
        &request,
        f.arena,
        &result,
        &f.error
    );
  }
  SECTION("removed link target type") {
    const holder_link_list* result = nullptr;
    status = holder_remove_card_link(
        f.context,
        f.project_id.c_str(),
        id.c_str(),
        HOLDER_REMOVE_CARD_LINK_TARGET_TYPE_MAX_ENUM,
        id.c_str(),
        "ref",
        nullptr,
        f.arena,
        &result,
        &f.error
    );
  }
  SECTION("project order") {
    const holder_project_list_item_list* result = nullptr;
    status = holder_list_projects(
        f.context,
        nullptr,
        nullptr,
        nullptr,
        HOLDER_LIST_PROJECTS_ORDER_MAX_ENUM,
        nullptr,
        0,
        false,
        f.arena,
        &result,
        &f.error
    );
  }
  SECTION("empty Trash type") {
    const holder_trash_empty_result* result = nullptr;
    status = holder_empty_trash(
        f.context,
        f.project_id.c_str(),
        HOLDER_EMPTY_TRASH_TYPE_MAX_ENUM,
        f.arena,
        &result,
        &f.error
    );
  }
  SECTION("global Trash deletion type") {
    const holder_trash_delete_result* result = nullptr;
    status = holder_delete_trash_item(
        f.context,
        HOLDER_DELETE_TRASH_ITEM_TYPE_MAX_ENUM,
        id.c_str(),
        f.arena,
        &result,
        &f.error
    );
  }
  SECTION("project Trash deletion type") {
    const holder_trash_delete_result* result = nullptr;
    status = holder_delete_project_trash_item(
        f.context,
        f.project_id.c_str(),
        HOLDER_DELETE_PROJECT_TRASH_ITEM_TYPE_MAX_ENUM,
        id.c_str(),
        f.arena,
        &result,
        &f.error
    );
  }
  SECTION("new project privacy mode") {
    holder_new_project request{};
    request.name = "Must not exist";
    request.privacy_mode = HOLDER_NEW_PROJECT_PRIVACY_MODE_MAX_ENUM;
    const holder_project* result = nullptr;
    status = holder_create_project(f.context, &request, f.arena, &result, &f.error);
  }
  SECTION("restore privacy mode") {
    holder_recovery_snapshot_restore request{};
    request.project_name = "Must not exist";
    request.privacy_mode = HOLDER_RECOVERY_SNAPSHOT_RESTORE_PRIVACY_MODE_MAX_ENUM;
    request.snapshot = "unused";
    request.commit_message = "unused";
    const holder_project* result = nullptr;
    status = holder_restore_recovery_snapshot(f.context, &request, f.arena, &result, &f.error);
  }
  REQUIRE(f.failed(status) == HOLDER_ERROR_INVALID_ARGUMENT);
  const holder_card* current = nullptr;
  REQUIRE(
      holder_get_card(f.context, f.project_id.c_str(), id.c_str(), f.arena, &current, &f.error) ==
      HOLDER_OK
  );
  REQUIRE(std::string(current->title) == "Original");
  REQUIRE(std::string(current->content) == "Keep this body");
  REQUIRE(current->deleted_at == nullptr);
  const holder_project_list_item_list* projects = nullptr;
  REQUIRE(
      holder_list_projects(
          f.context,
          nullptr,
          nullptr,
          nullptr,
          HOLDER_LIST_PROJECTS_ORDER_NONE,
          nullptr,
          0,
          false,
          f.arena,
          &projects,
          &f.error
      ) == HOLDER_OK
  );
  REQUIRE(projects->count == 1);
}
