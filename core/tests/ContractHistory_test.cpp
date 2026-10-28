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
