// The contract's Git sync operations against a local bare repository as the remote.
#if __has_include(<catch2/catch_test_macros.hpp>)
#include <catch2/catch_test_macros.hpp>
#else
#include <catch2/catch.hpp>
#endif

#include "api/Contract.h"
#include "capi/Context.h"
#include "core_test_helpers.h"
#include "git/GitOps.h"

#include <git2.h>
#include <holder/holder_api.h>
#include <memory>

#include <filesystem>
#include <string>

namespace {

namespace contract = holder::contract;

struct Host {
  std::filesystem::path dir = holder::test::make_temp_dir();
  holder::platform::Db db = holder::test::open_db_with_schema(dir / "holder.db");
  holder::index::FtsIndexer fts{db};
  holder::git::RealGitOps git;
  holder_context context{db, fts, &git};
  std::string project_id;

  Host() {
    context.projects_root = dir / "projects";
    contract::NewProject project;
    project.name = "Notes";
    project.privacy_mode = contract::NewProjectPrivacyMode::Plain;
    project_id = contract::create_project(context, project).project_id;
  }

  // A bare repository to push to and pull from.
  std::string remote() {
    const auto path = dir / "remote.git";
    git_repository* repo = nullptr;
    REQUIRE(git_repository_init(&repo, path.string().c_str(), 1) == 0);
    git_repository_free(repo);
    contract::ProjectUpdate update;
    update.git_remote_url = std::optional<std::string>(path.string());
    contract::update_project(context, project_id, update);
    return path.string();
  }
};

} // namespace

TEST_CASE("A project without a remote reports it, and records nothing as pushed", "[contract][git]") {
  Host host;
  auto state = contract::get_project_sync_state(host.context, host.project_id);
  REQUIRE(state.project_id == host.project_id);
  REQUIRE(state.last_push_at == std::nullopt);

  const auto pushed = contract::push_project(host.context, host.project_id, contract::PushOptions{});
  REQUIRE(pushed.project_id == host.project_id);
  REQUIRE(pushed.status == contract::SyncResultStatus::Failed);
  REQUIRE(pushed.remote_url == std::nullopt);
  REQUIRE_FALSE(pushed.pull.attempted);
  REQUIRE(pushed.pull.status == contract::PullPhaseStatus::NotAttempted);
  REQUIRE(pushed.push.status == contract::PushPhaseStatus::RemoteUnset);
  REQUIRE(contract::get_project_sync_state(host.context, host.project_id).last_push_status == "remote_unset");

  const auto tested = contract::test_project_remote(host.context, host.project_id, contract::RemoteTest{});
  REQUIRE(tested.project_id == host.project_id);
  REQUIRE(tested.status == contract::RemoteTestResultStatus::RemoteUnset);
  REQUIRE(tested.remote_url == std::nullopt);

  REQUIRE_THROWS_AS(contract::pull_project(host.context, "no-such-project"), contract::NotFound);
  REQUIRE_THROWS_AS(contract::get_project_sync_state(host.context, "no-such-project"), contract::NotFound);
}

TEST_CASE("A project pushes, pulls and syncs with its remote, and each records its outcome", "[contract][git]") {
  Host host;
  const auto remote = host.remote();

  const auto tested = contract::test_project_remote(host.context, host.project_id, contract::RemoteTest{});
  REQUIRE(tested.status == contract::RemoteTestResultStatus::Reachable);
  REQUIRE(tested.remote_url == remote);
  REQUIRE_FALSE(tested.remote_has_head);
  REQUIRE(tested.error_message == std::nullopt);

  const auto pushed = contract::push_project(host.context, host.project_id, contract::PushOptions{});
  REQUIRE(pushed.status == contract::SyncResultStatus::Succeeded);
  REQUIRE(pushed.remote_url == remote);
  REQUIRE(pushed.push.attempted);
  REQUIRE(pushed.push.status == contract::PushPhaseStatus::Pushed);
  REQUIRE(pushed.push.local_head_commit.has_value());
  REQUIRE_FALSE(pushed.pull.attempted);
  const auto state = contract::get_project_sync_state(host.context, host.project_id);
  REQUIRE(state.last_push_status == "pushed");
  REQUIRE(state.last_push_at.has_value());

  const auto pulled = contract::pull_project(host.context, host.project_id);
  REQUIRE(pulled.status == contract::SyncResultStatus::Succeeded);
  REQUIRE(pulled.pull.status == contract::PullPhaseStatus::Succeeded);
  REQUIRE_FALSE(pulled.push.attempted);
  REQUIRE(pulled.push.status == contract::PushPhaseStatus::NotAttempted);

  const auto synced = contract::sync_project(host.context, host.project_id, contract::PushOptions{});
  REQUIRE(synced.status == contract::SyncResultStatus::Succeeded);
  REQUIRE(synced.pull.status == contract::PullPhaseStatus::Succeeded);
  REQUIRE(synced.push.status == contract::PushPhaseStatus::UpToDate);

  // Another remote, tested without changing the project.
  contract::RemoteTest other;
  other.remote_url = (host.dir / "missing.git").string();
  const auto missing = contract::test_project_remote(host.context, host.project_id, other);
  REQUIRE(missing.status != contract::RemoteTestResultStatus::Reachable);
  REQUIRE(missing.remote_url == other.remote_url);
  REQUIRE(missing.error_message.has_value());
  REQUIRE(contract::get_project(host.context, host.project_id).git_remote_url == remote);
}

TEST_CASE("Scheduled contract sync respects remote and cadence", "[contract][git]") {
  Host host;
  const auto without_remote = contract::sync_project_if_due(host.context, host.project_id, 3600, 3600);
  REQUIRE_FALSE(without_remote.pull.attempted);
  REQUIRE_FALSE(without_remote.push.attempted);
  REQUIRE(without_remote.remote_url == std::nullopt);

  const auto remote = host.remote();
  const auto first = contract::sync_project_if_due(host.context, host.project_id, 3600, 3600);
  REQUIRE(first.remote_url == remote);
  REQUIRE(first.pull.attempted);
  REQUIRE(first.push.attempted);
  const auto second = contract::sync_project_if_due(host.context, host.project_id, 3600, 3600);
  REQUIRE_FALSE(second.pull.attempted);
  REQUIRE_FALSE(second.push.attempted);
  REQUIRE_THROWS_AS(contract::sync_project_if_due(host.context, "no-such-project", 0, 0), contract::NotFound);
}

namespace {
// Real local file operations with deterministic remote outcomes; no network is used.
class RemoteOutcomes final : public holder::git::GitOps {
 public:
  explicit RemoteOutcomes(holder::git::RealGitOps& real) : real_(real) {}
  holder::git::PushResult push_result;
  holder::git::RemoteProbeResult probe_result;
  std::string probed_url;
  void open_or_init(const std::filesystem::path& path) override { real_.open_or_init(path); }
  void write_file(const std::filesystem::path& path, const std::string& body) override {
    real_.write_file(path, body);
  }
  void stage_path(const std::filesystem::path& path) override { real_.stage_path(path); }
  void remove_path(const std::filesystem::path& path) override { real_.remove_path(path); }
  void commit(const std::string& message) override { real_.commit(message); }
  void set_remote(const std::string& name, const std::string& url) override {
    real_.set_remote(name, url);
  }
  void remove_remote(const std::string& name) override { real_.remove_remote(name); }
  void pull_remote_ff_only(const std::string& name) override { real_.pull_remote_ff_only(name); }
  holder::git::RemoteProbeResult probe_remote(const std::string&) override { return probe_result; }
  holder::git::RemoteProbeResult probe_remote_url(const std::string& url) override {
    probed_url = url;
    return probe_result;
  }
  holder::git::PushResult push_branch(const std::string&, const std::string&, bool) override {
    return push_result;
  }
  std::filesystem::path repo_dir() const override { return real_.repo_dir(); }

 private:
  holder::git::RealGitOps& real_;
};
} // namespace

TEST_CASE(
    "Contract push preserves remote failure reasons and records retry state",
    "[contract][remote-errors]"
) {
  Host host;
  host.remote();
  RemoteOutcomes remote(host.git);
  host.context.git = &remote;
  using Core = holder::git::PushStatus;
  using Api = contract::PushPhaseStatus;
  const std::pair<Core, Api> cases[] = {
      {Core::AuthFailed, Api::AuthFailed},
      {Core::NotFound, Api::NotFound},
      {Core::NetworkError, Api::NetworkError},
      {Core::NonFastForward, Api::NonFastForward},
      {Core::RemoteUnset, Api::RemoteUnset},
      {Core::UnknownError, Api::UnknownError}
  };
  for (const auto& [core, api] : cases) {
    remote.push_result = {core, 3, 2, "saved-head", "remote refused the push"};
    const auto pushed = contract::push_project(host.context, host.project_id, {});
    REQUIRE(pushed.status == contract::SyncResultStatus::Failed);
    REQUIRE(pushed.push.status == api);
    REQUIRE(pushed.push.attempted);
    REQUIRE_FALSE(pushed.pull.attempted);
    REQUIRE(pushed.push.ahead_count == 3);
    REQUIRE(pushed.push.behind_count == 2);
    REQUIRE(pushed.push.local_head_commit == "saved-head");
    REQUIRE(pushed.push.error_message == "remote refused the push");
    const auto state = contract::get_project_sync_state(host.context, host.project_id);
    REQUIRE(state.last_push_status == holder::git::push_status_name(core));
    REQUIRE(state.last_sync_error == "remote refused the push");
    REQUIRE(state.last_push_at == std::nullopt);
  }
  const holder_push_phase_status statuses[] = {
      HOLDER_PUSH_PHASE_STATUS_AUTH_FAILED,
      HOLDER_PUSH_PHASE_STATUS_NOT_FOUND,
      HOLDER_PUSH_PHASE_STATUS_NETWORK_ERROR,
      HOLDER_PUSH_PHASE_STATUS_NON_FAST_FORWARD,
      HOLDER_PUSH_PHASE_STATUS_REMOTE_UNSET,
      HOLDER_PUSH_PHASE_STATUS_UNKNOWN_ERROR
  };
  const std::unique_ptr<holder_arena, decltype(&holder_arena_free)> arena(
      holder_arena_new(),
      holder_arena_free
  );
  size_t index = 0;
  for (const auto& [core, api] : cases) {
    remote.push_result = {core, 3, 2, "saved-head", "remote refused the push"};
    const holder_sync_result* result = nullptr;
    holder_push_options options{};
    holder_error* error = nullptr;
    const auto status = holder_push_project(
        &host.context,
        host.project_id.c_str(),
        &options,
        arena.get(),
        &result,
        &error
    );
    const std::unique_ptr<holder_error, decltype(&holder_error_destroy)> owned_error(
        error,
        holder_error_destroy
    );
    REQUIRE(status == HOLDER_OK);
    REQUIRE(error == nullptr);
    REQUIRE(result->push.status == statuses[index++]);
    REQUIRE(result->status == HOLDER_SYNC_RESULT_STATUS_FAILED);
    REQUIRE(std::string(result->push.error_message) == "remote refused the push");
    REQUIRE(result->push.ahead_count == 3);
  }
  host.context.git = &host.git;
}

TEST_CASE(
    "Contract remote probes preserve failure categories and the explicit URL",
    "[contract][remote-errors]"
) {
  Host host;
  RemoteOutcomes remote(host.git);
  host.context.git = &remote;
  using Core = holder::git::RemoteProbeStatus;
  using Api = contract::RemoteTestResultStatus;
  const std::pair<Core, Api> cases[] = {
      {Core::AuthFailed, Api::AuthFailed},
      {Core::NotFound, Api::NotFound},
      {Core::NetworkError, Api::NetworkError},
      {Core::InvalidRemoteUrl, Api::InvalidRemoteUrl},
      {Core::RemoteUnset, Api::RemoteUnset},
      {Core::UnknownError, Api::UnknownError}
  };
  contract::RemoteTest request;
  request.remote_url = "ssh://example.invalid/explicit.git";
  for (const auto& [core, api] : cases) {
    remote.probe_result = {core, false, "cannot reach remote"};
    const auto result = contract::test_project_remote(host.context, host.project_id, request);
    REQUIRE(result.status == api);
    REQUIRE(result.error_message == "cannot reach remote");
    REQUIRE(result.remote_url == request.remote_url);
    REQUIRE(remote.probed_url == *request.remote_url);
    REQUIRE_FALSE(result.remote_has_head);
  }
  const holder_remote_test_result_status statuses[] = {
      HOLDER_REMOTE_TEST_RESULT_STATUS_AUTH_FAILED,
      HOLDER_REMOTE_TEST_RESULT_STATUS_NOT_FOUND,
      HOLDER_REMOTE_TEST_RESULT_STATUS_NETWORK_ERROR,
      HOLDER_REMOTE_TEST_RESULT_STATUS_INVALID_REMOTE_URL,
      HOLDER_REMOTE_TEST_RESULT_STATUS_REMOTE_UNSET,
      HOLDER_REMOTE_TEST_RESULT_STATUS_UNKNOWN_ERROR
  };
  const std::unique_ptr<holder_arena, decltype(&holder_arena_free)> arena(
      holder_arena_new(),
      holder_arena_free
  );
  size_t index = 0;
  for (const auto& [core, api] : cases) {
    remote.probe_result = {core, false, "cannot reach remote"};
    const holder_remote_test query{request.remote_url->c_str()};
    const holder_remote_test_result* result = nullptr;
    holder_error* error = nullptr;
    const auto status = holder_test_project_remote(
        &host.context,
        host.project_id.c_str(),
        &query,
        arena.get(),
        &result,
        &error
    );
    const std::unique_ptr<holder_error, decltype(&holder_error_destroy)> owned_error(
        error,
        holder_error_destroy
    );
    REQUIRE(status == HOLDER_OK);
    REQUIRE(error == nullptr);
    REQUIRE(result->status == statuses[index++]);
    REQUIRE(std::string(result->remote_url) == *request.remote_url);
    REQUIRE(std::string(result->error_message) == "cannot reach remote");
  }
  REQUIRE(
      contract::get_project_sync_state(host.context, host.project_id).last_push_at == std::nullopt
  );
  host.context.git = &host.git;
}
