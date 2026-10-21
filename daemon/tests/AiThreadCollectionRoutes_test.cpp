#if __has_include(<catch2/catch_test_macros.hpp>)
#include <catch2/catch_test_macros.hpp>
#else
#include <catch2/catch.hpp>
#endif

#include "api/GeneratedRoutes.h"
#include "api/routes/ai/threads/AiThreadCollectionRoutes.h"
#include "card/CardRepo.h"
#include "http_test_helpers.h"
#include "model/Card.h"

#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace {

namespace http = boost::beast::http;

http::request<http::string_body> make_request(
    http::verb method,
    const std::string& target,
    const std::string& body = ""
) {
  http::request<http::string_body> req{method, target, 11};
  req.set(http::field::host, "127.0.0.1");
  if (!body.empty()) {
    req.set(http::field::content_type, "application/json");
    req.body() = body;
    req.prepare_payload();
  }
  return req;
}

std::pair<http::status, nlohmann::json> list_threads(holder::platform::Db& db,
                                                     const std::string& target) {
  auto req = make_request(http::verb::get, target);
  http::response<http::string_body> res;
  REQUIRE(holder::api::dispatch_generated_routes(req, res, db));
  return {res.result(), nlohmann::json::parse(res.body())};
}

} // namespace

TEST_CASE("AiThreadCollectionRoutes guards and GET error handling", "[http]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");

  const auto uuid_v4 = []() -> std::string {
    return "unused";
  };
  const auto param_get = [](const std::string&) -> std::string {
    return {};
  };

  SECTION("non matching route returns false") {
    auto req = make_request(http::verb::get, "/ai/threadz");
    http::response<http::string_body> res;
    REQUIRE_FALSE(holder::api::routes::ai::threads::handle_ai_thread_collection_routes(
        "/ai/threadz",
        req,
        res,
        db,
        uuid_v4,
        param_get
    ));
  }

  SECTION("GET is generated and rejects a missing or empty project_id") {
    auto req = make_request(http::verb::get, "/ai/threads");
    http::response<http::string_body> res;
    REQUIRE_FALSE(holder::api::routes::ai::threads::handle_ai_thread_collection_routes(
        "/ai/threads",
        req,
        res,
        db,
        uuid_v4,
        param_get
    ));
    for (const std::string target : {"/ai/threads", "/ai/threads?project_id="}) {
      CAPTURE(target);
      const auto [status, payload] = list_threads(db, target);
      REQUIRE(status == http::status::bad_request);
      REQUIRE(payload["error"]["code"] == "bad_request");
    }
  }

  SECTION("GET lists nothing for an unknown project") {
    const auto [status, payload] = list_threads(db, "/ai/threads?project_id=missing");
    REQUIRE(status == http::status::ok);
    REQUIRE(payload == nlohmann::json{{"ok", true}, {"data", nlohmann::json::array()}});
  }

  SECTION("GET storage failure is an internal error") {
    db.exec("DROP TABLE ai_threads;");
    const auto [status, payload] = list_threads(db, "/ai/threads?project_id=proj-1");
    REQUIRE(status == http::status::internal_server_error);
    REQUIRE(payload["ok"] == false);
  }
}

TEST_CASE("AiThreadCollectionRoutes POST branch coverage", "[http]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  holder::test::create_project(db, "proj-1", (dir / "project").string());
  holder::card::CardRepo card_repo(db);
  holder::model::Card card;
  card.card_id = "card-1";
  card.project_id = "proj-1";
  card.title = "Linked";
  card.rel_path = "cards/c1.md";
  card.sort_key = 0.0;
  card.created_at = 5;
  card.updated_at = 5;
  card_repo.create(card);

  const auto param_get = [](const std::string&) -> std::string {
    return {};
  };

  SECTION("POST missing required fields") {
    auto req = make_request(http::verb::post, "/ai/threads", R"({"project_id":"proj-1"})");
    http::response<http::string_body> res;
    REQUIRE(holder::api::routes::ai::threads::handle_ai_thread_collection_routes(
        "/ai/threads",
        req,
        res,
        db,
        []() {
          return "unused";
        },
        param_get
    ));
    REQUIRE(res.result() == http::status::bad_request);
    const auto payload = nlohmann::json::parse(res.body());
    REQUIRE(payload["error"]["message"] == "Missing required fields.");
  }

  SECTION("POST generates thread_id and defaults timestamps and optional card_id") {
    auto req = make_request(
        http::verb::post,
        "/ai/threads",
        R"({"project_id":"proj-1","title":"T","card_id":"card-1"})"
    );
    http::response<http::string_body> res;
    REQUIRE(holder::api::routes::ai::threads::handle_ai_thread_collection_routes(
        "/ai/threads",
        req,
        res,
        db,
        []() {
          return "generated-id";
        },
        param_get
    ));
    REQUIRE(res.result() == http::status::created);
    const auto payload = nlohmann::json::parse(res.body());
    REQUIRE(payload["data"]["thread_id"] == "generated-id");

    const auto [get_status, listed] = list_threads(db, "/ai/threads?project_id=proj-1");
    REQUIRE(get_status == http::status::ok);
    REQUIRE(listed["data"].is_array());
    REQUIRE(listed["data"].size() == 1);
    REQUIRE(listed["data"][0]["thread_id"] == "generated-id");
    REQUIRE(listed["data"][0]["project_id"] == "proj-1");
    REQUIRE(listed["data"][0]["title"] == "T");
    REQUIRE(listed["data"][0]["card_id"] == "card-1");
    REQUIRE(listed["data"][0]["created_at"].get<long long>() > 0);
    REQUIRE(listed["data"][0]["updated_at"] == listed["data"][0]["created_at"]);

    auto unlinked = make_request(
        http::verb::post,
        "/ai/threads",
        R"({"project_id":"proj-1","title":"U","thread_id":"thread-unlinked"})"
    );
    http::response<http::string_body> unlinked_res;
    REQUIRE(holder::api::routes::ai::threads::handle_ai_thread_collection_routes(
        "/ai/threads", unlinked, unlinked_res, db, [] { return "unused"; }, param_get
    ));
    REQUIRE(unlinked_res.result() == http::status::created);
    const auto [both_status, both] = list_threads(db, "/ai/threads?project_id=proj-1");
    REQUIRE(both_status == http::status::ok);
    REQUIRE(both["data"].size() == 2);
    for (const auto& thread : both["data"]) {
      REQUIRE(thread.size() == 6);
      if (thread["thread_id"] == "thread-unlinked") REQUIRE(thread["card_id"].is_null());
    }
  }

  SECTION("POST maps conflict-prefixed exceptions to conflict response") {
    auto req =
        make_request(http::verb::post, "/ai/threads", R"({"project_id":"proj-1","title":"A"})");
    http::response<http::string_body> res;
    REQUIRE(holder::api::routes::ai::threads::handle_ai_thread_collection_routes(
        "/ai/threads",
        req,
        res,
        db,
        []() -> std::string {
          throw std::runtime_error("conflict: synthetic");
        },
        param_get
    ));
    REQUIRE(res.result() == http::status::conflict);
    const auto payload = nlohmann::json::parse(res.body());
    REQUIRE(payload["error"]["code"] == "conflict");
  }

  SECTION("POST bad json maps to bad_request catch path") {
    auto req = make_request(http::verb::post, "/ai/threads", "not-json");
    http::response<http::string_body> res;
    REQUIRE(holder::api::routes::ai::threads::handle_ai_thread_collection_routes(
        "/ai/threads",
        req,
        res,
        db,
        []() {
          return "unused";
        },
        param_get
    ));
    REQUIRE(res.result() == http::status::bad_request);
  }
}
