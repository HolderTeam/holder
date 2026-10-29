#if __has_include(<catch2/catch_test_macros.hpp>)
#include <catch2/catch_test_macros.hpp>
#else
#include <catch2/catch.hpp>
#endif

#include "ai/AiThreadRepo.h"
#include "api/GeneratedRoutes.h"
#include "card/CardStore.h"
#include "card/LinkRepo.h"
#include "http_test_helpers.h"
#include "identity/Uuid.h"
#include "index/FtsIndexer.h"
#include "resource/ResourceRepo.h"

#include "model/AiThread.h"
#include "model/Resource.h"

#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

#include <cctype>
#include <filesystem>
#include <fstream>
#include <regex>
#include <functional>
#include <iterator>
#include <tuple>
#include <unordered_map>

namespace {
namespace http = boost::beast::http;

http::request<http::string_body> make_request(
    http::verb method,
    const std::string& target,
    const std::string& body = ""
) {
  http::request<http::string_body> req{method, target, 11};
  req.set(http::field::host, "127.0.0.1");
  req.body() = body;
  req.prepare_payload();
  return req;
}

std::function<std::string(const std::string&)> map_param_getter(
    const std::unordered_map<std::string, std::string>& params
) {
  return [params](const std::string& key) {
    const auto it = params.find(key);
    return it == params.end() ? std::string() : it->second;
  };
}


// Routes a request through the generated routes, as the daemon does.
// The query comes from the parameter function (an empty value is left out).
template <typename Param>
bool route_card(
    const std::string& path,
    const http::request<http::string_body>& req,
    http::response<http::string_body>& res,
    holder::platform::Db& db,
    holder::card::CardStore* card_store,
    holder::index::FtsIndexer* fts,
    const std::function<std::string()>& uuid_v4,
    const Param& param_get
) {
  {
    std::string target = path;
    char separator = '?';
    for (const char* key :
         {"project_id", "tag", "view", "parent_card_id", "order", "count", "limit",
          "include_deleted", "children"}) {
      const std::string value = param_get(key);
      if (value.empty()) continue;
      target += separator;
      target += key;
      target += '=';
      for (const unsigned char ch : value) {
        if (std::isalnum(ch) != 0 || ch == '-' || ch == '_' || ch == '.' || ch == '~') {
          target += static_cast<char>(ch);
        } else {
          static constexpr char hex[] = "0123456789ABCDEF";
          target += '%';
          target += hex[ch >> 4U];
          target += hex[ch & 0x0FU];
        }
      }
      separator = '&';
    }
    http::request<http::string_body> generated{req.method(), target, 11};
    generated.body() = req.body();
    if (holder::api::dispatch_generated_routes(
            generated, res, db, nullptr, fts, nullptr, nullptr, uuid_v4, {}, card_store
        )) {
      return true;
    }
  }
  return false;
}

} // namespace

TEST_CASE("CardRoutes returns false for non-card paths", "[card-routes]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");

  auto req = make_request(http::verb::get, "/not-cards");
  http::response<http::string_body> res;
  const auto uuid_v4 = []() {
    return std::string("generated-id");
  };
  const auto param_get = [](const std::string&) {
    return std::string();
  };

  const bool handled = route_card(
      "/not-cards",
      req,
      res,
      db,
      nullptr,
      nullptr,
      uuid_v4,
      param_get
  );
  REQUIRE_FALSE(handled);
}

TEST_CASE("CardRoutes post generates card ids from persisted project schemes", "[card-routes]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  holder::test::create_project(
      db,
      "project-v4",
      (dir / "project-v4").string(),
      holder::model::IdScheme::Uuid4
  );
  holder::test::create_project(
      db,
      "project-v7",
      (dir / "project-v7").string(),
      holder::model::IdScheme::Uuid7
  );

  holder::index::FtsIndexer fts(db);
  holder::card::CardStore card_store(db, &fts);
  bool legacy_generator_called = false;
  const auto legacy_generator = [&legacy_generator_called]() {
    legacy_generator_called = true;
    return std::string("not-a-uuid");
  };
  const auto param_get = [](const std::string&) {
    return std::string();
  };
  auto post = [&](const std::string& project_id, const nlohmann::json& body) {
    const auto path = "/projects/" + project_id + "/cards";
    auto req = make_request(http::verb::post, path, body.dump());
    http::response<http::string_body> res;
    REQUIRE(route_card(
        path,
        req,
        res,
        db,
        &card_store,
        &fts,
        legacy_generator,
        param_get
    ));
    return std::make_pair(res.result(), nlohmann::json::parse(res.body()));
  };

  const auto [v4_status, v4_payload] = post(
      "project-v4", {{"title", "Version 4"}, {"content", "content"}}
  );
  REQUIRE(v4_status == http::status::created);
  REQUIRE(v4_payload["ok"] == true);
  const auto v4_id = v4_payload["data"]["card_id"].get<std::string>();
  REQUIRE(holder::identity::is_valid_uuid(v4_id));
  REQUIRE(v4_id.at(14) == '4');

  const auto [child_status, child_payload] = post(
      "project-v4",
      {{"title", "Child"},
       {"content", "content"},
       {"parent_card_id", v4_id}}
  );
  REQUIRE(child_status == http::status::created);
  REQUIRE(card_store.get(child_payload["data"]["card_id"])->parent_card_id == v4_id);

  const auto [v7_status, v7_payload] = post(
      "project-v7", {{"title", "Version 7"}, {"content", "content"}}
  );
  REQUIRE(v7_status == http::status::created);
  REQUIRE(v7_payload["ok"] == true);
  const auto v7_id = v7_payload["data"]["card_id"].get<std::string>();
  REQUIRE(holder::identity::is_valid_uuid(v7_id));
  REQUIRE(v7_id.at(14) == '7');
  REQUIRE_FALSE(legacy_generator_called);

  const auto [missing_status, missing_payload] = post(
      "missing-project", {{"title", "Missing"}, {"content", "content"}}
  );
  REQUIRE(missing_status == http::status::not_found);
  REQUIRE(missing_payload["error"]["code"] == "not_found");
  REQUIRE(missing_payload["error"]["message"] == "project not found: missing-project");

  holder::card::CardRepo card_repo(db);
  REQUIRE(card_repo.get(v4_id).has_value());
  REQUIRE(card_repo.get(v7_id).has_value());

  const std::string supplied_id = "11111111-1111-4111-8111-111111111111";
  const auto [rejected_status, rejected_payload] = post(
      "project-v4",
      {{"card_id", supplied_id},
       {"title", "Caller selected"},
       {"content", "content"}}
  );
  REQUIRE(rejected_status == http::status::bad_request);
  REQUIRE(rejected_payload["error"]["code"] == "invalid_argument");
  REQUIRE(rejected_payload["error"]["message"] == "NewCard has no field card_id");
  REQUIRE_FALSE(card_repo.get(supplied_id).has_value());
}

TEST_CASE("OpenAPI card create request does not expose card_id", "[openapi]") {
  std::ifstream input(OPENAPI_YAML_PATH);
  REQUIRE(input.is_open());
  const std::string document(
      (std::istreambuf_iterator<char>(input)),
      std::istreambuf_iterator<char>()
  );

  // The contract's NewCard, up to the next schema: Holder generates the ID.
  const auto request_begin = document.find("\n    NewCard:");
  REQUIRE(request_begin != std::string::npos);
  std::smatch next;
  const std::string rest = document.substr(request_begin + 1);
  REQUIRE(std::regex_search(rest.cbegin() + 1, rest.cend(), next, std::regex("\n    [A-Za-z]")));
  const auto request_schema = rest.substr(0, static_cast<std::size_t>(next.position(0)) + 1);
  REQUIRE(request_schema.find("\n        card_id:") == std::string::npos);
  REQUIRE(request_schema.find("\n        title:") != std::string::npos);
  REQUIRE(request_schema.find("additionalProperties: false") != std::string::npos);
}

TEST_CASE("CardRoutes item get and patch validation branches", "[card-routes]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  const auto project_root = dir / "repo";
  holder::test::create_project(db, "proj-1", project_root.string());
  holder::index::FtsIndexer fts(db);
  holder::card::CardStore card_store(db, &fts);

  const auto uuid_v4 = []() {
    return std::string("generated-id");
  };
  const auto param_get = [](const std::string&) {
    return std::string();
  };

  auto call = [&](http::verb method, const std::string& path, const nlohmann::json& body) {
    auto req = make_request(method, path, body.dump());
    http::response<http::string_body> res;
    const bool handled = route_card(
        path,
        req,
        res,
        db,
        &card_store,
        &fts,
        uuid_v4,
        param_get
    );
    REQUIRE(handled);
    return std::make_pair(res.result(), nlohmann::json::parse(res.body()));
  };

  auto create_card = [&](const std::string& card_id, const std::string& title, int t) {
    holder::test::create_card_fixture(card_store, card_id, "proj-1", title, title, t);
  };

  create_card("11111111-1111-4111-8111-111111111111", "A", 10);

  SECTION("get card not found") {
    auto [status, payload] = call(
        http::verb::get,
        "/projects/proj-1/cards/ffffffff-ffff-4fff-8fff-ffffffffffff",
        nlohmann::json::object()
    );
    REQUIRE(status == http::status::not_found);
    REQUIRE(payload["error"]["code"] == "not_found");
  }

  SECTION("get card content missing") {
    create_card("22222222-2222-4222-8222-222222222222", "B", 11);
    holder::card::CardRepo card_repo(db);
    const auto created = card_repo.get("22222222-2222-4222-8222-222222222222");
    REQUIRE(created.has_value());
    const auto rel = created->rel_path;
    std::filesystem::remove(project_root / rel);

    auto [status, payload] = call(
        http::verb::get,
        "/projects/proj-1/cards/22222222-2222-4222-8222-222222222222",
        nlohmann::json::object()
    );
    REQUIRE(status == http::status::not_found);
    REQUIRE(payload["error"]["code"] == "not_found");
  }

  SECTION("post to item route is not allowed") {
    auto [status, payload] = call(
        http::verb::post,
        "/projects/proj-1/cards/11111111-1111-4111-8111-111111111111",
        nlohmann::json::object()
    );
    REQUIRE(status == http::status::method_not_allowed);
    REQUIRE(payload["error"]["code"] == "method_not_allowed");
  }

  SECTION("unknown tail route matches no route") {
    const std::string path = "/cards/11111111-1111-4111-8111-111111111111/unknown";
    auto req = make_request(http::verb::get, path);
    http::response<http::string_body> res;
    REQUIRE_FALSE(route_card(path, req, res, db, &card_store, &fts, uuid_v4, param_get));
  }
}

TEST_CASE("CardRoutes DELETE trashes children by the requested policy", "[card-routes]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  holder::test::create_project(db, "proj-1", (dir / "repo").string());
  holder::index::FtsIndexer fts(db);
  holder::card::CardStore card_store(db, &fts);
  const std::string parent = "77777777-7777-4777-8777-777777777771";
  const std::string child = "77777777-7777-4777-8777-777777777772";
  const std::string grandchild = "77777777-7777-4777-8777-777777777773";
  holder::test::create_card_fixture(card_store, parent, "proj-1", "Parent", "parent", 10);
  holder::test::create_card_fixture(card_store, child, "proj-1", "Child", "child", 11, parent);
  holder::test::create_card_fixture(
      card_store,
      grandchild,
      "proj-1",
      "Grandchild",
      "grandchild",
      12,
      child
  );
  auto call = [&](const std::string& card_id, const std::string& children) {
    const std::string path = "/projects/proj-1/cards/" + card_id;
    auto req = make_request(http::verb::delete_, path, "");
    http::response<http::string_body> res;
    REQUIRE(route_card(
        path,
        req,
        res,
        db,
        &card_store,
        &fts,
        [] {
          return std::string("generated-id");
        },
        [&](const std::string& name) {
          return name == "children" ? children : std::string();
        }
    ));
    return std::make_pair(res.result(), nlohmann::json::parse(res.body()));
  };

  // An unknown policy is rejected before anything changes.
  auto [bad_status, bad] = call(parent, "everything");
  REQUIRE(bad_status == http::status::bad_request);
  REQUIRE_FALSE(card_store.get(parent)->deleted_at.has_value());

  std::string policy;
  SECTION("promote by default") { policy = ""; }
  SECTION("promote explicitly") { policy = "promote"; }
  SECTION("trash_subtree") { policy = "trash_subtree"; }
  auto [status, payload] = call(parent, policy);
  REQUIRE(status == http::status::ok);
  if (policy != "trash_subtree") {
    REQUIRE(card_store.get(parent)->deleted_at.has_value());
    REQUIRE_FALSE(card_store.get(child)->deleted_at.has_value());
    REQUIRE_FALSE(card_store.get(child)->parent_card_id.has_value());
    REQUIRE(card_store.get(grandchild)->parent_card_id == child);
  } else {
    for (const auto& id : {parent, child, grandchild})
      REQUIRE(card_store.get(id)->deleted_at.has_value());
    REQUIRE(card_store.get(grandchild)->parent_card_id == child);
  }
}
