// The contract's Card operations through the generated C API (holder_api.h), against a real data
// directory: holder_get_card, holder_list_cards and holder_create_card.
#if __has_include(<catch2/catch_test_macros.hpp>)
#include <catch2/catch_test_macros.hpp>
#else
#include <catch2/catch.hpp>
#endif

#include "core_test_helpers.h"

#include <holder/holder.h>
#include <nlohmann/json.hpp>

#include <fstream>
#include <set>
#include <string>

namespace {

std::string read_schema_sql() {
  std::ifstream in(SCHEMA_SQL_PATH);
  REQUIRE(in.is_open());
  return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

// A context on a fresh data directory with one plain project, freed with its arena.
struct Fixture {
  holder_context* context = nullptr;
  holder_arena* arena = holder_arena_new();
  holder_error* error = nullptr;
  std::string project_id;

  Fixture() {
    const auto data_dir = holder::test::make_temp_dir();
    const auto schema = read_schema_sql();
    REQUIRE(holder_context_open(data_dir.string().c_str(), schema.c_str(), &context, &error) == HOLDER_OK);
    char* json = nullptr;
    REQUIRE(holder_project_create(context, "Cards", nullptr, nullptr, &json, &error) == HOLDER_OK);
    project_id = nlohmann::json::parse(json).at("project_id").get<std::string>();
    holder_string_free(json);
  }
  ~Fixture() {
    holder_error_destroy(error);
    holder_arena_free(arena);
    holder_context_destroy(context);
  }

  const holder_card* create(const char* title, const char* content = nullptr, const char* parent = nullptr) {
    holder_new_card card{};
    card.project_id = project_id.c_str();
    card.title = title;
    card.content = content;
    card.parent_card_id = parent;
    const holder_card* out = nullptr;
    REQUIRE(holder_create_card(context, &card, arena, &out, &error) == HOLDER_OK);
    return out;
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
  REQUIRE(holder_get_card(f.context, card->card_id, f.arena, &fetched, &f.error) == HOLDER_OK);
  REQUIRE(std::string(fetched->card_id) == card->card_id);
  REQUIRE(std::string(fetched->content) == "Intro #draft");
}

TEST_CASE("holder_create_card honours the optional fields", "[capi][contract]") {
  Fixture f;
  const auto* parent = f.create("Parent");
  holder_new_card card{};
  card.project_id = f.project_id.c_str();
  card.title = "Imported";
  card.parent_card_id = parent->card_id;
  const double sort_key = 7.5;
  const int64_t created_at = 1700000000;
  const int64_t updated_at = 1700000100;
  card.sort_key = &sort_key;
  card.created_at = &created_at;
  card.updated_at = &updated_at;
  const holder_card* out = nullptr;
  REQUIRE(holder_create_card(f.context, &card, f.arena, &out, &f.error) == HOLDER_OK);
  REQUIRE(std::string(out->parent_card_id) == parent->card_id);
  REQUIRE(out->sort_key == 7.5);
  REQUIRE(out->created_at == created_at);
  REQUIRE(out->updated_at == updated_at);
  REQUIRE(std::string(out->content).empty());
}

TEST_CASE("holder_create_card reports missing projects, missing parents and bad arguments", "[capi][contract]") {
  Fixture f;
  holder_new_card card{};
  card.project_id = "no-such-project";
  card.title = "Lost";
  const holder_card* out = nullptr;
  REQUIRE(f.failed(holder_create_card(f.context, &card, f.arena, &out, &f.error)) == HOLDER_ERROR_NOT_FOUND);
  REQUIRE(out == nullptr);

  card.project_id = f.project_id.c_str();
  card.parent_card_id = "no-such-card";
  REQUIRE(f.failed(holder_create_card(f.context, &card, f.arena, &out, &f.error)) == HOLDER_ERROR_NOT_FOUND);

  card.parent_card_id = nullptr;
  card.title = "";
  REQUIRE(f.failed(holder_create_card(f.context, &card, f.arena, &out, &f.error)) ==
          HOLDER_ERROR_INVALID_ARGUMENT);
  REQUIRE(f.failed(holder_create_card(f.context, nullptr, f.arena, &out, &f.error)) ==
          HOLDER_ERROR_INVALID_ARGUMENT);
  card.title = "Fine";
  REQUIRE(f.failed(holder_create_card(f.context, &card, nullptr, &out, &f.error)) == HOLDER_ERROR_INVALID_ARGUMENT);
}

TEST_CASE("holder_get_card does not find trashed or unknown cards", "[capi][contract]") {
  Fixture f;
  const auto* card = f.create("Binned");
  REQUIRE(holder_card_delete(f.context, card->card_id, &f.error) == HOLDER_OK);
  const holder_card* out = nullptr;
  REQUIRE(f.failed(holder_get_card(f.context, card->card_id, f.arena, &out, &f.error)) == HOLDER_ERROR_NOT_FOUND);
  REQUIRE(f.failed(holder_get_card(f.context, "no-such-card", f.arena, &out, &f.error)) == HOLDER_ERROR_NOT_FOUND);
  REQUIRE(f.failed(holder_get_card(f.context, "", f.arena, &out, &f.error)) == HOLDER_ERROR_INVALID_ARGUMENT);
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
