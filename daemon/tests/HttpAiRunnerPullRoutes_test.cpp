#include "ai/AiRunnerRepo.h"
#include "api/GeneratedRoutes.h"
#include "api/routes/ai/runner/AiRunnerPullRoutes.h"
#include "http_test_helpers.h"
#include "llm/LocalModelRunner.h"
#include "llm/LocalRunnerClient.h"

#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace {

namespace http = boost::beast::http;

holder::api::routes::RunnerRouteDispatchResult call_pull_route(
    const std::string& path,
    http::verb method,
    const std::string& body,
    holder::llm::RunnerRegistry* runner_registry,
    http::response<http::string_body>& res
) {
  http::request<http::string_body> req{method, path, 11};
  req.set(http::field::host, "127.0.0.1");
  if (!body.empty()) {
    req.set(http::field::content_type, "application/json");
    req.body() = body;
  }
  const auto param_get = [](const std::string&) -> std::string {
    return {};
  };
  return holder::api::routes::ai::runner::handle_ai_runner_pull_routes(
      path,
      req,
      res,
      runner_registry,
      param_get
  );
}

bool call_generated_pull_route(
    const std::string& target,
    holder::platform::Db& db,
    holder::llm::RunnerRegistry* registry,
    http::response<http::string_body>& res
) {
  http::request<http::string_body> req{http::verb::get, target, 11};
  return holder::api::dispatch_generated_routes(req, res, db, registry);
}

} // namespace

TEST_CASE("Ai runner pull routes ignore unmatched paths", "[http]") {
  http::response<http::string_body> res;
  auto out = call_pull_route(
      "/ai/runner/unknown",
      http::verb::get,
      "",
      static_cast<holder::llm::RunnerRegistry*>(nullptr),
      res
  );
  REQUIRE_FALSE(out.handled);
  REQUIRE_FALSE(out.streamed);
}

TEST_CASE("Ai runner pull routes POST returns unavailable when runner not ready", "[http]") {
  holder::llm::LocalModelRunner runner;
  holder::llm::LocalRunnerClient local_runner_client(&runner);
  holder::llm::RunnerRegistry runner_registry(nullptr, &local_runner_client);
  http::response<http::string_body> res;

  auto out = call_pull_route(
      "/ai/runner/pull",
      http::verb::post,
      R"({"model":"fake-echo"})",
      &runner_registry,
      res
  );
  REQUIRE(out.handled);
  REQUIRE_FALSE(out.streamed);
  REQUIRE(res.result() == http::status::service_unavailable);
  REQUIRE(res.body().find("runner_unavailable") != std::string::npos);
}

TEST_CASE("Ai runner pull routes POST validates request body", "[http]") {
  holder::llm::LocalModelRunner runner;
  runner.set_fake_mode(true);
  (void)runner.retry();
  holder::llm::LocalRunnerClient local_runner_client(&runner);
  holder::llm::RunnerRegistry runner_registry(nullptr, &local_runner_client);

  SECTION("missing model") {
    http::response<http::string_body> res;
    auto out =
        call_pull_route("/ai/runner/pull", http::verb::post, R"({"x":"y"})", &runner_registry, res);
    REQUIRE(out.handled);
    REQUIRE(res.result() == http::status::bad_request);
    REQUIRE(res.body().find("Missing model.") != std::string::npos);
  }

  SECTION("invalid JSON catches exception") {
    http::response<http::string_body> res;
    auto out = call_pull_route("/ai/runner/pull", http::verb::post, "{", &runner_registry, res);
    REQUIRE(out.handled);
    REQUIRE(res.result() == http::status::bad_request);
    REQUIRE(res.body().find("bad_request") != std::string::npos);
  }
}

TEST_CASE("Ai runner pull routes POST returns error when pull start fails", "[http]") {
  holder::llm::LocalModelRunner runner;
  runner.set_fake_mode(true);
  (void)runner.retry();
  holder::llm::LocalRunnerClient local_runner_client(&runner);
  holder::llm::RunnerRegistry runner_registry(nullptr, &local_runner_client);
  http::response<http::string_body> res;

  auto out = call_pull_route(
      "/ai/runner/pull",
      http::verb::post,
      R"({"model":""})",
      &runner_registry,
      res
  );
  REQUIRE(out.handled);
  REQUIRE(res.result() == http::status::bad_request);
  REQUIRE(res.body().find("missing model") != std::string::npos);
}

TEST_CASE("Ai runner pull routes POST returns job on success", "[http]") {
  holder::llm::LocalModelRunner runner;
  runner.set_fake_mode(true);
  (void)runner.retry();
  holder::llm::LocalRunnerClient local_runner_client(&runner);
  holder::llm::RunnerRegistry runner_registry(nullptr, &local_runner_client);
  http::response<http::string_body> res;

  auto out = call_pull_route(
      "/ai/runner/pull",
      http::verb::post,
      R"({"model":"fake-echo"})",
      &runner_registry,
      res
  );
  REQUIRE(out.handled);
  REQUIRE(res.result() == http::status::ok);

  const auto payload = nlohmann::json::parse(res.body());
  REQUIRE(payload["ok"] == true);
  REQUIRE(payload["data"]["runner_id"] == "auto-local");
  REQUIRE(payload["data"]["model"] == "fake-echo");
  REQUIRE(payload["data"]["model_ref"] == "auto-local::fake-echo");
  REQUIRE(payload["data"]["status"].is_string());
  REQUIRE(payload["data"]["job_id"].is_string());
  REQUIRE(payload["data"]["job_id"].get<std::string>().rfind("pull-", 0) == 0);
}

TEST_CASE("Generated ai runner pull GET validates job lookup", "[http]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  holder::llm::LocalModelRunner runner;
  runner.set_fake_mode(true);
  holder::llm::LocalRunnerClient local_runner_client(&runner);
  holder::llm::RunnerRegistry runner_registry(nullptr, &local_runner_client);

  SECTION("empty job id") {
    http::response<http::string_body> res;
    REQUIRE_FALSE(call_generated_pull_route("/ai/runner/pull/", db, &runner_registry, res));
  }

  SECTION("missing job id") {
    http::response<http::string_body> res;
    REQUIRE(call_generated_pull_route("/ai/runner/pull/missing-job", db, &runner_registry, res));
    REQUIRE(res.result() == http::status::not_found);
    REQUIRE(res.body().find("Pull job not found.") != std::string::npos);
  }
}

TEST_CASE("Generated ai runner pull GET returns job payload", "[http]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  holder::llm::LocalModelRunner runner;
  runner.set_fake_mode(true);
  const auto started = runner.start_pull("fake-echo");
  REQUIRE(!started.job_id.empty());
  holder::llm::LocalRunnerClient local_runner_client(&runner);
  holder::llm::RunnerRegistry runner_registry(nullptr, &local_runner_client);

  http::response<http::string_body> res;
  REQUIRE(call_generated_pull_route("/ai/runner/pull/" + started.job_id,
                                    db, &runner_registry, res));
  REQUIRE(res.result() == http::status::ok);

  const auto payload = nlohmann::json::parse(res.body());
  REQUIRE(payload["ok"] == true);
  REQUIRE(payload["data"]["job_id"] == started.job_id);
  REQUIRE(payload["data"]["runner_id"] == "auto-local");
  REQUIRE(payload["data"]["model"] == "fake-echo");
  REQUIRE(payload["data"]["model_ref"] == "auto-local::fake-echo");
  REQUIRE(payload["data"]["status"].is_string());
  REQUIRE(payload["data"]["updated_at"].is_number_integer());
  REQUIRE(payload["data"]["progress"]["completed"].is_number_integer());
  REQUIRE(payload["data"]["progress"]["total"].is_number_integer());
  REQUIRE(payload["data"]["progress"]["percent"].is_number());
  REQUIRE(payload["data"]["progress"]["stage"].is_string());

  REQUIRE(call_generated_pull_route("/ai/runner/pull/" + started.job_id +
                                    "?runner_id=auto%2Dlocal", db, &runner_registry, res));
  REQUIRE(res.result() == http::status::ok);
  REQUIRE(nlohmann::json::parse(res.body())["data"]["runner_id"] == "auto-local");

  REQUIRE(call_generated_pull_route("/ai/runner/pull/" + started.job_id +
                                    "?runner_id=missing", db, &runner_registry, res));
  REQUIRE(res.result() == http::status::not_found);

  REQUIRE(call_generated_pull_route("/ai/runner/pull/" + started.job_id +
                                    "?runner_id=%XX", db, &runner_registry, res));
  REQUIRE(res.result() == http::status::bad_request);
  REQUIRE(nlohmann::json::parse(res.body())["error"]["code"] == "bad_request");
}

TEST_CASE("HTTP runner pull status uses the generated route", "[http]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  holder::llm::LocalModelRunner runner;
  runner.set_fake_mode(true);
  const auto started = runner.start_pull("fake-echo");
  holder::llm::LocalRunnerClient local_client(&runner);
  holder::llm::RunnerRegistry registry(nullptr, &local_client);

  const std::string token = "testtoken";
  holder::api::HttpServer server("127.0.0.1", 0, db, token, nullptr, nullptr,
                                 nullptr, &registry);
  holder::api::HttpServer::BoundInfo bound;
  try {
    bound = server.start();
  } catch (const std::exception& error) {
    SKIP(std::string("Socket bind not available in test environment: ") + error.what());
  }
  holder::core::SignalHandler signals;
  holder::test::HttpServerThreadGuard running(server, signals);
  REQUIRE(holder::test::wait_for_http_health_ready(bound.bind, bound.port, token));

  const auto get = [&](const std::string& path, const std::string& bearer) {
    return holder::test::http_request_raw(bound.bind, bound.port, bearer,
                                          http::verb::get, path);
  };
  const auto status = get("/ai/runner/pull/" + started.job_id +
                          "?runner_id=auto%2Dlocal", token);
  REQUIRE(status.status == http::status::ok);
  REQUIRE(status.content_type == "application/json");
  const auto payload = nlohmann::json::parse(status.body);
  REQUIRE(payload["data"]["job_id"] == started.job_id);
  REQUIRE(payload["data"]["runner_id"] == "auto-local");
  REQUIRE(payload["data"]["progress"].is_object());

  const auto missing = get("/ai/runner/pull/missing-job", token);
  REQUIRE(missing.status == http::status::not_found);
  REQUIRE(nlohmann::json::parse(missing.body)["error"]["code"] == "not_found");

  const auto empty = get("/ai/runner/pull/", token);
  REQUIRE(empty.status == http::status::not_found);

  const auto denied = get("/ai/runner/pull/" + started.job_id, "wrong-token");
  REQUIRE(denied.status == http::status::unauthorized);
}

TEST_CASE("Ai runner pull routes can target a manual runner by runner_id", "[http]") {
  const auto dir = holder::test::make_temp_dir();
  holder::test::EnvGuard fake_env("HOLDER_MODEL_RUNNER_FAKE", "1");
  auto db = holder::test::open_db_with_schema(dir / "holder.db");

  holder::ai::AiRunnerRepo repo(db);
  repo.upsert(holder::model::AiRunner{
      .runner_id = "manual-a",
      .name = "Office Ollama",
      .kind = "ollama",
      .base_url = std::optional<std::string>("http://office:11434"),
      .source = "manual",
      .enabled = true,
      .created_at = 1,
      .updated_at = 1,
  });

  holder::llm::RunnerRegistry runner_registry(&db, nullptr);
  http::response<http::string_body> res;
  http::request<http::string_body> req{http::verb::post, "/ai/runner/pull", 11};
  req.set(http::field::host, "127.0.0.1");
  req.set(http::field::content_type, "application/json");
  req.body() = R"({"runner_id":"manual-a","model":"fake-echo"})";

  const auto out = holder::api::routes::ai::runner::handle_ai_runner_pull_routes(
      "/ai/runner/pull",
      req,
      res,
      &runner_registry,
      [](const std::string&) -> std::string {
        return {};
      }
  );
  REQUIRE(out.handled);
  REQUIRE(res.result() == http::status::ok);

  const auto payload = nlohmann::json::parse(res.body());
  REQUIRE(payload["ok"] == true);
  REQUIRE(payload["data"]["runner_id"] == "manual-a");
  REQUIRE(payload["data"]["model_ref"] == "manual-a::fake-echo");

  const std::string path = "/ai/runner/pull/" + payload["data"]["job_id"].get<std::string>();
  http::response<http::string_body> get_res;
  REQUIRE(call_generated_pull_route(path + "?runner_id=manual-a", db, &runner_registry, get_res));
  REQUIRE(get_res.result() == http::status::ok);
  const auto status = nlohmann::json::parse(get_res.body());
  REQUIRE(status["data"]["runner_id"] == "manual-a");
  REQUIRE(status["data"]["model_ref"] == "manual-a::fake-echo");
}

TEST_CASE("Ai runner pull routes prefer param_get runner_id over request body", "[http]") {
  const auto dir = holder::test::make_temp_dir();
  holder::test::EnvGuard fake_env("HOLDER_MODEL_RUNNER_FAKE", "1");
  auto db = holder::test::open_db_with_schema(dir / "holder.db");

  holder::ai::AiRunnerRepo repo(db);
  repo.upsert(holder::model::AiRunner{
      .runner_id = "manual-a",
      .name = "Office Ollama",
      .kind = "ollama",
      .base_url = std::optional<std::string>("http://office:11434"),
      .source = "manual",
      .enabled = true,
      .created_at = 1,
      .updated_at = 1,
  });

  holder::llm::RunnerRegistry runner_registry(&db, nullptr);
  http::response<http::string_body> res;
  http::request<http::string_body> req{http::verb::post, "/ai/runner/pull", 11};
  req.set(http::field::host, "127.0.0.1");
  req.set(http::field::content_type, "application/json");
  req.body() = R"({"runner_id":"auto-local","model":"fake-echo"})";

  const auto out = holder::api::routes::ai::runner::handle_ai_runner_pull_routes(
      "/ai/runner/pull",
      req,
      res,
      &runner_registry,
      [](const std::string& key) -> std::string {
        if (key == "runner_id") {
          return "manual-a";
        }
        return {};
      }
  );
  REQUIRE(out.handled);
  REQUIRE(res.result() == http::status::ok);

  const auto payload = nlohmann::json::parse(res.body());
  REQUIRE(payload["ok"] == true);
  REQUIRE(payload["data"]["runner_id"] == "manual-a");
  REQUIRE(payload["data"]["model_ref"] == "manual-a::fake-echo");
}
