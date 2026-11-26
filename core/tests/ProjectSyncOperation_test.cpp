#if __has_include(<catch2/catch_test_macros.hpp>)
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#else
#include <catch2/catch.hpp>
#endif

#include "sync/ProjectSyncOperation.h"

#include "card/CardFrontMatter.h"
#include "card/CardPaths.h"
#include "card/CardRepo.h"
#include "card/CardStore.h"
#include "core_test_helpers.h"
#include "git/GitRepo.h"
#include "index/FtsIndexer.h"
#include "model/Card.h"
#include "model/Project.h"
#include "project/ProjectRepo.h"
#include "project/ProjectSyncRepo.h"
#include "project/Rebuilder.h"

#include <git2.h>
#include <sqlite3.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <future>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void create_project(
    holder::platform::Db& db,
    const std::string& project_id,
    const std::filesystem::path& root,
    const std::optional<std::string>& remote
) {
  std::filesystem::create_directories(root);
  holder::model::Project project;
  project.project_id = project_id;
  project.name = "Project";
  project.root_path = root.string();
  project.git_remote_url = remote;
  project.privacy_mode = "plain";
  project.created_at = 1;
  project.updated_at = 1;
  holder::project::ProjectRepo(db).create(project);
}

class RecordingGitOps final : public holder::git::GitOps {
 public:
  bool fail_pull = false;
  bool diverge_pull = false;
  bool fail_push = false;
  holder::git::PushResult push_result{holder::git::PushStatus::Pushed, 1, 0, "abc123", {}};
  std::filesystem::path root;
  std::vector<std::string> calls;

  void open_or_init(const std::filesystem::path& path) override {
    root = path;
    calls.push_back("open");
  }
  void write_file(const std::filesystem::path&, const std::string&) override {}
  void stage_path(const std::filesystem::path&) override {}
  void remove_path(const std::filesystem::path&) override {}
  void commit(const std::string&) override {}
  void set_remote(const std::string&, const std::string&) override { calls.push_back("remote"); }
  void remove_remote(const std::string&) override {}
  void pull_remote_ff_only(const std::string&) override {
    calls.push_back("pull");
    if (fail_pull) throw std::runtime_error("pull failed for tests");
    if (diverge_pull) throw holder::git::NonFastForwardPullError("local", "remote", "origin");
  }
  holder::git::GitRepo::DivergedMergeResult
  merge_remote_taking_theirs_for_conflicts(const std::string&, const std::string&, const std::string&)
      override {
    calls.push_back("merge");
    return {};
  }
  holder::git::RemoteProbeResult probe_remote(const std::string&) override { return {}; }
  holder::git::PushResult push_branch(const std::string&, const std::string&, bool) override {
    calls.push_back("push");
    if (fail_push) throw std::runtime_error("push failed for tests");
    return push_result;
  }
  std::filesystem::path repo_dir() const override { return root; }
};

} // namespace

TEST_CASE("ProjectSyncOperation reports and records an unset remote", "[sync][operation]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  holder::index::FtsIndexer fts(db);
  create_project(db, "proj-1", dir / "project", std::nullopt);
  RecordingGitOps git;

  const auto result = holder::sync::run_project_sync(
      db,
      &fts,
      git,
      "proj-1",
      {.pull = true,
       .push = false,
       .push_after_failed_pull = false,
       .branch = "",
       .set_upstream = true,
       .now = 100}
  );

  REQUIRE_FALSE(result.succeeded());
  REQUIRE(result.pull.attempted);
  REQUIRE(result.pull.status == holder::sync::PullPhaseStatus::RemoteUnset);
  REQUIRE_FALSE(result.push.attempted);
  REQUIRE(git.calls.empty());
  const auto state = holder::project::ProjectSyncRepo(db).get("proj-1");
  REQUIRE(state.has_value());
  REQUIRE(state->last_pull_status == "remote_unset");
  REQUIRE(state->last_sync_error == "Remote URL is not configured.");
}

TEST_CASE(
    "ProjectSyncOperation records an unset remote for a push-only request",
    "[sync][operation]"
) {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  holder::index::FtsIndexer fts(db);
  create_project(db, "proj-1", dir / "project", std::nullopt);
  RecordingGitOps git;

  const auto result = holder::sync::run_project_sync(
      db,
      &fts,
      git,
      "proj-1",
      {.pull = false,
       .push = true,
       .push_after_failed_pull = false,
       .branch = "",
       .set_upstream = true,
       .now = 100}
  );

  REQUIRE_FALSE(result.succeeded());
  REQUIRE_FALSE(result.pull.attempted);
  REQUIRE(result.push.attempted);
  REQUIRE(result.push.status == holder::git::PushStatus::RemoteUnset);
  REQUIRE(result.push.error_message == "Remote URL is not configured.");
  REQUIRE(git.calls.empty());
  const auto state = holder::project::ProjectSyncRepo(db).get("proj-1");
  REQUIRE(state.has_value());
  REQUIRE(
      state->last_push_status == holder::git::push_status_name(holder::git::PushStatus::RemoteUnset)
  );
}

TEST_CASE(
    "ProjectSyncOperation rejects a request that selects neither pull nor push",
    "[sync][operation]"
) {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  holder::index::FtsIndexer fts(db);
  create_project(db, "proj-1", dir / "project", std::nullopt);
  RecordingGitOps git;

  REQUIRE_THROWS_WITH(
      holder::sync::run_project_sync(
          db,
          &fts,
          git,
          "proj-1",
          {.pull = false,
           .push = false,
           .push_after_failed_pull = false,
           .branch = "",
           .set_upstream = true,
           .now = 100}
      ),
      "project sync must request pull, push, or both"
  );
  REQUIRE(git.calls.empty());
}

namespace {

// Changes a project's root_path through a second connection at a precise moment: when the
// sync connection prepares its second SELECT. run_project_sync reads the project, waits for the
// repository lock, then reads it again, so the second read is exactly the point at which another
// process could have moved the project while the lock was contended. Doing it from the
// authorizer keeps the test single-threaded and deterministic.
struct RootChangeInjector {
  sqlite3* other = nullptr;
  std::string project_id;
  std::string new_root;
  int selects = 0;
};

int change_root_on_second_select(void* data, int action, const char*, const char*, const char*, const char*) {
  auto* injector = static_cast<RootChangeInjector*>(data);
  if (action == SQLITE_SELECT && ++injector->selects == 2) {
    const std::string sql = "UPDATE projects SET root_path = '" + injector->new_root +
                            "' WHERE project_id = '" + injector->project_id + "';";
    sqlite3_exec(injector->other, sql.c_str(), nullptr, nullptr, nullptr);
  }
  return SQLITE_OK;
}

} // namespace

TEST_CASE(
    "ProjectSyncOperation aborts when the project root changes while waiting for the lock",
    "[sync][operation]"
) {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  holder::index::FtsIndexer fts(db);
  create_project(db, "proj-1", dir / "project", "https://example.invalid/repo.git");
  RecordingGitOps git;

  holder::platform::Db other;
  other.open(dir / "holder.db");
  RootChangeInjector injector{other.handle(), "proj-1", (dir / "moved").string(), 0};
  REQUIRE(
      sqlite3_set_authorizer(db.handle(), change_root_on_second_select, &injector) == SQLITE_OK
  );

  REQUIRE_THROWS_WITH(
      holder::sync::run_project_sync(
          db,
          &fts,
          git,
          "proj-1",
          {.pull = true,
           .push = false,
           .push_after_failed_pull = false,
           .branch = "",
           .set_upstream = true,
           .now = 100}
      ),
      "project root changed while waiting for sync: proj-1"
  );
  sqlite3_set_authorizer(db.handle(), nullptr, nullptr);
  // Nothing may run against the repository once the root is known to have moved.
  REQUIRE(git.calls.empty());
}

TEST_CASE("ProjectSyncOperation stops before push when pull fails", "[sync][operation]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  holder::index::FtsIndexer fts(db);
  create_project(db, "proj-1", dir / "project", "https://example.com/repo.git");
  RecordingGitOps git;
  git.fail_pull = true;

  const auto result = holder::sync::run_project_sync(
      db,
      &fts,
      git,
      "proj-1",
      {.pull = true,
       .push = true,
       .push_after_failed_pull = false,
       .branch = "",
       .set_upstream = true,
       .now = 200}
  );

  REQUIRE_FALSE(result.succeeded());
  REQUIRE(result.pull.status == holder::sync::PullPhaseStatus::Failed);
  REQUIRE(result.pull.error_message == "pull failed for tests");
  REQUIRE_FALSE(result.push.attempted);
  REQUIRE(std::find(git.calls.begin(), git.calls.end(), "push") == git.calls.end());
  const auto state = holder::project::ProjectSyncRepo(db).get("proj-1");
  REQUIRE(state.has_value());
  REQUIRE(state->last_pull_status == "failed");
  REQUIRE_FALSE(state->last_push_status.has_value());
}

TEST_CASE("ProjectSyncOperation pulls before a successful push", "[sync][operation]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  holder::index::FtsIndexer fts(db);
  create_project(db, "proj-1", dir / "project", "https://example.com/repo.git");
  RecordingGitOps git;

  const auto result = holder::sync::run_project_sync(
      db,
      &fts,
      git,
      "proj-1",
      {.pull = true,
       .push = true,
       .push_after_failed_pull = false,
       .branch = "",
       .set_upstream = true,
       .now = 300}
  );

  REQUIRE(result.succeeded());
  REQUIRE(result.pull.status == holder::sync::PullPhaseStatus::Succeeded);
  REQUIRE(result.push.attempted);
  REQUIRE(result.push.status == holder::git::PushStatus::Pushed);
  const auto pull = std::find(git.calls.begin(), git.calls.end(), "pull");
  const auto push = std::find(git.calls.begin(), git.calls.end(), "push");
  REQUIRE(pull != git.calls.end());
  REQUIRE(push != git.calls.end());
  REQUIRE(pull < push);
  const auto state = holder::project::ProjectSyncRepo(db).get("proj-1");
  REQUIRE(state.has_value());
  REQUIRE(state->last_pull_status == "succeeded");
  REQUIRE(state->last_push_status == "pushed");
}

TEST_CASE(
    "ProjectSyncOperation resolves a diverged pull before rebuilding the index",
    "[sync][operation]"
) {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  holder::index::FtsIndexer fts(db);
  create_project(db, "proj-1", dir / "project", "https://example.com/repo.git");
  RecordingGitOps git;
  git.diverge_pull = true;

  const auto result = holder::sync::run_project_sync(
      db,
      &fts,
      git,
      "proj-1",
      {.pull = true,
       .push = false,
       .push_after_failed_pull = false,
       .branch = "",
       .set_upstream = true,
       .now = 300}
  );

  REQUIRE(result.succeeded());
  REQUIRE(result.pull.status == holder::sync::PullPhaseStatus::Succeeded);
  REQUIRE(result.pull.conflicts_resolved == 0);
  REQUIRE(std::find(git.calls.begin(), git.calls.end(), "merge") != git.calls.end());
  const auto state = holder::project::ProjectSyncRepo(db).get("proj-1");
  REQUIRE(state.has_value());
  REQUIRE(state->last_pull_status == "succeeded");
}

TEST_CASE("ProjectSyncOperation records an unexpected push failure", "[sync][operation]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  holder::index::FtsIndexer fts(db);
  create_project(db, "proj-1", dir / "project", "https://example.com/repo.git");
  RecordingGitOps git;
  git.fail_push = true;

  const auto result = holder::sync::run_project_sync(
      db,
      &fts,
      git,
      "proj-1",
      {.pull = false,
       .push = true,
       .push_after_failed_pull = false,
       .branch = "",
       .set_upstream = true,
       .now = 300}
  );

  REQUIRE_FALSE(result.succeeded());
  REQUIRE(result.push.attempted);
  REQUIRE(result.push.status == holder::git::PushStatus::UnknownError);
  REQUIRE(result.push.error_message == "push failed for tests");
  const auto state = holder::project::ProjectSyncRepo(db).get("proj-1");
  REQUIRE(state.has_value());
  REQUIRE(state->last_push_status == "unknown_error");
  REQUIRE(state->last_sync_error == "push failed for tests");
}

TEST_CASE(
    "ProjectSyncOperation ignores a failing activity-metrics refresh after a successful pull",
    "[sync][operation]"
) {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  holder::index::FtsIndexer fts(db);
  create_project(db, "proj-1", dir / "project", "https://example.invalid/repo.git");
  // A .git entry that is not a valid gitfile makes the post-sync repository inspection throw,
  // while the stubbed git operations report a clean pull.
  { std::ofstream(dir / "project" / ".git") << "this is not a gitfile"; }
  RecordingGitOps git;

  const auto result = holder::sync::run_project_sync(
      db,
      &fts,
      git,
      "proj-1",
      {.pull = true,
       .push = false,
       .push_after_failed_pull = false,
       .branch = "",
       .set_upstream = true,
       .now = 100}
  );

  REQUIRE(result.succeeded());
  REQUIRE(result.pull.status == holder::sync::PullPhaseStatus::Succeeded);
  const auto state = holder::project::ProjectSyncRepo(db).get("proj-1");
  REQUIRE(state.has_value());
  REQUIRE(state->last_pull_status == "succeeded");
}

TEST_CASE(
    "ProjectSyncOperation fast-forward pull rebuilds the project index",
    "[sync][operation]"
) {
  const auto dir = holder::test::make_temp_dir();
  const auto remote_root = dir / "remote";
  const auto local_root = dir / "local";

  auto remote_db = holder::test::open_db_with_schema(dir / "remote.db");
  holder::index::FtsIndexer remote_fts(remote_db);
  create_project(remote_db, "proj-1", remote_root, std::nullopt);
  holder::git::RealGitOps remote_git;
  holder::model::Card first;
  first.card_id = "11111111-1111-4111-8111-111111111111";
  first.project_id = "proj-1";
  first.title = "First";
  holder::card::CardStore(remote_db, &remote_fts, nullptr, &remote_git).create(first, "first");

  holder::git::GitRepo clone;
  clone.open_or_init(local_root);
  clone.set_remote("origin", remote_root.string());
  clone.pull_remote_ff_only("origin");

  auto local_db = holder::test::open_db_with_schema(dir / "local.db");
  holder::index::FtsIndexer local_fts(local_db);
  create_project(local_db, "proj-1", local_root, remote_root.string());
  holder::store::Rebuilder(local_db, &local_fts)
      .rebuild_project(holder::project::ProjectRepo(local_db).get("proj-1").value());

  holder::model::Card second;
  second.card_id = "22222222-2222-4222-8222-222222222222";
  second.project_id = "proj-1";
  second.title = "Second";
  holder::card::CardStore(remote_db, &remote_fts, nullptr, &remote_git).create(second, "second");

  holder::git::RealGitOps local_git;
  const auto result = holder::sync::run_project_sync(
      local_db,
      &local_fts,
      local_git,
      "proj-1",
      {.pull = true,
       .push = false,
       .push_after_failed_pull = false,
       .branch = "",
       .set_upstream = true,
       .now = 400}
  );

  REQUIRE(result.succeeded());
  REQUIRE(result.pull.status == holder::sync::PullPhaseStatus::Succeeded);
  REQUIRE(result.pull.conflicts_resolved == 0);
  REQUIRE(holder::card::CardRepo(local_db).list_all("proj-1").size() == 2);
}

TEST_CASE("Project sync keeps external changes under the user's control", "[sync][operation]") {
  const auto dir = holder::test::make_temp_dir();
  const auto remote_root = dir / "remote";
  const auto local_root = dir / "local";
  auto remote_db = holder::test::open_db_with_schema(dir / "remote.db");
  create_project(remote_db, "proj-1", remote_root, std::nullopt);
  holder::git::RealGitOps remote_git;
  holder::card::CardStore remote_store(remote_db, nullptr, nullptr, &remote_git);
  holder::model::Card card;
  card.project_id = "proj-1";
  card.card_id = "11111111-1111-4111-8111-111111111111";
  card.title = "Live card";
  remote_store.create(card, "original body");
  auto trashed = card;
  trashed.card_id = "22222222-2222-4222-8222-222222222222";
  trashed.title = "Trashed card";
  remote_store.create(trashed, "trashed body");
  remote_store.trash(trashed.card_id, 10);
  const auto live_path = holder::core::card_rel_path(card.card_id);
  const auto trash_path = holder::core::card_trash_rel_path(trashed.card_id);

  holder::git::GitRepo local;
  local.open_or_init(local_root);
  local.set_remote("origin", remote_root.string());
  local.pull_remote_ff_only("origin");
  auto local_db = holder::test::open_db_with_schema(dir / "local.db");
  holder::index::FtsIndexer fts(local_db);
  create_project(local_db, "proj-1", local_root, remote_root.string());
  holder::store::Rebuilder(local_db, &fts)
      .rebuild_project(holder::project::ProjectRepo(local_db).get("proj-1").value());

  const auto read_file = [](const auto& path) {
    std::ifstream input(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(input), {});
  };
  const auto original_live = read_file(local_root / live_path);
  const auto original_live_blob = local.read_blob_at(*local.head_oid(), live_path);
  const auto edited_live = original_live + "\nExternal live edit\n";
  auto edited_path = live_path;
  auto edited_bytes = edited_live;
  auto remote_path = trash_path;
  std::string expected_error;
  bool checkout_completed = false;
  bool local_deletion = false;

  SECTION("incoming change overlaps an uncommitted live card") {
    remote_path = live_path;
    expected_error = "Pull would overwrite a local file (" + live_path + ")";
  }
  SECTION("incoming change overlaps an uncommitted Trash card") {
    edited_path = trash_path;
    edited_bytes = read_file(local_root / trash_path) + "\nExternal Trash edit\n";
    expected_error = "Pull would overwrite a local file (" + trash_path + ")";
  }
  SECTION("independent remote change leaves a valid edit uncommitted") {}
  SECTION("malformed front matter blocks reconciliation") {
    checkout_completed = true;
    edited_bytes = "---\ncard_id: [invalid\n---\nbody\n";
    expected_error = live_path + ": invalid card front matter";
  }
  SECTION("a card's identity must match its path") {
    checkout_completed = true;
    auto wrong_id = card;
    wrong_id.card_id = trashed.card_id;
    edited_bytes = holder::core::render_card_front_matter(wrong_id, {}, {}) + "external body";
    expected_error = "card path does not match card_id";
  }
  SECTION("an unstaged deletion is preserved") {
    local_deletion = true;
    expected_error = "Pull blocked by a local deletion or file type change";
  }
  SECTION("an unrelated staged change blocks even an independent pull") {
    local.write_file("notes.txt", "staged note\n");
    local.stage_path("notes.txt");
    expected_error = "Pull blocked by staged or unresolved Git changes";
  }
  SECTION("divergence must not restore a committed version over a newer external edit") {
    local.write_file(live_path, edited_live);
    local.stage_path(live_path);
    local.commit("Earlier local edit");
    edited_bytes += "A later uncommitted edit\n";
    expected_error = "Pull cannot merge diverged branches while the working tree has local changes";
  }

  const auto head_before = local.head_oid();
  if (local_deletion) {
    REQUIRE(std::filesystem::remove(local_root / edited_path));
  } else {
    local.write_file(edited_path, edited_bytes);
  }
  local.write_file("untracked.txt", "keep this untracked\n");
  const auto index_before = read_file(local_root / ".git/index");
  remote_git.write_file(remote_path, read_file(remote_root / remote_path) + "\nRemote edit\n");
  remote_git.stage_path(remote_path);
  remote_git.commit("Edit card on remote");
  holder::git::GitRepo remote;
  remote.open_existing(remote_root);

  holder::git::RealGitOps local_git;
  const auto result = holder::sync::run_project_sync(
      local_db,
      &fts,
      local_git,
      "proj-1",
      {.pull = true, .push = !expected_error.empty(), .push_after_failed_pull = true, .now = 400}
  );
  INFO(result.pull.error_message.value_or(""));
  CHECK_FALSE(result.push.attempted);
  if (local_deletion) {
    CHECK_FALSE(std::filesystem::exists(local_root / edited_path));
  } else {
    CHECK(read_file(local_root / edited_path) == edited_bytes);
  }
  CHECK(read_file(local_root / "untracked.txt") == "keep this untracked\n");
  if (expected_error.empty()) {
    REQUIRE(result.succeeded());
    CHECK(local.head_oid() == remote.head_oid());
    CHECK_THAT(
        read_file(local_root / trash_path),
        Catch::Matchers::ContainsSubstring("Remote edit")
    );
    CHECK(local.read_blob_at(*local.head_oid(), live_path) == original_live_blob);
    CHECK_FALSE(local.read_blob_at(*local.head_oid(), "untracked.txt").has_value());
  } else {
    REQUIRE_FALSE(result.succeeded());
    REQUIRE(result.pull.error_message.has_value());
    CHECK_THAT(*result.pull.error_message, Catch::Matchers::ContainsSubstring(expected_error));
    const auto state = holder::project::ProjectSyncRepo(local_db).get("proj-1");
    REQUIRE(state.has_value());
    CHECK(state->last_pull_status == "failed");
    CHECK(state->last_sync_error == result.pull.error_message);
    if (checkout_completed) {
      // Git reached the independent remote commit but the invalid edit never entered SQLite.
      CHECK(local.head_oid() == remote.head_oid());
      CHECK(holder::card::CardRepo(local_db).get(card.card_id)->title == "Live card");
    } else {
      CHECK(local.head_oid() == head_before);
      CHECK(read_file(local_root / ".git/index") == index_before);
    }
  }
}

TEST_CASE("Project sync names every persisted pull phase", "[sync]") {
  REQUIRE(
      std::string(holder::sync::pull_phase_status_name(holder::sync::PullPhaseStatus::NotAttempted)
      ) == "not_attempted"
  );
}

TEST_CASE(
    "ProjectSyncOperation repairs cards a pull leaves hidden under a trashed parent",
    "[sync][operation][repair]"
) {
  const auto dir = holder::test::make_temp_dir();
  const auto remote_root = dir / "remote";
  const auto local_root = dir / "local";
  const std::string parent_id = "11111111-1111-4111-8111-111111111111";
  const std::string child_id = "22222222-2222-4222-8222-222222222222";

  auto remote_db = holder::test::open_db_with_schema(dir / "remote.db");
  holder::index::FtsIndexer remote_fts(remote_db);
  create_project(remote_db, "proj-1", remote_root, std::nullopt);
  holder::git::RealGitOps remote_git;
  holder::card::CardStore remote_store(remote_db, &remote_fts, nullptr, &remote_git);
  holder::model::Card parent;
  parent.card_id = parent_id;
  parent.project_id = "proj-1";
  parent.title = "Parent";
  remote_store.create(parent, "parent");
  holder::model::Card child;
  child.card_id = child_id;
  child.project_id = "proj-1";
  child.title = "Child";
  child.parent_card_id = parent_id;
  remote_store.create(child, "child");

  holder::git::GitRepo clone;
  clone.open_or_init(local_root);
  clone.set_remote("origin", remote_root.string());
  clone.pull_remote_ff_only("origin");
  auto local_db = holder::test::open_db_with_schema(dir / "local.db");
  holder::index::FtsIndexer local_fts(local_db);
  create_project(local_db, "proj-1", local_root, remote_root.string());
  holder::store::Rebuilder(local_db, &local_fts)
      .rebuild_project(holder::project::ProjectRepo(local_db).get("proj-1").value());

  // Another client trashes the parent the way trash worked before children were promoted: only
  // the parent's file moves to Trash, and the child still names it.
  const auto live_rel = holder::core::card_rel_path(parent_id);
  const auto trash_rel = holder::core::card_trash_rel_path(parent_id);
  auto parsed = holder::core::parse_card_file([&] {
    std::ifstream in(remote_root / live_rel);
    return std::string(std::istreambuf_iterator<char>(in), {});
  }());
  parsed.card.deleted_at = 300;
  parsed.card.rel_path = trash_rel;
  std::filesystem::create_directories((remote_root / trash_rel).parent_path());
  remote_git.write_file(
      trash_rel,
      holder::core::render_card_front_matter(parsed.card, parsed.links, parsed.milestones) +
          parsed.body
  );
  remote_git.stage_path(trash_rel);
  std::filesystem::remove(remote_root / live_rel);
  remote_git.remove_path(live_rel);
  remote_git.commit("Delete card Parent (older client)");

  bool reject_repair = false;
  bool defer_repair = false;
  const auto child_rel = holder::core::card_rel_path(child_id);
  const auto read_child = [&] {
    std::ifstream in(local_root / child_rel, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(in), {});
  };
  const auto external_edit = read_child() + "\nExternal child edit\n";
  SECTION("repair succeeds during pull") {}
  SECTION("a failed optional repair can be retried after the successful pull") {
    reject_repair = true;
    // Imported records keep their historical timestamps; only the optional
    // repair stamps the new parent with this sync request's time (400).
    local_db.exec("CREATE TRIGGER reject_parent_change BEFORE UPDATE OF parent_card_id ON cards "
                  "WHEN NEW.updated_at=400 AND OLD.parent_card_id IS NOT NEW.parent_card_id "
                  "BEGIN SELECT RAISE(ABORT, 'parent repair denied'); END;");
  }
  SECTION("repair waits until an external edit to the child has been committed") {
    defer_repair = true;
    clone.write_file(child_rel, external_edit);
  }
  holder::git::RealGitOps local_git;
  const auto pull = [&](long long now) {
    return holder::sync::run_project_sync(
        local_db,
        &local_fts,
        local_git,
        "proj-1",
        {.pull = true, .now = now}
    );
  };
  const auto result = pull(400);

  INFO(
      result.pull.error_message.value_or("") + " / push: " + result.push.error_message.value_or("")
  );
  REQUIRE(result.succeeded());
  REQUIRE(result.pull.status == holder::sync::PullPhaseStatus::Succeeded);
  holder::card::CardRepo cards(local_db);
  if (reject_repair) {
    REQUIRE(result.pull.hidden_cards_repaired == 0);
    REQUIRE(cards.get(child_id)->parent_card_id == parent_id);
    const auto state = holder::project::ProjectSyncRepo(local_db).get("proj-1");
    REQUIRE(state.has_value());
    CHECK(state->last_pull_status == "succeeded");
    local_db.exec("DROP TRIGGER reject_parent_change;");
    const auto repaired =
        holder::card::CardStore(local_db, &local_fts).repair_hierarchy("proj-1", 500);
    REQUIRE(repaired.moved_card_ids.size() == 1);
    CHECK(repaired.moved_card_ids[0] == child_id);
  } else if (defer_repair) {
    CHECK(result.pull.hidden_cards_repaired == 0);
    CHECK(cards.get(child_id)->parent_card_id == parent_id);
    REQUIRE(read_child() == external_edit);
    clone.open_existing(local_root);
    clone.stage_path(child_rel);
    clone.commit("Save externally edited child");
    const auto retry = pull(500);
    REQUIRE(retry.succeeded());
    CHECK(retry.pull.hidden_cards_repaired == 1);
    CHECK_THAT(read_child(), Catch::Matchers::ContainsSubstring("External child edit"));
  } else {
    REQUIRE(result.pull.hidden_cards_repaired == 1);
  }
  REQUIRE(cards.get(parent_id)->deleted_at.has_value());
  REQUIRE_FALSE(cards.get(child_id)->parent_card_id.has_value());
  // The repair is in the child's file too, so it survives a rebuild and syncs onwards.
  holder::store::Rebuilder(local_db, &local_fts)
      .rebuild_project(holder::project::ProjectRepo(local_db).get("proj-1").value());
  REQUIRE_FALSE(cards.get(child_id)->parent_card_id.has_value());
}

TEST_CASE(
    "ProjectSyncOperation fast-forward pull applies edits and deletions to the working tree",
    "[sync][operation]"
) {
  const auto dir = holder::test::make_temp_dir();
  const auto remote_root = dir / "remote";
  const auto local_root = dir / "local";
  const std::string edited_id = "33333333-3333-4333-8333-333333333333";
  const std::string trashed_id = "44444444-4444-4444-8444-444444444444";

  auto remote_db = holder::test::open_db_with_schema(dir / "remote.db");
  holder::index::FtsIndexer remote_fts(remote_db);
  create_project(remote_db, "proj-1", remote_root, std::nullopt);
  holder::git::RealGitOps remote_git;
  holder::card::CardStore remote_store(remote_db, &remote_fts, nullptr, &remote_git);
  for (const auto& [id, title] :
       {std::pair{edited_id, "Edited"}, std::pair{trashed_id, "Trashed"}}) {
    holder::model::Card card;
    card.card_id = id;
    card.project_id = "proj-1";
    card.title = title;
    remote_store.create(card, "original body");
  }

  holder::git::GitRepo clone;
  clone.open_or_init(local_root);
  clone.set_remote("origin", remote_root.string());
  clone.pull_remote_ff_only("origin");
  auto local_db = holder::test::open_db_with_schema(dir / "local.db");
  holder::index::FtsIndexer local_fts(local_db);
  create_project(local_db, "proj-1", local_root, remote_root.string());
  holder::store::Rebuilder(local_db, &local_fts)
      .rebuild_project(holder::project::ProjectRepo(local_db).get("proj-1").value());

  remote_store.update_content(edited_id, "edited body", std::nullopt, 300);
  remote_store.trash(trashed_id, 301);

  holder::git::RealGitOps local_git;
  const auto result = holder::sync::run_project_sync(
      local_db,
      &local_fts,
      local_git,
      "proj-1",
      {.pull = true,
       .push = false,
       .push_after_failed_pull = false,
       .branch = "",
       .set_upstream = true,
       .now = 400}
  );

  INFO(result.pull.error_message.value_or(""));
  REQUIRE(result.succeeded());
  const auto read = [](const std::filesystem::path& path) {
    std::ifstream in(path);
    return std::string(std::istreambuf_iterator<char>(in), {});
  };
  CHECK(
      read(local_root / holder::core::card_rel_path(edited_id)).find("edited body") !=
      std::string::npos
  );
  CHECK_FALSE(std::filesystem::exists(local_root / holder::core::card_rel_path(trashed_id)));
  CHECK(std::filesystem::exists(local_root / holder::core::card_trash_rel_path(trashed_id)));
  holder::card::CardRepo cards(local_db);
  CHECK(cards.get(trashed_id)->deleted_at.has_value());
}

TEST_CASE(
    "ProjectSyncOperation reports and records what a diverged pull changed",
    "[sync][operation]"
) {
  const auto dir = holder::test::make_temp_dir();
  const auto remote_root = dir / "remote";
  const auto local_root = dir / "local";
  const std::string card_id = "11111111-1111-4111-8111-111111111111";

  auto remote_db = holder::test::open_db_with_schema(dir / "remote.db");
  holder::index::FtsIndexer remote_fts(remote_db);
  create_project(remote_db, "proj-1", remote_root, std::nullopt);
  holder::git::RealGitOps remote_git;
  holder::card::CardStore remote_store(remote_db, &remote_fts, nullptr, &remote_git);
  holder::model::Card card;
  card.card_id = card_id;
  card.project_id = "proj-1";
  card.title = "Notes";
  card.created_at = card.updated_at = 1;
  remote_store.create(card, "base\n");

  holder::git::GitRepo clone;
  clone.open_or_init(local_root);
  clone.set_remote("origin", remote_root.string());
  clone.pull_remote_ff_only("origin");
  auto local_db = holder::test::open_db_with_schema(dir / "local.db");
  holder::index::FtsIndexer local_fts(local_db);
  create_project(local_db, "proj-1", local_root, remote_root.string());
  holder::store::Rebuilder(local_db, &local_fts)
      .rebuild_project(holder::project::ProjectRepo(local_db).get("proj-1").value());

  // This device renames the card while the other edits its text, so the pull merges them.
  holder::git::RealGitOps local_git;
  holder::card::CardStore(local_db, &local_fts, nullptr, &local_git)
      .update_content(card_id, "base\n", std::string("Meeting notes"), 3);
  remote_store.update_content(card_id, "edited elsewhere\n", std::nullopt, 2);

  const holder::sync::ProjectSyncRequest request{
      .pull = true,
      .push = false,
      .push_after_failed_pull = false,
      .branch = "",
      .set_upstream = true,
      .now = 400
  };
  const auto result =
      holder::sync::run_project_sync(local_db, &local_fts, local_git, "proj-1", request);

  REQUIRE(result.succeeded());
  CHECK(result.pull.changes.merged_card_ids == std::vector<std::string>{card_id});
  CHECK(result.pull.changes.conflict_copy_ids.empty());
  const auto state = holder::project::ProjectSyncRepo(local_db).get("proj-1");
  REQUIRE(state.has_value());
  REQUIRE(state->last_pull_changes.has_value());
  CHECK(state->last_pull_changes->pulled_at == 400);
  CHECK(state->last_pull_changes->changes.merged_card_ids == std::vector<std::string>{card_id});

  // A later pull with nothing to bring together reports nothing and keeps the record.
  auto later = request;
  later.now = 500;
  const auto quiet =
      holder::sync::run_project_sync(local_db, &local_fts, local_git, "proj-1", later);
  REQUIRE(quiet.succeeded());
  CHECK(quiet.pull.changes.empty());
  CHECK(
      holder::project::ProjectSyncRepo(local_db).get("proj-1")->last_pull_changes->pulled_at == 400
  );
}

namespace {

// The real adapter, except that one network call (fetch or push) stops until the test lets it
// go, as a slow connection would. Every other call goes straight through.
class StallingGitOps final : public holder::git::GitOps {
 public:
  explicit StallingGitOps(std::string stall_in)
      : stall_in_(std::move(stall_in)) {}

  std::promise<void> entered;
  std::promise<void> release;

  void open_or_init(const std::filesystem::path& path) override { inner_.open_or_init(path); }
  void write_file(const std::filesystem::path& path, const std::string& content) override {
    inner_.write_file(path, content);
  }
  void stage_path(const std::filesystem::path& path) override { inner_.stage_path(path); }
  void remove_path(const std::filesystem::path& path) override { inner_.remove_path(path); }
  void commit(const std::string& message) override { inner_.commit(message); }
  void set_remote(const std::string& name, const std::string& url) override {
    inner_.set_remote(name, url);
  }
  void remove_remote(const std::string& name) override { inner_.remove_remote(name); }
  // Stalls here too, so the test also holds a sync that fetches as part of a whole pull.
  void pull_remote_ff_only(const std::string& name) override {
    stall("fetch");
    inner_.pull_remote_ff_only(name);
  }
  void fetch_remote(const std::string& name) override {
    stall("fetch");
    inner_.fetch_remote(name);
  }
  void pull_fetched_ff_only(const std::string& name) override { inner_.pull_fetched_ff_only(name); }
  holder::git::RemoteProbeResult probe_remote(const std::string& name) override {
    return inner_.probe_remote(name);
  }
  holder::git::PushResult push_branch(const std::string& name, const std::string& branch, bool up)
      override {
    stall("push");
    return inner_.push_branch(name, branch, up);
  }
  holder::git::GitRepo::DivergedMergeResult merge_remote_taking_theirs_for_conflicts(
      const std::string& name,
      const std::string& local,
      const std::string& remote
  ) override {
    return inner_.merge_remote_taking_theirs_for_conflicts(name, local, remote);
  }
  std::optional<std::string> read_blob_at(const std::string& oid, const std::filesystem::path& path)
      override {
    return inner_.read_blob_at(oid, path);
  }
  std::filesystem::path repo_dir() const override { return inner_.repo_dir(); }

 private:
  void stall(const std::string& where) {
    if (where != stall_in_) return;
    entered.set_value();
    release.get_future().wait();
  }

  std::string stall_in_;
  holder::git::RealGitOps inner_;
};

void init_bare_hub(const std::filesystem::path& path) {
  git_libgit2_init();
  git_repository* repo = nullptr;
  git_repository_init_options opts{};
  REQUIRE(git_repository_init_options_init(&opts, GIT_REPOSITORY_INIT_OPTIONS_VERSION) == 0);
  opts.flags = GIT_REPOSITORY_INIT_BARE | GIT_REPOSITORY_INIT_MKPATH;
  REQUIRE(git_repository_init_ext(&repo, path.string().c_str(), &opts) == 0);
  git_repository_free(repo);
  git_libgit2_shutdown();
}

holder::model::Card titled(const std::string& card_id, const std::string& title) {
  holder::model::Card card;
  card.card_id = card_id;
  card.project_id = "proj-1";
  card.title = title;
  return card;
}

// One device: its project's repository and database, and a second connection with its own
// adapter to save cards on, as another lane of the same app would.
struct Device {
  Device(
      const std::filesystem::path& dir,
      const std::string& name,
      const std::filesystem::path& hub
  )
      : root(dir / name),
        db(holder::test::open_db_with_schema(dir / (name + ".db"))),
        fts(db) {
    create_project(db, "proj-1", root, hub.string());
    other_db.open(dir / (name + ".db"));
  }

  void save(const holder::model::Card& card, const std::string& body) {
    holder::card::CardStore(other_db, nullptr, nullptr, &card_git).create(card, body);
  }

  holder::sync::ProjectSyncResult sync(
      holder::git::GitOps& git,
      bool pull,
      bool push,
      long long now
  ) {
    return holder::sync::run_project_sync(
        db,
        &fts,
        git,
        "proj-1",
        {.pull = pull,
         .push = push,
         .push_after_failed_pull = false,
         .branch = "",
         .set_upstream = true,
         .now = now}
    );
  }

  bool has_card_file(const std::string& card_id) const {
    return std::filesystem::exists(root / holder::core::card_rel_path(card_id));
  }

  std::filesystem::path root;
  holder::platform::Db db;
  holder::index::FtsIndexer fts;
  holder::platform::Db other_db;
  holder::git::RealGitOps card_git;
};

// How long a save may take while the sync is held in its network call. A save that has to wait
// for the sync's lock takes as long as the stall, which the test ends only afterwards.
constexpr auto kPrompt = std::chrono::seconds(5);

const std::string kFirst = "11111111-1111-4111-8111-111111111111";
const std::string kFromOther = "22222222-2222-4222-8222-222222222222";
const std::string kDuringSync = "33333333-3333-4333-8333-333333333333";

} // namespace

TEST_CASE(
    "A card saved while a sync fetches is not held up, and is kept",
    "[sync][operation][lock]"
) {
  const auto dir = holder::test::make_temp_dir();
  const auto hub = dir / "hub.git";
  init_bare_hub(hub);
  Device phone(dir, "phone", hub);
  Device other(dir, "other", hub);
  holder::git::RealGitOps phone_git;
  holder::git::RealGitOps other_git;

  phone.save(titled(kFirst, "First"), "first");
  REQUIRE(phone.sync(phone_git, false, true, 100).succeeded());
  REQUIRE(other.sync(other_git, true, false, 110).succeeded());
  other.save(titled(kFromOther, "From the other device"), "other");
  REQUIRE(other.sync(other_git, false, true, 120).succeeded());

  StallingGitOps stalling("fetch");
  auto syncing = std::async(std::launch::async, [&] {
    return phone.sync(stalling, true, true, 200);
  });
  REQUIRE(
      stalling.entered.get_future().wait_for(std::chrono::seconds(10)) == std::future_status::ready
  );
  auto saving = std::async(std::launch::async, [&] {
    phone.save(titled(kDuringSync, "Saved during sync"), "mid");
  });
  const bool prompt = saving.wait_for(kPrompt) == std::future_status::ready;
  stalling.release.set_value();
  saving.get();
  const auto result = syncing.get();

  CHECK(prompt);
  REQUIRE(result.succeeded());
  // The branch moved during the fetch and the remote moved too: the diverged merge keeps both.
  CHECK(phone.has_card_file(kFromOther));
  CHECK(phone.has_card_file(kDuringSync));
  REQUIRE(other.sync(other_git, true, false, 300).succeeded());
  CHECK(other.has_card_file(kDuringSync));
}

TEST_CASE(
    "A card saved while a sync pushes is not held up, and goes in the next push",
    "[sync][operation][lock]"
) {
  const auto dir = holder::test::make_temp_dir();
  const auto hub = dir / "hub.git";
  init_bare_hub(hub);
  Device phone(dir, "phone", hub);
  Device other(dir, "other", hub);
  holder::git::RealGitOps other_git;

  phone.save(titled(kFirst, "First"), "first");
  StallingGitOps stalling("push");
  auto syncing = std::async(std::launch::async, [&] {
    return phone.sync(stalling, false, true, 100);
  });
  REQUIRE(
      stalling.entered.get_future().wait_for(std::chrono::seconds(10)) == std::future_status::ready
  );
  auto saving = std::async(std::launch::async, [&] {
    phone.save(titled(kDuringSync, "Saved during sync"), "mid");
  });
  const bool prompt = saving.wait_for(kPrompt) == std::future_status::ready;
  stalling.release.set_value();
  saving.get();
  const auto result = syncing.get();

  CHECK(prompt);
  REQUIRE(result.succeeded());
  holder::git::RealGitOps phone_git;
  REQUIRE(phone.sync(phone_git, true, true, 200).succeeded());
  REQUIRE(other.sync(other_git, true, false, 300).succeeded());
  CHECK(other.has_card_file(kFirst));
  CHECK(other.has_card_file(kDuringSync));
}

TEST_CASE("Two syncs of one project take turns, network included", "[sync][operation][lock]") {
  const auto dir = holder::test::make_temp_dir();
  const auto hub = dir / "hub.git";
  init_bare_hub(hub);
  Device phone(dir, "phone", hub);
  holder::git::RealGitOps phone_git;
  phone.save(titled(kFirst, "First"), "first");
  REQUIRE(phone.sync(phone_git, false, true, 50).succeeded());

  StallingGitOps first("fetch");
  StallingGitOps second("fetch");
  auto second_entered = second.entered.get_future();
  auto first_sync = std::async(std::launch::async, [&] {
    return phone.sync(first, true, true, 100);
  });
  const bool first_entered = first.entered.get_future().wait_for(std::chrono::seconds(10)) ==
                             std::future_status::ready;
  auto second_sync = std::async(std::launch::async, [&] {
    return phone.sync(second, true, true, 110);
  });
  // Their fetches would race on refs/remotes, so the second waits for the whole first sync.
  const bool second_waited = second_entered.wait_for(std::chrono::milliseconds(500)) ==
                             std::future_status::timeout;
  first.release.set_value();
  const auto first_result = first_sync.get();
  const bool second_ran = second_entered.wait_for(std::chrono::seconds(10)) ==
                          std::future_status::ready;
  second.release.set_value();
  const auto second_result = second_sync.get();

  CHECK(first_entered);
  CHECK(second_waited);
  CHECK(second_ran);
  CHECK(first_result.succeeded());
  CHECK(second_result.succeeded());
}
