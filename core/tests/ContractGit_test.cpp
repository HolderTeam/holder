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
