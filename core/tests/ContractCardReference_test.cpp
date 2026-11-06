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

TEST_CASE("Contract card references keep resolution status and candidate shape", "[contract][card-reference]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  holder::index::FtsIndexer fts{db};
  holder::git::RealGitOps git;
  holder_context context{db, fts, &git};
  context.projects_root = dir / "projects";

  holder::contract::NewProject request;
  request.name = "References";
  request.privacy_mode = holder::contract::NewProjectPrivacyMode::Plain;
  const auto project = holder::contract::create_project(context, request);
  holder::card::CardRepo cards(db);
  const auto make_card = [&](const std::string& id, const std::string& title,
                             std::optional<long long> deleted_at = std::nullopt) {
    holder::model::Card card;
    card.card_id = id;
    card.project_id = project.project_id;
    card.title = title;
    card.rel_path = "cards/" + id + ".md";
    card.created_at = card.updated_at = 1;
    card.deleted_at = deleted_at;
    cards.create(card);
  };
  const std::string first = "12345678-1111-4111-8111-111111111111";
  const std::string second = "12345678-2222-4222-8222-222222222222";
  const std::string trashed = "77777777-7777-4777-8777-777777777777";
  make_card(first, "First");
  make_card(second, "Second");
  make_card(trashed, "Old", 42);

  using holder::contract::CardReferenceResolveDataStatus;
  using holder::contract::CardReferenceResolveRequestScope;
  const auto full = holder::contract::resolve_card_reference(
      context, {project.project_id, first, CardReferenceResolveRequestScope::Live});
  REQUIRE(full.status == CardReferenceResolveDataStatus::Resolved);
  REQUIRE(full.card);
  CHECK(full.card->card_id == first);
  CHECK(full.match_kind == "full_id");
  CHECK_FALSE(full.candidates);

  const auto ambiguous = holder::contract::resolve_card_reference(
      context, {project.project_id, "12345678", CardReferenceResolveRequestScope::Either});
  CHECK(ambiguous.status == CardReferenceResolveDataStatus::Ambiguous);
  CHECK(ambiguous.match_kind == "id_prefix");
  REQUIRE(ambiguous.candidates);
  CHECK(ambiguous.candidates->size() == 2);
  CHECK_FALSE(ambiguous.card);

  const auto old = holder::contract::resolve_card_reference(
      context, {project.project_id, "Old", CardReferenceResolveRequestScope::Trashed});
  REQUIRE(old.card);
  CHECK(old.card->deleted_at == 42);
  const auto absent = holder::contract::resolve_card_reference(
      context, {project.project_id, "Old", CardReferenceResolveRequestScope::Live});
  CHECK(absent.status == CardReferenceResolveDataStatus::NotFound);
  CHECK_FALSE(absent.match_kind);
  CHECK_FALSE(absent.card);
  CHECK_FALSE(absent.candidates);
  REQUIRE_THROWS_AS(holder::contract::resolve_card_reference(
                        context, {"missing", first, CardReferenceResolveRequestScope::Live}),
                    holder::contract::NotFound);
}
