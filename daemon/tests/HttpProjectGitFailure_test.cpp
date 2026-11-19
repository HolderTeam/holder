#include "http_test_helpers.h"

#include "git/GitOps.h"
#include "model/Project.h"
#include "project/ProjectManifest.h"
#include "project/ProjectRepo.h"

#include <filesystem>
#include <stdexcept>

namespace {

class FailingGitOps final : public holder::git::GitOps {
  holder::git::RealGitOps repo_;

 public:
  bool fail_set_remote = false;
  bool fail_remove_remote = false;

  void open_or_init(const std::filesystem::path& repo_dir) override {
    repo_.open_or_init(repo_dir);
  }
  void write_file(const std::filesystem::path&, const std::string&) override {}
  void stage_path(const std::filesystem::path&) override {}
  void remove_path(const std::filesystem::path&) override {}
  void commit(const std::string&) override {}
  void set_remote(const std::string&, const std::string&) override {
    if (fail_set_remote) throw std::runtime_error("set remote failed");
  }
  void remove_remote(const std::string&) override {
    if (fail_remove_remote) throw std::runtime_error("remove remote failed");
  }
  void pull_remote_ff_only(const std::string&) override {}
  holder::git::RemoteProbeResult probe_remote(const std::string&) override {
    return {
        .status = holder::git::RemoteProbeStatus::Reachable,
        .remote_has_head = true,
        .error_message = {}
    };
  }
  holder::git::PushResult push_branch(const std::string&, const std::string&, bool) override {
    return {
        .status = holder::git::PushStatus::Pushed,
        .ahead_count = 0,
        .behind_count = 0,
        .error_message = {}
    };
  }
  std::filesystem::path repo_dir() const override { return repo_.repo_dir(); }
};

} // namespace

TEST_CASE("Project patch preserves stored remote when git remove_remote fails", "[git][http]") {
  const auto dir = holder::test::make_temp_dir();
  const auto db_path = dir / "holder.db";
  auto db = holder::test::open_db_with_schema(db_path);

  holder::project::ProjectRepo repo(db);
  holder::model::Project project;
  project.project_id = "proj-1";
  project.name = "Project";
  project.root_path = (dir / "repo").string();
  project.git_remote_url = std::string("git@example.com:repo.git");
  project.created_at = 1;
  project.updated_at = 1;
  repo.create(project);

  FailingGitOps git;
  git.fail_remove_remote = true;

  const std::string token = "testtoken";
  holder::api::HttpServer server("127.0.0.1", 0, db, token, nullptr, nullptr, &git);
  holder::api::HttpServer::BoundInfo bound;
  try {
    bound = server.start();
  } catch (const std::exception& ex) {
    SKIP(std::string("Socket bind not available in test environment: ") + ex.what());
  }

  holder::core::SignalHandler signals;
  holder::test::HttpServerThreadGuard server_thread(server, signals);
  REQUIRE(holder::test::wait_for_http_health_ready(bound.bind, bound.port, token));

  nlohmann::json patch_body = {{"git_remote_url", nullptr}, {"updated_at", 2}};

  const auto patch = holder::test::http_json_request(
      bound.bind,
      bound.port,
      token,
      boost::beast::http::verb::patch,
      "/projects/proj-1",
      patch_body,
      boost::beast::http::status::bad_request
  );
  REQUIRE(patch["ok"] == false);

  const auto fetched = repo.get("proj-1");
  REQUIRE(fetched.has_value());
  REQUIRE(fetched->git_remote_url == project.git_remote_url);
  REQUIRE(fetched->updated_at == project.updated_at);
}

TEST_CASE("Project patch preserves stored remote when git set_remote fails", "[git][http]") {
  const auto dir = holder::test::make_temp_dir();
  const auto db_path = dir / "holder.db";
  auto db = holder::test::open_db_with_schema(db_path);

  holder::project::ProjectRepo repo(db);
  holder::model::Project project;
  project.project_id = "proj-1";
  project.name = "Project";
  project.root_path = (dir / "repo").string();
  project.created_at = 1;
  project.updated_at = 1;
  repo.create(project);

  FailingGitOps git;
  git.fail_set_remote = true;

  const std::string token = "testtoken";
  holder::api::HttpServer server("127.0.0.1", 0, db, token, nullptr, nullptr, &git);
  holder::api::HttpServer::BoundInfo bound;
  try {
    bound = server.start();
  } catch (const std::exception& ex) {
    SKIP(std::string("Socket bind not available in test environment: ") + ex.what());
  }

  holder::core::SignalHandler signals;
  holder::test::HttpServerThreadGuard server_thread(server, signals);
  REQUIRE(holder::test::wait_for_http_health_ready(bound.bind, bound.port, token));

  nlohmann::json patch_body = {{"git_remote_url", "git@example.com:repo.git"}, {"updated_at", 2}};

  const auto patch = holder::test::http_json_request(
      bound.bind,
      bound.port,
      token,
      boost::beast::http::verb::patch,
      "/projects/proj-1",
      patch_body,
      boost::beast::http::status::bad_request
  );
  REQUIRE(patch["ok"] == false);

  const auto fetched = repo.get("proj-1");
  REQUIRE(fetched.has_value());
  REQUIRE_FALSE(fetched->git_remote_url.has_value());
  REQUIRE(fetched->updated_at == project.updated_at);
}

TEST_CASE("Global recovery import keeps success when git remote setup fails", "[git][http]") {
  const auto dir = holder::test::make_temp_dir();
  holder::test::EnvGuard config_env("XDG_CONFIG_HOME", (dir / "config").string());
  const auto db_path = dir / "holder.db";
  auto db = holder::test::open_db_with_schema(db_path);
  holder::test::ensure_uuid_seeded();

  const auto projects_root = dir / "projects_root";
  std::filesystem::create_directories(projects_root);
  holder::test::EnvGuard root_env("HOLDER_PROJECTS_ROOT", projects_root.string());

  const std::string token = "testtoken";

  // First server exports a token with remote hint.
  {
    holder::api::HttpServer export_server("127.0.0.1", 0, db, token, nullptr, nullptr);
    holder::api::HttpServer::BoundInfo bound;
    try {
      bound = export_server.start();
    } catch (const std::exception& ex) {
      SKIP(std::string("Socket bind not available in test environment: ") + ex.what());
    }

    holder::core::SignalHandler signals;
    holder::test::HttpServerThreadGuard server_thread(export_server, signals);
    REQUIRE(holder::test::wait_for_http_health_ready(bound.bind, bound.port, token));

    holder::test::http_json_request(
        bound.bind,
        bound.port,
        token,
        boost::beast::http::verb::post,
        "/projects",
        {{"project_id", "proj-1"},
         {"name", "Project"},
         {"git_remote_url", "https://example.com/recovered.git"}},
        boost::beast::http::status::created
    );

    const auto exported = holder::test::http_json_request(
        bound.bind,
        bound.port,
        token,
        boost::beast::http::verb::post,
        "/projects/proj-1/recovery-token/export",
        {{"pin", "1234"}},
        boost::beast::http::status::ok
    );
    const std::string recovery_token = exported["data"]["recovery_token"].get<std::string>();

    holder::test::http_json_request(
        bound.bind,
        bound.port,
        token,
        boost::beast::http::verb::delete_,
        "/projects/proj-1",
        nlohmann::json::object(),
        boost::beast::http::status::ok
    );

    server_thread.stop();

    // Second server forces set_remote failure during import.
    FailingGitOps git;
    git.fail_set_remote = true;
    holder::api::HttpServer import_server("127.0.0.1", 0, db, token, nullptr, nullptr, &git);
    holder::api::HttpServer::BoundInfo import_bound;
    try {
      import_bound = import_server.start();
    } catch (const std::exception& ex) {
      SKIP(std::string("Socket bind not available in test environment: ") + ex.what());
    }

    holder::core::SignalHandler import_signals;
    holder::test::HttpServerThreadGuard import_thread(import_server, import_signals);
    REQUIRE(holder::test::wait_for_http_health_ready(import_bound.bind, import_bound.port, token));

    const auto imported = holder::test::http_json_request(
        import_bound.bind,
        import_bound.port,
        token,
        boost::beast::http::verb::post,
        "/recovery-token/import",
        {{"pin", "1234"}, {"recovery_token", recovery_token}},
        boost::beast::http::status::created
    );
    REQUIRE(imported["ok"] == true);
    REQUIRE(imported["data"]["project_id"] == "proj-1");
    REQUIRE(imported["data"]["project_created"] == true);
    REQUIRE(imported["data"]["remote_hint_present"] == true);
    REQUIRE(imported["data"]["remote_configured"] == false);
    REQUIRE(imported["data"]["remote_error"] == "set remote failed");
    REQUIRE(imported["data"]["pull_status"] == "not_attempted");
    REQUIRE(imported["data"]["pull_error"].is_null());

    import_thread.stop();
    const auto recovered = holder::project::ProjectRepo(db).get("proj-1");
    REQUIRE(recovered.has_value());
    const auto root = std::filesystem::path(recovered->root_path);
    REQUIRE(std::filesystem::exists(root / holder::project::kPendingRecoveryPath));
    REQUIRE_FALSE(holder::project::has_project_manifest(root));
  }
}
