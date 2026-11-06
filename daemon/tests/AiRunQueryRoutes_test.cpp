#if __has_include(<catch2/catch_test_macros.hpp>)
#include <catch2/catch_test_macros.hpp>
#else
#include <catch2/catch.hpp>
#endif

#include "ai/AiRunRepo.h"
#include "api/GeneratedRoutes.h"
#include "http_test_helpers.h"

#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

#include <string>
#include <utility>

namespace {

namespace http = boost::beast::http;

std::pair<http::status, nlohmann::json> get(holder::platform::Db& db, const std::string& target) {
  http::request<http::string_body> req{http::verb::get, target, 11};
  http::response<http::string_body> res;
  REQUIRE(holder::api::dispatch_generated_routes(req, res, db));
  return {res.result(), nlohmann::json::parse(res.body())};
}

holder::model::AiRun make_run(const std::string& run_id) {
  holder::model::AiRun run;
  run.run_id = run_id;
  run.project_id = "proj-1";
  run.mode = "auto";
  run.prompt = "hello";
  run.status = "completed";
  run.created_at = 1;
  run.updated_at = 2;
  return run;
}

} // namespace

TEST_CASE("Generated AI run listing validates its scope", "[http]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");

  for (const std::string target :
       {"/ai/runs", "/ai/runs?project_id=", "/ai/runs?thread_id=", "/ai/runs?project_id=&thread_id="}) {
    CAPTURE(target);
    const auto [status, payload] = get(db, target);
    REQUIRE(status == http::status::bad_request);
    REQUIRE(payload["error"]["code"] == "bad_request");
    REQUIRE(payload["error"]["message"] == "Missing project_id or thread_id.");
  }

  db.exec("INSERT INTO projects(project_id, name, root_path, created_at, updated_at) "
          "VALUES('proj-1', 'Project', '/tmp/project', 1, 1);");
  db.exec("INSERT INTO ai_threads(thread_id, project_id, title, created_at, updated_at) "
          "VALUES('thread-1', 'proj-1', 'Thread', 1, 1);");
  holder::ai::AiRunRepo repo(db);
  auto run = make_run("run-1");
  run.router_model = "auto-local::qwen-fast";
  run.chosen_model = "auto-local::qwen-strong";
  repo.create(run);
  auto threaded = make_run("run-2");
  threaded.thread_id = "thread-1";
  repo.create(threaded);

  const auto [status, listed] = get(db, "/ai/runs?project_id=proj-1");
  REQUIRE(status == http::status::ok);
  REQUIRE(listed["ok"] == true);
  REQUIRE(listed["data"].size() == 2);
  const auto& first = listed["data"][0]["run_id"] == "run-1" ? listed["data"][0] : listed["data"][1];
  REQUIRE(first["router_runner_id"] == "auto-local");
  REQUIRE(first["chosen_runner_id"] == "auto-local");
  REQUIRE(first["thread_id"].is_null());
  REQUIRE(first["error"].is_null());
  REQUIRE(first.size() == 18);

  // thread_id wins when both are given.
  const auto [thread_status, by_thread] =
      get(db, "/ai/runs?project_id=proj-1&thread_id=thread-1");
  REQUIRE(thread_status == http::status::ok);
  REQUIRE(by_thread["data"].size() == 1);
  REQUIRE(by_thread["data"][0]["run_id"] == "run-2");
  REQUIRE(by_thread["data"][0]["router_runner_id"].is_null());
}

TEST_CASE("Generated AI run fetch handles fallback policy parsing and not_found", "[http]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  db.exec("INSERT INTO projects(project_id, name, root_path, created_at, updated_at) "
          "VALUES('proj-1', 'Project', '/tmp/project', 1, 1);");

  holder::ai::AiRunRepo repo(db);
  auto legacy = make_run("run-legacy");
  legacy.ranked_json = std::string("{\"path\":\"cloud\",\"attempts\":[]}");
  repo.create(legacy);
  auto malformed = make_run("run-malformed");
  malformed.policy_trace_json = std::string("{");
  malformed.ranked_json = std::string("[1,2,3]");
  malformed.status = "failed";
  repo.create(malformed);

  const auto [legacy_status, legacy_payload] = get(db, "/ai/runs/run-legacy");
  REQUIRE(legacy_status == http::status::ok);
  REQUIRE(legacy_payload["data"]["policy_trace"].is_object());
  REQUIRE(legacy_payload["data"]["policy_trace"]["path"] == "cloud");
  REQUIRE(legacy_payload["data"]["ranked_json"] == "{\"path\":\"cloud\",\"attempts\":[]}");

  const auto [malformed_status, malformed_payload] = get(db, "/ai/runs/run-malformed");
  REQUIRE(malformed_status == http::status::ok);
  REQUIRE(malformed_payload["data"]["policy_trace"].is_null());
  REQUIRE(malformed_payload["data"]["policy_trace_json"] == "{");

  const auto [missing_status, missing_payload] = get(db, "/ai/runs/does-not-exist");
  REQUIRE(missing_status == http::status::not_found);
  REQUIRE(missing_payload["error"]["message"] == "Run not found.");

  http::request<http::string_body> empty_id{http::verb::get, "/ai/runs/", 11};
  http::response<http::string_body> res;
  REQUIRE_FALSE(holder::api::dispatch_generated_routes(empty_id, res, db));
}

TEST_CASE("Generated AI run reads report storage failures as internal errors", "[http]") {
  holder::platform::Db unopened_db;
  REQUIRE(get(unopened_db, "/ai/runs?project_id=proj-1").first ==
          http::status::internal_server_error);
  REQUIRE(get(unopened_db, "/ai/runs/run-1").first == http::status::internal_server_error);
}
