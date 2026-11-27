#include "http_test_helpers.h"

namespace {

namespace http = boost::beast::http;
using holder::api::GeneratedRequest;
using holder::api::GeneratedResponse;
using nlohmann::json;
constexpr auto version = "2026-07-28";

GeneratedRequest mcp_request(std::string method, json params = json::object()) {
  GeneratedRequest request{http::verb::post, "/mcp", 11};
  request.set(http::field::content_type, "application/json");
  request.set(http::field::accept, "application/json, text/event-stream");
  request.set(http::field::authorization, "Bearer mcp-test-token");
  request.set("Mcp-Protocol-Version", version);
  request.set("Mcp-Method", method);
  if (params.contains("name")) request.set("Mcp-Name", params.at("name").get<std::string>());
  params["_meta"] = {
      {"io.modelcontextprotocol/protocolVersion", version},
      {"io.modelcontextprotocol/clientCapabilities", json::object()}
  };
  request.body(
  ) = json{{"jsonrpc", "2.0"}, {"id", "test"}, {"method", method}, {"params", params}}.dump();
  request.prepare_payload();
  return request;
}

GeneratedResponse exchange(unsigned short port, GeneratedRequest request) {
  boost::asio::io_context context;
  boost::beast::tcp_stream stream(context);
  stream.connect({boost::asio::ip::make_address("127.0.0.1"), port});
  if (request.count(http::field::host) == 0) {
    request.set(http::field::host, "127.0.0.1:" + std::to_string(port));
  }
  http::write(stream, request);
  boost::beast::flat_buffer buffer;
  GeneratedResponse response;
  http::read(stream, buffer, response);
  return response;
}

json result(const GeneratedResponse& response) {
  INFO(response.body());
  REQUIRE(response.result() == http::status::ok);
  const auto body = json::parse(response.body());
  REQUIRE(body.at("id") == "test");
  REQUIRE(body.at("result").at("resultType") == "complete");
  return body.at("result");
}

} // namespace

TEST_CASE("Live MCP shares daemon reads and applies its own access policy", "[http][mcp]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  holder::test::create_project(db, "allowed", (dir / "allowed").string());
  holder::test::create_project(db, "other", (dir / "other").string());
  holder::index::FtsIndexer fts(db);
  holder::card::CardStore cards(db, &fts);
  holder::test::create_card_fixture(cards, "card-1", "allowed", "Card", "Hello", 10);
  holder::test::create_card_fixture(cards, "card-2", "other", "Other", "Private", 11);

  holder::api::HttpServer server(
      "127.0.0.1",
      0,
      db,
      "rest-test-token",
      &cards,
      &fts,
      nullptr,
      nullptr,
      {},
      holder::api::mcp::Access{"mcp-test-token", "allowed", "test"}
  );
  const auto bound = server.start();
  holder::core::SignalHandler signals;
  holder::test::HttpServerThreadGuard guard(server, signals);

  auto request = mcp_request("server/discover");
  CHECK(result(exchange(bound.port, request)).at("supportedVersions").at(0) == version);
  const auto listed = result(exchange(bound.port, mcp_request("tools/list")));
  CHECK(listed.at("tools").size() == 3);
  CHECK(listed.at("ttlMs") == 0);
  CHECK(listed.at("cacheScope") == "private");

  const auto read_card = [&](std::string project, std::string card) {
    return result(exchange(
        bound.port,
        mcp_request(
            "tools/call",
            {{"name", "get_card"}, {"arguments", {{"project_id", project}, {"card_id", card}}}}
        )
    ));
  };
  const auto mcp_card = read_card("allowed", "card-1");
  const auto rest_card = holder::test::http_json_request(
      bound.bind,
      bound.port,
      "rest-test-token",
      http::verb::get,
      "/projects/allowed/cards/card-1",
      nullptr,
      http::status::ok
  );
  CHECK(mcp_card.at("structuredContent").at("data") == rest_card.at("data"));
  CHECK(read_card("other", "card-2").at("isError") == true);

  request.set(http::field::authorization, "Bearer rest-test-token");
  CHECK(exchange(bound.port, request).result() == http::status::unauthorized);
  const GeneratedRequest rest_get{http::verb::get, "/projects/allowed/cards/card-1", 11};
  auto rest = rest_get;
  rest.set(http::field::authorization, "Bearer mcp-test-token");
  CHECK(exchange(bound.port, rest).result() == http::status::unauthorized);

  request = mcp_request("tools/list");
  request.set(http::field::origin, "http://example.invalid");
  CHECK(exchange(bound.port, request).result() == http::status::forbidden);
  request.erase(http::field::origin);
  request.set(http::field::host, "example.invalid");
  CHECK(exchange(bound.port, request).result() == http::status::forbidden);
  request.erase(http::field::host);
  request.set("Mcp-Method", "tools/call");
  const auto mismatched = exchange(bound.port, request);
  CHECK(mismatched.result() == http::status::bad_request);
  CHECK(json::parse(mismatched.body()).at("error").at("code") == -32020);

  request = mcp_request("tools/list");
  auto invalid_version = json::parse(request.body());
  invalid_version["params"]["_meta"]["io.modelcontextprotocol/protocolVersion"] = 123;
  request.body() = invalid_version.dump();
  request.prepare_payload();
  const auto non_string = exchange(bound.port, request);
  CHECK(non_string.result() == http::status::bad_request);
  CHECK(json::parse(non_string.body()).at("error").at("code") == -32020);
}

TEST_CASE("Live MCP is opt-in", "[http][mcp]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  holder::api::HttpServer server("127.0.0.1", 0, db, "rest-test-token", nullptr, nullptr);
  const auto bound = server.start();
  holder::core::SignalHandler signals;
  holder::test::HttpServerThreadGuard guard(server, signals);
  auto request = mcp_request("tools/list");
  request.set(http::field::authorization, "Bearer rest-test-token");
  CHECK(exchange(bound.port, request).result() == http::status::not_found);
}
