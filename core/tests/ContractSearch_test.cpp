#if __has_include(<catch2/catch_test_macros.hpp>)
#include <catch2/catch_test_macros.hpp>
#else
#include <catch2/catch.hpp>
#endif

#include "api/Contract.h"
#include "capi/Context.h"
#include "core_test_helpers.h"
#include "git/GitOps.h"

#include "ai/AiMessageRepo.h"
#include "ai/AiThreadRepo.h"
#include "model/AiMessage.h"
#include "model/AiThread.h"

TEST_CASE("Contract Search finds card and AI message matches", "[contract][search]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  holder::index::FtsIndexer fts{db};
  holder::git::RealGitOps git;
  holder_context context{db, fts, &git};
  context.projects_root = dir / "projects";

  holder::contract::NewProject project_request;
  project_request.name = "Search";
  project_request.privacy_mode = holder::contract::NewProjectPrivacyMode::Plain;
  const auto project = holder::contract::create_project(context, project_request);
  holder::contract::NewCard card_request;
  card_request.title = "Search card";
  card_request.content = "A unique sapphire phrase";
  const auto card = holder::contract::create_card(context, project.project_id, card_request);

  const auto cards = holder::contract::search_cards(context, project.project_id, "sapphire", 20, 0);
  REQUIRE(cards.size() == 1);
  REQUIRE(cards[0].card_id == card.card_id);
  REQUIRE(cards[0].snippet.find("[sapphire]") != std::string::npos);
  REQUIRE(holder::contract::search_cards(context, project.project_id, "sapphire", 20, 1).empty());

  holder::model::AiThread thread;
  thread.thread_id = "search-thread";
  thread.project_id = project.project_id;
  thread.title = "Conversation";
  thread.created_at = thread.updated_at = 10;
  holder::ai::AiThreadRepo(db).create(thread);
  holder::model::AiMessage message;
  message.message_id = "search-message";
  message.thread_id = thread.thread_id;
  message.role = "user";
  message.source = "manual";
  message.content = "A unique sapphire message";
  message.created_at = 11;
  holder::ai::AiMessageRepo(db, &fts).append(message);

  const auto messages = holder::contract::search_ai_messages(context, project.project_id, "sapphire", 20, 0);
  REQUIRE(messages.size() == 1);
  REQUIRE(messages[0].message_id == message.message_id);
  REQUIRE_THROWS_AS(holder::contract::search_cards(context, project.project_id, "\"unbalanced", 20, 0),
                    holder::contract::InvalidArgument);
  REQUIRE_THROWS_AS(holder::contract::search_ai_messages(context, project.project_id, "\"unbalanced", 20, 0),
                    holder::contract::InvalidArgument);
}
