#if __has_include(<catch2/catch_test_macros.hpp>)
#include <catch2/catch_test_macros.hpp>
#else
#include <catch2/catch.hpp>
#endif

#include "api/GeneratedRoutes.h"
#include "card/CardStore.h"
#include "http_test_helpers.h"
#include "index/FtsIndexer.h"

#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

#include <string>
#include <utility>

namespace {

namespace http = boost::beast::http;

struct Routed {
  bool handled;
  http::status status;
  nlohmann::json body;
};

Routed route(holder::platform::Db& db,
             holder::index::FtsIndexer& fts,
             holder::card::CardStore* card_store,
             const std::string& target) {
  http::request<http::string_body> req{http::verb::delete_, target, 11};
  http::response<http::string_body> res;
  if (!holder::api::dispatch_generated_routes(req, res, db, nullptr, &fts, nullptr, nullptr, {},
                                              {}, card_store))
    return {false, {}, {}};
  return {true, res.result(), nlohmann::json::parse(res.body())};
}

} // namespace

TEST_CASE("Trash deletes validate requests and report missing services", "[trash-routes]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  holder::test::create_project(db, "proj-1", (dir / "repo").string());
  holder::index::FtsIndexer fts(db);
  holder::card::CardStore card_store(db, &fts);
  const std::string live = "12121212-1212-4212-8212-121212121212";
  holder::test::create_card_fixture(card_store, live, "proj-1", "Live", "live", 10);

  for (const std::string target : {"/trash", "/trash?project_id=", "/trash?project_id=proj-1&type=x",
                                   "/trash/note/id", "/projects/proj-1/trash/note/id"}) {
    CAPTURE(target);
    const auto rejected = route(db, fts, &card_store, target);
    REQUIRE(rejected.status == http::status::bad_request);
    REQUIRE(rejected.body["error"]["code"] == "invalid_argument");
  }
  // An empty ID matches no route, so the router answers 404.
  REQUIRE_FALSE(route(db, fts, &card_store, "/trash/card/").handled);

  // Core can construct a card store from the request context.
  REQUIRE(route(db, fts, nullptr, "/trash?project_id=proj-1").status == http::status::ok);
  REQUIRE(route(db, fts, nullptr, "/trash?project_id=proj-1&type=ai_message").status ==
          http::status::ok);
  REQUIRE(route(db, fts, nullptr, "/trash/card/" + live).status == http::status::conflict);

  const auto not_trashed = route(db, fts, &card_store, "/projects/proj-1/trash/card/" + live);
  REQUIRE(not_trashed.status == http::status::conflict);
  REQUIRE(not_trashed.body["error"]["code"] == "not_trashed");
  REQUIRE(route(db, fts, &card_store, "/projects/proj-2/trash/card/" + live).status ==
          http::status::not_found);
  REQUIRE(route(db, fts, &card_store, "/trash/ai_message/missing").status ==
          http::status::not_found);

  card_store.trash(live, 20);
  const auto removed = route(db, fts, &card_store, "/trash/card/" + live);
  REQUIRE(removed.status == http::status::ok);
  REQUIRE(removed.body == nlohmann::json{{"ok", true}, {"data", {{"card_id", live}, {"message_id", nullptr}}}});
  const auto emptied = route(db, fts, &card_store, "/trash?project_id=proj-1&type=all");
  REQUIRE(emptied.body == nlohmann::json{{"ok", true}, {"data", {{"project_id", "proj-1"}}}});
}
