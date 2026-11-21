#if __has_include(<catch2/catch_test_macros.hpp>)
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#else
#include <catch2/catch.hpp>
#endif

#include "sync/PullConflictResolution.h"

#include "capi/Context.h"
#include "card/CardFrontMatter.h"
#include "card/CardPaths.h"
#include "card/CardRepo.h"
#include "card/CardStore.h"
#include "card/LinkRepo.h"
#include "card/MilestoneRepo.h"
#include "core_test_helpers.h"
#include "git/GitOps.h"
#include "git/GitRepo.h"
#include "holder/holder_api.h"
#include "index/FtsIndexer.h"
#include "model/Card.h"
#include "model/Project.h"
#include "privacy/ProjectPrivacy.h"
#include "project/ProjectRepo.h"
#include "project/Rebuilder.h"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <map>
#include <memory>

namespace {

using holder::test::EnvGuard;
using holder::test::make_temp_dir;
using holder::test::open_db_with_schema;

constexpr const char* kSharedCardId = "550e8400-e29b-41d4-a716-446655440000";

void create_plain_project(
    holder::project::ProjectRepo& projects,
    const std::string& project_id,
    const std::filesystem::path& root_path,
    holder::model::IdScheme id_scheme = holder::model::IdScheme::Uuid7
) {
  holder::model::Project project;
  project.project_id = project_id;
  project.name = "Project";
  project.root_path = root_path.string();
  project.privacy_mode = "plain";
  project.id_scheme = id_scheme;
  project.created_at = 1;
  project.updated_at = 1;
  projects.create(project);
}

} // namespace

TEST_CASE(
    "Conflict preservation rejects misidentified cards and cyclic parents",
    "[sync][conflict-validation]"
) {
  const auto dir = make_temp_dir();
  const std::string project_id = "proj-conflicts";
  const std::string second_id = "22222222-2222-4222-8222-222222222222";
  const auto remote_dir = dir / "remote";
  const auto local_dir = dir / "local";
  auto remote_db = open_db_with_schema(dir / "remote.db");
  holder::index::FtsIndexer remote_fts(remote_db);
  holder::project::ProjectRepo remote_projects(remote_db);
  create_plain_project(remote_projects, project_id, remote_dir);
  holder::git::RealGitOps remote_git;
  holder::card::CardStore remote_store(remote_db, &remote_fts, nullptr, &remote_git);
  for (const std::string id : {std::string(kSharedCardId), second_id}) {
    holder::model::Card card;
    card.card_id = id;
    card.project_id = project_id;
    card.title = id == kSharedCardId ? "First" : "Second";
    card.created_at = card.updated_at = 1;
    remote_store.create(card, "base body\n");
  }
  holder::git::RealGitOps local_git;
  local_git.open_or_init(local_dir);
  local_git.set_remote("origin", remote_dir.string());
  local_git.pull_remote_ff_only("origin");
  auto local_db = open_db_with_schema(dir / "local.db");
  holder::index::FtsIndexer local_fts(local_db);
  holder::project::ProjectRepo local_projects(local_db);
  create_plain_project(local_projects, project_id, local_dir);
  const auto local_project = local_projects.get(project_id).value();
  holder::store::Rebuilder(local_db, &local_fts).rebuild_project(local_project);

  bool wrong_identity = false;
  bool malformed = false;
  std::string expected;
  SECTION("a conflicted card names a different identity than its path") {
    wrong_identity = true;
    expected = "conflicted card ID does not match its path: ";
  }
  SECTION("conflicted parents form a cycle") {
    expected = "cannot preserve conflicted card parent cycle from commit ";
  }
  SECTION("malformed local files cannot be treated as card copies") { malformed = true; }
  std::map<std::string, std::string> local_bytes;
  for (const std::string id : {std::string(kSharedCardId), second_id}) {
    const auto path = holder::core::card_rel_path(id);
    std::ifstream input(local_dir / path);
    REQUIRE(input.is_open());
    const std::string bytes{
        std::istreambuf_iterator<char>(input),
        std::istreambuf_iterator<char>()
    };
    auto parsed = holder::core::parse_card_file(bytes);
    REQUIRE(parsed.has_front_matter);
    if (wrong_identity)
      parsed.card.card_id = kSharedCardId;
    else
      parsed.card.parent_card_id = id == kSharedCardId ? second_id : std::string(kSharedCardId);
    const auto changed = malformed ? std::string("not card front matter\nlocal body to preserve\n")
                                   : holder::core::render_card_front_matter(
                                         parsed.card,
                                         parsed.links,
                                         parsed.milestones
                                     ) + "local body to preserve\n";
    local_bytes.emplace(path, changed);
    local_git.write_file(path, changed);
    local_git.stage_path(path);
    remote_store.update_content(id, "remote body\n", std::nullopt, 2);
  }
  local_git.commit("Local conflicting metadata");
  holder::git::NonFastForwardPullError diverged("", "", "");
  bool seen = false;
  try {
    local_git.pull_remote_ff_only("origin");
  } catch (const holder::git::NonFastForwardPullError& error) {
    diverged = error;
    seen = true;
  }
  REQUIRE(seen);
  if (malformed) {
    CHECK(
        holder::sync::resolve_pull_conflicts(
            local_db,
            &local_fts,
            local_project,
            local_git,
            diverged,
            3
        ) == 0
    );
  } else {
    REQUIRE_THROWS_WITH(
        holder::sync::resolve_pull_conflicts(
            local_db,
            &local_fts,
            local_project,
            local_git,
            diverged,
            3
        ),
        Catch::Matchers::ContainsSubstring(expected)
    );
  }
  const auto cards = holder::card::CardRepo(local_db).list_all(project_id);
  REQUIRE(cards.size() == 2);
  for (const auto& card : cards) {
    CHECK((card.card_id == kSharedCardId || card.card_id == second_id));
    CHECK(card.title.find("conflicted copy") == std::string::npos);
  }
  // The pre-merge local commit remains available for explicit recovery after
  // preservation fails; no partial set of replacement cards is created.
  for (const auto& [path, bytes] : local_bytes)
    CHECK(local_git.read_blob_at(diverged.local_oid_hex, path) == bytes);
}

TEST_CASE(
    "resolve_pull_conflicts decrypts the pre-merge local card for an encrypted_git project",
    "[sync]"
) {
  const auto dir = make_temp_dir();
  EnvGuard keystore_env("HOLDER_TEST_KEYSTORE_DIR", (dir / "keystore").string());
  const std::string project_id = "proj-1";
  const auto remote_dir = dir / "remote";
  const auto local_dir = dir / "local";

  // Seed the encrypted project and its base card on what becomes "remote".
  auto remote_db = open_db_with_schema(dir / "remote.db");
  holder::index::FtsIndexer remote_fts(remote_db);
  holder::project::ProjectRepo remote_projects(remote_db);
  holder::git::RealGitOps remote_git;

  holder::model::Project project;
  project.project_id = project_id;
  project.name = "Project";
  project.root_path = remote_dir.string();
  project.privacy_mode = "encrypted_git";
  project.created_at = 1;
  project.updated_at = 1;
  remote_projects.create(project);
  holder::privacy::ensure_encrypted_project_ready(
      remote_git,
      remote_projects,
      project_id,
      project.root_path,
      std::nullopt,
      1,
      []() {
        return "test-key-1";
      }
  );
  const auto seeded_project = remote_projects.get(project_id).value();

  holder::model::Card card;
  card.card_id = kSharedCardId;
  card.project_id = project_id;
  card.title = "Shared";
  holder::model::CardLink self_link;
  self_link.project_id = project_id;
  self_link.from_card_id = card.card_id;
  self_link.to_card_id = card.card_id;
  self_link.to_type = "card";
  self_link.kind = "ref";
  self_link.label = "self";
  self_link.created_at = 1;
  auto external_link = self_link;
  external_link.target_project_id = "another-project";
  external_link.label = "outside";
  auto resource_link = self_link;
  resource_link.to_card_id = "resource-id";
  resource_link.to_type = "resource";
  resource_link.label = "attachment";
  holder::model::Milestone milestone;
  milestone.milestone_id = "milestone-original";
  milestone.project_id = project_id;
  milestone.card_id = card.card_id;
  milestone.dates = {
      {"start", std::string("2026-10-16"), std::nullopt},
      {"resolved", std::nullopt, std::string("2026-10-17T09:30:00+01:00")}
  };
  milestone.kind = "Deadline";
  milestone.description = "Ship it";
  milestone.created_at = 1;
  milestone.updated_at = 1;
  holder::card::CardStore(remote_db, &remote_fts, nullptr, &remote_git)
      .create(card, "base", std::nullopt, {self_link, external_link, resource_link}, {milestone});

  // Local clones that base state.
  holder::git::RealGitOps local_git;
  local_git.open_or_init(local_dir);
  local_git.set_remote("origin", remote_dir.string());
  local_git.pull_remote_ff_only("origin");

  // Diverge: remote edits the card...
  holder::card::CardStore(remote_db, &remote_fts, nullptr, &remote_git)
      .update_content(kSharedCardId, "remote edit", std::nullopt, 2);

  // ...and local independently edits the same card via its own DB row -- same project_id/
  // project_key_id, so decryption works identically, exactly the real cross-device scenario.
  auto local_db = open_db_with_schema(dir / "local.db");
  holder::index::FtsIndexer local_fts(local_db);
  holder::project::ProjectRepo local_projects(local_db);
  holder::model::Project local_project = seeded_project;
  local_project.root_path = local_dir.string();
  local_project.id_scheme = holder::model::IdScheme::Uuid7;
  local_projects.create(local_project);
  holder::store::Rebuilder(local_db, &local_fts).rebuild_project(local_project);
  holder::card::CardStore(local_db, &local_fts, nullptr, &local_git)
      .update_content(kSharedCardId, "local edit", std::nullopt, 2);

  bool diverged_seen = false;
  holder::git::NonFastForwardPullError diverged("", "", "");
  try {
    local_git.pull_remote_ff_only("origin");
  } catch (const holder::git::NonFastForwardPullError& e) {
    diverged_seen = true;
    diverged = e;
  }
  REQUIRE(diverged_seen);

  auto stale_project = local_project;
  stale_project.id_scheme = holder::model::IdScheme::Uuid4;
  const int resolved = holder::sync::resolve_pull_conflicts(
      local_db,
      &local_fts,
      stale_project,
      local_git,
      diverged,
      3
  );
  REQUIRE(resolved == 1);

  const auto cards = holder::card::CardRepo(local_db).list_all(project_id);
  const auto duplicate = std::find_if(cards.begin(), cards.end(), [](const auto& candidate) {
    return candidate.card_id != kSharedCardId;
  });
  REQUIRE(duplicate != cards.end());
  REQUIRE(duplicate->card_id.size() == 36);
  REQUIRE(duplicate->card_id[14] == '7');
  REQUIRE(duplicate->title == "Shared (conflicted copy)");

  holder::card::CardStore local_store(local_db, &local_fts, nullptr, &local_git);
  const auto content = local_store.get_content(*duplicate);
  REQUIRE(content.has_value());
  REQUIRE(*content == "local edit");

  auto assert_copy_metadata = [&]() {
    const auto links =
        holder::card::LinkRepo(local_db).list_outgoing(project_id, duplicate->card_id);
    REQUIRE(links.size() == 4);
    bool found_self = false;
    bool found_external = false;
    bool found_resource = false;
    bool found_original = false;
    for (const auto& link : links) {
      REQUIRE(link.from_card_id == duplicate->card_id);
      if (link.kind == holder::model::kConflictCopyLinkKind) {
        REQUIRE(link.to_type == "card");
        REQUIRE(link.to_card_id == kSharedCardId);
        REQUIRE_FALSE(link.target_project_id.has_value());
        found_original = true;
      } else if (link.to_type == "resource") {
        REQUIRE(link.to_card_id == "resource-id");
        REQUIRE(link.label == "attachment");
        found_resource = true;
      } else if (!link.target_project_id) {
        REQUIRE(link.to_card_id == duplicate->card_id);
        REQUIRE(link.label == "self");
        found_self = true;
      } else {
        REQUIRE(link.target_project_id == "another-project");
        REQUIRE(link.to_card_id == kSharedCardId);
        REQUIRE(link.label == "outside");
        found_external = true;
      }
    }
    REQUIRE(found_self);
    REQUIRE(found_external);
    REQUIRE(found_resource);
    REQUIRE(found_original);
    const auto milestones =
        holder::card::MilestoneRepo(local_db).list_for_card(project_id, duplicate->card_id);
    REQUIRE(milestones.size() == 1);
    REQUIRE(milestones[0].milestone_id != "milestone-original");
    REQUIRE(milestones[0].card_id == duplicate->card_id);
    REQUIRE(
        holder::model::milestone_unique_date_for_role(milestones[0], "start")->date == "2026-10-16"
    );
    REQUIRE(
        holder::model::milestone_unique_date_for_role(milestones[0], "resolved")->datetime ==
        "2026-10-17T09:30:00+01:00"
    );
    REQUIRE(milestones[0].kind == "Deadline");
    REQUIRE(milestones[0].description == "Ship it");
  };
  assert_copy_metadata();
  holder::store::Rebuilder(local_db, &local_fts).rebuild_project(local_project);
  assert_copy_metadata();
}

TEST_CASE(
    "resolve_pull_conflicts skips a conflict whose pre-merge blob can't be decrypted",
    "[sync]"
) {
  // Same divergence shape as the test above, but the local side's pre-merge blob is corrupted
  // (not a real encrypted envelope) -- resolve_pull_conflicts must skip it rather than throw and
  // fail the whole pull over one unreadable conflict.
  const auto dir = make_temp_dir();
  EnvGuard keystore_env("HOLDER_TEST_KEYSTORE_DIR", (dir / "keystore").string());
  const std::string project_id = "proj-1";
  const auto remote_dir = dir / "remote";
  const auto local_dir = dir / "local";

  auto remote_db = open_db_with_schema(dir / "remote.db");
  holder::index::FtsIndexer remote_fts(remote_db);
  holder::project::ProjectRepo remote_projects(remote_db);
  holder::git::RealGitOps remote_git;

  holder::model::Project project;
  project.project_id = project_id;
  project.name = "Project";
  project.root_path = remote_dir.string();
  project.privacy_mode = "encrypted_git";
  project.created_at = 1;
  project.updated_at = 1;
  remote_projects.create(project);
  holder::privacy::ensure_encrypted_project_ready(
      remote_git,
      remote_projects,
      project_id,
      project.root_path,
      std::nullopt,
      1,
      []() {
        return "test-key-1";
      }
  );
  const auto seeded_project = remote_projects.get(project_id).value();

  holder::model::Card card;
  card.card_id = kSharedCardId;
  card.project_id = project_id;
  card.title = "Shared";
  holder::card::CardStore(remote_db, &remote_fts, nullptr, &remote_git).create(card, "base");

  holder::git::RealGitOps local_git;
  local_git.open_or_init(local_dir);
  local_git.set_remote("origin", remote_dir.string());
  local_git.pull_remote_ff_only("origin");

  remote_git.write_file(
      holder::core::card_rel_path(kSharedCardId),
      "remote edit -- doesn't matter, never decrypted in this test"
  );
  remote_git.stage_path(holder::core::card_rel_path(kSharedCardId));
  remote_git.commit("remote edit");

  // Corrupt the local side's copy directly, in the working tree, then commit it -- this is what
  // makes the (still real, git-level) divergence's local content undecryptable.
  local_git.write_file(holder::core::card_rel_path(kSharedCardId), "not a real encrypted envelope");
  local_git.stage_path(holder::core::card_rel_path(kSharedCardId));
  local_git.commit("local corruption");

  auto local_db = open_db_with_schema(dir / "local.db");
  holder::index::FtsIndexer local_fts(local_db);
  holder::project::ProjectRepo local_projects(local_db);
  holder::model::Project local_project = seeded_project;
  local_project.root_path = local_dir.string();
  local_projects.create(local_project);

  bool diverged_seen = false;
  holder::git::NonFastForwardPullError diverged("", "", "");
  try {
    local_git.pull_remote_ff_only("origin");
  } catch (const holder::git::NonFastForwardPullError& e) {
    diverged_seen = true;
    diverged = e;
  }
  REQUIRE(diverged_seen);

  const int resolved = holder::sync::resolve_pull_conflicts(
      local_db,
      &local_fts,
      local_project,
      local_git,
      diverged,
      3
  );
  REQUIRE(resolved == 0);
}

TEST_CASE("resolve_pull_conflicts creates UUIDv4 copies for UUIDv4 projects", "[sync]") {
  const auto dir = make_temp_dir();
  const std::string project_id = "proj-1";
  const auto remote_dir = dir / "remote";
  const auto local_dir = dir / "local";

  auto remote_db = open_db_with_schema(dir / "remote.db");
  holder::index::FtsIndexer remote_fts(remote_db);
  holder::project::ProjectRepo remote_projects(remote_db);
  holder::git::RealGitOps remote_git;
  create_plain_project(remote_projects, project_id, remote_dir, holder::model::IdScheme::Uuid4);

  holder::model::Card card;
  card.card_id = kSharedCardId;
  card.project_id = project_id;
  card.title = "Shared";
  holder::card::CardStore(remote_db, &remote_fts, nullptr, &remote_git).create(card, "base");
  // An unrelated file with valid card-shaped front matter still isn't a card.
  const auto memo = holder::core::render_card_front_matter(card, {}, {});
  remote_git.write_file("memo.md", memo + "base memo");
  remote_git.stage_path("memo.md");
  remote_git.commit("Initial memo");

  holder::git::RealGitOps local_git;
  local_git.open_or_init(local_dir);
  local_git.set_remote("origin", remote_dir.string());
  local_git.pull_remote_ff_only("origin");

  holder::card::CardStore(remote_db, &remote_fts, nullptr, &remote_git)
      .update_content(kSharedCardId, "remote edit", std::nullopt, 2);
  remote_git.write_file("memo.md", memo + "remote memo");
  remote_git.stage_path("memo.md");
  remote_git.commit("Remote memo");

  auto local_db = open_db_with_schema(dir / "local.db");
  holder::index::FtsIndexer local_fts(local_db);
  holder::project::ProjectRepo local_projects(local_db);
  create_plain_project(local_projects, project_id, local_dir, holder::model::IdScheme::Uuid4);
  const auto local_project = local_projects.get(project_id).value();
  holder::store::Rebuilder(local_db, &local_fts).rebuild_project(local_project);
  holder::card::CardStore(local_db, &local_fts, nullptr, &local_git)
      .update_content(kSharedCardId, "local edit", std::nullopt, 2);
  local_git.write_file("memo.md", memo + "local memo");
  local_git.stage_path("memo.md");
  local_git.commit("Local memo");

  bool diverged_seen = false;
  holder::git::NonFastForwardPullError diverged("", "", "");
  try {
    local_git.pull_remote_ff_only("origin");
  } catch (const holder::git::NonFastForwardPullError& e) {
    diverged_seen = true;
    diverged = e;
  }
  REQUIRE(diverged_seen);

  const int resolved = holder::sync::resolve_pull_conflicts(
      local_db,
      &local_fts,
      local_project,
      local_git,
      diverged,
      3
  );
  REQUIRE(resolved == 1);

  const auto cards = holder::card::CardRepo(local_db).list_all(project_id);
  const auto duplicate = std::find_if(cards.begin(), cards.end(), [](const auto& candidate) {
    return candidate.card_id != kSharedCardId;
  });
  REQUIRE(duplicate != cards.end());
  REQUIRE(duplicate->card_id.size() == 36);
  REQUIRE(duplicate->card_id[14] == '4');
}

TEST_CASE("resolve_pull_conflicts preserves links and parents between copied cards", "[sync]") {
  const auto dir = make_temp_dir();
  const std::string project_id = "proj-1";
  const auto remote_dir = dir / "remote";
  const auto local_dir = dir / "local";
  const std::string child_id = "22222222-2222-4222-8222-222222222222";

  auto remote_db = open_db_with_schema(dir / "remote.db");
  holder::index::FtsIndexer remote_fts(remote_db);
  holder::project::ProjectRepo remote_projects(remote_db);
  holder::git::RealGitOps remote_git;
  create_plain_project(remote_projects, project_id, remote_dir);

  holder::model::Card parent;
  parent.card_id = kSharedCardId;
  parent.project_id = project_id;
  parent.title = "Parent";
  holder::card::CardStore(remote_db, &remote_fts, nullptr, &remote_git)
      .create(parent, "parent base");

  holder::model::CardLink child_link;
  child_link.project_id = project_id;
  child_link.from_card_id = child_id;
  child_link.to_card_id = parent.card_id;
  child_link.to_type = "card";
  child_link.kind = "ref";
  child_link.created_at = 1;
  holder::model::Card child;
  child.card_id = child_id;
  child.project_id = project_id;
  child.title = "Child";
  child.parent_card_id = parent.card_id;
  holder::card::CardStore(remote_db, &remote_fts, nullptr, &remote_git)
      .create(child, "child base", std::nullopt, {child_link});

  holder::git::RealGitOps local_git;
  local_git.open_or_init(local_dir);
  local_git.set_remote("origin", remote_dir.string());
  local_git.pull_remote_ff_only("origin");

  holder::card::CardStore(remote_db, &remote_fts, nullptr, &remote_git)
      .update_content(parent.card_id, "parent remote", std::nullopt, 2);
  holder::card::CardStore(remote_db, &remote_fts, nullptr, &remote_git)
      .update_content(child.card_id, "child remote", std::nullopt, 2);

  auto local_db = open_db_with_schema(dir / "local.db");
  holder::index::FtsIndexer local_fts(local_db);
  holder::project::ProjectRepo local_projects(local_db);
  create_plain_project(local_projects, project_id, local_dir);
  const auto local_project = local_projects.get(project_id).value();
  holder::store::Rebuilder(local_db, &local_fts).rebuild_project(local_project);
  holder::card::CardStore(local_db, &local_fts, nullptr, &local_git)
      .update_content(parent.card_id, "parent local", std::nullopt, 2);
  holder::card::CardStore(local_db, &local_fts, nullptr, &local_git)
      .update_content(child.card_id, "child local", std::nullopt, 2);

  holder::git::NonFastForwardPullError diverged("", "", "");
  bool diverged_seen = false;
  try {
    local_git.pull_remote_ff_only("origin");
  } catch (const holder::git::NonFastForwardPullError& error) {
    diverged_seen = true;
    diverged = error;
  }
  REQUIRE(diverged_seen);
  bool fail_second_copy = false;
  SECTION("a failed second copy leaves the first copy accurately reported") {
    fail_second_copy = true;
  }
  SECTION("both copies remain discoverable after changes and reconstruction") {}
  if (fail_second_copy) {
    local_db.exec("CREATE TRIGGER reject_child_copy BEFORE INSERT ON cards "
                  "WHEN NEW.title = 'Child (conflicted copy)' "
                  "BEGIN SELECT RAISE(ABORT, 'child copy denied'); END;");
    int preserved_count = 0;
    REQUIRE_THROWS_WITH(
        holder::sync::resolve_pull_conflicts(
            local_db,
            &local_fts,
            local_project,
            local_git,
            diverged,
            3,
            &preserved_count
        ),
        Catch::Matchers::ContainsSubstring("failed to preserve conflicted card")
    );
    CHECK(preserved_count == 1);
    const auto preserved = holder::card::LinkRepo(local_db).list_conflict_copy_links(project_id);
    REQUIRE(preserved.size() == 1);
    CHECK(preserved[0].to_card_id == parent.card_id);
    return;
  }
  REQUIRE(
      holder::sync::resolve_pull_conflicts(
          local_db,
          &local_fts,
          local_project,
          local_git,
          diverged,
          3
      ) == 2
  );

  const auto cards = holder::card::CardRepo(local_db).list_all(project_id);
  const auto parent_copy = std::find_if(cards.begin(), cards.end(), [&](const auto& card) {
    return card.title == "Parent (conflicted copy)";
  });
  const auto child_copy = std::find_if(cards.begin(), cards.end(), [&](const auto& card) {
    return card.title == "Child (conflicted copy)";
  });
  REQUIRE(parent_copy != cards.end());
  REQUIRE(child_copy != cards.end());
  REQUIRE(child_copy->parent_card_id == parent_copy->card_id);
  auto links = holder::card::LinkRepo(local_db).list_outgoing(project_id, child_copy->card_id);
  REQUIRE(links.size() == 2);
  CHECK(std::any_of(links.begin(), links.end(), [&](const auto& link) {
    return link.kind != holder::model::kConflictCopyLinkKind &&
           link.to_card_id == parent_copy->card_id;
  }));

  const auto arena = std::unique_ptr<holder_arena, decltype(&holder_arena_free)>(
      holder_arena_new(),
      holder_arena_free
  );
  const auto conflict_pairs = [&](holder_context& context) {
    const holder_sync_state* state = nullptr;
    holder_error* error = nullptr;
    REQUIRE(
        holder_get_project_sync_state(&context, project_id.c_str(), arena.get(), &state, &error) ==
        HOLDER_OK
    );
    REQUIRE(state->conflict_copies != nullptr);
    std::map<std::string, holder_conflict_copy> pairs;
    for (std::size_t i = 0; i < state->conflict_copies->count; ++i) {
      const auto& pair = state->conflict_copies->items[i];
      pairs.emplace(pair.original_card_id, pair);
    }
    return pairs;
  };
  holder_context context(local_db, local_fts, &local_git);
  auto pairs = conflict_pairs(context);
  REQUIRE(pairs.size() == 2);
  CHECK(std::string(pairs.at(parent.card_id).copy_card.card_id) == parent_copy->card_id);
  CHECK(std::string(pairs.at(child.card_id).copy_card.card_id) == child_copy->card_id);

  holder::card::CardStore store(local_db, &local_fts, nullptr, &local_git);
  store.update_content(child.card_id, "original update", "Renamed original", 4);
  store.update_content(child_copy->card_id, "preserved update", "Renamed copy", 5);
  pairs = conflict_pairs(context);
  REQUIRE(pairs.at(child.card_id).original != nullptr);
  CHECK(std::string(pairs.at(child.card_id).original->title) == "Renamed original");
  CHECK(std::string(pairs.at(child.card_id).copy_card.title) == "Renamed copy");

  const holder_conflict_comparison* comparison = nullptr;
  holder_error* comparison_error = nullptr;
  REQUIRE(
      holder_compare_conflict_copy(
          &context,
          project_id.c_str(),
          child_copy->card_id.c_str(),
          arena.get(),
          &comparison,
          &comparison_error
      ) == HOLDER_OK
  );
  CHECK(std::string(comparison->original.card_id) == child.card_id);
  CHECK(std::string(comparison->copy_card.card_id) == child_copy->card_id);
  CHECK(std::string(comparison->original.content) == "original update");
  CHECK(std::string(comparison->copy_card.content) == "preserved update");
  REQUIRE(comparison->original_changed_lines.count == 1);
  CHECK(comparison->original_changed_lines.items[0].line == 1);
  REQUIRE(comparison->copy_changed_lines.count == 1);
  CHECK(comparison->copy_changed_lines.items[0].line == 1);
  CHECK_FALSE(comparison->truncated);

  const std::string stale_token = comparison->review_token;
  const holder_conflict_resolution stale{stale_token.c_str(), "Merged title", "Combined text"};
  const holder_conflict_resolution_result* resolved = nullptr;
  // Two edits can have the same timestamp. The reviewed bytes still have to match.
  store.update_content(child_copy->card_id, "newer copy text", std::nullopt, 5);
  CHECK(
      holder_resolve_conflict_copy(
          &context,
          project_id.c_str(),
          child_copy->card_id.c_str(),
          &stale,
          arena.get(),
          &resolved,
          &comparison_error
      ) == HOLDER_ERROR_CONFLICT
  );
  holder_error_destroy(comparison_error);
  comparison_error = nullptr;
  REQUIRE(store.get_content(*store.get(child.card_id)) == "original update");
  REQUIRE(
      holder_compare_conflict_copy(
          &context,
          project_id.c_str(),
          child_copy->card_id.c_str(),
          arena.get(),
          &comparison,
          &comparison_error
      ) == HOLDER_OK
  );
  const std::string before_move = comparison->review_token;
  store.move(child.card_id, true, std::nullopt, std::nullopt, 5);
  const holder_conflict_resolution moved_review{
      before_move.c_str(),
      "Merged title",
      "Combined text"
  };
  CHECK(
      holder_resolve_conflict_copy(
          &context,
          project_id.c_str(),
          child_copy->card_id.c_str(),
          &moved_review,
          arena.get(),
          &resolved,
          &comparison_error
      ) == HOLDER_ERROR_CONFLICT
  );
  holder_error_destroy(comparison_error);
  comparison_error = nullptr;
  REQUIRE(store.get_content(*store.get(child.card_id)) == "original update");
  REQUIRE_FALSE(store.get(child.card_id)->parent_card_id.has_value());
  store.move(child.card_id, true, parent.card_id, std::nullopt, 5);
  REQUIRE(
      holder_compare_conflict_copy(
          &context,
          project_id.c_str(),
          child_copy->card_id.c_str(),
          arena.get(),
          &comparison,
          &comparison_error
      ) == HOLDER_OK
  );

  holder::git::GitRepo local_history;
  local_history.open_existing(local_dir);
  const auto head_before_resolution = local_history.head_oid();
  REQUIRE(head_before_resolution.has_value());
  const holder_conflict_resolution keep{
      comparison->review_token,
      comparison->original.title,
      comparison->original.content
  };
  REQUIRE(
      holder_resolve_conflict_copy(
          &context,
          project_id.c_str(),
          child_copy->card_id.c_str(),
          &keep,
          arena.get(),
          &resolved,
          &comparison_error
      ) == HOLDER_OK
  );
  CHECK(local_history.head_oid() == head_before_resolution);

  const holder_conflict_resolution reviewed{
      comparison->review_token,
      "Merged title",
      "Combined text"
  };
  REQUIRE(
      holder_resolve_conflict_copy(
          &context,
          project_id.c_str(),
          child_copy->card_id.c_str(),
          &reviewed,
          arena.get(),
          &resolved,
          &comparison_error
      ) == HOLDER_OK
  );
  CHECK(std::string(resolved->card.card_id) == child.card_id);
  CHECK(std::string(resolved->card.title) == "Merged title");
  CHECK(std::string(resolved->card.content) == "Combined text");
  CHECK(std::string(resolved->card.parent_card_id) == parent.card_id);
  REQUIRE(resolved->cleanup.copy_links.count == 1);
  CHECK(std::string(resolved->cleanup.copy_links.items[0].target_id) == parent_copy->card_id);
  CHECK(local_history.head_oid() != head_before_resolution);
  const auto original_links =
      holder::card::LinkRepo(local_db).list_outgoing(project_id, child.card_id);
  REQUIRE(original_links.size() == 1);
  CHECK(original_links[0].kind == child_link.kind);
  CHECK(original_links[0].to_card_id == parent.card_id);
  REQUIRE(store.get_content(*store.get(child_copy->card_id)) == "newer copy text");
  REQUIRE(conflict_pairs(context).at(child.card_id).copy_card.card_id != nullptr);

  store.trash(child.card_id, 6);
  CHECK(
      holder_compare_conflict_copy(
          &context,
          project_id.c_str(),
          child_copy->card_id.c_str(),
          arena.get(),
          &comparison,
          &comparison_error
      ) == HOLDER_ERROR_NOT_FOUND
  );
  holder_error_destroy(comparison_error);
  pairs = conflict_pairs(context);
  REQUIRE(pairs.at(child.card_id).original != nullptr);
  CHECK(pairs.at(child.card_id).original->deleted_at != nullptr);
  store.hard_delete(child.card_id);
  pairs = conflict_pairs(context);
  CHECK(pairs.at(child.card_id).original == nullptr);
  CHECK(std::string(pairs.at(child.card_id).copy_card.card_id) == child_copy->card_id);

  auto rebuilt_db = open_db_with_schema(dir / "rebuilt.db");
  holder::index::FtsIndexer rebuilt_fts(rebuilt_db);
  holder::project::ProjectRepo(rebuilt_db).create(local_project);
  holder::store::Rebuilder(rebuilt_db, &rebuilt_fts).rebuild_project(local_project);
  const auto rebuilt_child = holder::card::CardRepo(rebuilt_db).get(child_copy->card_id);
  REQUIRE(rebuilt_child.has_value());
  REQUIRE(rebuilt_child->parent_card_id == parent_copy->card_id);
  holder_context rebuilt_context(rebuilt_db, rebuilt_fts, &local_git);
  pairs = conflict_pairs(rebuilt_context);
  CHECK(pairs.at(child.card_id).original == nullptr);
  CHECK(std::string(pairs.at(child.card_id).copy_card.title) == "Renamed copy");

  holder::model::Milestone milestone;
  milestone.project_id = project_id;
  milestone.card_id = parent_copy->card_id;
  milestone.description = "Only on the copy";
  milestone.dates = {{"start", "2026-11-20", std::nullopt}};
  store.add_milestone(milestone, 7);
  store.update_content(parent_copy->card_id, "# Local notes\n\n#copy-only", std::nullopt, 7);
  const auto save_parent_result = [&] {
    REQUIRE(
        holder_compare_conflict_copy(
            &context,
            project_id.c_str(),
            parent_copy->card_id.c_str(),
            arena.get(),
            &comparison,
            &comparison_error
        ) == HOLDER_OK
    );
    const holder_conflict_resolution resolution{
        comparison->review_token,
        comparison->original.title,
        comparison->original.content
    };
    REQUIRE(
        holder_resolve_conflict_copy(
            &context,
            project_id.c_str(),
            parent_copy->card_id.c_str(),
            &resolution,
            arena.get(),
            &resolved,
            &comparison_error
        ) == HOLDER_OK
    );
  };
  save_parent_result();
  REQUIRE(resolved->cleanup.copy_milestones.count == 1);
  CHECK(std::string(resolved->cleanup.copy_milestones.items[0].description) == "Only on the copy");
  REQUIRE(resolved->cleanup.copy_tags.count == 1);
  CHECK(std::string(resolved->cleanup.copy_tags.items[0]) == "copy-only");
  REQUIRE(resolved->cleanup.children.count == 1);
  CHECK(std::string(resolved->cleanup.children.items[0].card_id) == child_copy->card_id);
  CHECK(resolved->cleanup.preserves_original);

  const auto trash_parent_copy = [&](const char* token) {
    const holder_trash_conflict_copy_request confirmation{token};
    const holder_card_metadata* trashed = nullptr;
    return holder_trash_conflict_copy(
        &context,
        project_id.c_str(),
        parent_copy->card_id.c_str(),
        &confirmation,
        arena.get(),
        &trashed,
        &comparison_error
    );
  };
  const std::string cleanup_token = resolved->cleanup.cleanup_token;
  store.move(child_copy->card_id, true, std::nullopt, std::nullopt, 8);
  CHECK(trash_parent_copy(cleanup_token.c_str()) == HOLDER_ERROR_CONFLICT);
  holder_error_destroy(comparison_error);
  comparison_error = nullptr;
  CHECK_FALSE(store.get(parent_copy->card_id)->deleted_at.has_value());

  store.move(parent.card_id, true, parent_copy->card_id, std::nullopt, 8);
  save_parent_result();
  CHECK_FALSE(resolved->cleanup.preserves_original);
  CHECK(trash_parent_copy(resolved->cleanup.cleanup_token) == HOLDER_ERROR_CONFLICT);
  holder_error_destroy(comparison_error);
  comparison_error = nullptr;
  CHECK(store.get(parent.card_id)->parent_card_id == parent_copy->card_id);
  store.move(parent.card_id, true, std::nullopt, std::nullopt, 8);
  store.move(child_copy->card_id, true, parent_copy->card_id, std::nullopt, 8);
  save_parent_result();
  const auto saved_parent = store.get(parent.card_id);
  REQUIRE(trash_parent_copy(resolved->cleanup.cleanup_token) == HOLDER_OK);
  CHECK(store.get(parent_copy->card_id)->deleted_at.has_value());
  CHECK_FALSE(store.get(child_copy->card_id)->parent_card_id.has_value());
  CHECK(store.get(parent.card_id)->sort_key == saved_parent->sort_key);
  CHECK(store.get(parent.card_id)->updated_at == saved_parent->updated_at);
  CHECK_FALSE(conflict_pairs(context).contains(parent.card_id));
  store.restore(parent_copy->card_id, 9);
  CHECK(store.get_content(*store.get(parent_copy->card_id)) == "# Local notes\n\n#copy-only");
  CHECK(conflict_pairs(context).contains(parent.card_id));
}

TEST_CASE("resolve_pull_conflicts rejects a project that no longer exists", "[sync]") {
  const auto dir = make_temp_dir();
  auto db = open_db_with_schema(dir / "holder.db");
  holder::index::FtsIndexer fts(db);

  // The caller holds a Project value, but the DB row is gone (for example deleted by another
  // process while the pull was running), so there is no id scheme to mint copies with.
  holder::model::Project project;
  project.project_id = "proj-gone";
  project.name = "Gone";
  project.root_path = (dir / "repo").string();
  project.privacy_mode = "plain";
  holder::git::RealGitOps git;

  REQUIRE_THROWS_WITH(
      holder::sync::resolve_pull_conflicts(
          db,
          &fts,
          project,
          git,
          holder::git::NonFastForwardPullError("", "", ""),
          3
      ),
      "project not found: proj-gone"
  );
}

TEST_CASE("resolve_pull_conflicts reports a conflict copy it cannot create", "[sync]") {
  // Same divergence as the UUIDv4 test above, but the project's recorded root is then pointed at
  // a regular file, so CardStore::create cannot open a repository there. The pull must report
  // the failure rather than claiming the local conflict was preserved.
  const auto dir = make_temp_dir();
  const std::string project_id = "proj-1";
  const auto remote_dir = dir / "remote";
  const auto local_dir = dir / "local";

  auto remote_db = open_db_with_schema(dir / "remote.db");
  holder::index::FtsIndexer remote_fts(remote_db);
  holder::project::ProjectRepo remote_projects(remote_db);
  holder::git::RealGitOps remote_git;
  create_plain_project(remote_projects, project_id, remote_dir);

  holder::model::Card card;
  card.card_id = kSharedCardId;
  card.project_id = project_id;
  card.title = "Shared";
  holder::card::CardStore(remote_db, &remote_fts, nullptr, &remote_git).create(card, "base");

  holder::git::RealGitOps local_git;
  local_git.open_or_init(local_dir);
  local_git.set_remote("origin", remote_dir.string());
  local_git.pull_remote_ff_only("origin");

  holder::card::CardStore(remote_db, &remote_fts, nullptr, &remote_git)
      .update_content(kSharedCardId, "remote edit", std::nullopt, 2);

  auto local_db = open_db_with_schema(dir / "local.db");
  holder::index::FtsIndexer local_fts(local_db);
  holder::project::ProjectRepo local_projects(local_db);
  create_plain_project(local_projects, project_id, local_dir);
  const auto local_project = local_projects.get(project_id).value();
  holder::store::Rebuilder(local_db, &local_fts).rebuild_project(local_project);
  holder::card::CardStore(local_db, &local_fts, nullptr, &local_git)
      .update_content(kSharedCardId, "local edit", std::nullopt, 2);

  bool diverged_seen = false;
  holder::git::NonFastForwardPullError diverged("", "", "");
  try {
    local_git.pull_remote_ff_only("origin");
  } catch (const holder::git::NonFastForwardPullError& e) {
    diverged_seen = true;
    diverged = e;
  }
  REQUIRE(diverged_seen);

  const auto not_a_directory = dir / "not-a-directory";
  std::ofstream(not_a_directory) << "plain file";
  holder::project::ProjectRepo(local_db).update_root_path(project_id, not_a_directory.string(), 3);

  REQUIRE_THROWS_WITH(
      holder::sync::resolve_pull_conflicts(
          local_db,
          &local_fts,
          local_project,
          local_git,
          diverged,
          3
      ),
      Catch::Matchers::ContainsSubstring("failed to preserve conflicted card")
  );
  const auto cards = holder::card::CardRepo(local_db).list_all(project_id);
  REQUIRE(cards.size() == 1);
  REQUIRE(cards.front().card_id == kSharedCardId);
}

TEST_CASE(
    "resolve_pull_conflicts merges cards whose two versions changed different fields",
    "[sync][merge]"
) {
  const auto dir = make_temp_dir();
  EnvGuard keystore_env("HOLDER_TEST_KEYSTORE_DIR", (dir / "keystore").string());
  const std::string project_id = "proj-merge";
  const std::string clash_id = "22222222-2222-4222-8222-222222222222";
  const auto remote_dir = dir / "remote";
  const auto local_dir = dir / "local";

  bool encrypted = false;
  SECTION("in a plain project") {}
  SECTION("in an encrypted project") { encrypted = true; }

  auto remote_db = open_db_with_schema(dir / "remote.db");
  holder::index::FtsIndexer remote_fts(remote_db);
  holder::project::ProjectRepo remote_projects(remote_db);
  holder::git::RealGitOps remote_git;
  create_plain_project(remote_projects, project_id, remote_dir);
  if (encrypted) {
    remote_db.exec("UPDATE projects SET privacy_mode = 'encrypted_git'");
    holder::privacy::ensure_encrypted_project_ready(
        remote_git,
        remote_projects,
        project_id,
        remote_dir.string(),
        std::nullopt,
        1,
        []() {
          return "test-key-merge";
        }
    );
  }
  const auto remote_project = remote_projects.get(project_id).value();
  holder::card::CardStore remote_store(remote_db, &remote_fts, nullptr, &remote_git);
  for (const std::string id : {std::string(kSharedCardId), clash_id}) {
    holder::model::Card card;
    card.card_id = id;
    card.project_id = project_id;
    card.title = id == kSharedCardId ? "Notes" : "Clash";
    card.created_at = card.updated_at = 1;
    remote_store.create(card, "base body\n");
  }

  holder::git::RealGitOps local_git;
  local_git.open_or_init(local_dir);
  local_git.set_remote("origin", remote_dir.string());
  local_git.pull_remote_ff_only("origin");
  auto local_db = open_db_with_schema(dir / "local.db");
  holder::index::FtsIndexer local_fts(local_db);
  holder::project::ProjectRepo local_projects(local_db);
  auto local_project = remote_project;
  local_project.root_path = local_dir.string();
  local_projects.create(local_project);
  holder::store::Rebuilder(local_db, &local_fts).rebuild_project(local_project);
  holder::card::CardStore local_store(local_db, &local_fts, nullptr, &local_git);

  // Notes: this device renames it and the other edits the body, so they combine.
  // Clash: both edit the body differently, so it still becomes a conflict copy.
  local_store.update_content(kSharedCardId, "base body\n", std::string("Meeting notes"), 3);
  local_store.update_content(clash_id, "local body\n", std::nullopt, 3);
  remote_store.update_content(kSharedCardId, "remote body\n", std::nullopt, 2);
  remote_store.update_content(clash_id, "remote body\n", std::nullopt, 2);

  holder::git::NonFastForwardPullError diverged("", "", "");
  bool seen = false;
  try {
    local_git.pull_remote_ff_only("origin");
  } catch (const holder::git::NonFastForwardPullError& error) {
    diverged = error;
    seen = true;
  }
  REQUIRE(seen);
  CHECK(
      holder::sync::resolve_pull_conflicts(
          local_db,
          &local_fts,
          local_project,
          local_git,
          diverged,
          4
      ) == 1
  );
  holder::sync::reconcile_index_after_pull(local_db, &local_fts, local_project);

  const auto cards = holder::card::CardRepo(local_db).list_all(project_id);
  REQUIRE(cards.size() == 3);
  const auto notes = holder::card::CardRepo(local_db).get(kSharedCardId);
  REQUIRE(notes.has_value());
  CHECK(notes->title == "Meeting notes");
  CHECK(notes->updated_at == 3);
  CHECK(std::ranges::count_if(cards, [](const auto& card) {
          return card.title == "Clash (conflicted copy)";
        }) == 1);
  CHECK(std::ranges::none_of(cards, [](const auto& card) {
    return card.title.starts_with("Notes") || card.title.starts_with("Meeting notes (");
  }));

  const auto path = holder::core::card_rel_path(kSharedCardId);
  std::ifstream input(local_dir / path, std::ios::binary);
  REQUIRE(input.is_open());
  std::string bytes{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
  if (encrypted) {
    CHECK(bytes.find("remote body") == std::string::npos);
    bytes = holder::privacy::decrypt_project_blob(project_id, *local_project.project_key_id, bytes);
  }
  const auto merged = holder::core::parse_card_file(bytes);
  CHECK(merged.card.title == "Meeting notes");
  CHECK(merged.body == "remote body\n");

  // The merge is a commit of its own in the card's history, so it can be undone.
  holder::git::GitRepo repo;
  repo.open_or_init(local_dir);
  const auto history = repo.history_for_paths({path}, 1, std::nullopt);
  REQUIRE(history.commits.size() == 1);
  CHECK(history.commits[0].message.starts_with("Merge changes to 1 card made on two devices"));
}

TEST_CASE(
    "resolve_pull_conflicts keeps a card edited on another device out of local Trash",
    "[sync][trash]"
) {
  const auto dir = make_temp_dir();
  const std::string project_id = "proj-trash";
  const auto remote_dir = dir / "remote";
  const auto local_dir = dir / "local";

  auto remote_db = open_db_with_schema(dir / "remote.db");
  holder::index::FtsIndexer remote_fts(remote_db);
  holder::project::ProjectRepo remote_projects(remote_db);
  holder::git::RealGitOps remote_git;
  create_plain_project(remote_projects, project_id, remote_dir);
  holder::card::CardStore remote_store(remote_db, &remote_fts, nullptr, &remote_git);
  holder::model::Card card;
  card.card_id = kSharedCardId;
  card.project_id = project_id;
  card.title = "Idea";
  card.created_at = card.updated_at = 1;
  remote_store.create(card, "base body\n");

  holder::git::RealGitOps local_git;
  local_git.open_or_init(local_dir);
  local_git.set_remote("origin", remote_dir.string());
  local_git.pull_remote_ff_only("origin");
  auto local_db = open_db_with_schema(dir / "local.db");
  holder::index::FtsIndexer local_fts(local_db);
  holder::project::ProjectRepo local_projects(local_db);
  create_plain_project(local_projects, project_id, local_dir);
  const auto local_project = local_projects.get(project_id).value();
  holder::store::Rebuilder(local_db, &local_fts).rebuild_project(local_project);

  holder::card::CardStore(local_db, &local_fts, nullptr, &local_git).trash(kSharedCardId, 3);
  remote_store.update_content(kSharedCardId, "edited elsewhere\n", std::nullopt, 2);

  holder::git::NonFastForwardPullError diverged("", "", "");
  bool seen = false;
  try {
    local_git.pull_remote_ff_only("origin");
  } catch (const holder::git::NonFastForwardPullError& error) {
    diverged = error;
    seen = true;
  }
  REQUIRE(seen);
  CHECK(
      holder::sync::resolve_pull_conflicts(
          local_db,
          &local_fts,
          local_project,
          local_git,
          diverged,
          4
      ) == 0
  );
  // Before the fix the card existed both live and in Trash, and this rebuild failed.
  REQUIRE_NOTHROW(holder::sync::reconcile_index_after_pull(local_db, &local_fts, local_project));

  CHECK_FALSE(std::filesystem::exists(local_dir / holder::core::card_trash_rel_path(kSharedCardId))
  );
  const auto cards = holder::card::CardRepo(local_db).list_all(project_id);
  REQUIRE(cards.size() == 1);
  CHECK(cards[0].card_id == kSharedCardId);
  CHECK_FALSE(cards[0].deleted_at.has_value());
  std::ifstream input(local_dir / holder::core::card_rel_path(kSharedCardId), std::ios::binary);
  REQUIRE(input.is_open());
  std::string bytes{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
  // Git's checkout wrote this file, so it has CRLF endings where the runner sets core.autocrlf.
  std::erase(bytes, '\r');
  CHECK(holder::core::parse_card_file(bytes).body == "edited elsewhere\n");

  holder::git::GitRepo repo;
  repo.open_or_init(local_dir);
  const auto history =
      repo.history_for_paths({holder::core::card_trash_rel_path(kSharedCardId)}, 1, std::nullopt);
  REQUIRE(history.commits.size() == 1);
  CHECK(history.commits[0].message.starts_with("Keep 1 card edited on another device out of Trash")
  );
}
