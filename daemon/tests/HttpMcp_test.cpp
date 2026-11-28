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
  holder::test::create_card_fixture(cards, "card-3", "allowed", "Second", "More", 12);

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
  CHECK(listed.at("tools").size() == 14);
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

  const auto list_cards = [&](json arguments) {
    return result(exchange(
        bound.port,
        mcp_request("tools/call", {{"name", "list_cards"}, {"arguments", arguments}})
    ));
  };
  auto filters = json{{"project_id", "allowed"}, {"limit", 1}, {"include_child_count", true}};
  const auto first_page = list_cards(filters).at("structuredContent").at("data");
  REQUIRE(first_page.at("items").size() == 1);
  const auto& first = first_page.at("items").at(0);
  CHECK(first.at("card_id") == "card-1");
  CHECK(first.at("child_count") == 0);
  CHECK_FALSE(first.contains("content"));
  filters["cursor"] = first_page.at("next_cursor");
  const auto second_page = list_cards(filters).at("structuredContent").at("data");
  REQUIRE(second_page.at("items").size() == 1);
  CHECK(second_page.at("items").at(0).at("card_id") == "card-3");
  CHECK(second_page.at("next_cursor").is_null());

  const auto updated = list_cards(
      {{"project_id", "allowed"}, {"order", "updated_desc"}, {"parent", "roots"}, {"tag", nullptr}}
  );
  CHECK(updated.at("structuredContent").at("data").at("items").at(0).at("card_id") == "card-3");
  CHECK(list_cards({{"project_id", "other"}}).at("isError") == true);
  CHECK(list_cards({{"project_id", "allowed"}, {"limit", 101}}).at("isError") == true);
  CHECK(list_cards({{"project_id", "allowed"}, {"include_content", true}}).at("isError") == true);
  CHECK(list_cards({{"project_id", "allowed"}, {"include_child_count", 1}}).at("isError") == true);
  CHECK(list_cards({{"project_id", "allowed"}, {"order", "unknown"}}).at("isError") == true);
  CHECK(list_cards({{"project_id", nullptr}}).at("isError") == true);

  const auto resource =
      holder::test::http_json_request(
          bound.bind,
          bound.port,
          "rest-test-token",
          http::verb::post,
          "/projects/allowed/resources",
          {{"resource_id", "resource-1"}, {"type", "book"}, {"label", "A reference"}},
          http::status::created
      )
          .at("data");
  holder::test::http_json_request(
      bound.bind,
      bound.port,
      "rest-test-token",
      http::verb::put,
      "/projects/allowed/cards/card-3/resources/resource-1",
      nullptr,
      http::status::ok
  );

  const auto add_link = [&](const std::string& source_project,
                            const std::string& source_id,
                            const std::string& target_project,
                            const std::string& target_id) {
    holder::test::http_json_request(
        bound.bind,
        bound.port,
        "rest-test-token",
        http::verb::post,
        "/projects/" + source_project + "/cards/" + source_id + "/links",
        {{"target_type", "card"}, {"target_project_id", target_project}, {"target_id", target_id}},
        http::status::ok
    );
  };
  add_link("allowed", "card-1", "allowed", "card-3");
  add_link("allowed", "card-1", "other", "card-2");
  add_link("allowed", "card-3", "allowed", "card-1");
  add_link("other", "card-2", "allowed", "card-1");

  const auto query_card = [&](const std::string& name, json arguments) {
    return result(
        exchange(bound.port, mcp_request("tools/call", {{"name", name}, {"arguments", arguments}}))
    );
  };
  const auto selection = json{{"project_id", "allowed"}, {"card_id", "card-1"}};
  CHECK(
      query_card("list_card_resources", {{"card_id", "card-3"}})
          .at("structuredContent")
          .at("data") == json::array({resource})
  );
  CHECK(query_card("list_card_resources", {{"card_id", "card-2"}}).at("isError") == true);
  const auto outgoing = query_card("list_card_links", selection).at("structuredContent").at("data");
  REQUIRE(outgoing.size() == 1);
  CHECK(outgoing.at(0).at("target_id") == "card-3");
  CHECK(outgoing.at(0).at("target_project_id") == "allowed");
  const auto incoming =
      query_card("list_card_backlinks", selection).at("structuredContent").at("data");
  REQUIRE(incoming.size() == 1);
  CHECK(incoming.at(0).at("source_id") == "card-3");
  CHECK(incoming.at(0).at("source_project_id") == "allowed");
  auto external_scope = selection;
  external_scope["scope"] = "external";
  CHECK(query_card("list_card_backlinks", external_scope).at("isError") == true);
  auto deleted_scope = selection;
  deleted_scope["include_deleted"] = true;
  CHECK(query_card("list_card_links", deleted_scope).at("isError") == true);
  CHECK(
      query_card("list_card_links", {{"project_id", "other"}, {"card_id", "card-2"}})
          .at("isError") == true
  );
  const auto breadcrumbs =
      query_card("get_card_breadcrumbs", selection).at("structuredContent").at("data");
  CHECK(breadcrumbs.back().at("id") == "card-1");
  CHECK(query_card("list_card_milestones", selection).at("structuredContent").at("data").empty());
  const json milestone_body{{"dates", json::array({{{"role", "due"}, {"date", "2026-12-01"}}})}};
  for (const auto* path :
       {"/projects/allowed/cards/card-1/milestones", "/projects/other/cards/card-2/milestones"}) {
    holder::test::http_json_request(
        bound.bind,
        bound.port,
        "rest-test-token",
        http::verb::post,
        path,
        milestone_body,
        http::status::created
    );
  }
  const json window{{"first_day", "2026-12-01"}, {"last_day", "2026-12-02"}, {"time_zone", "UTC"}};
  const auto calendar = query_card("get_calendar", window).at("structuredContent").at("data");
  REQUIRE(calendar.at("milestones").size() == 1);
  CHECK(calendar.at("milestones").at(0).at("card_id") == "card-1");
  const auto rest_calendar = holder::test::http_json_request(
      bound.bind,
      bound.port,
      "rest-test-token",
      http::verb::get,
      "/projects/allowed/calendar?first_day=2026-12-01&last_day=2026-12-02&time_zone=UTC",
      nullptr,
      http::status::ok
  );
  CHECK(calendar == rest_calendar.at("data"));
  auto denied_window = window;
  denied_window["project_id"] = "other";
  CHECK(query_card("get_calendar", denied_window).at("isError") == true);
  auto invalid_window = window;
  invalid_window["first_day"] = "2026-02-30";
  CHECK(query_card("get_calendar", invalid_window).at("isError") == true);
  CHECK(query_card("list_project_tags", {{"project_id", "allowed"}})
            .at("structuredContent")
            .at("data")
            .empty());

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
