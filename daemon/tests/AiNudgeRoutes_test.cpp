#if __has_include(<catch2/catch_test_macros.hpp>)
#include <catch2/catch_test_macros.hpp>
#else
#include <catch2/catch.hpp>
#endif

#include "ai/AiNudgeRepo.h"
#include "ai/NudgeService.h"
#include "api/GeneratedRoutes.h"
#include "http_test_helpers.h"

#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>

namespace {

namespace http = boost::beast::http;

http::request<http::string_body> make_request(
    http::verb method,
    const std::string& target,
    const std::string& body
) {
  http::request<http::string_body> req{method, target, 11};
  req.set(http::field::host, "127.0.0.1");
  req.body() = body;
  req.prepare_payload();
  return req;
}

void create_card_fixture(
    holder::platform::Db& db,
    const std::string& project_id,
    const std::string& card_id
) {
  db.exec(
      "INSERT INTO cards(card_id, project_id, title, rel_path, created_at, updated_at) "
      "VALUES('" +
      card_id + "', '" + project_id + "', 'Fixture', 'cards/" + card_id + ".md', 1, 1);"
  );
}

void write_text(const std::filesystem::path& path, const std::string& content) {
  std::filesystem::create_directories(path.parent_path());
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  REQUIRE(out.is_open());
  out << content;
}

bool dispatch(
    const http::request<http::string_body>& req,
    http::response<http::string_body>& res,
    holder::platform::Db& db
) {
  return holder::api::dispatch_generated_routes(req, res, db);
}

} // namespace

TEST_CASE("AiNudgeRepo creates and finds nudges directly", "[ai][nudges]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  holder::test::create_project(db, "proj-1");
  create_card_fixture(db, "proj-1", "card-1");
  holder::ai::AiNudgeRepo repo(db);

  REQUIRE_FALSE(repo.find_by_id("missing-nudge").has_value());

  repo.create({
      .nudge_id = "nudge-1",
      .kind = "card.title_only",
      .project_id = "proj-1",
      .card_id = "card-1",
      .title = "Start this card",
      .body = "Add the first useful detail.",
      .meta_json = {{"source", "direct-test"}},
      .basis_fingerprint = "fingerprint-1",
      .basis_commit = "commit-1",
      .created_at = 123,
  });

  const auto stored = repo.find_by_id("nudge-1");
  REQUIRE(stored.has_value());
  REQUIRE(stored->nudge_id == "nudge-1");
  REQUIRE(stored->kind == "card.title_only");
  REQUIRE(stored->project_id == "proj-1");
  REQUIRE(stored->card_id == "card-1");
  REQUIRE(stored->title == "Start this card");
  REQUIRE(stored->body == "Add the first useful detail.");
  REQUIRE(stored->meta_json == nlohmann::json{{"source", "direct-test"}});
  REQUIRE(stored->basis_fingerprint == "fingerprint-1");
  REQUIRE(stored->basis_commit == "commit-1");
  REQUIRE(stored->created_at == 123);
  REQUIRE_FALSE(stored->dismissed);
}

TEST_CASE("AI nudge routes evaluates known nudge candidates", "[ai][nudges]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  holder::test::create_project(db, "proj-1");
  create_card_fixture(db, "proj-1", "card-1");

  SECTION("card title only candidate can be accepted for nudging") {
    auto req = make_request(
        http::verb::post,
        "/ai/nudges/evaluate",
        R"({
                              "kind":"card.title_only",
                              "project_id":"proj-1",
                              "card_id":"card-1",
                              "created_at":123,
                              "basis_fingerprint":"abc123",
                              "facts":{
                                "title":"Frog",
                                "body_empty":true,
                                "doc_chars":12,
                                "body_chars":0
                              }
                            })"
    );
    http::response<http::string_body> res;

    REQUIRE(dispatch(req, res, db));
    REQUIRE(res.result() == http::status::ok);

    const auto payload = nlohmann::json::parse(res.body());
    REQUIRE(payload["ok"] == true);
    REQUIRE(payload["data"]["kind"] == "card.title_only");
    REQUIRE(payload["data"]["accepted"] == true);
    REQUIRE(payload["data"]["should_nudge"] == true);
    REQUIRE(payload["data"]["reason"] == "title_only_candidate_ready");
    REQUIRE(payload["data"]["nudge"]["kind"] == "card.title_only");
    REQUIRE(payload["data"]["nudge"]["title"] == "Start this card");
  }

  SECTION("card stuck drafting candidate can be accepted for nudging") {
    auto req = make_request(
        http::verb::post,
        "/ai/nudges/evaluate",
        R"({
                              "kind":"card.stuck_drafting",
                              "project_id":"proj-1",
                              "card_id":"card-1",
                              "created_at":123,
                              "basis_fingerprint":"abc123",
                              "facts":{
                                "title":"Frog",
                                "autosave_count":4,
                                "doc_chars":72,
                                "body_chars":48,
                                "duration_seconds":180
                              }
                            })"
    );
    http::response<http::string_body> res;

    REQUIRE(dispatch(req, res, db));
    REQUIRE(res.result() == http::status::ok);

    const auto payload = nlohmann::json::parse(res.body());
    REQUIRE(payload["data"]["kind"] == "card.stuck_drafting");
    REQUIRE(payload["data"]["accepted"] == true);
    REQUIRE(payload["data"]["should_nudge"] == true);
    REQUIRE(payload["data"]["reason"] == "stuck_drafting_candidate_ready");
  }

  SECTION("repeated failed push candidate can be accepted for nudging") {
    auto req = make_request(
        http::verb::post,
        "/ai/nudges/evaluate",
        R"({
                              "kind":"git.push_failed_repeated",
                              "project_id":"proj-1",
                              "created_at":123,
                              "basis_commit":"deadbeef",
                              "facts":{
                                "failure_count":3,
                                "latest_status":"auth_failed",
                                "branch":"main"
                              }
                            })"
    );
    http::response<http::string_body> res;

    REQUIRE(dispatch(req, res, db));
    REQUIRE(res.result() == http::status::ok);

    const auto payload = nlohmann::json::parse(res.body());
    REQUIRE(payload["data"]["accepted"] == true);
    REQUIRE(payload["data"]["should_nudge"] == true);
    REQUIRE(payload["data"]["reason"] == "git_push_failure_candidate_ready");
  }

  SECTION("title suggestion candidate returns three suggestions") {
    const auto repo_dir = dir / "repo";
    write_text(
        repo_dir / "cards/card-1.md",
        "# Untitled\n\nFrogs use wetlands as practical nurseries. Their eggs and tadpoles depend on shallow water, shelter, and seasonal changes."
    );
    db.exec(
        "UPDATE projects SET root_path = '" + repo_dir.string() +
        "', privacy_mode = 'plain' WHERE project_id = 'proj-1';"
    );
    db.exec(
        "UPDATE cards SET title = 'Untitled', rel_path = 'cards/card-1.md' WHERE card_id = 'card-1';"
    );
    auto req = make_request(
        http::verb::post,
        "/ai/nudges/evaluate",
        R"({
                              "kind":"card.title_suggestion",
                              "project_id":"proj-1",
                              "card_id":"card-1",
                              "created_at":123,
                              "basis_fingerprint":"title-fp",
                              "facts":{
                                "title":"Untitled",
                                "body_empty":false,
                                "doc_chars":150,
                                "body_chars":120
                              }
                            })"
    );
    http::response<http::string_body> res;

    REQUIRE(dispatch(req, res, db));
    REQUIRE(res.result() == http::status::ok);

    const auto payload = nlohmann::json::parse(res.body());
    REQUIRE(payload["data"]["accepted"] == true);
    REQUIRE(payload["data"]["should_nudge"] == true);
    REQUIRE(payload["data"]["nudge"]["kind"] == "card.title_suggestion");
    REQUIRE(payload["data"]["nudge"]["suggestions"].is_array());
    REQUIRE(payload["data"]["nudge"]["suggestions"].size() == 3);
    REQUIRE(payload["data"]["nudge"]["meta_json"]["suggestions"].size() == 3);
  }
}

TEST_CASE("AI nudge routes rejects malformed or non-actionable candidates", "[ai][nudges]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  holder::test::create_project(db, "proj-1");
  create_card_fixture(db, "proj-1", "card-1");

  SECTION("unrelated path returns false") {
    auto req = make_request(http::verb::post, "/ai/nudges/nope", "{}");
    http::response<http::string_body> res;

    REQUIRE_FALSE(dispatch(req, res, db));
  }

  SECTION("wrong method is not allowed") {
    auto req = make_request(http::verb::get, "/ai/nudges/evaluate", "");
    http::response<http::string_body> res;

    REQUIRE(dispatch(req, res, db));
    REQUIRE(res.result() == http::status::method_not_allowed);
  }

  SECTION("invalid json returns bad request") {
    auto req = make_request(http::verb::post, "/ai/nudges/evaluate", "{");
    http::response<http::string_body> res;

    REQUIRE(dispatch(req, res, db));
    REQUIRE(res.result() == http::status::bad_request);

    const auto payload = nlohmann::json::parse(res.body());
    REQUIRE(payload["ok"] == false);
    REQUIRE(payload["error"]["code"] == "bad_request");
  }

  SECTION("invalid body returns bad request") {
    auto req =
        make_request(http::verb::post, "/ai/nudges/evaluate", R"({"kind":"card.title_only"})");
    http::response<http::string_body> res;

    REQUIRE(dispatch(req, res, db));
    REQUIRE(res.result() == http::status::bad_request);

    const auto payload = nlohmann::json::parse(res.body());
    REQUIRE(payload["ok"] == false);
    REQUIRE(payload["error"]["code"] == "bad_request");
  }

  SECTION("empty kind or project_id returns bad request") {
    auto req = make_request(
        http::verb::post,
        "/ai/nudges/evaluate",
        R"({
                              "kind":"",
                              "project_id":"",
                              "created_at":123,
                              "facts":{}
                            })"
    );
    http::response<http::string_body> res;

    REQUIRE(dispatch(req, res, db));
    REQUIRE(res.result() == http::status::bad_request);

    const auto payload = nlohmann::json::parse(res.body());
    REQUIRE(payload["ok"] == false);
    REQUIRE(payload["error"]["code"] == "bad_request");
  }

  SECTION("placeholder title only candidate is accepted but not actionable") {
    auto req = make_request(
        http::verb::post,
        "/ai/nudges/evaluate",
        R"({
                              "kind":"card.title_only",
                              "project_id":"proj-1",
                              "card_id":"card-1",
                              "created_at":123,
                              "facts":{
                                "title":"Untitled child of Cats",
                                "body_empty":true,
                                "doc_chars":24,
                                "body_chars":0
                              }
                            })"
    );
    http::response<http::string_body> res;

    REQUIRE(dispatch(req, res, db));
    REQUIRE(res.result() == http::status::ok);

    const auto payload = nlohmann::json::parse(res.body());
    REQUIRE(payload["data"]["accepted"] == true);
    REQUIRE(payload["data"]["should_nudge"] == false);
    REQUIRE(payload["data"]["reason"] == "title_only_not_actionable");
  }

  SECTION("insufficient stuck drafting evidence is not actionable") {
    auto req = make_request(
        http::verb::post,
        "/ai/nudges/evaluate",
        R"({
                              "kind":"card.stuck_drafting",
                              "project_id":"proj-1",
                              "card_id":"card-1",
                              "created_at":123,
                              "facts":{
                                "title":"Frog",
                                "autosave_count":2,
                                "doc_chars":32,
                                "body_chars":24,
                                "duration_seconds":40
                              }
                            })"
    );
    http::response<http::string_body> res;

    REQUIRE(dispatch(req, res, db));
    REQUIRE(res.result() == http::status::ok);

    const auto payload = nlohmann::json::parse(res.body());
    REQUIRE(payload["data"]["accepted"] == true);
    REQUIRE(payload["data"]["should_nudge"] == false);
    REQUIRE(payload["data"]["reason"] == "stuck_drafting_not_actionable");
  }

  SECTION("unknown kind is accepted false") {
    auto req = make_request(
        http::verb::post,
        "/ai/nudges/evaluate",
        R"({
                              "kind":"unknown.kind",
                              "project_id":"proj-1",
                              "created_at":123,
                              "facts":{}
                            })"
    );
    http::response<http::string_body> res;

    REQUIRE(dispatch(req, res, db));
    REQUIRE(res.result() == http::status::ok);

    const auto payload = nlohmann::json::parse(res.body());
    REQUIRE(payload["data"]["accepted"] == false);
    REQUIRE(payload["data"]["should_nudge"] == false);
    REQUIRE(payload["data"]["reason"] == "unknown_candidate_kind");
  }
}

TEST_CASE("AI nudge routes list prunes stale nudges", "[ai][nudges]") {
  const auto dir = holder::test::make_temp_dir();
  const auto repo_dir = dir / "repo";
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  holder::test::create_project(db, "proj-1", repo_dir.string());
  create_card_fixture(db, "proj-1", "card-1");
  write_text(repo_dir / "cards" / "card-1.md", "# Frog\n\nNow with body.\n");

  auto create_req = make_request(
      http::verb::post,
      "/ai/nudges/evaluate",
      R"({
                                   "kind":"card.title_only",
                                   "project_id":"proj-1",
                                   "card_id":"card-1",
                                   "created_at":123,
                                   "basis_fingerprint":"deadbeefcafe",
                                   "facts":{
                                     "title":"Frog",
                                     "body_empty":true,
                                     "doc_chars":12,
                                     "body_chars":0
                                   }
                                 })"
  );
  http::response<http::string_body> create_res;
  REQUIRE(dispatch(create_req, create_res, db));
  REQUIRE(create_res.result() == http::status::ok);

  auto list_req = make_request(http::verb::get, "/ai/nudges?project_id=proj-1&card_id=card-1", "");
  http::response<http::string_body> list_res;
  REQUIRE(dispatch(list_req, list_res, db));
  REQUIRE(list_res.result() == http::status::ok);

  const auto payload = nlohmann::json::parse(list_res.body());
  REQUIRE(payload["ok"] == true);
  REQUIRE(payload["data"]["nudges"].is_array());
  REQUIRE(payload["data"]["nudges"].empty());
}

TEST_CASE("AI nudge routes lists and dismisses created nudges", "[ai][nudges]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  holder::test::create_project(db, "proj-1");
  create_card_fixture(db, "proj-1", "card-1");

  auto create_req = make_request(
      http::verb::post,
      "/ai/nudges/evaluate",
      R"({
                                   "kind":"card.title_only",
                                   "project_id":"proj-1",
                                   "card_id":"card-1",
                                   "created_at":123,
                                   "basis_fingerprint":"abc123",
                                   "facts":{
                                     "title":"Frog",
                                     "body_empty":true,
                                     "doc_chars":12,
                                     "body_chars":0
                                   }
                                 })"
  );
  http::response<http::string_body> create_res;
  REQUIRE(dispatch(create_req, create_res, db));
  const auto created_payload = nlohmann::json::parse(create_res.body());
  const auto nudge_id = created_payload["data"]["nudge"]["nudge_id"].get<std::string>();

  SECTION("list returns created nudge for matching card") {
    auto req = make_request(http::verb::get, "/ai/nudges?project_id=proj-1&card_id=card-1", "");
    http::response<http::string_body> res;

    REQUIRE(dispatch(req, res, db));
    REQUIRE(res.result() == http::status::ok);

    const auto payload = nlohmann::json::parse(res.body());
    REQUIRE(payload["data"]["nudges"].is_array());
    REQUIRE(payload["data"]["nudges"].size() == 1);
    REQUIRE(payload["data"]["nudges"][0]["nudge_id"] == nudge_id);
  }

  SECTION("dismiss hides nudge from subsequent list") {
    auto dismiss_req = make_request(http::verb::post, "/ai/nudges/" + nudge_id + "/dismiss", "");
    http::response<http::string_body> dismiss_res;
    REQUIRE(dispatch(dismiss_req, dismiss_res, db));
    REQUIRE(dismiss_res.result() == http::status::ok);

    auto list_req =
        make_request(http::verb::get, "/ai/nudges?project_id=proj-1&card_id=card-1", "");
    http::response<http::string_body> list_res;
    REQUIRE(dispatch(list_req, list_res, db)
    );

    const auto payload = nlohmann::json::parse(list_res.body());
    REQUIRE(payload["data"]["nudges"].is_array());
    REQUIRE(payload["data"]["nudges"].empty());
  }
}

TEST_CASE("AI nudge routes handle query and lookup errors", "[ai][nudges]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  holder::test::create_project(db, "proj-1");
  create_card_fixture(db, "proj-1", "card-1");

  SECTION("list requires project_id query parameter") {
    auto req = make_request(http::verb::get, "/ai/nudges?card_id=card-1", "");
    http::response<http::string_body> res;

    REQUIRE(dispatch(req, res, db));
    REQUIRE(res.result() == http::status::bad_request);

    const auto payload = nlohmann::json::parse(res.body());
    REQUIRE(payload["error"]["code"] == "bad_request");
  }

  SECTION("dismiss without a nudge ID is not a route") {
    auto req = make_request(http::verb::post, "/ai/nudges//dismiss", "");
    http::response<http::string_body> res;

    REQUIRE_FALSE(dispatch(req, res, db));
  }

  SECTION("dismiss returns not found for unknown nudge id") {
    auto req = make_request(http::verb::post, "/ai/nudges/missing/dismiss", "");
    http::response<http::string_body> res;

    REQUIRE(dispatch(req, res, db));
    REQUIRE(res.result() == http::status::not_found);

    const auto payload = nlohmann::json::parse(res.body());
    REQUIRE(payload["error"]["code"] == "not_found");
    REQUIRE(payload["error"]["message"] == "Nudge not found.");
  }

  SECTION("an empty card_id lists every card's nudges") {
    auto req = make_request(http::verb::get, "/ai/nudges?project_id=proj-1&card_id=", "");
    http::response<http::string_body> res;

    REQUIRE(dispatch(req, res, db));
    REQUIRE(res.result() == http::status::ok);
    REQUIRE(nlohmann::json::parse(res.body())["data"]["nudges"].empty());
  }
}
