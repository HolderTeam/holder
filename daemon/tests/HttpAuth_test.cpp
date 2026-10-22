#include "http_test_helpers.h"

#include <fstream>
#include <iterator>

using holder::test::EnvGuard;
using holder::test::http_request_raw;
using holder::test::make_temp_dir;
using holder::test::open_db_with_schema;

TEST_CASE("HTTP endpoints require auth token", "[http]") {
  const auto dir = make_temp_dir();
  const auto db_path = dir / "holder.db";

  auto db = open_db_with_schema(db_path);

  namespace fs = std::filesystem;
  const auto docs_root = dir / "assets" / "swagger-ui";
  fs::create_directories(docs_root);
  std::ofstream(docs_root / "index.html") << "<!doctype html><title>Docs</title>";
  EnvGuard docs_env("HOLDER_DOCS_ROOT", docs_root.string());

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

  const auto health =
      http_request_raw(bound.bind, bound.port, "", boost::beast::http::verb::get, "/health");
  REQUIRE(health.status == boost::beast::http::status::unauthorized);

  const auto ping =
      http_request_raw(bound.bind, bound.port, "", boost::beast::http::verb::get, "/ping");
  REQUIRE(ping.status == boost::beast::http::status::ok);
  REQUIRE(ping.content_type == "text/plain");
  REQUIRE(ping.body == "pong");

  // A RIG_PUBLIC operation needs no token, but only for its declared method.
  const auto ping_post =
      http_request_raw(bound.bind, bound.port, "", boost::beast::http::verb::post, "/ping");
  REQUIRE(ping_post.status == boost::beast::http::status::unauthorized);
  const auto ping_post_authorized =
      http_request_raw(bound.bind, bound.port, token, boost::beast::http::verb::post, "/ping");
  REQUIRE(ping_post_authorized.status == boost::beast::http::status::method_not_allowed);

  // The daemon serves only the routes the contract declares, including its own handlers'.
  const auto docs_post =
      http_request_raw(bound.bind, bound.port, token, boost::beast::http::verb::post, "/docs");
  REQUIRE(docs_post.status == boost::beast::http::status::method_not_allowed);
  for (const auto* undeclared :
       {"/nowhere", "/docs/a/b", "/locations/", "/locations/l-1/unknown", "/resources/",
        "/resources/r-1/unknown", "/resources/r-1/assets/a-1/content/extra"}) {
    const auto response = http_request_raw(
        bound.bind, bound.port, token, boost::beast::http::verb::get, undeclared
    );
    REQUIRE(response.status == boost::beast::http::status::not_found);
    REQUIRE(nlohmann::json::parse(response.body)["error"]["code"] == "not_found");
  }

  const auto alive =
      http_request_raw(bound.bind, bound.port, "", boost::beast::http::verb::get, "/alive");
  REQUIRE(alive.status == boost::beast::http::status::ok);
  REQUIRE(alive.content_type == "application/json");
  REQUIRE(nlohmann::json::parse(alive.body) == nlohmann::json{{"ok", true}});

  const auto alive_post =
      http_request_raw(bound.bind, bound.port, "", boost::beast::http::verb::post, "/alive");
  REQUIRE(alive_post.status == boost::beast::http::status::unauthorized);

  // The contract is compiled into the daemon, byte for byte.
  const auto contract =
      http_request_raw(bound.bind, bound.port, "", boost::beast::http::verb::get, "/openapi.json");
  REQUIRE(contract.status == boost::beast::http::status::ok);
  REQUIRE(contract.content_type == "application/json");
  std::ifstream contract_file(OPENAPI_JSON_PATH, std::ios::binary);
  const std::string contract_bytes{std::istreambuf_iterator<char>(contract_file), {}};
  REQUIRE(contract.body == contract_bytes);
  REQUIRE(nlohmann::json::parse(contract.body)["openapi"] == "3.2.0");

  // So is the YAML the docs page loads.
  const auto contract_yaml =
      http_request_raw(bound.bind, bound.port, "", boost::beast::http::verb::get, "/openapi.yaml");
  REQUIRE(contract_yaml.status == boost::beast::http::status::ok);
  REQUIRE(contract_yaml.content_type == "application/yaml");
  std::ifstream contract_yaml_file(OPENAPI_YAML_PATH, std::ios::binary);
  const std::string contract_yaml_bytes{std::istreambuf_iterator<char>(contract_yaml_file), {}};
  REQUIRE(contract_yaml.body == contract_yaml_bytes);

  const auto projects =
      http_request_raw(bound.bind, bound.port, "", boost::beast::http::verb::get, "/projects");
  REQUIRE(projects.status == boost::beast::http::status::unauthorized);

  const auto docs =
      http_request_raw(bound.bind, bound.port, "", boost::beast::http::verb::get, "/docs");
  REQUIRE(docs.status == boost::beast::http::status::ok);

  auto raw_with_auth = [&](const std::string& auth_value) {
    namespace http = boost::beast::http;
    using tcp = boost::asio::ip::tcp;

    boost::asio::io_context ioc;
    tcp::resolver resolver(ioc);
    auto endpoints = resolver.resolve(bound.bind, std::to_string(bound.port));

    tcp::socket socket(ioc);
    boost::asio::connect(socket, endpoints);

    http::request<http::string_body> req{http::verb::get, "/health", 11};
    req.set(http::field::host, bound.bind);
    req.set(http::field::user_agent, "holder-tests");
    req.set(http::field::authorization, auth_value);

    http::write(socket, req);

    boost::beast::flat_buffer buffer;
    http::response<http::string_body> res;
    http::read(socket, buffer, res);
    holder::test::close_http_socket(socket);

    return res.result();
  };

  const auto bad_auth = raw_with_auth("Token nope");
  REQUIRE(bad_auth == boost::beast::http::status::unauthorized);

  const auto missing_bearer = raw_with_auth("testtoken");
  REQUIRE(missing_bearer == boost::beast::http::status::unauthorized);

  const auto lower_bearer = raw_with_auth("bearer " + token);
  REQUIRE(lower_bearer == boost::beast::http::status::unauthorized);

  const auto spaced_bearer = raw_with_auth("Bearer    " + token);
  REQUIRE(spaced_bearer == boost::beast::http::status::unauthorized);

  auto raw_with_multi_auth = [&](const std::string& first, const std::string& second) {
    namespace http = boost::beast::http;
    using tcp = boost::asio::ip::tcp;

    boost::asio::io_context ioc;
    tcp::resolver resolver(ioc);
    auto endpoints = resolver.resolve(bound.bind, std::to_string(bound.port));

    tcp::socket socket(ioc);
    boost::asio::connect(socket, endpoints);

    http::request<http::string_body> req{http::verb::get, "/health", 11};
    req.set(http::field::host, bound.bind);
    req.set(http::field::user_agent, "holder-tests");
    req.insert(http::field::authorization, first);
    req.insert(http::field::authorization, second);

    http::write(socket, req);

    boost::beast::flat_buffer buffer;
    http::response<http::string_body> res;
    http::read(socket, buffer, res);
    holder::test::close_http_socket(socket);

    return res.result();
  };

  const auto multi_auth = raw_with_multi_auth("Token nope", "Bearer " + token);
  REQUIRE(multi_auth == boost::beast::http::status::unauthorized);

  std::raise(SIGTERM);
  server_thread.join();
}
