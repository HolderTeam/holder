#include "ai/AiRunRepo.h"
#include "api/generated/AiJson.h"
#include "api/routes/ai/runs/AiRunQueryRoutes.h"
#include "api/support/RunEventStore.h"
#include "http_test_helpers.h"
#include "llm/LocalModelRunner.h"
#include "llm/LocalRunnerClient.h"

namespace http = boost::beast::http;

using holder::test::http_request_raw;
using holder::test::make_temp_dir;
using holder::test::open_db_with_schema;

TEST_CASE("Generated AI stream dispatch contains a missing response consumer", "[http][generated]") {
  const auto dir = make_temp_dir();
  auto db = open_db_with_schema(dir / "holder.db");
  holder::test::create_project(db, "project", (dir / "project").string());
  db.exec("INSERT INTO ai_threads(thread_id, project_id, title, created_at, updated_at) "
          "VALUES('thread', 'project', 'Thread', 1, 1);");
  holder::model::AiRun run;
  run.run_id = "missing-consumer-run";
  run.project_id = "project";
  run.thread_id = "thread";
  run.mode = "auto";
  run.prompt = "hello";
  run.status = "completed";
  run.created_at = run.updated_at = 1;
  holder::ai::AiRunRepo(db).create(run);
  http::request<http::string_body> req{http::verb::get,
      "/ai/runs/missing-consumer-run/events", 11};
  http::response<http::string_body> res;
  REQUIRE(holder::api::dispatch_generated_routes(req, res, db));
  CHECK(res.result() == http::status::internal_server_error);
  const auto error = nlohmann::json::parse(res.body()).at("error");
  CHECK(error.at("code") == "error");
  CHECK(error.at("message") == "no one to serve a host-written response");
}

TEST_CASE("HTTP ai run events streams terminal status", "[http]") {
  const auto dir = make_temp_dir();
  const auto db_path = dir / "holder.db";

  holder::platform::Db db = open_db_with_schema(db_path);
  db.exec("INSERT INTO projects(project_id, name, root_path, created_at, updated_at) "
          "VALUES('proj-1', 'Project', '/tmp/project', 1, 1);");
  db.exec("INSERT INTO ai_threads(thread_id, project_id, title, created_at, updated_at) "
          "VALUES('thread-1', 'proj-1', 'Thread', 1, 1);");

  holder::ai::AiRunRepo repo(db);
  holder::model::AiRun run;
  run.run_id = "run-1";
  run.project_id = "proj-1";
  run.thread_id = "thread-1";
  run.mode = "auto";
  run.prompt = "hello";
  run.status = "completed";
  run.chosen_model = "auto-local::fake-echo";
  run.created_at = 1;
  run.updated_at = 2;
  repo.create(run);

  const std::string token = "testtoken";
  holder::api::HttpServer server("127.0.0.1", 0, db, token, nullptr, nullptr);
  holder::api::HttpServer::BoundInfo bound;
  try {
    bound = server.start();
  } catch (const std::exception& ex) {
    SKIP(std::string("Socket bind not available in test environment: ") + ex.what());
  }

  holder::core::SignalHandler signals;
  std::thread server_thread([&server, &signals]() {
    server.run(signals);
  });
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  const auto res = http_request_raw(
      bound.bind,
      bound.port,
      token,
      boost::beast::http::verb::get,
      "/ai/runs/run-1/events"
  );
  REQUIRE(res.status == boost::beast::http::status::ok);
  REQUIRE(res.content_type.find("text/event-stream") != std::string::npos);
  REQUIRE(res.body.find("event: done") != std::string::npos);
  REQUIRE(res.body.find("\"run_id\":\"run-1\"") != std::string::npos);
  REQUIRE(res.body.find("\"runner_id\":\"auto-local\"") != std::string::npos);
  REQUIRE(res.body.find("\"model_ref\":\"auto-local::fake-echo\"") != std::string::npos);
  REQUIRE(res.body.find("\"model\":\"fake-echo\"") != std::string::npos);

  std::raise(SIGTERM);
  server_thread.join();
}

TEST_CASE("HTTP ai run events streams terminal failed status with error", "[http]") {
  const auto dir = make_temp_dir();
  const auto db_path = dir / "holder.db";

  holder::platform::Db db = open_db_with_schema(db_path);
  db.exec("INSERT INTO projects(project_id, name, root_path, created_at, updated_at) "
          "VALUES('proj-1', 'Project', '/tmp/project', 1, 1);");
  db.exec("INSERT INTO ai_threads(thread_id, project_id, title, created_at, updated_at) "
          "VALUES('thread-1', 'proj-1', 'Thread', 1, 1);");

  holder::ai::AiRunRepo repo(db);
  holder::model::AiRun run;
  run.run_id = "run-failed-1";
  run.project_id = "proj-1";
  run.thread_id = "thread-1";
  run.mode = "auto";
  run.prompt = "hello";
  run.status = "failed";
  run.error = "boom";
  run.created_at = 1;
  run.updated_at = 2;
  repo.create(run);

  const std::string token = "testtoken";
  holder::api::HttpServer server("127.0.0.1", 0, db, token, nullptr, nullptr);
  holder::api::HttpServer::BoundInfo bound;
  try {
    bound = server.start();
  } catch (const std::exception& ex) {
    SKIP(std::string("Socket bind not available in test environment: ") + ex.what());
  }

  holder::core::SignalHandler signals;
  std::thread server_thread([&server, &signals]() {
    server.run(signals);
  });
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  const auto res = http_request_raw(
      bound.bind,
      bound.port,
      token,
      boost::beast::http::verb::get,
      "/ai/runs/run-failed-1/events"
  );
  REQUIRE(res.status == boost::beast::http::status::ok);
  REQUIRE(res.content_type.find("text/event-stream") != std::string::npos);
  REQUIRE(res.body.find("event: failed") != std::string::npos);
  REQUIRE(res.body.find("\"error\":\"boom\"") != std::string::npos);

  std::raise(SIGTERM);
  server_thread.join();
}

TEST_CASE(
    "HTTP ai run events terminal status falls back to raw chosen_model when not a model ref",
    "[http]"
) {
  const auto dir = make_temp_dir();
  const auto db_path = dir / "holder.db";

  holder::platform::Db db = open_db_with_schema(db_path);
  db.exec("INSERT INTO projects(project_id, name, root_path, created_at, updated_at) "
          "VALUES('proj-1', 'Project', '/tmp/project', 1, 1);");
  db.exec("INSERT INTO ai_threads(thread_id, project_id, title, created_at, updated_at) "
          "VALUES('thread-1', 'proj-1', 'Thread', 1, 1);");

  holder::ai::AiRunRepo repo(db);
  holder::model::AiRun run;
  run.run_id = "run-plain-model-1";
  run.project_id = "proj-1";
  run.thread_id = "thread-1";
  run.mode = "auto";
  run.prompt = "hello";
  run.status = "completed";
  run.chosen_model = "plain-model";
  run.created_at = 1;
  run.updated_at = 2;
  repo.create(run);

  const std::string token = "testtoken";
  holder::api::HttpServer server("127.0.0.1", 0, db, token, nullptr, nullptr);
  holder::api::HttpServer::BoundInfo bound;
  try {
    bound = server.start();
  } catch (const std::exception& ex) {
    SKIP(std::string("Socket bind not available in test environment: ") + ex.what());
  }

  holder::core::SignalHandler signals;
  std::thread server_thread([&server, &signals]() {
    server.run(signals);
  });
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  const auto res = http_request_raw(
      bound.bind,
      bound.port,
      token,
      boost::beast::http::verb::get,
      "/ai/runs/run-plain-model-1/events"
  );
  REQUIRE(res.status == boost::beast::http::status::ok);
  REQUIRE(res.content_type.find("text/event-stream") != std::string::npos);
  REQUIRE(res.body.find("event: done") != std::string::npos);
  REQUIRE(res.body.find("\"model_ref\":\"plain-model\"") != std::string::npos);
  REQUIRE(res.body.find("\"model\":\"plain-model\"") != std::string::npos);
  REQUIRE(res.body.find("\"runner_id\"") == std::string::npos);

  std::raise(SIGTERM);
  server_thread.join();
}

TEST_CASE("HTTP ai run events returns not_found for missing run", "[http]") {
  const auto dir = make_temp_dir();
  const auto db_path = dir / "holder.db";

  holder::platform::Db db = open_db_with_schema(db_path);
  const std::string token = "testtoken";
  holder::api::HttpServer server("127.0.0.1", 0, db, token, nullptr, nullptr);
  holder::api::HttpServer::BoundInfo bound;
  try {
    bound = server.start();
  } catch (const std::exception& ex) {
    SKIP(std::string("Socket bind not available in test environment: ") + ex.what());
  }

  holder::core::SignalHandler signals;
  std::thread server_thread([&server, &signals]() {
    server.run(signals);
  });
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  const auto res = http_request_raw(
      bound.bind,
      bound.port,
      token,
      boost::beast::http::verb::get,
      "/ai/runs/does-not-exist/events"
  );
  REQUIRE(res.status == boost::beast::http::status::not_found);
  REQUIRE(res.body.find("Run not found.") != std::string::npos);

  std::raise(SIGTERM);
  server_thread.join();
}

TEST_CASE("Ai run events route handles direct empty/missing identifiers", "[http]") {
  boost::asio::io_context ioc;
  boost::asio::ip::tcp::socket socket(ioc);
  http::response<http::string_body> res;

  SECTION("empty run_id path is not a route") {
    holder::platform::Db db = open_db_with_schema(make_temp_dir() / "holder.db");
    http::request<http::string_body> req{http::verb::get, "/ai/runs//events", 11};
    REQUIRE_FALSE(holder::test::serve_generated(req, res, socket, db).handled);
  }

  SECTION("db exception during get returns not_found") {
    holder::platform::Db unopened_db;
    http::request<http::string_body> req{http::verb::get, "/ai/runs/run-1/events", 11};
    auto out = holder::test::serve_generated(req, res, socket, unopened_db);
    REQUIRE(out.handled == true);
    REQUIRE(out.streamed == false);
    REQUIRE(res.result() == http::status::not_found);
  }
}

TEST_CASE("HTTP ai run events streams from in-memory event stream", "[http]") {
  const auto dir = make_temp_dir();
  const auto db_path = dir / "holder.db";
  holder::platform::Db db = open_db_with_schema(db_path);

  db.exec("INSERT INTO projects(project_id, name, root_path, created_at, updated_at) "
          "VALUES('proj-1', 'Project', '/tmp/project', 1, 1);");
  db.exec("INSERT INTO ai_threads(thread_id, project_id, title, created_at, updated_at) "
          "VALUES('thread-1', 'proj-1', 'Thread', 1, 1);");

  holder::ai::AiRunRepo repo(db);
  holder::model::AiRun run;
  run.run_id = "run-stream-1";
  run.project_id = "proj-1";
  run.thread_id = "thread-1";
  run.mode = "auto";
  run.prompt = "hello";
  run.status = "started";
  run.created_at = 1;
  run.updated_at = 2;
  repo.create(run);

  holder::api::support::append_run_event(run.run_id, ai_events::run_started, {}, false);
  holder::api::support::append_run_event(
      "run-stream-1",
      ai_events::progress,
      {.message = "step 1"},
      false
  );

  bool truncated = false;
  SECTION("complete history is replayed") {}
  SECTION("truncated history requests a resync") {
    truncated = true;
    for (int i = 0; i < 512; ++i)
      holder::api::support::append_run_event(run.run_id, ai_events::progress, {}, false);
  }

  holder::api::support::append_run_event(
      "run-stream-1",
      ai_events::done,
      {.status = "completed"},
      true
  );

  const std::string token = "testtoken";
  holder::api::HttpServer server("127.0.0.1", 0, db, token, nullptr, nullptr);
  holder::api::HttpServer::BoundInfo bound;
  try {
    bound = server.start();
  } catch (const std::exception& ex) {
    SKIP(std::string("Socket bind not available in test environment: ") + ex.what());
  }

  holder::core::SignalHandler signals;
  holder::test::HttpServerThreadGuard guard(server, signals);
  REQUIRE(holder::test::wait_for_http_health_ready(bound.bind, bound.port, token));

  const auto res = http_request_raw(
      bound.bind,
      bound.port,
      token,
      boost::beast::http::verb::get,
      "/ai/runs/run-stream-1/events"
  );
  REQUIRE(res.status == boost::beast::http::status::ok);
  REQUIRE(res.content_type.find("text/event-stream") != std::string::npos);
  if (truncated) {
    CHECK(res.body.find("event: resync_required") != std::string::npos);
    CHECK(res.body.find("history_unavailable") != std::string::npos);
    CHECK(res.body.find("event: progress") == std::string::npos);
  } else {
    CHECK(res.body.find("event: progress") != std::string::npos);
    CHECK(res.body.find("event: done") != std::string::npos);
    CHECK(res.body.find("\"run_id\":\"run-stream-1\"") != std::string::npos);
  }
}

TEST_CASE("RunEventStore trims in-memory event stream to last 512 events", "[http]") {
  const std::string run_id = "run-trim-coverage-1";
  for (int i = 0; i < 520; ++i) {
    holder::api::support::append_run_event(
        run_id,
        ai_events::progress,
        {.message = std::to_string(i)},
        i == 519
    );
  }

  const auto stream_opt = holder::api::support::get_run_event_stream(run_id);
  REQUIRE(stream_opt.has_value());
  REQUIRE(stream_opt->events.size() == 512);
  REQUIRE(stream_opt->events.front().data["message"] == "8");
  REQUIRE(stream_opt->events.back().data["message"] == "519");
  REQUIRE(stream_opt->finished == true);
}

TEST_CASE("Ai run events route tells a reconnect past a finished run to stop", "[http]") {
  holder::platform::Db db = open_db_with_schema(make_temp_dir() / "holder.db");
  db.exec("INSERT INTO projects(project_id, name, root_path, created_at, updated_at) "
          "VALUES('proj-1', 'Project', '/tmp/project', 1, 1);");
  db.exec("INSERT INTO ai_threads(thread_id, project_id, title, created_at, updated_at) "
          "VALUES('thread-1', 'proj-1', 'Thread', 1, 1);");
  holder::model::AiRun run;
  run.run_id = "run-finished-reconnect";
  run.project_id = "proj-1";
  run.thread_id = "thread-1";
  run.mode = "auto";
  run.prompt = "hello";
  run.status = "completed";
  run.created_at = 1;
  run.updated_at = 2;
  holder::ai::AiRunRepo(db).create(run);
  holder::api::support::append_run_event(run.run_id, ai_events::run_started, {}, false);
  const auto started = holder::api::support::read_run_events(run.run_id, "")->cursor;
  const auto done = holder::api::support::append_run_event(run.run_id, ai_events::done, {}, true);

  boost::asio::io_context ioc;
  boost::asio::ip::tcp::socket socket(ioc);
  const auto path = "/ai/runs/" + run.run_id + "/events";
  http::response<http::string_body> res;
  http::request<http::string_body> req{http::verb::get, path, 11};
  req.set("Last-Event-ID", "invalid-cursor");
  const auto invalid = holder::test::serve_generated(req, res, socket, db);
  REQUIRE(invalid.handled);
  REQUIRE_FALSE(invalid.streamed);
  REQUIRE(res.result() == http::status::bad_request);
  CHECK(res.body().find("Invalid Last-Event-ID") != std::string::npos);

  req.set("Last-Event-ID", done.id);
  auto out = holder::test::serve_generated(req, res, socket, db);
  REQUIRE(out.handled);
  REQUIRE_FALSE(out.streamed);
  REQUIRE(res.result() == http::status::no_content);
  REQUIRE(res.body().empty());

  // A reconnect that has not yet received the final event is not told to stop.
  http::response<http::string_body> pending;
  req.set("Last-Event-ID", started);
  out = holder::test::serve_generated(req, pending, socket, db);
  REQUIRE(pending.result() != http::status::no_content);
}

TEST_CASE("HTTP AI run failures use the response path appropriate to their timing", "[http]") {
  const auto dir = make_temp_dir();
  auto db = open_db_with_schema(dir / "holder.db");
  holder::test::create_project(db, "proj-1", (dir / "project").string());
  db.exec("INSERT INTO ai_threads(thread_id, project_id, title, created_at, updated_at) "
          "VALUES('thread-1', 'proj-1', 'Thread', 1, 1)");

  bool insert_fails = false;
  SECTION("a rejected insert returns JSON before starting the stream") {
    insert_fails = true;
    db.exec("CREATE TRIGGER reject_run BEFORE INSERT ON ai_runs "
            "BEGIN SELECT RAISE(ABORT, 'fixture rejects run insert'); END");
  }
  SECTION("a model exception after stream start publishes a terminal failure") {}

  holder::llm::LocalModelRunner model;
  holder::llm::RunnerStatus status;
  status.available = true;
  status.models = {{.name = "fixture-model", .size = 1}};
  model.set_status_override_for_tests(status);
  model.set_stream_generate_override_for_tests(
      [](const std::string&, const std::string&, const std::string&,
         const std::function<void(const std::string&)>&, std::string*) -> bool {
        throw std::runtime_error("fixture model failed");
      });
  holder::llm::LocalRunnerClient client(&model);
  holder::llm::RunnerRegistry registry(&db, &client);
  const std::string token = "testtoken";
  holder::api::HttpServer server("127.0.0.1", 0, db, token, nullptr, nullptr, nullptr, &registry);
  const auto bound = server.start();
  holder::core::SignalHandler signals;
  holder::test::HttpServerThreadGuard guard(server, signals);
  REQUIRE(holder::test::wait_for_http_health_ready(bound.bind, bound.port, token));

  const auto response = http_request_raw(
      bound.bind, bound.port, token, http::verb::post, "/ai/runs",
      {{"project_id", "proj-1"}, {"thread_id", "thread-1"}, {"prompt", "Run a test"}});
  guard.stop();
  const auto runs = holder::ai::AiRunRepo(db).list_by_thread("thread-1");
  if (insert_fails) {
    CHECK(response.status == http::status::internal_server_error);
    CHECK(response.content_type.find("application/json") != std::string::npos);
    CHECK(nlohmann::json::parse(response.body).at("ok") == false);
    CHECK(runs.empty());
  } else {
    REQUIRE(response.status == http::status::ok);
    CHECK(response.content_type.find("text/event-stream") != std::string::npos);
    CHECK(response.body.find("event: run_started") != std::string::npos);
    CHECK(response.body.find("event: failed") != std::string::npos);
    CHECK(response.body.find("event: done") == std::string::npos);
    REQUIRE(runs.size() == 1);
    CHECK(runs.front().status == "failed");
    const auto events = holder::api::support::read_run_events(runs.front().run_id, "");
    REQUIRE(events);
    REQUIRE_FALSE(events->events.empty());
    CHECK(events->finished);
    CHECK(events->events.back().name == "failed");
  }
}
