#include "api/GeneratedRoutes.h"
#include "http_test_helpers.h"

#include <boost/beast/http.hpp>

namespace {

namespace http = boost::beast::http;

std::pair<http::status, nlohmann::json> reindex(holder::platform::Db& db, http::verb method) {
  http::request<http::string_body> req{method, "/reindex", 11};
  http::response<http::string_body> res;
  REQUIRE(holder::api::dispatch_generated_routes(req, res, db));
  return {res.result(), nlohmann::json::parse(res.body())};
}

} // namespace

TEST_CASE("Reindex answers other methods with 405", "[http][reindex]") {
  holder::platform::Db db;
  REQUIRE(reindex(db, http::verb::get).first == http::status::method_not_allowed);
}

TEST_CASE("Reindex runs full DB reindex", "[http][reindex]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  const auto [status, payload] = reindex(db, http::verb::post);
  REQUIRE(status == http::status::ok);
  REQUIRE(payload == nlohmann::json{{"ok", true}, {"data", {{"message", "Reindex complete."}}}});
}

TEST_CASE("Reindex reports DB errors as internal errors", "[http][reindex]") {
  holder::platform::Db db;
  const auto [status, payload] = reindex(db, http::verb::post);
  REQUIRE(status == http::status::internal_server_error);
  REQUIRE(payload["ok"] == false);
}
