#if __has_include(<catch2/catch_test_macros.hpp>)
#include <catch2/catch_test_macros.hpp>
#else
#include <catch2/catch.hpp>
#endif

#include "api/Contract.h"
#include "capi/Context.h"
#include "core_test_helpers.h"
#include "git/GitOps.h"

TEST_CASE("Contract Trash lists and permanently removes a scoped card", "[contract][trash]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  holder::index::FtsIndexer fts{db};
  holder::git::RealGitOps git;
  holder_context context{db, fts, &git};
  context.projects_root = dir / "projects";

  holder::contract::NewProject project_request;
  project_request.name = "Trash";
  project_request.privacy_mode = holder::contract::NewProjectPrivacyMode::Plain;
  const auto project = holder::contract::create_project(context, project_request);
  holder::contract::NewCard card_request;
  card_request.title = "Discarded";
  card_request.content = "Saved body";
  const auto card = holder::contract::create_card(context, project.project_id, card_request);

  REQUIRE_THROWS_AS(holder::contract::delete_project_trash_item(
                        context, project.project_id, holder::contract::DeleteProjectTrashItemType::Card,
                        card.card_id), holder::contract::NotTrashed);
  holder::contract::trash_card(context, project.project_id, card.card_id,
                               holder::contract::TrashCardChildren::Promote);
  const auto items = holder::contract::list_trash(context, project.project_id, "card");
  REQUIRE(items.size() == 1);
  REQUIRE(items[0].card_id == card.card_id);
  REQUIRE(items[0].title == "Discarded");
  REQUIRE(items[0].message_id == std::nullopt);
  REQUIRE_THROWS_AS(holder::contract::delete_project_trash_item(
                        context, "another-project", holder::contract::DeleteProjectTrashItemType::Card,
                        card.card_id), holder::contract::NotFound);
  const auto removed = holder::contract::delete_project_trash_item(
      context, project.project_id, holder::contract::DeleteProjectTrashItemType::Card, card.card_id);
  REQUIRE(removed.card_id == card.card_id);
  REQUIRE(holder::contract::list_trash(context, project.project_id, std::nullopt).empty());
  REQUIRE(holder::contract::empty_trash(context, project.project_id,
                                        holder::contract::EmptyTrashType::All).project_id == project.project_id);
}
