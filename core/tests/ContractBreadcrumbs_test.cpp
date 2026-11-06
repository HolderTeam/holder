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

TEST_CASE("Contract card breadcrumbs follow live ancestors and reject broken chains", "[contract][breadcrumbs]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  holder::index::FtsIndexer fts{db};
  holder::git::RealGitOps git;
  holder_context context{db, fts, &git};
  context.projects_root = dir / "projects";

  holder::contract::NewProject project_request;
  project_request.name = "Breadcrumbs";
  project_request.privacy_mode = holder::contract::NewProjectPrivacyMode::Plain;
  const auto project = holder::contract::create_project(context, project_request);
  holder::contract::NewCard parent_request;
  parent_request.title = "Parent";
  const auto parent = holder::contract::create_card(context, project.project_id, parent_request);
  holder::contract::NewCard child_request;
  child_request.title = "Child";
  child_request.parent_card_id = parent.card_id;
  const auto child = holder::contract::create_card(context, project.project_id, child_request);

  const auto path = holder::contract::get_card_breadcrumbs(context, project.project_id, child.card_id);
  REQUIRE(path.size() == 3);
  CHECK(path[0].kind == holder::contract::CardBreadcrumbKind::Project);
  CHECK(path[0].id == project.project_id);
  CHECK(path[1].id == parent.card_id);
  CHECK(path[1].title == "Parent");
  CHECK(path[2].kind == holder::contract::CardBreadcrumbKind::Card);
  CHECK(path[2].id == child.card_id);
  REQUIRE_THROWS_AS(holder::contract::get_card_breadcrumbs(context, project.project_id, "missing"),
                    holder::contract::NotFound);
  REQUIRE_THROWS_AS(holder::contract::get_card_breadcrumbs(context, "other-project", child.card_id),
                    holder::contract::NotFound);

  holder::card::CardRepo cards(db);
  cards.move(parent.card_id, child.card_id, 0.0, 3);
  REQUIRE_THROWS_AS(holder::contract::get_card_breadcrumbs(context, project.project_id, child.card_id),
                    holder::contract::HierarchyInvalid);
  cards.move(parent.card_id, std::nullopt, 0.0, 4);
  cards.soft_delete(parent.card_id, 5, 5);
  REQUIRE_THROWS_AS(holder::contract::get_card_breadcrumbs(context, project.project_id, child.card_id),
                    holder::contract::HierarchyInvalid);
}
