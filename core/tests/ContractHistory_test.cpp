// Project activity through the contract, including its filters and cursor checks.
#if __has_include(<catch2/catch_test_macros.hpp>)
#include <catch2/catch_test_macros.hpp>
#else
#include <catch2/catch.hpp>
#endif

#include "api/Contract.h"
#include "capi/Context.h"
#include "core_test_helpers.h"
#include "git/GitOps.h"

TEST_CASE("Contract project history reads activity and validates its cursor", "[contract][history]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  holder::index::FtsIndexer fts{db};
  holder::git::RealGitOps git;
  holder_context context{db, fts, &git};
  context.projects_root = dir / "projects";

  holder::contract::NewProject request;
  request.name = "History";
  request.privacy_mode = holder::contract::NewProjectPrivacyMode::Plain;
  const auto project = holder::contract::create_project(context, request);

  const auto all = holder::contract::list_project_history(context, project.project_id, 50, std::nullopt, std::nullopt);
  REQUIRE(all.head_oid.has_value());
  REQUIRE_FALSE(all.activities.empty());
  REQUIRE(all.activities.front().oid == all.head_oid);
  REQUIRE_FALSE(all.activities.front().affected_objects.empty());

  const auto cards = holder::contract::list_project_history(
      context, project.project_id, 50, std::nullopt, holder::contract::ListProjectHistoryKind::Card);
  REQUIRE(cards.activities.size() <= all.activities.size());
  REQUIRE_THROWS_AS(holder::contract::list_project_history(context, project.project_id, 50,
                                                           std::string("not-a-commit"), std::nullopt),
                    holder::contract::InvalidArgument);
  REQUIRE_THROWS_AS(holder::contract::list_project_history(context, "missing", 50, std::nullopt, std::nullopt),
                    holder::contract::NotFound);
}

TEST_CASE("Contract card history lists, compares, snapshots, and restores", "[contract][history]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  holder::index::FtsIndexer fts{db};
  holder::git::RealGitOps git;
  holder_context context{db, fts, &git};
  context.projects_root = dir / "projects";

  holder::contract::NewProject request;
  request.name = "Card history";
  request.privacy_mode = holder::contract::NewProjectPrivacyMode::Plain;
  const auto project = holder::contract::create_project(context, request);
  holder::contract::NewCard new_card;
  new_card.title = "Saved card";
  new_card.content = "First body";
  const auto card = holder::contract::create_card(context, project.project_id, new_card);

  const auto page = holder::contract::list_card_history(context, project.project_id, card.card_id, 50, std::nullopt);
  REQUIRE_FALSE(page.entries.empty());
  const auto oid = page.entries.front().last_oid;
  const auto snapshot = holder::contract::get_card_history_snapshot(context, project.project_id, card.card_id,
                                                                    oid.substr(0, 8));
  REQUIRE(snapshot.snapshot.exists);
  REQUIRE(snapshot.snapshot.oid == oid);
  REQUIRE(snapshot.snapshot.title == "Saved card");
  const auto comparison = holder::contract::compare_card_history(
      context, project.project_id, card.card_id, oid, std::nullopt,
      holder::contract::CompareCardHistoryMode::Change);
  REQUIRE(comparison.after.oid == oid);
  REQUIRE_FALSE(comparison.before.exists);
  REQUIRE_THROWS_AS(holder::contract::compare_card_history(
                        context, project.project_id, card.card_id, oid, std::nullopt,
                        holder::contract::CompareCardHistoryMode::Since),
                    holder::contract::InvalidArgument);
  const auto restored = holder::contract::restore_card_history_snapshot(context, project.project_id,
                                                                         card.card_id, oid);
  REQUIRE(restored.restored_from_oid == oid);
  REQUIRE(restored.result_oid != oid);
}
