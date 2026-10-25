#if __has_include(<catch2/catch_test_macros.hpp>)
#include <catch2/catch_test_macros.hpp>
#else
#include <catch2/catch.hpp>
#endif

#include "api/GeneratedRoutes.h"
#include "http_test_helpers.h"
#include "index/FtsIndexer.h"

#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

#include <string>
#include <utility>

namespace {
namespace http = boost::beast::http;

std::pair<http::status, nlohmann::json> search(holder::platform::Db& db,
                                               holder::index::FtsIndexer* fts,
                                               const std::string& target) {
  http::request<http::string_body> req{http::verb::get, target, 11};
  http::response<http::string_body> res;
  REQUIRE(holder::api::dispatch_generated_routes(req, res, db, nullptr, fts, nullptr, nullptr, {},
                                                 {}));
  return {res.result(), nlohmann::json::parse(res.body())};
}

} // namespace

TEST_CASE("Search reports an invalid FTS query as a bad request", "[search-routes]") {
  const auto dir = holder::test::make_temp_dir();
  holder::platform::Db db = holder::test::open_db_with_schema(dir / "holder.db");
  holder::index::FtsIndexer fts(db);

  for (const auto* path : {"/search/cards", "/search/ai"}) {
    const auto [status, payload] =
        search(db, &fts, std::string(path) + "?project_id=proj-1&q=%22unbalanced");
    CHECK(status == http::status::bad_request);
    CHECK(payload["error"]["code"] == "bad_request");
  }
}

TEST_CASE("Search reports a storage failure as an internal error", "[search-routes]") {
  const auto dir = holder::test::make_temp_dir();
  holder::platform::Db db = holder::test::open_db_with_schema(dir / "holder.db");
  holder::index::FtsIndexer fts(db);
  db.close(); // the search itself fails, not the query

  for (const auto* path : {"/search/cards", "/search/ai"}) {
    const auto [status, payload] =
        search(db, &fts, std::string(path) + "?project_id=proj-1&q=needle&limit=10&offset=0");
    CHECK(status == http::status::internal_server_error);
    CHECK(payload["ok"] == false);
  }
}

TEST_CASE("Search validates its parameters", "[search-routes]") {
  const auto dir = holder::test::make_temp_dir();
  holder::platform::Db db = holder::test::open_db_with_schema(dir / "holder.db");
  holder::index::FtsIndexer fts(db);

  for (const auto* query :
       {"q=needle", "project_id=proj-1", "project_id=&q=needle", "project_id=proj-1&q=",
        "project_id=proj-1&q=needle&limit=0", "project_id=proj-1&q=needle&limit=2junk",
        "project_id=proj-1&q=needle&offset=-1"}) {
    CHECK(search(db, &fts, std::string("/search/cards?") + query).first ==
          http::status::bad_request);
  }
  const auto [status, payload] = search(db, nullptr, "/search/ai?project_id=proj-1&q=needle");
  CHECK(status == http::status::not_implemented);
  CHECK(payload["error"]["code"] == "not_implemented");
}

TEST_CASE("Search routes leave unrelated paths alone", "[search-routes]") {
  const auto dir = holder::test::make_temp_dir();
  holder::platform::Db db = holder::test::open_db_with_schema(dir / "holder.db");
  http::request<http::string_body> req{http::verb::get, "/search/unknown", 11};
  http::response<http::string_body> res;
  CHECK_FALSE(holder::api::dispatch_generated_routes(req, res, db, nullptr, nullptr, nullptr,
                                                     nullptr, {}, {}));
}
