#if __has_include(<catch2/catch_test_macros.hpp>)
#include <catch2/catch_test_macros.hpp>
#else
#include <catch2/catch.hpp>
#endif

#include "ai/AiMessageRepo.h"
#include "ai/AiThreadRepo.h"
#include "api/GeneratedRoutes.h"
#include "api/routes/ai/AiMessageRoutes.h"
#include "http_test_helpers.h"
#include "model/AiThread.h"

#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace {

namespace http = boost::beast::http;

http::request<http::string_body> make_request(
    http::verb method,
    const std::string& target,
    const std::string& body = ""
) {
  http::request<http::string_body> req{method, target, 11};
  req.set(http::field::host, "127.0.0.1");
  if (!body.empty()) {
    req.set(http::field::content_type, "application/json");
    req.body() = body;
    req.prepare_payload();
  }
  return req;
}

void seed_project_and_thread(holder::platform::Db& db, const std::filesystem::path& root) {
  holder::test::create_project(db, "proj-1", root.string());
  holder::model::AiThread thread;
  thread.thread_id = "thread-1";
  thread.project_id = "proj-1";
  thread.title = "Thread";
  thread.created_at = 1;
  thread.updated_at = 1;
  holder::ai::AiThreadRepo(db).create(thread);
}

// Routes a request as the daemon does: generated routes first, then the host-written fallback.
bool route_message(const http::request<http::string_body>& req,
                   http::response<http::string_body>& res,
                   holder::platform::Db& db,
                   holder::index::FtsIndexer* fts,
                   std::function<std::string()> uuid_v4 = [] { return std::string("u"); }) {
  if (holder::api::dispatch_generated_routes(req, res, db, nullptr, fts, nullptr, nullptr, uuid_v4,
                                             {})) {
    return true;
  }
  const auto target = req.target();
  return holder::api::routes::handle_ai_message_routes(
      std::string(target.data(), target.size()), req, res, db, fts, uuid_v4);
}

} // namespace

TEST_CASE("AiMessageCrudRoutes collection branches", "[http]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  seed_project_and_thread(db, dir / "project_repo");
  holder::index::FtsIndexer fts(db);

  SECTION("list filters deleted unless include_deleted=1") {
    auto post1 = make_request(
        http::verb::post,
        "/ai/messages",
        R"({"message_id":"msg-0001","thread_id":"thread-1","role":"user","source":"manual","content":"a","created_at":10})"
    );
    http::response<http::string_body> post1_res;
    REQUIRE(route_message(post1, post1_res, db, &fts, [] {
          return std::string("x");
        }));
    REQUIRE(post1_res.result() == http::status::created);

    auto post2 = make_request(
        http::verb::post,
        "/ai/messages",
        R"({"message_id":"msg-0002","thread_id":"thread-1","role":"user","source":"manual","content":"b","created_at":11})"
    );
    http::response<http::string_body> post2_res;
    REQUIRE(route_message(post2, post2_res, db, &fts, [] {
          return std::string("y");
        }));
    REQUIRE(post2_res.result() == http::status::created);

    holder::ai::AiMessageRepo(db, &fts).trash("msg-0002", 99);

    auto list_req = make_request(http::verb::get, "/ai/messages?thread_id=thread-1");
    http::response<http::string_body> list_res;
    REQUIRE(holder::api::dispatch_generated_routes(list_req, list_res, db, nullptr, &fts));
    REQUIRE(list_res.result() == http::status::ok);
    const auto listed = nlohmann::json::parse(list_res.body());
    REQUIRE(listed["data"].is_array());
    REQUIRE(listed["data"].size() == 1);
    REQUIRE(listed["data"][0]["message_id"] == "msg-0001");
    REQUIRE(listed["data"][0]["provider"].is_null());
    REQUIRE(listed["data"][0]["deleted_at"].is_null());

    auto list_all_req =
        make_request(http::verb::get, "/ai/messages?thread_id=thread-1&include_deleted=1");
    http::response<http::string_body> list_all_res;
    REQUIRE(holder::api::dispatch_generated_routes(list_all_req, list_all_res, db, nullptr, &fts));
    REQUIRE(list_all_res.result() == http::status::ok);
    const auto listed_all = nlohmann::json::parse(list_all_res.body());
    REQUIRE(listed_all["data"].size() == 2);
  }

  SECTION("list validates its query and reports storage failures as internal errors") {
    for (const std::string target :
         {"/ai/messages", "/ai/messages?thread_id=", "/ai/messages?thread_id=t&include_deleted=2"}) {
      CAPTURE(target);
      auto req = make_request(http::verb::get, target);
      http::response<http::string_body> res;
      REQUIRE(holder::api::dispatch_generated_routes(req, res, db, nullptr, &fts));
      REQUIRE(res.result() == http::status::bad_request);
    }
    db.exec("DROP TABLE ai_messages;");
    auto req = make_request(http::verb::get, "/ai/messages?thread_id=thread-1");
    http::response<http::string_body> res;
    REQUIRE(holder::api::dispatch_generated_routes(req, res, db, nullptr, &fts));
    REQUIRE(res.result() == http::status::internal_server_error);
  }

  SECTION("post missing required fields") {
    auto req = make_request(http::verb::post, "/ai/messages", R"({"thread_id":"thread-1"})");
    http::response<http::string_body> res;
    REQUIRE(route_message(req, res, db, &fts));
    REQUIRE(res.result() == http::status::bad_request);
  }

  SECTION("post sets optional fields and created_at fallback") {
    auto req = make_request(
        http::verb::post,
        "/ai/messages",
        R"({"thread_id":"thread-1","role":"assistant","source":"runner","content":"hello","provider":"prov","model":"mod","prompt_hash":"ph","meta_json":"{}","created_at":0})"
    );
    http::response<http::string_body> res;
    REQUIRE(route_message(req, res, db, &fts, [] {
          return std::string("generated-m");
        }));
    REQUIRE(res.result() == http::status::created);
    const auto body = nlohmann::json::parse(res.body());
    const auto id = body["data"]["message_id"].get<std::string>();

    auto get_req = make_request(http::verb::get, "/ai/messages/" + id);
    http::response<http::string_body> get_res;
    REQUIRE(holder::api::dispatch_generated_routes(get_req, get_res, db, nullptr, &fts));
    REQUIRE(get_res.result() == http::status::ok);
    const auto got = nlohmann::json::parse(get_res.body());
    REQUIRE(got["data"]["provider"] == "prov");
    REQUIRE(got["data"]["model"] == "mod");
    REQUIRE(got["data"]["prompt_hash"] == "ph");
    REQUIRE(got["data"]["meta_json"] == "{}");
    REQUIRE(got["data"]["created_at"].get<long long>() > 0);

    holder::ai::AiMessageRepo(db, &fts).trash(id, 99);
    http::response<http::string_body> trashed_res;
    REQUIRE(holder::api::dispatch_generated_routes(get_req, trashed_res, db, nullptr, &fts));
    REQUIRE(trashed_res.result() == http::status::not_found);
    REQUIRE(nlohmann::json::parse(trashed_res.body())["error"]["message"] == "AI message not found.");
  }

  SECTION("post conflict, unknown thread, failures and bad JSON") {
    const std::string body =
        R"({"message_id":"msg-dup","thread_id":"thread-1","role":"assistant","source":"runner","content":"hello"})";
    http::response<http::string_body> first_res;
    REQUIRE(route_message(make_request(http::verb::post, "/ai/messages", body), first_res, db, &fts));
    REQUIRE(first_res.result() == http::status::created);
    http::response<http::string_body> conflict_res;
    REQUIRE(route_message(make_request(http::verb::post, "/ai/messages", body), conflict_res, db, &fts));
    REQUIRE(conflict_res.result() == http::status::conflict);
    REQUIRE(nlohmann::json::parse(conflict_res.body())["error"]["code"] == "conflict");

    http::response<http::string_body> thread_res;
    REQUIRE(route_message(
        make_request(
            http::verb::post,
            "/ai/messages",
            R"({"thread_id":"missing","role":"user","source":"manual","content":"x"})"
        ),
        thread_res, db, &fts));
    REQUIRE(thread_res.result() == http::status::bad_request);
    REQUIRE(nlohmann::json::parse(thread_res.body())["error"]["message"] == "Thread not found.");

    // A failure other than a bad request or a conflict is an internal error.
    http::response<http::string_body> failed_res;
    REQUIRE(route_message(
        make_request(
            http::verb::post,
            "/ai/messages",
            R"({"thread_id":"thread-1","role":"user","source":"manual","content":"x"})"
        ),
        failed_res, db, &fts, []() -> std::string { throw std::runtime_error("synthetic"); }));
    REQUIRE(failed_res.result() == http::status::internal_server_error);

    http::response<http::string_body> bad_res;
    REQUIRE(route_message(make_request(http::verb::post, "/ai/messages", "not-json"), bad_res, db, &fts));
    REQUIRE(bad_res.result() == http::status::bad_request);
  }
}

TEST_CASE("AiMessageCrudRoutes item and restore branches", "[http]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  seed_project_and_thread(db, dir / "project_repo");
  holder::index::FtsIndexer fts(db);

  auto create_req = make_request(
      http::verb::post,
      "/ai/messages",
      R"({"message_id":"msg-1","thread_id":"thread-1","role":"user","source":"manual","content":"c","provider":"p","model":"m","prompt_hash":"h","meta_json":"{}","created_at":10})"
  );
  http::response<http::string_body> create_res;
  REQUIRE(route_message(create_req, create_res, db, &fts));
  REQUIRE(create_res.result() == http::status::created);

  SECTION("route guards and fallbacks") {
    http::response<http::string_body> res;
    REQUIRE_FALSE(route_message(make_request(http::verb::get, "/ai/messagez/msg-1"), res, db, &fts));

    REQUIRE_FALSE(route_message(make_request(http::verb::get, "/ai/messages/"), res, db, &fts));

    // An empty message ID matches no route, so the router answers 404.
    REQUIRE_FALSE(route_message(make_request(http::verb::post, "/ai/messages//restore"), res, db, &fts));

    REQUIRE_FALSE(route_message(make_request(http::verb::get, "/ai/messages/msg-1/other"), res, db, &fts));
  }

  SECTION("restore method and catch branches") {
    auto wrong_method = make_request(http::verb::get, "/ai/messages/msg-1/restore");
    http::response<http::string_body> wrong_method_res;
    REQUIRE(route_message(wrong_method, wrong_method_res, db, &fts));
    REQUIRE(wrong_method_res.result() == http::status::method_not_allowed);

    auto missing_restore = make_request(http::verb::post, "/ai/messages/missing/restore");
    http::response<http::string_body> missing_restore_res;
    REQUIRE(route_message(missing_restore, missing_restore_res, db, &fts));
    REQUIRE(missing_restore_res.result() == http::status::not_found);

    http::response<http::string_body> live_res;
    REQUIRE(route_message(make_request(http::verb::post, "/ai/messages/msg-1/restore"), live_res, db, &fts));
    REQUIRE(live_res.result() == http::status::bad_request);
  }

  SECTION("delete trashes once, then reports state, missing messages and failures") {
    auto req = make_request(http::verb::delete_, "/ai/messages/msg-1");
    http::response<http::string_body> res;
    REQUIRE(route_message(req, res, db, &fts));
    REQUIRE(res.result() == http::status::ok);
    REQUIRE(route_message(req, res, db, &fts));
    REQUIRE(res.result() == http::status::bad_request);
    REQUIRE(route_message(make_request(http::verb::delete_, "/ai/messages/missing"), res, db, &fts));
    REQUIRE(res.result() == http::status::not_found);
    REQUIRE(route_message(make_request(http::verb::post, "/ai/messages/msg-1/restore"), res, db, &fts));
    REQUIRE(res.result() == http::status::ok);

    db.exec("DROP TABLE ai_messages;");
    REQUIRE(route_message(req, res, db, &fts));
    REQUIRE(res.result() == http::status::internal_server_error);
  }

  SECTION("patch not-found, deleted, optional clears, and catch") {
    auto patch_missing =
        make_request(http::verb::patch, "/ai/messages/missing", R"({"content":"x"})");
    http::response<http::string_body> missing_res;
    REQUIRE(route_message(patch_missing, missing_res, db, &fts));
    REQUIRE(missing_res.result() == http::status::not_found);

    holder::ai::AiMessageRepo(db, &fts).trash("msg-1", 123);
    auto patch_deleted =
        make_request(http::verb::patch, "/ai/messages/msg-1", R"({"content":"x"})");
    http::response<http::string_body> deleted_res;
    REQUIRE(route_message(patch_deleted, deleted_res, db, &fts));
    REQUIRE(deleted_res.result() == http::status::bad_request);

    holder::ai::AiMessageRepo(db, &fts).restore("msg-1");
    auto patch_clear = make_request(
        http::verb::patch,
        "/ai/messages/msg-1",
        R"({"role":"assistant","source":"runner","provider":null,"model":null,"prompt_hash":null,"meta_json":null})"
    );
    http::response<http::string_body> patch_res;
    REQUIRE(route_message(patch_clear, patch_res, db, &fts));
    REQUIRE(patch_res.result() == http::status::ok);

    auto get_req = make_request(http::verb::get, "/ai/messages/msg-1");
    http::response<http::string_body> get_res;
    REQUIRE(holder::api::dispatch_generated_routes(get_req, get_res, db, nullptr, &fts));
    const auto got = nlohmann::json::parse(get_res.body());
    REQUIRE(got["data"]["role"] == "assistant");
    REQUIRE(got["data"]["source"] == "runner");
    REQUIRE(got["data"]["provider"].is_null());
    REQUIRE(got["data"]["model"].is_null());
    REQUIRE(got["data"]["prompt_hash"].is_null());
    REQUIRE(got["data"]["meta_json"].is_null());

    auto bad_json = make_request(http::verb::patch, "/ai/messages/msg-1", "{");
    http::response<http::string_body> bad_res;
    REQUIRE(route_message(bad_json, bad_res, db, &fts));
    REQUIRE(bad_res.result() == http::status::bad_request);
  }

  SECTION("patch sets model, prompt_hash, and meta_json to non-null values") {
    auto patch_set = make_request(
        http::verb::patch,
        "/ai/messages/msg-1",
        R"({"model":"m2","prompt_hash":"ph2","meta_json":"{\"a\":1}"})"
    );
    http::response<http::string_body> patch_res;
    REQUIRE(route_message(patch_set, patch_res, db, &fts));
    REQUIRE(patch_res.result() == http::status::ok);

    auto get_req = make_request(http::verb::get, "/ai/messages/msg-1");
    http::response<http::string_body> get_res;
    REQUIRE(holder::api::dispatch_generated_routes(get_req, get_res, db, nullptr, &fts));
    REQUIRE(get_res.result() == http::status::ok);
    const auto got = nlohmann::json::parse(get_res.body());
    REQUIRE(got["data"]["model"] == "m2");
    REQUIRE(got["data"]["prompt_hash"] == "ph2");
    REQUIRE(got["data"]["meta_json"] == "{\"a\":1}");
  }

  SECTION("item post route and item get catch") {
    auto post_req = make_request(http::verb::post, "/ai/messages/msg-1");
    http::response<http::string_body> post_res;
    REQUIRE(route_message(post_req, post_res, db, &fts));
    REQUIRE(post_res.result() == http::status::method_not_allowed);
    REQUIRE(post_res[http::field::allow] == "DELETE, GET, PATCH");

    db.exec("DROP TABLE ai_messages;");
    auto get_req = make_request(http::verb::get, "/ai/messages/msg-1");
    http::response<http::string_body> get_res;
    REQUIRE(holder::api::dispatch_generated_routes(get_req, get_res, db, nullptr, &fts));
    REQUIRE(get_res.result() == http::status::internal_server_error);
  }
}
