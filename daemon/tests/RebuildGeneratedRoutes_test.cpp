#include "api/GeneratedRoutes.h"
#include "http_test_helpers.h"

#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace http = boost::beast::http;

TEST_CASE("Generated rebuild route decodes its required body", "[http][rebuild]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  http::response<http::string_body> res;

  const auto dispatch = [&](http::verb method, const std::string& target,
                            const std::string& body) {
    http::request<http::string_body> req{method, target, 11};
    req.body() = body;
    req.prepare_payload();
    return holder::api::dispatch_generated_routes(req, res, db);
  };

  REQUIRE_FALSE(dispatch(http::verb::get, "/rebuild", "{}"));
  REQUIRE(dispatch(http::verb::post, "/rebuild", "{}"));
  REQUIRE(res.result() == http::status::bad_request);
  REQUIRE(nlohmann::json::parse(res.body())["error"]["code"] == "bad_request");

  REQUIRE(dispatch(http::verb::post, "/rebuild", "{"));
  REQUIRE(res.result() == http::status::bad_request);

  REQUIRE(dispatch(http::verb::post, "/rebuild", R"({"project_id":7})"));
  REQUIRE(res.result() == http::status::bad_request);

  REQUIRE(dispatch(http::verb::post, "/rebuild", R"({"project_id":"missing"})"));
  REQUIRE(res.result() == http::status::not_found);
  REQUIRE(nlohmann::json::parse(res.body())["error"]["message"] == "Project not found.");
}
