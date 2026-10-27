// The contract's Card operations through the generated C API (holder_api.h), against a real data
// directory: cards, their tags, their links and their milestones.
#if __has_include(<catch2/catch_test_macros.hpp>)
#include <catch2/catch_test_macros.hpp>
#else
#include <catch2/catch.hpp>
#endif

#include "core_test_helpers.h"

#include <holder/holder.h>
#include <nlohmann/json.hpp>

#include <fstream>
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
    card.title = title;
    card.content = content;
    card.parent_card_id = parent;
    const holder_card* out = nullptr;
    REQUIRE(holder_create_card(context, project_id.c_str(), &card, arena, &out, &error) == HOLDER_OK);
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
  REQUIRE(holder_card_delete(f.context, card->card_id, &f.error) == HOLDER_OK);
  const holder_card* out = nullptr;
  REQUIRE(f.failed(holder_get_card(f.context, f.project_id.c_str(), card->card_id, f.arena, &out, &f.error)) ==
          HOLDER_ERROR_NOT_FOUND);
  REQUIRE(f.failed(holder_get_card(f.context, f.project_id.c_str(), "no-such-card", f.arena, &out, &f.error)) ==
          HOLDER_ERROR_NOT_FOUND);
  REQUIRE(f.failed(holder_get_card(f.context, f.project_id.c_str(), "", f.arena, &out, &f.error)) ==
          HOLDER_ERROR_INVALID_ARGUMENT);

  const auto* live = f.create("Elsewhere");
  char* json = nullptr;
  REQUIRE(holder_project_create(f.context, "Other", nullptr, nullptr, &json, &f.error) == HOLDER_OK);
  const auto other = nlohmann::json::parse(json).at("project_id").get<std::string>();
  holder_string_free(json);
  REQUIRE(f.failed(holder_get_card(f.context, other.c_str(), live->card_id, f.arena, &out, &f.error)) ==
          HOLDER_ERROR_NOT_FOUND);
  REQUIRE(f.failed(holder_get_card(f.context, "no-such-project", live->card_id, f.arena, &out, &f.error)) ==
          HOLDER_ERROR_NOT_FOUND);
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
  REQUIRE(holder_card_delete(f.context, trashed->card_id, &f.error) == HOLDER_OK);

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

std::string new_project(Fixture& f, const char* name) {
  char* json = nullptr;
  REQUIRE(holder_project_create(f.context, name, nullptr, nullptr, &json, &f.error) == HOLDER_OK);
  auto id = nlohmann::json::parse(json).at("project_id").get<std::string>();
  holder_string_free(json);
  return id;
}

holder_new_link card_link(const char* target, const char* kind = "ref", const char* project = nullptr) {
  holder_new_link link{};
  link.target_type = HOLDER_NEW_LINK_TARGET_TYPE_CARD;
  link.target_id = target;
  link.kind = kind;
  link.target_project_id = project;
  return link;
}

} // namespace

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
  REQUIRE(holder_card_delete(f.context, to.c_str(), &f.error) == HOLDER_OK);
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
  const auto other = new_project(f, "Other");
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
  char* json = nullptr;
  REQUIRE(holder_project_create(f.context, "Other", nullptr, nullptr, &json, &f.error) == HOLDER_OK);
  const auto other = nlohmann::json::parse(json).at("project_id").get<std::string>();
  holder_string_free(json);
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

  // A window of October where the caller is: its days, and its instants for card activity and timed milestones.
  const holder_calendar* calendar = nullptr;
  REQUIRE(holder_get_calendar(f.context, project, 0, std::numeric_limits<int64_t>::max(), "2026-10-01",
                              "2026-10-31", f.arena, &calendar, &f.error) == HOLDER_OK);
  std::map<std::string, std::string> titles;
  for (std::size_t i = 0; i < calendar->milestones.count; ++i)
    titles[calendar->milestones.items[i].milestone_id] = calendar->milestones.items[i].card_title;
  REQUIRE(titles == std::map<std::string, std::string>{{on_day, "First"}, {at_instant, "Second"}});
  REQUIRE(calendar->created_cards.count == 2);
  const auto& made = calendar->created_cards.items;
  REQUIRE(std::tuple(made[0].created_at, std::string(made[0].card_id)) <
          std::tuple(made[1].created_at, std::string(made[1].card_id)));

  // Only the days: the timed milestone and the cards fall outside the instants.
  REQUIRE(holder_get_calendar(f.context, project, 0, 0, "2026-10-01", "2026-10-31", f.arena, &calendar, &f.error) ==
          HOLDER_OK);
  REQUIRE(calendar->milestones.count == 1);
  REQUIRE(calendar->created_cards.count == 0);
  REQUIRE(calendar->updated_cards.count == 0);

  // Milestones on a card in the trash are left out.
  const holder_card* trashed = nullptr;
  REQUIRE(holder_trash_card(f.context, project, first.c_str(), HOLDER_TRASH_CARD_CHILDREN_DEFAULT, f.arena, &trashed,
                            &f.error) == HOLDER_OK);
  REQUIRE(holder_get_calendar(f.context, project, 0, std::numeric_limits<int64_t>::max(), "2026-10-01",
                              "2026-10-31", f.arena, &calendar, &f.error) == HOLDER_OK);
  REQUIRE(calendar->milestones.count == 1);
  REQUIRE(calendar->created_cards.count == 1);
  REQUIRE(f.failed(holder_get_calendar(f.context, project, 0, 1, "2026-10-01", "October", f.arena, &calendar,
                                       &f.error)) == HOLDER_ERROR_INVALID_ARGUMENT);
}
