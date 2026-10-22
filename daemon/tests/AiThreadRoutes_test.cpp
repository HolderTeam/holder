#if __has_include(<catch2/catch_test_macros.hpp>)
#include <catch2/catch_test_macros.hpp>
#else
#include <catch2/catch.hpp>
#endif

#include "api/GeneratedRoutes.h"
#include "card/CardRepo.h"
#include "http_test_helpers.h"
#include "model/Card.h"

#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

#include <functional>
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
             http::verb method,
             const std::string& target,
             const std::string& body = "",
             std::function<std::string()> uuid_v4 = [] { return std::string("generated-id"); }) {
  http::request<http::string_body> req{method, target, 11};
  req.body() = body;
  http::response<http::string_body> res;
  if (!holder::api::dispatch_generated_routes(req, res, db, nullptr, nullptr, nullptr, nullptr,
                                              std::move(uuid_v4), {})) {
    return {false, {}, {}};
  }
  return {true, res.result(), nlohmann::json::parse(res.body())};
}

void create_card(holder::platform::Db& db, const std::string& card_id) {
  holder::model::Card card;
  card.card_id = card_id;
  card.project_id = "proj-1";
  card.title = "Linked";
  card.rel_path = "cards/" + card_id + ".md";
  card.created_at = 5;
  card.updated_at = 5;
  holder::card::CardRepo(db).create(card);
}

} // namespace

TEST_CASE("AI thread reads validate and report failures", "[http][ai][threads]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");

  for (const std::string target : {"/ai/threads", "/ai/threads?project_id="}) {
    CAPTURE(target);
    const auto listed = route(db, http::verb::get, target);
    REQUIRE(listed.status == http::status::bad_request);
    REQUIRE(listed.body["error"]["code"] == "bad_request");
  }
  const auto empty = route(db, http::verb::get, "/ai/threads?project_id=missing");
  REQUIRE(empty.status == http::status::ok);
  REQUIRE(empty.body == nlohmann::json{{"ok", true}, {"data", nlohmann::json::array()}});

  const auto missing = route(db, http::verb::get, "/ai/threads/missing");
  REQUIRE(missing.status == http::status::not_found);
  REQUIRE(missing.body["error"]["message"] == "AI thread not found.");

  // Unrelated and empty-ID paths match no route, so the router answers 404.
  REQUIRE_FALSE(route(db, http::verb::get, "/ai/threadz").handled);
  REQUIRE_FALSE(route(db, http::verb::get, "/ai/threads/").handled);
  // A thread path with a method it does not have is a 405.
  const auto wrong = route(db, http::verb::post, "/ai/threads/thread-1", R"({"title":"x"})");
  REQUIRE(wrong.status == http::status::method_not_allowed);
  REQUIRE(wrong.body["error"]["code"] == "method_not_allowed");

  db.exec("DROP TABLE ai_threads;");
  REQUIRE(route(db, http::verb::get, "/ai/threads?project_id=proj-1").status ==
          http::status::internal_server_error);
  REQUIRE(route(db, http::verb::get, "/ai/threads/thread-1").status ==
          http::status::internal_server_error);
}

TEST_CASE("AI thread create defaults, validates and reports conflicts", "[http][ai][threads]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  holder::test::create_project(db, "proj-1", (dir / "project").string());
  create_card(db, "card-1");

  const auto created = route(db, http::verb::post, "/ai/threads",
                             R"({"project_id":"proj-1","title":"T","card_id":"card-1"})");
  REQUIRE(created.status == http::status::created);
  REQUIRE(created.body["data"]["thread_id"] == "generated-id");
  const auto unlinked = route(db, http::verb::post, "/ai/threads",
                              R"({"project_id":"proj-1","title":"U","thread_id":"thread-unlinked"})");
  REQUIRE(unlinked.status == http::status::created);

  const auto listed = route(db, http::verb::get, "/ai/threads?project_id=proj-1");
  REQUIRE(listed.body["data"].size() == 2);
  for (const auto& thread : listed.body["data"]) {
    REQUIRE(thread.size() == 6);
    REQUIRE(thread["created_at"].get<long long>() > 0);
    REQUIRE(thread["updated_at"] == thread["created_at"]);
    if (thread["thread_id"] == "thread-unlinked") REQUIRE(thread["card_id"].is_null());
    if (thread["thread_id"] == "generated-id") REQUIRE(thread["card_id"] == "card-1");
  }

  const auto conflict = route(db, http::verb::post, "/ai/threads",
                              R"({"project_id":"proj-1","title":"A","thread_id":"thread-unlinked"})");
  REQUIRE(conflict.status == http::status::conflict);
  REQUIRE(conflict.body["error"]["code"] == "conflict");

  for (const std::string body :
       {R"({"project_id":"proj-1"})", "not-json", R"({"project_id":"missing","title":"A"})",
        R"({"project_id":"proj-1","title":"A","card_id":"missing"})"}) {
    CAPTURE(body);
    REQUIRE(route(db, http::verb::post, "/ai/threads", body).status == http::status::bad_request);
  }

  const auto failed = route(db, http::verb::post, "/ai/threads",
                            R"({"project_id":"proj-1","title":"A"})",
                            []() -> std::string { throw std::runtime_error("synthetic"); });
  REQUIRE(failed.status == http::status::internal_server_error);
}

TEST_CASE("AI thread update and delete", "[http][ai][threads]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  holder::test::create_project(db, "proj-1", (dir / "project").string());
  create_card(db, "card-1");
  db.exec("INSERT INTO ai_threads(thread_id, project_id, title, card_id, created_at, updated_at) "
          "VALUES('thread-1', 'proj-1', 'T', NULL, 1, 1);");
  const auto read = [&] { return route(db, http::verb::get, "/ai/threads/thread-1").body["data"]; };

  REQUIRE(route(db, http::verb::patch, "/ai/threads/thread-1", R"({"title":"X"})").status ==
          http::status::bad_request);
  REQUIRE(route(db, http::verb::patch, "/ai/threads/thread-1", "nope").status ==
          http::status::bad_request);
  REQUIRE(route(db, http::verb::patch, "/ai/threads/missing", R"({"updated_at":2})").status ==
          http::status::not_found);
  REQUIRE(route(db, http::verb::patch, "/ai/threads/thread-1",
                R"({"updated_at":2,"card_id":"missing"})")
              .status == http::status::bad_request);

  // Without a title only updated_at changes; null leaves the title as it is.
  REQUIRE(route(db, http::verb::patch, "/ai/threads/thread-1", R"({"updated_at":99,"title":null})")
              .status == http::status::ok);
  REQUIRE(read()["updated_at"] == 99);
  REQUIRE(read()["title"] == "T");

  REQUIRE(route(db, http::verb::patch, "/ai/threads/thread-1",
                R"({"updated_at":100,"title":"Renamed","card_id":"card-1"})")
              .status == http::status::ok);
  REQUIRE(read()["title"] == "Renamed");
  REQUIRE(read()["card_id"] == "card-1");
  REQUIRE(route(db, http::verb::patch, "/ai/threads/thread-1", R"({"updated_at":101,"card_id":null})")
              .status == http::status::ok);
  REQUIRE(read()["card_id"].is_null());

  const auto missing = route(db, http::verb::delete_, "/ai/threads/missing");
  REQUIRE(missing.status == http::status::not_found);
  REQUIRE(missing.body["error"]["message"] == "AI thread not found.");
  REQUIRE(route(db, http::verb::delete_, "/ai/threads/thread-1").status == http::status::ok);
  REQUIRE(route(db, http::verb::get, "/ai/threads/thread-1").status == http::status::not_found);

  db.exec("DROP TABLE ai_threads;");
  REQUIRE(route(db, http::verb::delete_, "/ai/threads/thread-1").status ==
          http::status::internal_server_error);
}
