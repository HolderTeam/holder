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

  // Contract routes report invalid_argument; the remaining RIG route reports bad_request.
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
