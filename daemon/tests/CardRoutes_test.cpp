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

TEST_CASE("CardRoutes context errors for missing and unknown project", "[card-routes]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");

  const auto uuid_v4 = []() {
    return std::string("generated-id");
  };

  SECTION("missing project_id") {
    auto req = make_request(http::verb::get, "/cards/context");
    http::response<http::string_body> res;
    const auto param_get = map_param_getter({});

    const bool handled = route_card(
        "/cards/context",
        req,
        res,
        db,
        nullptr,
        nullptr,
        uuid_v4,
        param_get
    );
    REQUIRE(handled);
    REQUIRE(res.result() == http::status::bad_request);
  }

  SECTION("project not found") {
    auto req = make_request(http::verb::get, "/cards/context");
    http::response<http::string_body> res;
    const auto param_get = map_param_getter({{"project_id", "missing-proj"}});

    const bool handled = route_card(
        "/cards/context",
        req,
        res,
        db,
        nullptr,
        nullptr,
        uuid_v4,
        param_get
    );
    REQUIRE(handled);
    REQUIRE(res.result() == http::status::not_found);
  }
}

TEST_CASE("CardRoutes recent view validates limit param", "[card-routes]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");

  holder::test::create_project(db, "proj-1", (dir / "repo").string());

  auto req = make_request(http::verb::get, "/cards");
  http::response<http::string_body> res;
  const auto uuid_v4 = []() {
    return std::string("generated-id");
  };
  const auto param_get = map_param_getter({
      {"project_id", "proj-1"},
      {"view", "recent"},
      {"limit", "0"},
  });

  const bool handled = route_card(
      "/cards",
      req,
      res,
      db,
      nullptr,
      nullptr,
      uuid_v4,
      param_get
  );
  REQUIRE(handled);
  REQUIRE(res.result() == http::status::bad_request);
  const auto payload = nlohmann::json::parse(res.body());
  REQUIRE(payload["ok"] == false);
  REQUIRE(payload["error"]["code"] == "bad_request");
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
  auto post = [&](const nlohmann::json& body) {
    auto req = make_request(http::verb::post, "/cards", body.dump());
    http::response<http::string_body> res;
    REQUIRE(route_card(
        "/cards",
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
      {{"project_id", "project-v4"}, {"title", "Version 4"}, {"content", "content"}}
  );
  REQUIRE(v4_status == http::status::created);
  REQUIRE(v4_payload["ok"] == true);
  const auto v4_id = v4_payload["data"]["card_id"].get<std::string>();
  REQUIRE(holder::identity::is_valid_uuid(v4_id));
  REQUIRE(v4_id.at(14) == '4');

  const auto [child_status, child_payload] = post(
      {{"project_id", "project-v4"},
       {"title", "Child"},
       {"content", "content"},
       {"parent_card_id", v4_id}}
  );
  REQUIRE(child_status == http::status::created);
  REQUIRE(card_store.get(child_payload["data"]["card_id"])->parent_card_id == v4_id);

  const auto [v7_status, v7_payload] = post(
      {{"project_id", "project-v7"}, {"title", "Version 7"}, {"content", "content"}}
  );
  REQUIRE(v7_status == http::status::created);
  REQUIRE(v7_payload["ok"] == true);
  const auto v7_id = v7_payload["data"]["card_id"].get<std::string>();
  REQUIRE(holder::identity::is_valid_uuid(v7_id));
  REQUIRE(v7_id.at(14) == '7');
  REQUIRE_FALSE(legacy_generator_called);

  const auto [missing_status, missing_payload] = post(
      {{"project_id", "missing-project"}, {"title", "Missing"}, {"content", "content"}}
  );
  REQUIRE(missing_status == http::status::not_found);
  REQUIRE(missing_payload["error"]["code"] == "not_found");
  REQUIRE(missing_payload["error"]["message"] == "project not found: missing-project");

  holder::card::CardRepo card_repo(db);
  REQUIRE(card_repo.get(v4_id).has_value());
  REQUIRE(card_repo.get(v7_id).has_value());

  const std::string supplied_id = "11111111-1111-4111-8111-111111111111";
  const auto [rejected_status, rejected_payload] = post(
      {{"card_id", supplied_id},
       {"project_id", "project-v4"},
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

TEST_CASE("CardRoutes links and backlinks edge branches", "[card-routes]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  holder::test::create_project(db, "proj-1", (dir / "repo1").string());
  holder::test::create_project(db, "proj-2", (dir / "repo2").string());
  holder::index::FtsIndexer fts(db);
  holder::card::CardStore card_store(db, &fts);

  const auto uuid_v4 = []() {
    return std::string("generated-id");
  };
  const auto empty_param_get = [](const std::string&) {
    return std::string();
  };

  auto call_with_param_get = [&](http::verb method,
                                 const std::string& path,
                                 const nlohmann::json& body,
                                 const std::function<std::string(const std::string&)>& param_get) {
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
  auto call = [&](http::verb method, const std::string& path, const nlohmann::json& body) {
    return call_with_param_get(method, path, body, empty_param_get);
  };
  auto call_generated_links = [&](const std::string& target) {
    auto req = make_request(http::verb::get, target);
    http::response<http::string_body> res;
    REQUIRE(holder::api::dispatch_generated_routes(req, res, db, nullptr, &fts));
    return std::make_pair(res.result(), nlohmann::json::parse(res.body()));
  };

  auto create_card =
      [&](const std::string& card_id, const std::string& project_id, const std::string& title, int t
      ) {
        holder::test::create_card_fixture(card_store, card_id, project_id, title, title, t);
      };

  create_card("11111111-1111-4111-8111-111111111111", "proj-1", "A", 10);
  create_card("22222222-2222-4222-8222-222222222222", "proj-1", "B", 11);

  holder::ai::AiThreadRepo thread_repo(db);
  holder::model::AiThread th1{
      .thread_id = "thread-1",
      .project_id = "proj-1",
      .card_id = std::nullopt,
      .title = "T1",
      .created_at = 1,
      .updated_at = 1,
  };
  thread_repo.create(th1);
  holder::model::AiThread th2{
      .thread_id = "thread-2",
      .project_id = "proj-2",
      .card_id = std::nullopt,
      .title = "T2",
      .created_at = 1,
      .updated_at = 1,
  };
  thread_repo.create(th2);

  holder::resource::ResourceRepo resource_repo(db);
  holder::model::Resource r1{
      .resource_id = "res-1",
      .project_id = "proj-1",
      .type = "website",
      .label = "R1",
      .metadata = {{"identifier", {"https://example.com/1"}}},
      .created_at = 1,
      .updated_at = 1,
  };
  resource_repo.add(r1);

  db.exec(
      "INSERT INTO ai_messages(message_id,thread_id,role,source,provider,model,content,created_at,deleted_at,prompt_hash,meta_json) "
      "VALUES('msg-proj2','thread-2','assistant','manual_paste',NULL,NULL,'body',101,NULL,NULL,NULL);"
  );

  SECTION("links card not found") {
    auto [status, payload] = call_generated_links(
        "/cards/ffffffff-ffff-4fff-8fff-ffffffffffff/links");
    REQUIRE(status == http::status::not_found);
    REQUIRE(payload["error"]["code"] == "not_found");
  }

  SECTION("links method not allowed") {
    auto [status, payload] = call(
        http::verb::patch,
        "/cards/11111111-1111-4111-8111-111111111111/links",
        nlohmann::json::object()
    );
    REQUIRE(status == http::status::method_not_allowed);
    REQUIRE(payload["error"]["code"] == "method_not_allowed");
  }

  SECTION("links missing to_card_id") {
    auto [status, payload] = call(
        http::verb::post,
        "/cards/11111111-1111-4111-8111-111111111111/links",
        nlohmann::json::object()
    );
    REQUIRE(status == http::status::bad_request);
    REQUIRE(payload["error"]["code"] == "bad_request");
  }

  SECTION("links default to_type card") {
    auto [status, payload] = call(
        http::verb::post,
        "/cards/11111111-1111-4111-8111-111111111111/links",
        {{"to_card_id", "22222222-2222-4222-8222-222222222222"}}
    );
    REQUIRE(status == http::status::created);
    REQUIRE(payload["data"]["to_type"] == "card");
  }

  SECTION("links unsupported to_type") {
    auto [status, payload] = call(
        http::verb::post,
        "/cards/11111111-1111-4111-8111-111111111111/links",
        {{"to_card_id", "x"}, {"to_type", "banana"}}
    );
    REQUIRE(status == http::status::bad_request);
    REQUIRE(payload["error"]["code"] == "bad_request");
  }

  SECTION("links ai_message target missing") {
    auto [status, payload] = call(
        http::verb::post,
        "/cards/11111111-1111-4111-8111-111111111111/links",
        {{"to_card_id", "msg-missing"}, {"to_type", "ai_message"}}
    );
    REQUIRE(status == http::status::bad_request);
    REQUIRE(payload["error"]["code"] == "bad_request");
  }

  SECTION("links ai_message cross-project target") {
    auto [status, payload] = call(
        http::verb::post,
        "/cards/11111111-1111-4111-8111-111111111111/links",
        {{"to_card_id", "msg-proj2"}, {"to_type", "ai_message"}}
    );
    REQUIRE(status == http::status::bad_request);
    REQUIRE(payload["ok"] == false);
    REQUIRE(payload["error"]["code"] == "bad_request");
  }

  SECTION("links ai_thread cross-project target") {
    auto [status, payload] = call(
        http::verb::post,
        "/cards/11111111-1111-4111-8111-111111111111/links",
        {{"to_card_id", "thread-2"}, {"to_type", "ai_thread"}}
    );
    REQUIRE(status == http::status::bad_request);
    REQUIRE(payload["error"]["code"] == "bad_request");
  }

  SECTION("links resource target success and delete-all") {
    auto [create_status, create_payload] = call(
        http::verb::post,
        "/cards/11111111-1111-4111-8111-111111111111/links",
        {{"to_card_id", "res-1"}, {"to_type", "resource"}}
    );
    REQUIRE(create_status == http::status::created);
    REQUIRE(create_payload["data"]["to_type"] == "resource");

    auto [list_status, list_payload] = call_generated_links(
        "/cards/11111111-1111-4111-8111-111111111111/links");
    REQUIRE(list_status == http::status::ok);
    REQUIRE(list_payload["data"].is_array());
    REQUIRE(list_payload["data"].size() >= 1);

    auto [del_status, del_payload] = call(
        http::verb::delete_,
        "/cards/11111111-1111-4111-8111-111111111111/links",
        nlohmann::json::object()
    );
    REQUIRE(del_status == http::status::ok);
    REQUIRE(del_payload["ok"] == true);
  }

  SECTION("links include_deleted accepts only 0 or 1") {
    const std::string source = "11111111-1111-4111-8111-111111111111";
    const std::string target = "22222222-2222-4222-8222-222222222222";
    auto [create_status, create_payload] = call(
        http::verb::post, "/cards/" + source + "/links", {{"to_card_id", target}}
    );
    REQUIRE(create_status == http::status::created);
    REQUIRE(create_payload["ok"] == true);
    auto [trash_status, trash_payload] = call(
        http::verb::delete_, "/cards/" + target, nlohmann::json::object()
    );
    REQUIRE(trash_status == http::status::ok);
    REQUIRE(trash_payload["ok"] == true);

    const std::string path = "/cards/" + source + "/links";
    auto [filtered_status, filtered_payload] = call_generated_links(path + "?include_deleted=0");
    REQUIRE(filtered_status == http::status::ok);
    REQUIRE(filtered_payload["data"].empty());

    auto [included_status, included_payload] = call_generated_links(path + "?include_deleted=1");
    REQUIRE(included_status == http::status::ok);
    REQUIRE(included_payload["data"].size() == 1);
    REQUIRE(included_payload["data"][0]["to_card_id"] == target);

    auto [invalid_status, invalid_payload] = call_generated_links(path + "?include_deleted=2");
    REQUIRE(invalid_status == http::status::bad_request);
    REQUIRE(invalid_payload["error"]["code"] == "bad_request");
  }

  SECTION("resource attachments validate methods and identifiers") {
    auto [method_status, method_payload] = call(
        http::verb::get,
        "/cards/11111111-1111-4111-8111-111111111111/resources",
        nlohmann::json::object()
    );
    REQUIRE(method_status == http::status::method_not_allowed);
    REQUIRE(method_payload["error"]["code"] == "method_not_allowed");

    auto [fields_status, fields_payload] = call(
        http::verb::post,
        "/cards/11111111-1111-4111-8111-111111111111/resources",
        {{"project_id", ""}, {"resource_id", ""}}
    );
    REQUIRE(fields_status == http::status::bad_request);
    REQUIRE(fields_payload["error"]["code"] == "bad_request");
  }

  SECTION("backlinks card not found and invalid queries") {
    auto [not_found_status, not_found_payload] =
        call_generated_links("/cards/ffffffff-ffff-4fff-8fff-ffffffffffff/backlinks");
    REQUIRE(not_found_status == http::status::not_found);
    REQUIRE(not_found_payload["error"]["code"] == "not_found");
    REQUIRE(not_found_payload["error"]["message"] == "Card not found.");

    for (const std::string query : {"?scope=everywhere", "?scope=", "?include_deleted=2"}) {
      CAPTURE(query);
      auto [status, payload] = call_generated_links(
          "/cards/11111111-1111-4111-8111-111111111111/backlinks" + query
      );
      REQUIRE(status == http::status::bad_request);
      REQUIRE(payload["error"]["code"] == "bad_request");
    }

    // Backlinks have only GET, so the generated routes answer other methods with 405.
    auto [method_status, method_payload] = call(
        http::verb::post,
        "/cards/11111111-1111-4111-8111-111111111111/backlinks",
        nlohmann::json::object()
    );
    REQUIRE(method_status == http::status::method_not_allowed);
    REQUIRE(method_payload["error"]["code"] == "method_not_allowed");
  }

  SECTION("backlinks include_deleted toggles filtering") {
    auto [create_status, create_payload] = call(
        http::verb::post,
        "/cards/22222222-2222-4222-8222-222222222222/links",
        {{"to_card_id", "11111111-1111-4111-8111-111111111111"}}
    );
    REQUIRE(create_status == http::status::created);
    REQUIRE(create_payload["ok"] == true);

    auto [del_status, del_payload] = call(
        http::verb::delete_,
        "/cards/22222222-2222-4222-8222-222222222222",
        nlohmann::json::object()
    );
    REQUIRE(del_status == http::status::ok);
    REQUIRE(del_payload["ok"] == true);

    auto [filtered_status, filtered_payload] =
        call_generated_links("/cards/11111111-1111-4111-8111-111111111111/backlinks");
    REQUIRE(filtered_status == http::status::ok);
    REQUIRE(filtered_payload["data"].is_array());
    REQUIRE(filtered_payload["data"].empty());
    REQUIRE(filtered_payload["scope"] == "local");
    REQUIRE(filtered_payload["coverage"] == "target_project");

    auto [included_status, included_payload] = call_generated_links(
        "/cards/11111111-1111-4111-8111-111111111111/backlinks?include_deleted=1"
    );
    REQUIRE(included_status == http::status::ok);
    REQUIRE(included_payload["data"].is_array());
    REQUIRE(included_payload["data"].size() == 1);
    REQUIRE(included_payload["data"][0]["from_card_id"] == "22222222-2222-4222-8222-222222222222");
  }
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
        "/cards/ffffffff-ffff-4fff-8fff-ffffffffffff",
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
        "/cards/22222222-2222-4222-8222-222222222222",
        nlohmann::json::object()
    );
    REQUIRE(status == http::status::not_found);
    REQUIRE(payload["error"]["code"] == "not_found");
  }

  SECTION("post to item route is not allowed") {
    auto [status, payload] = call(
        http::verb::post,
        "/cards/11111111-1111-4111-8111-111111111111",
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

TEST_CASE("CardRoutes additional uncovered branches", "[card-routes]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  holder::test::create_project(db, "proj-1", (dir / "repo1").string());
  holder::test::create_project(db, "proj-2", (dir / "repo2").string());
  holder::index::FtsIndexer fts(db);
  holder::card::CardStore card_store(db, &fts);

  const auto uuid_v4 = []() {
    return std::string("generated-id");
  };
  const auto empty_param_get = [](const std::string&) {
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
        empty_param_get
    );
    REQUIRE(handled);
    return std::make_pair(res.result(), nlohmann::json::parse(res.body()));
  };
  auto call_with_params = [&](http::verb method,
                              const std::string& path,
                              const nlohmann::json& body,
                              const std::unordered_map<std::string, std::string>& params) {
    auto req = make_request(method, path, body.dump());
    http::response<http::string_body> res;
    const auto param_get = map_param_getter(params);
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

  auto create_card = [&](const std::string& card_id,
                         const std::string& project_id,
                         const std::string& title,
                         int t,
                         const nlohmann::json& extra = nlohmann::json::object()) {
    const auto parent = extra.contains("parent_card_id")
                            ? std::optional<std::string>(
                                  extra.at("parent_card_id").get<std::string>()
                              )
                            : std::nullopt;
    const auto sort_key = extra.contains("sort_key")
                              ? std::optional<double>(extra.at("sort_key").get<double>())
                              : std::nullopt;
    holder::test::create_card_fixture(
        card_store,
        card_id,
        project_id,
        title,
        title,
        t,
        parent,
        sort_key
    );
  };

  create_card("11111111-1111-4111-8111-111111111111", "proj-1", "alpha", 10, {{"sort_key", 100.0}});
  create_card("22222222-2222-4222-8222-222222222222", "proj-1", "Alpha", 10, {{"sort_key", 100.0}});
  create_card("33333333-3333-4333-8333-333333333333", "proj-1", "alpha", 9, {{"sort_key", 100.0}});
  create_card("aaaaaaaa-aaaa-4aaa-8aaa-aaaaaaaaaaaa", "proj-2", "x", 5);

  SECTION("context accepts blank parent and explicit tree_default/count false") {
    auto [status, payload] = call_with_params(
        http::verb::get,
        "/cards/context",
        nlohmann::json::object(),
        {{"project_id", "proj-1"},
         {"parent_card_id", "   \t"},
         {"order", "tree_default"},
         {"count", "0"}}
    );
    REQUIRE(status == http::status::ok);
    REQUIRE(payload["ok"] == true);
    REQUIRE(payload["data"]["current_parent_card_id"].is_null());
  }

  SECTION("recent cards supports explicit tree_default order and count") {
    auto [status, payload] = call_with_params(
        http::verb::get,
        "/cards",
        nlohmann::json::object(),
        {{"project_id", "proj-1"},
         {"view", "recent"},
         {"order", "tree_default"},
         {"count", "1"},
         {"limit", "10"}}
    );
    REQUIRE(status == http::status::ok);
    REQUIRE(payload["ok"] == true);
    REQUIRE(payload["data"].is_array());
    REQUIRE(payload["data"].size() >= 1);
    REQUIRE(payload["data"][0].contains("child_count"));
  }

  SECTION("tree list filters deleted by default and can include child_count") {
    auto [trash_status, trash_payload] = call(
        http::verb::delete_,
        "/cards/33333333-3333-4333-8333-333333333333",
        nlohmann::json::object()
    );
    REQUIRE(trash_status == http::status::ok);
    REQUIRE(trash_payload["ok"] == true);

    auto [status, payload] = call_with_params(
        http::verb::get,
        "/cards",
        nlohmann::json::object(),
        {{"project_id", "proj-1"}, {"view", "tree"}, {"count", "1"}}
    );
    REQUIRE(status == http::status::ok);
    for (const auto& row : payload["data"]) {
      REQUIRE(row["card_id"] != "33333333-3333-4333-8333-333333333333");
      REQUIRE(row.contains("child_count"));
    }
  }

  SECTION("post /cards takes the contract's NewCard and rejects fields it does not have") {
    auto [plain_status, plain_payload] =
        call(http::verb::post, "/cards", {{"project_id", "proj-1"}, {"title", "no content"}});
    REQUIRE(plain_status == http::status::created);
    REQUIRE(plain_payload["data"]["content"] == "");

    auto [bad_status, bad_payload] = call(
        http::verb::post,
        "/cards",
        {{"project_id", "proj-1"},
         {"title", "manual"},
         {"content", "manual"},
         {"created_at", 20},
         {"updated_at", 20},
         {"sort_key", 555.0},
         {"rel_path", "cards/custom.md"}}
    );
    REQUIRE(bad_status == http::status::bad_request);
    REQUIRE(bad_payload["error"]["code"] == "invalid_argument");
    REQUIRE(bad_payload["error"]["message"] == "NewCard has no field rel_path");
  }

  SECTION("backlinks filter unknown source id when include_deleted is false") {
    holder::card::LinkRepo link_repo(db);
    holder::model::CardLink ghost{
        .project_id = "proj-1",
        .from_card_id = "ghost-source",
        .to_card_id = "11111111-1111-4111-8111-111111111111",
        .to_type = "card",
        .kind = "ref",
        .label = std::nullopt,
        .created_at = 1,
    };
    link_repo.upsert_links("proj-1", "ghost-source", {ghost});

    auto req = make_request(http::verb::get, "/cards/11111111-1111-4111-8111-111111111111/backlinks");
    http::response<http::string_body> res;
    REQUIRE(holder::api::dispatch_generated_routes(req, res, db, nullptr, &fts));
    const auto status = res.result();
    const auto payload = nlohmann::json::parse(res.body());
    REQUIRE(status == http::status::ok);
    REQUIRE(payload["data"].is_array());
    REQUIRE(payload["data"].empty());
  }

  SECTION("links delete parses to_type and catches invalid json") {
    auto [create_status, create_payload] = call(
        http::verb::post,
        "/cards/11111111-1111-4111-8111-111111111111/links",
        {{"to_card_id", "22222222-2222-4222-8222-222222222222"}, {"to_type", "card"}}
    );
    REQUIRE(create_status == http::status::created);
    REQUIRE(create_payload["ok"] == true);

    auto req = make_request(
        http::verb::delete_,
        "/cards/11111111-1111-4111-8111-111111111111/links",
        R"({"to_card_id":"22222222-2222-4222-8222-222222222222","to_type":"card"})"
    );
    http::response<http::string_body> res;
    const bool handled = route_card(
        "/cards/11111111-1111-4111-8111-111111111111/links",
        req,
        res,
        db,
        &card_store,
        &fts,
        uuid_v4,
        empty_param_get
    );
    REQUIRE(handled);
    REQUIRE(res.result() == http::status::ok);

    auto bad_req =
        make_request(http::verb::post, "/cards/11111111-1111-4111-8111-111111111111/links", "{");
    http::response<http::string_body> bad_res;
    const bool bad_handled = route_card(
        "/cards/11111111-1111-4111-8111-111111111111/links",
        bad_req,
        bad_res,
        db,
        &card_store,
        &fts,
        uuid_v4,
        empty_param_get
    );
    REQUIRE(bad_handled);
    REQUIRE(bad_res.result() == http::status::bad_request);
  }
}

TEST_CASE("CardRoutes residual branch coverage", "[card-routes]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  holder::test::create_project(db, "proj-1", (dir / "repo1").string());
  holder::test::create_project(db, "proj-2", (dir / "repo2").string());
  holder::index::FtsIndexer fts(db);
  holder::card::CardStore card_store(db, &fts);

  const auto uuid_v4 = []() {
    return std::string("generated-id");
  };
  const auto empty_param_get = [](const std::string&) {
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
        empty_param_get
    );
    REQUIRE(handled);
    return std::make_pair(res.result(), nlohmann::json::parse(res.body()));
  };
  auto call_with_params = [&](http::verb method,
                              const std::string& path,
                              const nlohmann::json& body,
                              const std::unordered_map<std::string, std::string>& params) {
    auto req = make_request(method, path, body.dump());
    http::response<http::string_body> res;
    const auto param_get = map_param_getter(params);
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
  auto create_card = [&](const std::string& card_id,
                         const std::string& project_id,
                         const std::string& title,
                         int t,
                         const nlohmann::json& extra = nlohmann::json::object()) {
    const auto parent = extra.contains("parent_card_id")
                            ? std::optional<std::string>(
                                  extra.at("parent_card_id").get<std::string>()
                              )
                            : std::nullopt;
    const auto sort_key = extra.contains("sort_key")
                              ? std::optional<double>(extra.at("sort_key").get<double>())
                              : std::nullopt;
    holder::test::create_card_fixture(
        card_store,
        card_id,
        project_id,
        title,
        title,
        t,
        parent,
        sort_key
    );
  };

  create_card("00000000-0000-4000-8000-000000000001", "proj-1", "A", 10, {{"sort_key", 100.0}});
  create_card("00000000-0000-4000-8000-000000000002", "proj-1", "B", 10, {{"sort_key", 100.0}});
  create_card("00000000-0000-4000-8000-000000000003", "proj-1", "C", 11, {{"sort_key", 100.00001}});
  create_card("00000000-0000-4000-8000-000000000004", "proj-2", "X", 20);

  SECTION("context returns early on invalid count and catches db failure") {
    auto [bad_count_status, bad_count_payload] = call_with_params(
        http::verb::get,
        "/cards/context",
        nlohmann::json::object(),
        {{"project_id", "proj-1"}, {"count", "maybe"}}
    );
    REQUIRE(bad_count_status == http::status::bad_request);
    REQUIRE(bad_count_payload["error"]["code"] == "bad_request");

    holder::platform::Db unopened;
    auto req = make_request(http::verb::get, "/cards/context");
    http::response<http::string_body> res;
    const auto param_get = map_param_getter({{"project_id", "proj-1"}});
    const bool handled = route_card(
        "/cards/context",
        req,
        res,
        unopened,
        &card_store,
        &fts,
        uuid_v4,
        param_get
    );
    REQUIRE(handled);
    REQUIRE(res.result() == http::status::internal_server_error);
  }

  SECTION("context skips deleted level cards and broken breadcrumb chain") {
    auto [trash_status, trash_payload] = call(
        http::verb::delete_,
        "/cards/00000000-0000-4000-8000-000000000002",
        nlohmann::json::object()
    );
    REQUIRE(trash_status == http::status::ok);
    REQUIRE(trash_payload["ok"] == true);

    // Create a valid chain then delete the ancestor so breadcrumb traversal breaks.
    create_card("00000000-0000-4000-8000-000000000099", "proj-1", "BrokenParent", 12);
    create_card(
        "00000000-0000-4000-8000-000000000100",
        "proj-1",
        "ChildBroken",
        13,
        {{"parent_card_id", "00000000-0000-4000-8000-000000000099"}}
    );
    auto [del_parent_status, del_parent_payload] = call(
        http::verb::delete_,
        "/cards/00000000-0000-4000-8000-000000000099",
        nlohmann::json::object()
    );
    REQUIRE(del_parent_status == http::status::ok);
    REQUIRE(del_parent_payload["ok"] == true);

    auto [status, payload] = call_with_params(
        http::verb::get,
        "/cards/context",
        nlohmann::json::object(),
        {{"project_id", "proj-1"},
         {"parent_card_id", "00000000-0000-4000-8000-000000000100"},
         {"count", "true"}}
    );
    REQUIRE(status == http::status::ok);
    REQUIRE(payload["ok"] == true);
    for (const auto& row : payload["data"]["cards"]) {
      REQUIRE(row["card_id"] != "00000000-0000-4000-8000-000000000002");
    }
  }

  SECTION("recent updated_desc tie-breakers and cards get db catch path") {
    auto [s1, p1] = call_with_params(
        http::verb::get,
        "/cards",
        nlohmann::json::object(),
        {{"project_id", "proj-1"},
         {"view", "recent"},
         {"order", "updated_desc"},
         {"count", "0"},
         {"limit", "10"}}
    );
    REQUIRE(s1 == http::status::ok);
    REQUIRE(p1["ok"] == true);

    holder::platform::Db unopened;
    auto req = make_request(http::verb::get, "/cards");
    http::response<http::string_body> res;
    const auto param_get = map_param_getter({{"project_id", "proj-1"}, {"view", "tree"}});
    const bool handled = route_card(
        "/cards",
        req,
        res,
        unopened,
        &card_store,
        &fts,
        uuid_v4,
        param_get
    );
    REQUIRE(handled);
    REQUIRE(res.result() == http::status::internal_server_error);
  }

  SECTION("backlinks and item get catch branches plus final route-not-found branch") {
    // Create a card in db/card_store and then force route db failure for backlinks.
    create_card("00000000-0000-4000-8000-000000000020", "proj-1", "Back", 40);

    holder::platform::Db unopened;
    auto backlinks_req =
        make_request(http::verb::get, "/cards/00000000-0000-4000-8000-000000000020/backlinks");
    http::response<http::string_body> backlinks_res;
    REQUIRE(holder::api::dispatch_generated_routes(backlinks_req, backlinks_res, unopened, nullptr, &fts));
    REQUIRE(backlinks_res.result() == http::status::internal_server_error);

    // Force item get exception by breaking rel_path invariant.
    db.exec(
        "UPDATE cards SET rel_path='cards/bad-path.md' WHERE card_id='00000000-0000-4000-8000-000000000020';"
    );
    auto get_req = make_request(http::verb::get, "/cards/00000000-0000-4000-8000-000000000020");
    http::response<http::string_body> get_res;
    const bool get_handled = route_card(
        "/cards/00000000-0000-4000-8000-000000000020",
        get_req,
        get_res,
        db,
        &card_store,
        &fts,
        uuid_v4,
        empty_param_get
    );
    REQUIRE(get_handled);
    REQUIRE(get_res.result() == http::status::internal_server_error);

    // Method not handled by any item branch -> final not_found branch.
    auto put_req = make_request(http::verb::put, "/cards/00000000-0000-4000-8000-000000000020");
    http::response<http::string_body> put_res;
    const bool put_handled = route_card(
        "/cards/00000000-0000-4000-8000-000000000020",
        put_req,
        put_res,
        db,
        &card_store,
        &fts,
        uuid_v4,
        empty_param_get
    );
    REQUIRE(put_handled);
    REQUIRE(put_res.result() == http::status::method_not_allowed);
  }
}

TEST_CASE(
    "CardRoutes exposes indexed tags on cards and supports tag filtering",
    "[card-routes][tags]"
) {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  holder::test::create_project(db, "proj-1", (dir / "repo").string());
  holder::index::FtsIndexer fts(db);
  holder::card::CardStore card_store(db, &fts);
  const auto uuid_v4 = []() {
    return std::string("generated-id");
  };

  auto call = [&](http::verb method,
                  const std::string& path,
                  const nlohmann::json& body,
                  const std::unordered_map<std::string, std::string>& params = {}) {
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
        map_param_getter(params)
    );
    REQUIRE(handled);
    return std::make_pair(res.result(), nlohmann::json::parse(res.body()));
  };

  auto create = [&](const std::string& id, const std::string& title, const std::string& content) {
    holder::test::create_card_fixture(card_store, id, "proj-1", title, content, 10);
  };

  create("11111111-1111-4111-8111-111111111111", "Tagged", "Fix #Android and #sync");
  create("22222222-2222-4222-8222-222222222222", "Other", "Only #sync");

  const auto [detail_status, detail] = call(
      http::verb::get,
      "/cards/11111111-1111-4111-8111-111111111111",
      nlohmann::json::object()
  );
  REQUIRE(detail_status == http::status::ok);
  REQUIRE(detail["data"]["tags"] == nlohmann::json::array({"android", "sync"}));
  REQUIRE(detail["data"]["tag_occurrences"].size() == 2);
  REQUIRE(detail["data"]["tag_occurrences"][0]["tag"] == "android");
  REQUIRE(detail["data"]["tag_occurrences"][0]["byte_start"] == 4);
  REQUIRE(detail["data"]["tag_occurrences"][0]["byte_end"] == 12);

  const auto [filter_status, filtered] = call(
      http::verb::get,
      "/cards",
      nlohmann::json::object(),
      {{"project_id", "proj-1"}, {"tag", "android"}}
  );
  REQUIRE(filter_status == http::status::ok);
  REQUIRE(filtered["ok"] == true);
  REQUIRE(filtered["data"].size() == 1);
  REQUIRE(filtered["data"][0]["card_id"] == "11111111-1111-4111-8111-111111111111");
  REQUIRE(filtered["data"][0]["project_id"] == "proj-1");
  REQUIRE(filtered["data"][0]["title"] == "Tagged");
  REQUIRE(filtered["data"][0]["deleted_at"].is_null());

  const auto [exact_status, exact] = call(
      http::verb::get,
      "/cards",
      nlohmann::json::object(),
      {{"project_id", "proj-1"}, {"tag", "Android"}}
  );
  REQUIRE(exact_status == http::status::ok);
  REQUIRE(exact["data"].empty());
}

TEST_CASE("CardRoutes preserves CardStore tag mutation outcomes", "[card-routes][tags]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  holder::test::create_project(db, "proj-1", (dir / "repo-1").string());
  holder::test::create_project(db, "proj-2", (dir / "repo-2").string());
  holder::index::FtsIndexer fts(db);
  holder::card::CardStore card_store(db, &fts);
  const auto uuid_v4 = []() {
    return std::string("generated-id");
  };

  const std::string card_id = "11111111-1111-4111-8111-111111111111";
  const std::string prose_card_id = "22222222-2222-4222-8222-222222222222";
  const std::string other_card_id = "33333333-3333-4333-8333-333333333333";
  const std::string trashed_card_id = "44444444-4444-4444-8444-444444444444";
  holder::test::create_card_fixture(card_store, card_id, "proj-1", "Tagged", "Body", 10);
  holder::test::create_card_fixture(
      card_store,
      prose_card_id,
      "proj-1",
      "Prose",
      "A prose #Reference remains here.",
      11
  );
  holder::test::create_card_fixture(card_store, other_card_id, "proj-2", "Other", "Other body", 12);
  holder::test::create_card_fixture(
      card_store,
      trashed_card_id,
      "proj-1",
      "Trashed",
      "Trashed body",
      13
  );
  card_store.trash(trashed_card_id, 14);

  auto call = [&](const std::string& id, http::verb method, const nlohmann::json& body) {
    auto req = make_request(method, "/cards/" + id + "/tags", body.dump());
    http::response<http::string_body> res;
    const bool handled = route_card(
        "/cards/" + id + "/tags",
        req,
        res,
        db,
        &card_store,
        &fts,
        uuid_v4,
        map_param_getter({})
    );
    REQUIRE(handled);
    return std::make_pair(res.result(), nlohmann::json::parse(res.body()));
  };

  const auto [add_status, added] =
      call(card_id, http::verb::post, {{"project_id", "proj-1"}, {"tag", "Android"}});
  REQUIRE(add_status == http::status::ok);
  const nlohmann::json expected_added = {
      {"ok", true},
      {"data", {{"card_id", card_id}, {"tag", "android"}, {"outcome", "added"}, {"changed", true}}}
  };
  REQUIRE(added == expected_added);
  REQUIRE(card_store.get_content(*card_store.get(card_id)).value() == "Body\n\n#android");

  const auto [repeat_status, repeated] =
      call(card_id, http::verb::post, {{"project_id", "proj-1"}, {"tag", "ANDROID"}});
  REQUIRE(repeat_status == http::status::ok);
  REQUIRE(repeated["data"]["outcome"] == "already_present");
  REQUIRE(repeated["data"]["changed"] == false);

  const auto [remove_status, removed] =
      call(card_id, http::verb::delete_, {{"project_id", "proj-1"}, {"tag", "ANDROID"}});
  REQUIRE(remove_status == http::status::ok);
  REQUIRE(removed["data"]["outcome"] == "removed");
  REQUIRE(removed["data"]["changed"] == true);
  REQUIRE(card_store.get_content(*card_store.get(card_id)).value() == "Body");

  const auto [missing_status, missing] =
      call(card_id, http::verb::delete_, {{"project_id", "proj-1"}, {"tag", "android"}});
  REQUIRE(missing_status == http::status::ok);
  REQUIRE(missing["data"]["outcome"] == "not_present");
  REQUIRE(missing["data"]["changed"] == false);

  const auto prose_before = card_store.get_content(*card_store.get(prose_card_id));
  const auto [prose_status, prose] =
      call(prose_card_id, http::verb::delete_, {{"project_id", "proj-1"}, {"tag", "Reference"}});
  REQUIRE(prose_status == http::status::ok);
  REQUIRE(prose["data"]["outcome"] == "present_outside_editable_tag_line");
  REQUIRE(prose["data"]["changed"] == false);
  REQUIRE(card_store.get_content(*card_store.get(prose_card_id)) == prose_before);

  CHECK(
      call(card_id, http::verb::delete_, {{"project_id", "proj-1"}, {"tag", "123invalid"}}).first ==
      http::status::bad_request
  );
  CHECK(
      call(card_id, http::verb::post, {{"project_id", ""}, {"tag", "work"}}).first ==
      http::status::bad_request
  );
  CHECK(
      call(card_id, http::verb::post, {{"project_id", 12}, {"tag", "work"}}).first ==
      http::status::bad_request
  );

  const auto [invalid_status, invalid] =
      call(card_id, http::verb::post, {{"project_id", "proj-1"}, {"tag", "123invalid"}});
  REQUIRE(invalid_status == http::status::bad_request);
  REQUIRE(invalid["error"]["code"] == "invalid_tag");

  const auto [cross_status, cross] =
      call(other_card_id, http::verb::post, {{"project_id", "proj-1"}, {"tag", "work"}});
  REQUIRE(cross_status == http::status::unprocessable_entity);
  REQUIRE(cross["error"]["code"] == "cross_project_tag_forbidden");

  const auto [trashed_status, trashed] =
      call(trashed_card_id, http::verb::post, {{"project_id", "proj-1"}, {"tag", "work"}});
  REQUIRE(trashed_status == http::status::not_found);
  REQUIRE(trashed["error"]["code"] == "not_found");

  const auto [fields_status, fields] = call(card_id, http::verb::post, {{"tag", "work"}});
  REQUIRE(fields_status == http::status::bad_request);
  REQUIRE(fields["error"]["code"] == "bad_request");

  const auto [method_status, method] =
      call(card_id, http::verb::get, {{"project_id", "proj-1"}, {"tag", "work"}});
  REQUIRE(method_status == http::status::method_not_allowed);
  REQUIRE(method["error"]["code"] == "method_not_allowed");
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
    const std::string path = "/cards/" + card_id;
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

TEST_CASE("CardRoutes card actions report their failures", "[card-routes]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  holder::test::create_project(db, "proj-1", (dir / "repo").string());
  holder::index::FtsIndexer fts(db);
  holder::card::CardStore card_store(db, &fts);
  const std::string parent = "99999999-9999-4999-8999-999999999991";
  const std::string child = "99999999-9999-4999-8999-999999999992";
  holder::test::create_card_fixture(card_store, parent, "proj-1", "Parent", "parent", 10);
  holder::test::create_card_fixture(card_store, child, "proj-1", "Child", "child", 11, parent);
  const auto call = [&](http::verb method, const std::string& path, const nlohmann::json& body,
                        holder::card::CardStore* store) {
    auto req = make_request(method, path, body.is_null() ? "" : body.dump());
    http::response<http::string_body> res;
    REQUIRE(route_card(path, req, res, db, store, &fts, [] { return std::string("id"); },
                       [](const std::string&) { return std::string(); }));
    return std::make_pair(res.result(), nlohmann::json::parse(res.body()));
  };
  const std::string card = "/cards/" + parent;

  for (const auto& [method, tail, body] :
       {std::tuple{http::verb::post, std::string("/tags"),
                   nlohmann::json{{"project_id", "proj-1"}, {"tag", "x"}}},
        std::tuple{http::verb::delete_, std::string("/tags"),
                   nlohmann::json{{"project_id", "proj-1"}, {"tag", "x"}}},
        std::tuple{http::verb::post, std::string("/links"), nlohmann::json{{"to_card_id", child}}},
        std::tuple{http::verb::delete_, std::string("/links"), nlohmann::json()}}) {
    CAPTURE(tail);
    const auto [status, payload] = call(method, card + tail, body, nullptr);
    REQUIRE(status == http::status::not_implemented);
    REQUIRE(payload["error"]["code"] == "not_implemented");
  }

  // A zero created_at takes the current time.
  const auto [link_status, link] = call(
      http::verb::post, "/cards/" + child + "/links", {{"to_card_id", parent}, {"created_at", 0}},
      &card_store);
  REQUIRE(link_status == http::status::created);
  REQUIRE(link["data"]["created_at"].get<long long>() > 0);
  REQUIRE(link["data"]["label"].is_null());
}
