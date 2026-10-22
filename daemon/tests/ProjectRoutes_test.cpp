#if __has_include(<catch2/catch_test_macros.hpp>)
#include <catch2/catch_test_macros.hpp>
#else
#include <catch2/catch.hpp>
#endif

#include "api/GeneratedRoutes.h"
#include "api/support/PrivacyErrors.h"
#include "card/TagRepo.h"
#include "git/GitOps.h"
#include "http_test_helpers.h"
#include "model/Project.h"
#include "platform/Paths.h"
#include "platform/ProjectRegistry.h"
#include "project/ProjectRepo.h"
#include "project/ProjectSyncRepo.h"

#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {
namespace http = boost::beast::http;

http::request<http::string_body> make_request(
    http::verb method,
    const std::string& target,
    const nlohmann::json& body = nlohmann::json()
) {
  http::request<http::string_body> req{method, target, 11};
  req.set(http::field::host, "127.0.0.1");
  if (!body.is_null() && !body.empty()) {
    req.set(http::field::content_type, "application/json");
    req.body() = body.dump();
    req.prepare_payload();
  }
  return req;
}

class ProjectRoutesTestGitOps final : public holder::git::GitOps {
 public:
  holder::git::RemoteProbeResult probe_result{
      .status = holder::git::RemoteProbeStatus::Reachable,
      .remote_has_head = true,
      .error_message = {},
  };
  holder::git::PushResult push_result{
      .status = holder::git::PushStatus::Pushed,
      .ahead_count = 0,
      .behind_count = 0,
      .error_message = {},
  };
  bool throw_on_set_remote = false;
  bool throw_on_pull = false;
  std::vector<std::string> calls;

  void open_or_init(const std::filesystem::path&) override {}
  void write_file(const std::filesystem::path&, const std::string&) override {}
  void stage_path(const std::filesystem::path&) override {}
  void remove_path(const std::filesystem::path&) override {}
  void commit(const std::string&) override {}
  void set_remote(const std::string&, const std::string&) override {
    calls.push_back("remote");
    if (throw_on_set_remote) {
      throw std::runtime_error("set_remote failed");
    }
  }
  void remove_remote(const std::string&) override {}
  void pull_remote_ff_only(const std::string&) override {
    calls.push_back("pull");
    if (throw_on_pull) {
      throw std::runtime_error("pull failed");
    }
  }
  holder::git::RemoteProbeResult probe_remote(const std::string&) override { return probe_result; }
  holder::git::RemoteProbeResult probe_remote_url(const std::string&) override {
    return probe_result;
  }
  holder::git::PushResult push_branch(const std::string&, const std::string&, bool) override {
    calls.push_back("push");
    return push_result;
  }
  std::filesystem::path repo_dir() const override { return {}; }
};


// Routes a request through the generated routes, as the daemon does.
bool route_project(const boost::beast::http::request<boost::beast::http::string_body>& req,
                   boost::beast::http::response<boost::beast::http::string_body>& res,
                   holder::platform::Db& db,
                   holder::git::GitOps* git) {
  return holder::api::dispatch_generated_routes(req, res, db, nullptr, nullptr, nullptr, git,
                                                [] { return std::string("generated-id"); });
}

} // namespace

TEST_CASE("Generated project deletion records a removal without deleting files", "[project-routes]") {
  const auto dir = holder::test::make_temp_dir();
  holder::test::EnvGuard data_env("XDG_DATA_HOME", (dir / "data").string());
  holder::test::EnvGuard config_env("XDG_CONFIG_HOME", (dir / "config").string());
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  const auto managed_root = dir / "projects";
  const auto project_root = managed_root / "to-delete";
  std::filesystem::create_directories(project_root / ".holder");
  std::ofstream(project_root / ".holder" / "project.json")
      << nlohmann::json{{"project_id", "proj-delete"}};

  holder::model::Project project;
  project.project_id = "proj-delete";
  project.name = "To delete";
  project.root_path = project_root.string();
  project.privacy_mode = "plain";
  project.created_at = 1;
  project.updated_at = 1;
  holder::project::ProjectRepo repo(db);
  repo.create(project);
  holder::project::ProjectSyncRepo sync_repo(db);
  sync_repo.record_push_result(project.project_id, "pushed", true, std::nullopt, 2);
  holder::core::ProjectRegistry registry(
      holder::core::Paths::resolve("holder").project_registry_path());
  registry.remember({project});
  REQUIRE(registry.discover_roots(managed_root).size() == 1);

  auto req = make_request(http::verb::delete_, "/projects/proj-delete");
  http::response<http::string_body> res;
  REQUIRE(holder::api::dispatch_generated_routes(req, res, db));
  REQUIRE(res.result() == http::status::ok);
  REQUIRE(nlohmann::json::parse(res.body()) ==
          nlohmann::json{{"ok", true}, {"data", {{"project_id", "proj-delete"}}}});
  REQUIRE_FALSE(repo.get(project.project_id).has_value());
  REQUIRE_FALSE(sync_repo.get(project.project_id).has_value());
  REQUIRE(std::filesystem::exists(project_root / ".holder" / "project.json"));
  REQUIRE(registry.discover_roots(managed_root).empty());
}

TEST_CASE("ProjectRoutes returns false when path does not match", "[project-routes]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");

  auto req = make_request(http::verb::get, "/not-projects");
  http::response<http::string_body> res;

  const bool handled = route_project(req, res, db, nullptr);
  REQUIRE_FALSE(handled);
}

TEST_CASE("ProjectRoutes global recovery import validates required fields", "[project-routes]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");

  auto req = make_request(http::verb::post, "/recovery-token/import", nlohmann::json::object());
  http::response<http::string_body> res;

  const bool handled = route_project(req, res, db, nullptr);
  REQUIRE(handled);
  REQUIRE(res.result() == http::status::bad_request);
  const auto payload = nlohmann::json::parse(res.body());
  REQUIRE(payload["ok"] == false);
  REQUIRE(payload["error"]["code"] == "bad_request");
}

TEST_CASE("ProjectRoutes leave an empty project id segment unrouted", "[project-routes]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");

  auto req = make_request(http::verb::get, "/projects/");
  http::response<http::string_body> res;

  // An empty project ID matches no route, so the router answers 404.
  REQUIRE_FALSE(route_project(req, res, db, nullptr));
}

TEST_CASE("ProjectRoutes covers additional uncovered branches", "[project-routes]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  holder::project::ProjectRepo repo(db);

  holder::model::Project project;
  project.project_id = "proj-1";
  project.name = "Proj One";
  project.root_path = (dir / "repo1").string();
  project.privacy_mode = "plain";
  project.created_at = 1;
  project.updated_at = 1;
  repo.create(project);

  ProjectRoutesTestGitOps git;

  auto call = [&](http::verb method,
                  const std::string& path,
                  const nlohmann::json& body = nlohmann::json()) {
    auto req = make_request(method, path, body);
    http::response<http::string_body> res;
    const bool handled = route_project(req, res, db, &git);
    REQUIRE(handled);
    return std::make_pair(res.result(), nlohmann::json::parse(res.body()));
  };

  SECTION("global recovery import missing fields path") {
    auto [status, payload] = call(http::verb::post, "/recovery-token/import", {{"x", 1}});
    REQUIRE(status == http::status::bad_request);
    REQUIRE(payload["error"]["code"] == "bad_request");
  }

  SECTION("generated projects list returns an internal error with closed db") {
    db.close();
    auto req = make_request(http::verb::get, "/projects");
    http::response<http::string_body> res;
    REQUIRE(holder::api::dispatch_generated_routes(req, res, db));
    REQUIRE(res.result() == http::status::internal_server_error);
    const auto payload = nlohmann::json::parse(res.body());
    REQUIRE(payload["error"]["code"] == "error");
  }
}

TEST_CASE("ProjectRoutes git and project route error/status branches", "[project-routes]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  holder::project::ProjectRepo repo(db);

  holder::model::Project project;
  project.project_id = "proj-1";
  project.name = "Proj One";
  project.root_path = (dir / "repo1").string();
  project.privacy_mode = "plain";
  project.created_at = 1;
  project.updated_at = 1;
  project.git_remote_url = std::string("git@example.com:test/repo.git");
  project.git_provider = std::string("github");
  std::filesystem::create_directories(project.root_path);
  repo.create(project);

  const auto uuid_v4 = []() {
    return std::string("generated-id");
  };
  ProjectRoutesTestGitOps git;

  auto call = [&](http::verb method,
                  const std::string& path,
                  const nlohmann::json& body = nlohmann::json()) {
    auto req = make_request(method, path, body);
    http::response<http::string_body> res;
    const bool handled = route_project(req, res, db, &git);
    REQUIRE(handled);
    return std::make_pair(res.result(), nlohmann::json::parse(res.body()));
  };

  SECTION("git test-remote emits status_name and null remote override") {
    git.probe_result = {
        .status = holder::git::RemoteProbeStatus::AuthFailed,
        .remote_has_head = false,
        .error_message = "auth",
    };
    auto [status, payload] = call(
        http::verb::post,
        "/projects/proj-1/git/test-remote",
        {{"remote_url", nullptr}, {"branch", "main"}}
    );
    REQUIRE(status == http::status::ok);
    REQUIRE(payload["data"]["status"] == "remote_unset");
    REQUIRE(payload["data"]["error_code"] == "remote_unset");
    REQUIRE(payload["data"]["remote_has_head"] == false);
  }

  SECTION("git test-remote emits reachable status with null error_code") {
    git.probe_result = {
        .status = holder::git::RemoteProbeStatus::Reachable,
        .remote_has_head = true,
        .error_message = {},
    };
    auto [status, payload] =
        call(http::verb::post, "/projects/proj-1/git/test-remote", {{"branch", "main"}});
    REQUIRE(status == http::status::ok);
    REQUIRE(payload["data"]["status"] == "reachable");
    REQUIRE(payload["data"]["error_code"].is_null());
    REQUIRE(payload["data"]["remote_has_head"] == true);
  }

  SECTION("git test-remote bad json catch") {
    auto req = make_request(http::verb::post, "/projects/proj-1/git/test-remote");
    req.body() = "{";
    req.prepare_payload();
    http::response<http::string_body> res;
    const bool handled = holder::api::dispatch_generated_routes(
        req, res, db, nullptr, nullptr, nullptr, &git, uuid_v4);
    REQUIRE(handled);
    REQUIRE(res.result() == http::status::bad_request);
  }

  SECTION("git push emits push status name") {
    git.push_result = {
        .status = holder::git::PushStatus::AuthFailed,
        .ahead_count = 0,
        .behind_count = 0,
        .error_message = "auth failed",
    };
    auto [status, payload] =
        call(http::verb::post, "/projects/proj-1/git/push", nlohmann::json::object());
    REQUIRE(status == http::status::ok);
    REQUIRE(payload["data"]["status"] == "auth_failed");
    REQUIRE(payload["data"]["error_code"] == "auth_failed");
  }

  SECTION("git push refuses plaintext blobs in an encrypted project") {
    repo.update_privacy_mode(project.project_id, "encrypted_git", 2);
    std::filesystem::create_directories(std::filesystem::path(project.root_path) / "cards");
    std::ofstream(std::filesystem::path(project.root_path) / "cards" / "unsafe.md")
        << "plaintext";
    auto [status, payload] =
        call(http::verb::post, "/projects/proj-1/git/push", nlohmann::json::object());
    REQUIRE(status == http::status::bad_request);
    REQUIRE(payload["error"]["message"].get<std::string>().find("cards/unsafe.md") !=
            std::string::npos);
    REQUIRE(git.calls == std::vector<std::string>{"remote"});
  }

  SECTION("git push serializes empty local_head_commit as null") {
    git.push_result = {
        .status = holder::git::PushStatus::Pushed,
        .ahead_count = 0,
        .behind_count = 0,
        .local_head_commit = "",
        .error_message = {},
    };
    auto [status, payload] =
        call(http::verb::post, "/projects/proj-1/git/push", nlohmann::json::object());
    REQUIRE(status == http::status::ok);
    REQUIRE(payload["data"]["status"] == "pushed");
    REQUIRE(payload["data"]["local_head_commit"].is_null());
  }

  SECTION("git push serializes non-empty local_head_commit") {
    git.push_result = {
        .status = holder::git::PushStatus::Pushed,
        .ahead_count = 0,
        .behind_count = 0,
        .local_head_commit = "abc123def456",
        .error_message = {},
    };
    auto [status, payload] =
        call(http::verb::post, "/projects/proj-1/git/push", nlohmann::json::object());
    REQUIRE(status == http::status::ok);
    REQUIRE(payload["data"]["status"] == "pushed");
    REQUIRE(payload["data"]["local_head_commit"] == "abc123def456");
  }

  SECTION("git push bad json catch") {
    auto req = make_request(http::verb::post, "/projects/proj-1/git/push");
    req.body() = "{";
    req.prepare_payload();
    http::response<http::string_body> res;
    const bool handled = holder::api::dispatch_generated_routes(
        req, res, db, nullptr, nullptr, nullptr, &git, uuid_v4);
    REQUIRE(handled);
    REQUIRE(res.result() == http::status::bad_request);
  }

  SECTION("git routes report a missing project") {
    for (const std::string action : {"test-remote", "push", "pull", "sync"}) {
      CAPTURE(action);
      auto [status, payload] = call(http::verb::post, "/projects/missing/git/" + action);
      REQUIRE(status == http::status::not_found);
      REQUIRE(payload["error"]["message"] == "Project not found.");
    }
  }

  SECTION("missing key material is an internal privacy failure") {
    const holder::privacy::PrivacyError missing(
        holder::privacy::PrivacyErrorCode::KeyMaterialMissing, "missing");
    REQUIRE_THROWS_AS(holder::api::support::throw_privacy_failure(missing),
                      holder::api::support::PrivacyBroken);
  }

  SECTION("forced pull returns structured success") {
    auto [status, payload] =
        call(http::verb::post, "/projects/proj-1/git/pull", nlohmann::json::object());
    REQUIRE(status == http::status::ok);
    REQUIRE(payload["data"]["status"] == "succeeded");
    REQUIRE(payload["data"]["error_code"].is_null());
    REQUIRE(payload["data"]["pull"]["attempted"] == true);
    REQUIRE(payload["data"]["pull"]["status"] == "succeeded");
    REQUIRE(payload["data"]["push"]["attempted"] == false);
    REQUIRE(git.calls == std::vector<std::string>{"remote", "pull"});
  }

  SECTION("forced sync pulls before pushing") {
    auto [status, payload] =
        call(http::verb::post, "/projects/proj-1/git/sync", nlohmann::json::object());
    REQUIRE(status == http::status::ok);
    REQUIRE(payload["data"]["status"] == "succeeded");
    REQUIRE(payload["data"]["pull"]["status"] == "succeeded");
    REQUIRE(payload["data"]["push"]["attempted"] == true);
    REQUIRE(payload["data"]["push"]["status"] == "pushed");
    REQUIRE(git.calls == std::vector<std::string>{"remote", "pull", "remote", "push"});
  }

  SECTION("forced sync does not push after pull failure") {
    git.throw_on_pull = true;
    auto [status, payload] =
        call(http::verb::post, "/projects/proj-1/git/sync", nlohmann::json::object());
    REQUIRE(status == http::status::ok);
    REQUIRE(payload["data"]["status"] == "failed");
    REQUIRE(payload["data"]["error_code"] == "pull_failed");
    REQUIRE(payload["data"]["pull"]["status"] == "failed");
    REQUIRE(payload["data"]["push"]["attempted"] == false);
    REQUIRE(git.calls == std::vector<std::string>{"remote", "pull"});
  }

  SECTION("forced sync reports push failure") {
    git.push_result = {
        .status = holder::git::PushStatus::AuthFailed,
        .ahead_count = 1,
        .behind_count = 0,
        .error_message = "push authentication failed",
    };
    auto [status, payload] = call(
        http::verb::post,
        "/projects/proj-1/git/sync",
        {{"branch", "main"}, {"set_upstream", nullptr}}
    );
    REQUIRE(status == http::status::ok);
    REQUIRE(payload["data"]["status"] == "failed");
    REQUIRE(payload["data"]["error_code"] == "auth_failed");
    REQUIRE(payload["data"]["error_message"] == "push authentication failed");
  }

  SECTION("forced sync rejects a missing project") {
    auto [status, payload] =
        call(http::verb::post, "/projects/missing/git/sync", nlohmann::json::object());
    REQUIRE(status == http::status::not_found);
    REQUIRE(payload["error"]["code"] == "not_found");
  }

  SECTION("forced sync rejects malformed input") {
    auto req = make_request(http::verb::post, "/projects/proj-1/git/sync");
    req.body() = "{";
    req.prepare_payload();
    http::response<http::string_body> res;
    const bool handled = holder::api::dispatch_generated_routes(
        req, res, db, nullptr, nullptr, nullptr, &git, uuid_v4);
    REQUIRE(handled);
    REQUIRE(res.result() == http::status::bad_request);
  }

  SECTION("sync-status storage failure is an internal error") {
    db.close();
    auto [status, payload] = call(http::verb::get, "/projects/proj-1/git/sync-status");
    REQUIRE(status == http::status::internal_server_error);
    REQUIRE(payload["ok"] == false);
  }

  SECTION("sync-status is served by the generated route") {
    auto req = make_request(http::verb::get, "/projects/proj-1/git/sync-status");
    http::response<http::string_body> res;
    REQUIRE(holder::api::dispatch_generated_routes(req, res, db));
    REQUIRE(res.result() == http::status::ok);
  }

  SECTION("sync-status returns default sync object when no row exists") {
    auto [status, payload] = call(http::verb::get, "/projects/proj-1/git/sync-status");
    REQUIRE(status == http::status::ok);
    const nlohmann::json expected = {
        {"ok", true},
        {"data",
         {{"project_id", "proj-1"},
          {"sync",
           {{"last_commit_at", nullptr},
            {"last_push_at", nullptr},
            {"last_pull_at", nullptr},
            {"uncommitted_changes_count", 0},
            {"unpushed_commits_count", 0},
            {"last_push_status", nullptr},
            {"last_pull_status", nullptr},
            {"last_sync_error", nullptr},
            {"last_sync_error_at", nullptr},
            {"retry_count", 0},
            {"next_retry_at", nullptr},
            {"pull_retry_count", 0},
            {"next_pull_retry_at", nullptr},
            {"updated_at", nullptr}}}}}
    };
    REQUIRE(payload == expected);
  }

  SECTION("sync-status maps existing zero updated_at to null") {
    holder::project::ProjectSyncRepo sync_repo(db);
    (void)sync_repo;
    db.exec(
        "INSERT INTO project_sync_state(project_id,uncommitted_changes_count,unpushed_commits_count,"
        "retry_count,pull_retry_count,updated_at) VALUES('proj-1',0,0,0,0,0) "
        "ON CONFLICT(project_id) DO UPDATE SET updated_at=0;"
    );

    auto [status, payload] = call(http::verb::get, "/projects/proj-1/git/sync-status");
    REQUIRE(status == http::status::ok);
    REQUIRE(payload["data"]["sync"]["updated_at"].is_null());
  }

  SECTION("git push best-effort metrics catch still returns payload") {
    // No repo exists at root_path; inspect_repo_sync_metrics will throw and be swallowed.
    git.push_result = {
        .status = holder::git::PushStatus::Pushed,
        .ahead_count = 0,
        .behind_count = 0,
        .error_message = {},
    };
    auto [status, payload] =
        call(http::verb::post, "/projects/proj-1/git/push", nlohmann::json::object());
    REQUIRE(status == http::status::ok);
    REQUIRE(payload["ok"] == true);
    REQUIRE(payload["data"]["status"] == "pushed");
  }

  SECTION("generated project get returns an internal error with closed db") {
    db.close();
    auto req = make_request(http::verb::get, "/projects/proj-1");
    http::response<http::string_body> res;
    REQUIRE(holder::api::dispatch_generated_routes(req, res, db));
    REQUIRE(res.result() == http::status::internal_server_error);
    const auto payload = nlohmann::json::parse(res.body());
    REQUIRE(payload["error"]["code"] == "error");
  }

  SECTION("project delete internal error catch with closed db") {
    db.close();
    auto [status, payload] = call(http::verb::delete_, "/projects/proj-1");
    REQUIRE(status == http::status::internal_server_error);
    REQUIRE(payload["error"]["code"] == "error");
  }

  SECTION("project metadata update persists successfully") {
    auto [status, payload] =
        call(http::verb::patch, "/projects/proj-1", {{"name", "Renamed"}, {"updated_at", 2}});
    REQUIRE(status == http::status::ok);
    REQUIRE(payload["data"]["project_id"] == "proj-1");
    REQUIRE_FALSE(payload["data"].contains("git_remote_changed"));
    REQUIRE(holder::project::ProjectRepo(db).get("proj-1")->name == "Renamed");
  }

  SECTION("project update ignores null values for name, root and privacy") {
    auto [status, payload] = call(http::verb::patch, "/projects/proj-1",
                                  {{"name", nullptr}, {"root_path", nullptr},
                                   {"privacy_mode", nullptr}, {"updated_at", 2}});
    REQUIRE(status == http::status::bad_request);
    REQUIRE(payload["error"]["code"] == "bad_request");
    REQUIRE(holder::project::ProjectRepo(db).get("proj-1")->name == "Proj One");
  }

  SECTION("project update clears nullable remote and provider") {
    auto [status, payload] = call(http::verb::patch, "/projects/proj-1",
                                  {{"git_remote_url", nullptr}, {"git_provider", nullptr},
                                   {"updated_at", 2}});
    REQUIRE(status == http::status::ok);
    REQUIRE(payload["data"]["git_remote_changed"] == true);
    const auto updated = holder::project::ProjectRepo(db).get("proj-1");
    REQUIRE_FALSE(updated->git_remote_url.has_value());
    REQUIRE_FALSE(updated->git_provider.has_value());
  }

  SECTION("project update does not persist a rejected remote") {
    git.throw_on_set_remote = true;
    auto [status, payload] = call(http::verb::patch, "/projects/proj-1",
                                  {{"git_remote_url", "https://example.com/new.git"},
                                   {"updated_at", 2}});
    REQUIRE(status == http::status::bad_request);
    REQUIRE(payload["error"]["code"] == "bad_request");
    REQUIRE(git.calls == std::vector<std::string>{"remote"});
    REQUIRE(holder::project::ProjectRepo(db).get("proj-1")->git_remote_url ==
            project.git_remote_url);
  }
}

TEST_CASE("RIG route lists project tags with card counts", "[project-routes][tags]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  holder::test::create_project(db, "proj-1", (dir / "repo").string());
  db.exec(
      "INSERT INTO cards(card_id, project_id, title, rel_path, sort_key, created_at, updated_at) "
      "VALUES('card-1','proj-1','One','cards/one.md',1,1,1),"
      "('card-2','proj-1','Two','cards/two.md',2,2,2);"
  );
  holder::card::TagRepo tags(db);
  tags.set_tags_for_card("proj-1", "card-1", {"android", "sync"}, 1);
  tags.set_tags_for_card("proj-1", "card-2", {"sync"}, 2);

  auto req = make_request(http::verb::get, "/projects/proj-1/tags");
  http::response<http::string_body> res;
  const bool handled = holder::api::dispatch_generated_routes(req, res, db);

  REQUIRE(handled);
  REQUIRE(res.result() == http::status::ok);
  const auto payload = nlohmann::json::parse(res.body());
  const nlohmann::json expected = {
      {"ok", true},
      {"data",
       nlohmann::json::array({
           {{"tag", "sync"}, {"card_count", 2}},
           {{"tag", "android"}, {"card_count", 1}},
       })}
  };
  REQUIRE(payload == expected);

  req = make_request(http::verb::get, "/projects/missing/tags");
  REQUIRE_FALSE(holder::api::is_public_generated_route(req));
  REQUIRE(holder::api::dispatch_generated_routes(req, res, db));
  REQUIRE(res.result() == http::status::not_found);
  REQUIRE(nlohmann::json::parse(res.body())["error"]["code"] == "not_found");

  req = make_request(http::verb::get, "/projects/bad%XX/tags");
  REQUIRE(holder::api::dispatch_generated_routes(req, res, db));
  REQUIRE(res.result() == http::status::bad_request);
  REQUIRE(nlohmann::json::parse(res.body())["error"]["code"] == "bad_request");

  req = make_request(http::verb::get, "/projects");
  REQUIRE(holder::api::dispatch_generated_routes(req, res, db));
  REQUIRE(res.result() == http::status::ok);
  const auto listed = nlohmann::json::parse(res.body());
  REQUIRE(listed["data"].size() == 1);
  REQUIRE(listed["data"][0]["git_remote_url"].is_null());
  REQUIRE(listed["data"][0]["project_key_id"].is_null());
  REQUIRE_FALSE(listed["data"][0].contains("card_count"));
  REQUIRE(listed["data"][0]["sync"]["uncommitted_changes_count"] == 0);
  REQUIRE(listed["data"][0]["sync"]["updated_at"].is_null());

  req = make_request(http::verb::get, "/projects/proj-1");
  REQUIRE_FALSE(holder::api::is_public_generated_route(req));
  REQUIRE(holder::api::dispatch_generated_routes(req, res, db));
  REQUIRE(res.result() == http::status::ok);
  const auto fetched = nlohmann::json::parse(res.body());
  REQUIRE(fetched["data"] == listed["data"][0]);

  req = make_request(http::verb::get, "/projects/missing");
  REQUIRE(holder::api::dispatch_generated_routes(req, res, db));
  REQUIRE(res.result() == http::status::not_found);
  REQUIRE(nlohmann::json::parse(res.body())["error"]["code"] == "not_found");

  req = make_request(http::verb::delete_, "/projects/missing");
  REQUIRE(holder::api::dispatch_generated_routes(req, res, db));
  REQUIRE(res.result() == http::status::not_found);
  REQUIRE(nlohmann::json::parse(res.body())["error"]["code"] == "not_found");

  req = make_request(http::verb::patch, "/projects/missing",
                     {{"name", "Renamed"}, {"updated_at", 2}});
  REQUIRE(holder::api::dispatch_generated_routes(req, res, db));
  REQUIRE(res.result() == http::status::not_found);
  REQUIRE(nlohmann::json::parse(res.body())["error"]["code"] == "not_found");

  req = make_request(http::verb::post, "/projects/proj-1/recovery-token/export");
  REQUIRE(holder::api::dispatch_generated_routes(req, res, db));
  REQUIRE(res.result() == http::status::bad_request);

  // Hand-served operations are declared for the contract; the host serves them.
  req = make_request(http::verb::get, "/openapi.yaml");
  REQUIRE(holder::api::is_public_generated_route(req));
  REQUIRE_FALSE(holder::api::dispatch_generated_routes(req, res, db));

  db.close();
  req = make_request(http::verb::get, "/projects/proj-1/tags");
  REQUIRE(holder::api::dispatch_generated_routes(req, res, db));
  REQUIRE(res.result() == http::status::internal_server_error);
  REQUIRE(nlohmann::json::parse(res.body())["error"]["code"] == "error");
}

TEST_CASE("ProjectRoutes recovery import and encryption-check branches", "[project-routes]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  ProjectRoutesTestGitOps git;

  auto call = [&](http::verb method,
                  const std::string& path,
                  const nlohmann::json& body = nlohmann::json()) {
    auto req = make_request(method, path, body);
    http::response<http::string_body> res;
    const bool handled = route_project(req, res, db, &git);
    REQUIRE(handled);
    return std::make_pair(res.result(), nlohmann::json::parse(res.body()));
  };

  SECTION("global recovery import can mark pull succeeded with remote hint") {
    const auto repo_root = dir / "enc-repo";
    std::filesystem::create_directories(repo_root);

    auto [create_status, create_payload] = call(
        http::verb::post,
        "/projects",
        {{"project_id", "proj-enc"},
         {"name", "Encrypted"},
         {"root_path", repo_root.string()},
         {"privacy_mode", "encrypted_git"},
         {"git_remote_url", "https://example.com/recovered.git"},
         {"created_at", 10},
         {"updated_at", 10}}
    );
    REQUIRE(create_status == http::status::created);
    REQUIRE(create_payload["ok"] == true);

    auto [export_status, export_payload] =
        call(http::verb::post, "/projects/proj-enc/recovery-token/export", {{"pin", "1234"}});
    REQUIRE(export_status == http::status::ok);
    const std::string token = export_payload["data"]["recovery_token"].get<std::string>();

    auto [del_status, del_payload] = call(http::verb::delete_, "/projects/proj-enc");
    REQUIRE(del_status == http::status::ok);
    REQUIRE(del_payload["ok"] == true);

    auto [import_status, import_payload] = call(
        http::verb::post,
        "/recovery-token/import",
        {{"pin", "1234"}, {"recovery_token", token}}
    );
    REQUIRE(import_status == http::status::created);
    REQUIRE(import_payload["ok"] == true);
    REQUIRE(import_payload["data"]["remote_hint_present"] == true);
    REQUIRE(import_payload["data"]["remote_configured"] == true);
    REQUIRE(import_payload["data"]["pull_status"] == "succeeded");
  }

  SECTION("encryption-check plain and encrypted branches plus storage failure") {
    holder::project::ProjectRepo repo(db);

    holder::model::Project plain;
    plain.project_id = "proj-plain";
    plain.name = "Plain";
    plain.root_path = (dir / "plain").string();
    plain.privacy_mode = "plain";
    plain.created_at = 1;
    plain.updated_at = 1;
    repo.create(plain);

    auto [plain_status, plain_payload] =
        call(http::verb::get, "/projects/proj-plain/encryption-check");
    REQUIRE(plain_status == http::status::ok);
    REQUIRE(plain_payload["data"]["check"]["checked_files"] == 0);
    REQUIRE(plain_payload["data"]["check"]["unsafe_files"] == 0);

    holder::model::Project enc;
    enc.project_id = "proj-encrypted";
    enc.name = "Encrypted";
    enc.root_path = (dir / "encrypted").string();
    enc.privacy_mode = "encrypted_git";
    enc.created_at = 2;
    enc.updated_at = 2;
    repo.create(enc);
    std::filesystem::create_directories(enc.root_path);

    auto [enc_status, enc_payload] =
        call(http::verb::get, "/projects/proj-encrypted/encryption-check");
    REQUIRE(enc_status == http::status::ok);
    REQUIRE(enc_payload["data"]["check"].contains("unsafe_files"));
    const bool unsafe_files_is_int =
        enc_payload["data"]["check"]["unsafe_files"].is_number_unsigned() ||
        enc_payload["data"]["check"]["unsafe_files"].is_number_integer();
    REQUIRE(unsafe_files_is_int);

    db.close();
    auto [err_status, err_payload] =
        call(http::verb::get, "/projects/proj-encrypted/encryption-check");
    REQUIRE(err_status == http::status::internal_server_error);
    REQUIRE(err_payload["ok"] == false);
  }
}

TEST_CASE("ProjectRoutes create and recovery required field branches", "[project-routes]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  ProjectRoutesTestGitOps git;

  auto call = [&](http::verb method,
                  const std::string& path,
                  const nlohmann::json& body = nlohmann::json()) {
    auto req = make_request(method, path, body);
    http::response<http::string_body> res;
    const bool handled = route_project(req, res, db, &git);
    REQUIRE(handled);
    return std::make_pair(res.result(), nlohmann::json::parse(res.body()));
  };

  SECTION("projects post missing name") {
    auto [status, payload] = call(http::verb::post, "/projects", {{"x", 1}});
    REQUIRE(status == http::status::bad_request);
    REQUIRE(payload["error"]["code"] == "bad_request");
  }

  SECTION("projects post parses project_key_id") {
    auto [status, payload] = call(
        http::verb::post,
        "/projects",
        {{"project_id", "proj-2"},
         {"name", "Two"},
         {"root_path", (dir / "repo2").string()},
         {"project_key_id", "k1"},
         {"privacy_mode", "plain"},
         {"created_at", 1},
         {"updated_at", 1}}
    );
    REQUIRE(status == http::status::created);
    REQUIRE(payload["ok"] == true);
  }

  SECTION("project recovery export missing pin") {
    holder::project::ProjectRepo repo(db);
    holder::model::Project project;
    project.project_id = "proj-1";
    project.name = "One";
    project.root_path = (dir / "repo1").string();
    project.privacy_mode = "plain";
    project.project_key_id = std::string("kid");
    project.created_at = 1;
    project.updated_at = 1;
    repo.create(project);

    auto [status, payload] =
        call(http::verb::post, "/projects/proj-1/recovery-token/export", {{"x", 1}});
    REQUIRE(status == http::status::bad_request);
    REQUIRE(payload["error"]["code"] == "bad_request");
  }

  SECTION("project recovery import missing fields") {
    holder::project::ProjectRepo repo(db);
    holder::model::Project project;
    project.project_id = "proj-1";
    project.name = "One";
    project.root_path = (dir / "repo1").string();
    project.privacy_mode = "plain";
    project.created_at = 1;
    project.updated_at = 1;
    repo.create(project);

    auto [status, payload] =
        call(http::verb::post, "/projects/proj-1/recovery-token/import", {{"pin", "1234"}});
    REQUIRE(status == http::status::bad_request);
    REQUIRE(payload["error"]["code"] == "bad_request");
  }
}
