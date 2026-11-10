#include "api/GeneratedRoutes.h"
#include "http_test_helpers.h"

#include <boost/asio.hpp>
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

using holder::test::create_project;
using holder::test::make_temp_dir;
using holder::test::open_db_with_schema;

TEST_CASE("HTTP endpoints reject invalid JSON bodies", "[http]") {
  const auto dir = make_temp_dir();
  const auto db_path = dir / "holder.db";

  auto db = open_db_with_schema(db_path);
  create_project(db, "proj-1", (dir / "repo").string());

  holder::index::FtsIndexer fts(db);
  holder::card::CardStore card_store(db, &fts);

  const std::string token = "testtoken";
  holder::api::HttpServer server("127.0.0.1", 0, db, token, &card_store, &fts);
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

  namespace http = boost::beast::http;
  using tcp = boost::asio::ip::tcp;

  boost::asio::io_context ioc;
  tcp::resolver resolver(ioc);
  auto endpoints = resolver.resolve(bound.bind, std::to_string(bound.port));
  const auto post = [&](const std::string& target, const std::string& body) {
    tcp::socket socket(ioc);
    boost::asio::connect(socket, endpoints);
    http::request<http::string_body> req{http::verb::post, target, 11};
    req.set(http::field::host, bound.bind);
    req.set(http::field::user_agent, "holder-tests");
    req.set(http::field::authorization, "Bearer " + token);
    req.set(http::field::content_type, "application/json");
    req.body() = body;
    req.prepare_payload();
    http::write(socket, req);
    boost::beast::flat_buffer buffer;
    http::response<http::string_body> res;
    http::read(socket, buffer, res);
    holder::test::close_http_socket(socket);
    return res;
  };

  // Core contract routes report invalid_argument; this daemon route reports bad_request.
  struct Case {
    const char* target;
    const char* error_code;
  };
  for (const auto& test : {Case{"/projects/proj-1/cards", "invalid_argument"},
                           Case{"/locations", "invalid_argument"},
                           Case{"/imports", "invalid_argument"},
                           Case{"/ai/runners", "bad_request"}}) {
    const auto res = post(test.target, "{");
    CHECK(res.result() == http::status::bad_request);

    // The parser's message quotes bytes that are not UTF-8; the failure is still a JSON 400.
    const auto not_utf8 = post(test.target, "\xff\xfe{");
    CHECK(not_utf8.result() == http::status::bad_request);
    const auto payload = nlohmann::json::parse(not_utf8.body());
    CHECK(payload["ok"] == false);
    CHECK(payload["error"]["code"] == test.error_code);
  }

  std::raise(SIGTERM);
  server_thread.join();
}

TEST_CASE("Milestone range routes validate required bounds and project IDs", "[http]") {
  namespace http = boost::beast::http;
  const auto dir = make_temp_dir();
  auto db = open_db_with_schema(dir / "holder.db");
  create_project(db, "project", (dir / "project").string());

  struct Case {
    const char* path;
    http::status status;
    const char* code;
    const char* message;
  };
  for (const auto& item : {
           Case{"/projects/project/milestones/days", http::status::bad_request,
                "invalid_argument", "first_day is required"},
           Case{"/projects/project/milestones/days?first_day=2026-01-01",
                http::status::bad_request, "invalid_argument", "last_day is required"},
           Case{"/projects/project/milestones/days?first_day=not-a-date&last_day=2026-01-02",
                http::status::bad_request, "invalid_argument", "first_day must be a day"},
           Case{"/projects/missing/milestones/days?first_day=2026-01-01&last_day=2026-01-02",
                http::status::not_found, "not_found", "project not found"},
           Case{"/projects/project/milestones/instants", http::status::bad_request,
                "invalid_argument", "first_instant is required"},
           Case{"/projects/project/milestones/instants?first_instant=1",
                http::status::bad_request, "invalid_argument", "last_instant is required"},
           Case{"/projects/project/milestones/instants?first_instant=no&last_instant=2",
                http::status::bad_request, "invalid_argument", "first_instant"},
           Case{"/projects/missing/milestones/instants?first_instant=1&last_instant=2",
                http::status::not_found, "not_found", "project not found"},
       }) {
    CAPTURE(item.path);
    http::request<http::string_body> req{http::verb::get, item.path, 11};
    http::response<http::string_body> res;
    REQUIRE(holder::api::dispatch_generated_routes(req, res, db));
    REQUIRE(res.result() == item.status);
    const auto error = nlohmann::json::parse(res.body()).at("error");
    CHECK(error.at("code") == item.code);
    CHECK(error.at("message").get<std::string>().find(item.message) != std::string::npos);
  }
}

TEST_CASE("Project routes report missing cards resources and projects consistently", "[http]") {
  namespace http = boost::beast::http;
  const auto dir = make_temp_dir();
  auto db = open_db_with_schema(dir / "holder.db");
  create_project(db, "project", (dir / "project").string());

  struct Case {
    http::verb method;
    const char* path;
  };
  for (const auto& item : {
           Case{http::verb::get, "/projects/missing/history"},
           Case{http::verb::get, "/projects/missing/resources"},
           Case{http::verb::get,
                "/projects/missing/calendar?first_day=2026-01-01&last_day=2026-01-02&time_zone=UTC"},
           Case{http::verb::get, "/projects/project/cards/missing/metadata"},
           Case{http::verb::get, "/projects/project/cards/missing/links?include_deleted=true"},
           Case{http::verb::get, "/projects/project/cards/missing/backlinks?include_deleted=false"},
           Case{http::verb::get, "/projects/project/cards/missing/milestones"},
           Case{http::verb::get, "/projects/project/cards/missing/resources"},
           Case{http::verb::get, "/projects/project/resources/missing/backlinks"},
           Case{http::verb::delete_, "/projects/project/resources/missing"},
           Case{http::verb::delete_, "/projects/project/cards/missing"},
           Case{http::verb::delete_, "/projects/project/cards/missing/resources/missing"},
       }) {
    CAPTURE(item.path);
    http::request<http::string_body> req{item.method, item.path, 11};
    http::response<http::string_body> res;
    REQUIRE(holder::api::dispatch_generated_routes(req, res, db));
    INFO(res.body());
    REQUIRE(res.result() == http::status::not_found);
    CHECK(nlohmann::json::parse(res.body()).at("error").at("code") == "not_found");
  }
}

TEST_CASE("Calendar requests validate dates and accept an explicit time zone", "[http]") {
  namespace http = boost::beast::http;
  const auto dir = make_temp_dir();
  auto db = open_db_with_schema(dir / "holder.db");
  create_project(db, "project", (dir / "project").string());
  const auto get = [&](const std::string& query) {
    CAPTURE(query);
    http::request<http::string_body> req{
        http::verb::get, "/projects/project/calendar" + query, 11};
    http::response<http::string_body> res;
    REQUIRE(holder::api::dispatch_generated_routes(req, res, db));
    return res;
  };

  for (const auto& [query, message] :
       std::initializer_list<std::pair<const char*, const char*>>{
           {"", "first_day is required"},
           {"?first_day=2026-02-01", "last_day is required"},
           {"?first_day=not-a-date&last_day=2026-02-02", "first_day must be a day"},
       }) {
    const auto res = get(query);
    REQUIRE(res.result() == http::status::bad_request);
    const auto error = nlohmann::json::parse(res.body()).at("error");
    CHECK(error.at("code") == "invalid_argument");
    CHECK(error.at("message").get<std::string>().find(message) != std::string::npos);
  }

  const auto res = get("?first_day=2026-02-01&last_day=2026-02-02&time_zone=UTC");
  REQUIRE(res.result() == http::status::ok);
  const auto data = nlohmann::json::parse(res.body()).at("data");
  CHECK(data.at("created_cards") == nlohmann::json::array());
  CHECK(data.at("updated_cards") == nlohmann::json::array());
  CHECK(data.at("milestones") == nlohmann::json::array());
}
