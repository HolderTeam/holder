#include "api/GeneratedRoutes.h"
#include "api/support/HttpQuery.h"
#include "card/CardRepo.h"
#include "card/TagRepo.h"
#include "http_test_helpers.h"
#include <cctype>
#include <unordered_map>

namespace {
namespace http = boost::beast::http;
using Params = std::unordered_map<std::string, std::string>;

std::string query_encode(const std::string& value) {
  constexpr char hex[] = "0123456789ABCDEF";
  std::string result;
  for (const char character : value) {
    const auto byte = static_cast<unsigned char>(character);
    if (std::isalnum(byte) || byte == '-' || byte == '_' || byte == '.' || byte == '~') {
      result += character;
    } else {
      result += '%';
      result += hex[byte >> 4];
      result += hex[byte & 15];
    }
  }
  return result;
}

// Routes a request through the generated routes, as the daemon does.
// An empty value leaves the parameter out.
std::pair<http::status, nlohmann::json> route_project_cards(
    holder::platform::Db& db,
    holder::card::CardStore* card_store,
    const std::string& path,
    const Params& params = {},
    http::verb method = http::verb::get
) {
  std::string target = path;
  char separator = '?';
  for (const auto& [key, value] : params) {
    if (value.empty()) continue;
    target += separator + key + "=" + query_encode(value);
    separator = '&';
  }
  http::request<http::string_body> req{method, target, 11};
  http::response<http::string_body> res;
  REQUIRE(holder::api::dispatch_generated_routes(
      req, res, db, nullptr, nullptr, nullptr, nullptr, {}, {}, card_store));
  return std::make_pair(res.result(), nlohmann::json::parse(res.body()));
}
} // namespace

TEST_CASE(
    "Project card collection pages combine filters and preserve display behaviour",
    "[card-pages]"
) {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  holder::test::create_project(db, "p", (dir / "repo").string());
  holder::test::create_project(db, "other", (dir / "other").string());
  holder::index::FtsIndexer fts(db);
  holder::card::CardStore store(db, &fts);
  const std::string first = "00000000-0000-4000-8000-000000000001";
  const std::string second = "00000000-0000-4000-8000-000000000002";
  const std::string third = "00000000-0000-4000-8000-000000000003";
  const std::string deleted = "00000000-0000-4000-8000-000000000004";
  const std::string foreign = "00000000-0000-4000-8000-000000000005";
  holder::test::create_card_fixture(store, first, "p", "one", "one\n#work", 10);
  holder::test::create_card_fixture(store, second, "p", "two", "two\n#work", 20, first);
  holder::test::create_card_fixture(store, third, "p", "three", "three\n#work", 20, first);
  holder::test::create_card_fixture(store, deleted, "p", "deleted", "deleted\n#work", 40);
  holder::test::create_card_fixture(store, foreign, "other", "foreign", "foreign\n#work", 50);
  store.trash(deleted, 60);
  auto call = [&](Params params,
                  std::string path = "/projects/p/cards",
                  http::verb method = http::verb::get) {
    return route_project_cards(db, nullptr, path, params, method);
  };
  auto [status, payload] = call({{"limit", "2"}});
  REQUIRE(status == http::status::ok);
  REQUIRE(payload["data"]["items"].size() == 2);
  CHECK(payload["data"]["items"][0]["card_id"] == first);
  CHECK(payload["data"]["items"][1]["card_id"] == second);
  const auto cursor = payload["data"]["next_cursor"].get<std::string>();
  auto [next_status, next] = call({{"limit", "2"}, {"cursor", cursor}});
  REQUIRE(next_status == http::status::ok);
  REQUIRE(next["data"]["items"].size() == 1);
  CHECK(next["data"]["items"][0]["card_id"] == third);
  CHECK(next["data"]["next_cursor"].is_null());
  CHECK(call({{"limit", "3"}}).second["data"]["next_cursor"].is_null());
  CHECK(call({{"cursor", cursor}}, "/projects/other/cards").first == http::status::bad_request);
  for (const auto& change :
       Params{{"tag", "work"}, {"parent", "roots"}, {"order", "updated_desc"}}) {
    Params params{change, {"cursor", cursor}};
    CHECK(call(params).first == http::status::bad_request);
  }
  Params filtered{{"tag", "Work"}, {"parent", first}, {"order", "updated_desc"}, {"limit", "1"}};
  auto page = call(filtered).second["data"];
  REQUIRE(page["items"].size() == 1);
  CHECK(page["items"][0]["card_id"] == third);
  filtered["cursor"] = page["next_cursor"].get<std::string>();
  filtered["tag"] = "WORK";
  page = call(filtered).second["data"];
  REQUIRE(page["items"].size() == 1);
  CHECK(page["items"][0]["card_id"] == second);
  CHECK(page["next_cursor"].is_null());
  CHECK(call({{"tag", "work"}, {"parent", "roots"}}).second["data"]["items"].size() == 1);
  CHECK(call({}).second["data"]["items"].size() == 3);
  CHECK(call({{"tag", "work"}}).second["data"]["items"].size() == 3);
  CHECK(call({{"tag", "WORK"}}).second["data"] == call({{"tag", "work"}}).second["data"]);
  CHECK(call({{"tag", "missing"}}).second["data"]["items"].empty());
  for (const auto& limit : {"0", "5001", "bad", "2junk", "-1", "99999999999999999"})
    CHECK(call({{"limit", limit}}).first == http::status::bad_request);
  for (const auto& bad :
       Params{{"cursor", "bad"}, {"order", "bad"}, {"parent", "bad"}, {"tag", "#work"}})
    CHECK(call({bad}).first == http::status::bad_request);
  CHECK(call({}, "/projects/missing/cards").first == http::status::not_found);
  CHECK(call({}, "/projects/p/cards", http::verb::post).first == http::status::method_not_allowed);

  // The display endpoint retains its original hierarchy, recent mode and tag precedence.
  auto display = [&](Params params) {
    params["project_id"] = "p";
    std::string target = "/cards";
    char separator = '?';
    for (const auto& [key, value] : params) {
      target += separator + key + "=" + value;
      separator = '&';
    }
    http::request<http::string_body> req{http::verb::get, target, 11};
    http::response<http::string_body> res;
    REQUIRE(holder::api::dispatch_generated_routes(
        req, res, db, nullptr, &fts, nullptr, nullptr, {}, {}, &store
    ));
    return std::make_pair(res.result(), nlohmann::json::parse(res.body()));
  };
  CHECK(display({}).second["data"].size() == 1);
  CHECK(display({{"parent_card_id", first}}).second["data"].size() == 2);
  CHECK(display({{"tag", "work"}, {"parent_card_id", first}}).second["data"].size() == 3);
  CHECK(display({{"view", "recent"}, {"limit", "2"}}).second["data"].size() == 2);
  CHECK(display({{"view", "recent"}, {"limit", "2junk"}}).first == http::status::bad_request);
  CHECK(display({{"view", "all"}}).first == http::status::bad_request);
  CHECK(display({{"after_card_id", third}}).second["data"].size() == 1);

  // Real trash promotes live children to roots; flat pages still expose them.
  store.trash(first, 70);
  page = call({{"limit", "1"}}).second["data"];
  REQUIRE(page["items"].size() == 1);
  CHECK(page["items"][0]["card_id"] == second);
  REQUIRE(page["next_cursor"].is_string());
  page = call({{"limit", "1"}, {"cursor", page["next_cursor"].get<std::string>()}}).second["data"];
  REQUIRE(page["items"].size() == 1);
  CHECK(page["items"][0]["card_id"] == third);
  CHECK(page["next_cursor"].is_null());
  const auto roots = call({{"parent", "roots"}}).second["data"]["items"];
  REQUIRE(roots.size() == 2);
  CHECK(roots[0]["card_id"] == second);
  CHECK(roots[1]["card_id"] == third);
  CHECK(roots[0]["parent_card_id"].is_null());
  CHECK(roots[1]["parent_card_id"].is_null());
  CHECK(call({{"parent", first}}).second["data"]["items"].empty());
}

TEST_CASE("Project card pages follow sibling sort keys across cursors", "[card-pages]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  holder::test::create_project(db, "p", (dir / "repo").string());
  holder::index::FtsIndexer fts(db);
  holder::card::CardStore store(db, &fts);
  const std::string parent = "00000000-0000-4000-8000-000000000010";
  const std::string first = "00000000-0000-4000-8000-000000000003";
  const std::string second = "00000000-0000-4000-8000-000000000001";
  const std::string third = "00000000-0000-4000-8000-000000000002";
  holder::test::create_card_fixture(store, parent, "p", "parent", "parent", 1);
  holder::test::create_card_fixture(store, first, "p", "first", "first", 2, parent, 10.0);
  holder::test::create_card_fixture(store, second, "p", "second", "second", 3, parent, 20.0);
  holder::test::create_card_fixture(store, third, "p", "third", "third", 4, parent, 20.0);
  auto call = [&](Params params) {
    return route_project_cards(db, &store, "/projects/p/cards", params);
  };
  Params params{{"parent", parent}, {"order", "sort_key_asc"}, {"limit", "1"}};
  auto [status, payload] = call(params);
  REQUIRE(status == http::status::ok);
  CHECK(payload["data"]["items"][0]["card_id"] == first);
  params["cursor"] = payload["data"]["next_cursor"].get<std::string>();
  payload = call(params).second;
  CHECK(payload["data"]["items"][0]["card_id"] == second);
  params["cursor"] = payload["data"]["next_cursor"].get<std::string>();
  params["include_content"] = "true";
  payload = call(params).second;
  CHECK(payload["data"]["items"][0]["card_id"] == third);
  CHECK(payload["data"]["items"][0]["content"] == "third");
  CHECK(payload["data"]["next_cursor"].is_null());
  params["order"] = "updated_desc";
  CHECK(call(params).first == http::status::bad_request);
}

TEST_CASE(
    "Project card pages include requested child counts and live breadcrumbs",
    "[card-pages]"
) {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  holder::test::create_project(db, "p", (dir / "repo").string());
  holder::test::create_project(db, "other", (dir / "other").string());
  holder::index::FtsIndexer fts(db);
  holder::card::CardStore store(db, &fts);
  const std::string parent = "00000000-0000-4000-8000-000000000010";
  const std::string child = "00000000-0000-4000-8000-000000000011";
  holder::test::create_card_fixture(store, parent, "p", "Parent", "parent body", 1);
  holder::test::create_card_fixture(store, child, "p", "Child", "child body", 2, parent);
  auto call = [&](const std::string& path,
                  Params params = {},
                  http::verb method = http::verb::get) {
    return route_project_cards(db, &store, path, params, method);
  };
  const auto list_path = std::string("/projects/p/cards");
  const auto plain = call(list_path, {{"parent", "roots"}}).second["data"]["items"];
  REQUIRE(plain.size() == 1);
  CHECK_FALSE(plain[0].contains("child_count"));
  for (const auto& content : {"false", "true"}) {
    const auto items =
        call(
            list_path,
            {{"parent", "roots"}, {"include_child_count", "true"}, {"include_content", content}}
        ).second["data"]["items"];
    REQUIRE(items.size() == 1);
    CHECK(items[0]["child_count"] == 1);
  }
  const auto children =
      call(list_path, {{"parent", parent}, {"include_child_count", "1"}}).second["data"]["items"];
  CHECK(children[0]["child_count"] == 0);
  CHECK(call(list_path, {{"include_child_count", "yes"}}).first == http::status::bad_request);

  const auto path = "/projects/p/cards/" + child + "/breadcrumbs";
  const auto [status, response] = call(path);
  REQUIRE(status == http::status::ok);
  REQUIRE(response["data"].size() == 3);
  CHECK(
      response["data"][0] == nlohmann::json{{"kind", "project"}, {"id", "p"}, {"title", "Project"}}
  );
  CHECK(response["data"][1]["id"] == parent);
  CHECK(response["data"][1]["title"] == "Parent");
  CHECK(response["data"][2]["id"] == child);
  CHECK(call(path, {}, http::verb::post).first == http::status::method_not_allowed);
  CHECK(call("/projects/other/cards/" + child + "/breadcrumbs").first == http::status::not_found);
  holder::card::CardRepo(db).move(parent, child, 0.0, 3);
  const auto cyclic = call(path);
  CHECK(cyclic.first == http::status::conflict);
  CHECK(cyclic.second["error"]["code"] == "hierarchy_invalid");
  holder::card::CardRepo(db).move(parent, std::nullopt, 0.0, 4);
  holder::card::CardRepo(db).soft_delete(parent, 3, 3);
  const auto broken = call(path);
  CHECK(broken.first == http::status::conflict);
  CHECK(broken.second["error"]["code"] == "hierarchy_invalid");
}

TEST_CASE("Project query values decode once and match complete parameter names", "[http-query]") {
  using holder::api::support::decoded_query_param_value;
  CHECK(
      decoded_query_param_value("name=Collection+integration", "name") == "Collection integration"
  );
  CHECK(decoded_query_param_value("name=caf%C3%A9%20%26%2B", "name") == "café &+");
  CHECK(decoded_query_param_value("name=%252B", "name") == "%2B");
  CHECK(decoded_query_param_value("other_name=wrong&name=right", "name") == "right");
}

TEST_CASE("Project card collection pages can include card content", "[card-pages]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  holder::test::create_project(db, "p", (dir / "repo").string());
  holder::index::FtsIndexer fts(db);
  holder::card::CardStore store(db, &fts);
  const std::string first = "00000000-0000-4000-8000-000000000001";
  const std::string second = "00000000-0000-4000-8000-000000000002";
  holder::test::create_card_fixture(store, first, "p", "one", "one body\n#work", 10);
  holder::test::create_card_fixture(store, second, "p", "two", "two body", 20);
  auto call = [&](Params params, holder::card::CardStore* card_store) {
    return route_project_cards(db, card_store, "/projects/p/cards", params);
  };

  auto [status, payload] = call({{"include_content", "true"}, {"limit", "1"}}, &store);
  REQUIRE(status == http::status::ok);
  REQUIRE(payload["data"]["items"].size() == 1);
  CHECK(payload["data"]["items"][0]["card_id"] == first);
  CHECK(payload["data"]["items"][0]["content"] == "one body\n#work");
  // The cursor is the same as for metadata pages, so the two can be mixed.
  const auto cursor = payload["data"]["next_cursor"].get<std::string>();
  auto [next_status, next] = call({{"include_content", "1"}, {"cursor", cursor}}, &store);
  REQUIRE(next_status == http::status::ok);
  REQUIRE(next["data"]["items"].size() == 1);
  CHECK(next["data"]["items"][0]["content"] == "two body");
  CHECK(next["data"]["next_cursor"].is_null());
  CHECK(call({{"cursor", cursor}}, &store).second["data"]["items"][0]["card_id"] == second);

  // Metadata pages carry no content field.
  for (const auto& off : {"false", "0", ""}) {
    const auto metadata = call({{"include_content", off}}, &store).second;
    REQUIRE(metadata["data"]["items"].size() == 2);
    CHECK_FALSE(metadata["data"]["items"][0].contains("content"));
  }
  CHECK(call({{"include_content", "yes"}}, &store).first == http::status::bad_request);
  CHECK(
      call({{"include_content", "true"}, {"limit", "1001"}}, &store).first ==
      http::status::bad_request
  );
  CHECK(call({{"limit", "1001"}}, &store).first == http::status::ok);
  CHECK(call({{"include_content", "true"}}, nullptr).first == http::status::not_implemented);
}

TEST_CASE("Project hierarchy repair reports and fixes hidden cards", "[card-pages][repair]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  holder::test::create_project(db, "p", (dir / "repo").string());
  holder::index::FtsIndexer fts(db);
  holder::card::CardStore store(db, &fts);
  const std::string parent = "00000000-0000-4000-8000-000000000011";
  const std::string child = "00000000-0000-4000-8000-000000000012";
  holder::test::create_card_fixture(store, parent, "p", "parent", "parent", 10);
  holder::test::create_card_fixture(store, child, "p", "child", "child", 20, parent);
  // Hide the child: its parent is trashed in the database, as an older client's trash left it.
  holder::card::CardRepo(db).soft_delete(parent, 30, 30);

  auto call = [&](const std::string& body,
                  http::verb method = http::verb::post,
                  const std::string& path = "/projects/p/repair-hierarchy",
                  holder::card::CardStore* card_store = nullptr) {
    http::request<http::string_body> req{method, path, 11};
    req.body() = body;
    http::response<http::string_body> res;
    REQUIRE(holder::api::dispatch_generated_routes(req, res, db, nullptr, nullptr, nullptr,
                                                   nullptr, {}, {},
                                                   card_store ? card_store : &store));
    return std::make_pair(res.result(), nlohmann::json::parse(res.body()));
  };

  auto [plan_status, plan] = call("");
  REQUIRE(plan_status == http::status::ok);
  CHECK(plan["data"]["applied"] == false);
  CHECK(plan["data"]["changed_card_count"] == 1);
  REQUIRE(plan["data"]["moved"].size() == 1);
  CHECK(plan["data"]["moved"][0]["card_id"] == child);
  CHECK(plan["data"]["moved"][0]["parent_card_id"].is_null());
  CHECK(plan["data"]["cycle_card_ids"].empty());
  CHECK(store.get(child)->parent_card_id == parent);

  auto [apply_status, applied] = call(R"({"apply": true})");
  REQUIRE(apply_status == http::status::ok);
  CHECK(applied["data"]["applied"] == true);
  CHECK_FALSE(store.get(child)->parent_card_id.has_value());
  CHECK(call(R"({"apply": true})").second["data"]["applied"] == false);

  CHECK(call(R"({"apply": "yes"})").first == http::status::bad_request);
  CHECK(call("[]").first == http::status::bad_request);
  CHECK(call("{").first == http::status::bad_request);
  CHECK(call("", http::verb::get).first == http::status::method_not_allowed);
  CHECK(
      call("", http::verb::post, "/projects/missing/repair-hierarchy").first ==
      http::status::not_found
  );

  http::request<http::string_body> no_store{http::verb::post, "/projects/p/repair-hierarchy", 11};
  http::response<http::string_body> res;
  REQUIRE(holder::api::dispatch_generated_routes(no_store, res, db));
  CHECK(res.result() == http::status::not_implemented);
}
