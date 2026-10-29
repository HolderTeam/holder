#if __has_include(<catch2/catch_test_macros.hpp>)
#include <catch2/catch_test_macros.hpp>
#else
#include <catch2/catch.hpp>
#endif

#include "api/Contract.h"
#include "capi/Context.h"
#include "core_test_helpers.h"
#include "git/GitOps.h"

#include "card/CardRepo.h"
#include "project/ProjectRepo.h"

TEST_CASE("Contract project maintenance plans repairs and checks encryption safety",
          "[contract][project-maintenance]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  holder::index::FtsIndexer fts{db};
  holder::git::RealGitOps git;
  holder_context context{db, fts, &git};
  context.projects_root = dir / "projects";

  holder::contract::NewProject project_request;
  project_request.name = "Maintenance";
  project_request.privacy_mode = holder::contract::NewProjectPrivacyMode::Plain;
  const auto project = holder::contract::create_project(context, project_request);

  const auto plain = holder::contract::check_project_encryption(context, project.project_id);
  CHECK(plain.privacy_mode == holder::contract::ProjectEncryptionCheckPrivacyMode::Plain);
  CHECK(plain.check.ok);
  CHECK(plain.check.checked_files == 0);
  REQUIRE_THROWS_AS(holder::contract::check_project_encryption(context, "missing"),
                    holder::contract::NotFound);

  holder::contract::NewCard parent_request;
  parent_request.title = "Parent";
  const auto parent = holder::contract::create_card(context, project.project_id, parent_request);
  holder::contract::NewCard child_request;
  child_request.title = "Child";
  child_request.parent_card_id = parent.card_id;
  const auto child = holder::contract::create_card(context, project.project_id, child_request);
  holder::card::CardRepo cards(db);
  cards.soft_delete(parent.card_id, 50, 50);

  const auto planned = holder::contract::repair_project_hierarchy(context, project.project_id, {});
  CHECK_FALSE(planned.applied);
  CHECK(planned.changed_card_count == 1);
  REQUIRE(planned.moved.size() == 1);
  CHECK(planned.moved[0].card_id == child.card_id);
  CHECK_FALSE(planned.moved[0].parent_card_id);
  CHECK(cards.get(child.card_id)->parent_card_id == parent.card_id);

  const auto applied = holder::contract::repair_project_hierarchy(context, project.project_id, {true});
  CHECK(applied.applied);
  CHECK(applied.changed_card_count == 1);
  CHECK_FALSE(cards.get(child.card_id)->parent_card_id);
  CHECK_FALSE(holder::contract::repair_project_hierarchy(context, project.project_id, {true}).applied);
  REQUIRE_THROWS_AS(holder::contract::repair_project_hierarchy(context, "missing", {}),
                    holder::contract::NotFound);

  holder::project::ProjectRepo(db).update_privacy_mode(project.project_id, "encrypted_git", 60);
  const auto unsafe = holder::contract::check_project_encryption(context, project.project_id);
  CHECK(unsafe.privacy_mode == holder::contract::ProjectEncryptionCheckPrivacyMode::EncryptedGit);
  CHECK_FALSE(unsafe.check.ok);
  CHECK(unsafe.check.unsafe_files >= 1);
  CHECK_FALSE(unsafe.check.unsafe_paths.empty());
}
