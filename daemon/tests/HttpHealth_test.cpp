#include "http_test_helpers.h"

#include "api/GeneratedRoutes.h"
#include "api/Health.h"

#include <chrono>

using holder::test::get_health;
using holder::test::make_temp_dir;
using holder::test::wait_for_http_health_ready;

TEST_CASE("HTTP /health returns ok with valid token", "[http]") {
  const auto dir = make_temp_dir();
  const auto db_path = dir / "holder.db";

  holder::platform::Db db;
  db.open(db_path);

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

  REQUIRE(wait_for_http_health_ready(bound.bind, bound.port, token));

  const auto payload = get_health(bound.bind, bound.port, token);
  REQUIRE(payload["ok"] == true);
  REQUIRE(payload["data"]["db_ok"] == true);
  REQUIRE(payload["data"]["api_version"] == CARD_SERVER_API_VERSION);
  REQUIRE(payload["data"]["privacy"].is_object());
  REQUIRE(payload["data"]["privacy"]["backend"] == "libsodium_xchacha20poly1305_ietf");
  REQUIRE(payload["data"]["privacy"]["project_mode_supported"] == true);

  server.stop();
  server_thread.join();
}

TEST_CASE("Generated /health reports db_ok false when its DB is closed", "[http]") {
  // Requests run on a worker connection; this checks the connection the route is given.
  const auto dir = make_temp_dir();
  holder::platform::Db db;
  db.open(dir / "holder.db");
  db.close();

  const auto started_at = std::chrono::steady_clock::now() - std::chrono::seconds(2);
  const auto direct = holder::api::get_health(db, started_at);
  REQUIRE(direct.ok);
  REQUIRE_FALSE(direct.data.db_ok);
  REQUIRE(direct.data.uptime_ms >= 2000);

  boost::beast::http::request<boost::beast::http::string_body> req{
      boost::beast::http::verb::get, "/health", 11};
  boost::beast::http::response<boost::beast::http::string_body> res;
  REQUIRE(holder::api::dispatch_generated_routes(
      req, res, db, nullptr, nullptr, nullptr, nullptr, {}, started_at));
  REQUIRE(res.result() == boost::beast::http::status::ok);
  const auto payload = nlohmann::json::parse(res.body());
  REQUIRE(payload["ok"] == true);
  REQUIRE(payload["data"]["db_ok"] == false);
  REQUIRE(payload["data"]["uptime_ms"].get<long long>() >= 2000);
  REQUIRE(payload["data"]["privacy"] == nlohmann::json{
      {"backend", "libsodium_xchacha20poly1305_ietf"}, {"project_mode_supported", true}});
  REQUIRE_FALSE(payload["data"].contains("crypt_filter"));
}
