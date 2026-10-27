// The contract's link operations with every kind of target, and with AI messages as the sources of
// backlinks, on a context a host makes from its own services.
#if __has_include(<catch2/catch_test_macros.hpp>)
#include <catch2/catch_test_macros.hpp>
#else
#include <catch2/catch.hpp>
#endif

#include "ai/AiMessageRepo.h"
#include "ai/AiThreadRepo.h"
#include "api/Contract.h"
#include "capi/Context.h"
#include "card/LinkRepo.h"
#include "core_test_helpers.h"
#include "git/GitOps.h"
#include "project/ProjectRepo.h"
#include "resource/ResourceRepo.h"

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

  Host() {
    for (const char* id : {"proj-1", "proj-2"}) {
      holder::model::Project project;
      project.project_id = id;
      project.name = id;
      project.root_path = (dir / id).string();
      std::filesystem::create_directories(project.root_path);
      project.privacy_mode = "plain";
      project.created_at = project.updated_at = 1;
      holder::project::ProjectRepo(db).create(project);
      holder::model::AiThread thread;
      thread.thread_id = std::string("thread-") + id;
      thread.project_id = id;
      thread.title = "Thread";
      thread.created_at = thread.updated_at = 1;
      holder::ai::AiThreadRepo(db).create(thread);
      holder::model::Resource resource;
      resource.resource_id = std::string("resource-") + id;
      resource.project_id = id;
      resource.type = "website";
      resource.metadata["identifier"] = {std::string("https://example.com/") + id};
      resource.label = "Resource";
      resource.created_at = resource.updated_at = 1;
      holder::resource::ResourceRepo(db).add(resource);
    }
  }

  std::string card(const std::string& project_id, const std::string& title) {
    contract::NewCard request;
    request.title = title;
    return contract::create_card(context, project_id, request).card_id;
  }

  void message(const std::string& message_id, const std::string& thread_id, long long created_at) {
    holder::model::AiMessage message;
    message.message_id = message_id;
    message.thread_id = thread_id;
    message.role = "user";
    message.source = "manual";
    message.content = "A message";
    message.created_at = created_at;
    holder::ai::AiMessageRepo(db, &fts).append(message);
  }

  // A link from an AI message, as the AI message routes write one.
  void message_link(const std::string& project_id, const std::string& message_id, const std::string& card_id,
                    const std::string& card_project_id, long long created_at) {
    holder::model::CardLink link;
    link.project_id = project_id;
    link.from_card_id = message_id;
    link.to_card_id = card_id;
    link.to_type = "card";
    link.kind = "ref";
    link.created_at = created_at;
    if (card_project_id != project_id) link.target_project_id = card_project_id;
    holder::card::LinkRepo(db).upsert_links(project_id, message_id, {link});
  }
};

contract::NewLink to(contract::NewLinkTargetType type, const std::string& id) {
  contract::NewLink link;
  link.target_type = type;
  link.target_id = id;
  return link;
}

} // namespace

TEST_CASE("A card links to threads, messages and resources in its own project", "[contract][links]") {
  Host host;
  using Type = contract::NewLinkTargetType;
  const auto card = host.card("proj-1", "Card");
  host.message("message-1", "thread-proj-1", 2);
  host.message("message-2", "thread-proj-2", 3);

  for (const auto& [type, id] : {std::pair{Type::AiThread, "thread-proj-1"},
                                 std::pair{Type::Resource, "resource-proj-1"},
                                 std::pair{Type::AiMessage, "message-1"}}) {
    const auto link = contract::add_card_link(host.context, "proj-1", card, to(type, id));
    REQUIRE(link.target_id == id);
    REQUIRE(link.target_project_id == "proj-1");
  }
  REQUIRE(contract::list_card_links(host.context, "proj-1", card, false).size() == 3);

  // Not ones in another project, and not a message in the trash.
  for (const auto& [type, id] : {std::pair{Type::AiThread, "thread-proj-2"},
                                 std::pair{Type::Resource, "resource-proj-2"},
                                 std::pair{Type::AiMessage, "message-2"}}) {
    REQUIRE_THROWS_AS(contract::add_card_link(host.context, "proj-1", card, to(type, id)), contract::NotFound);
  }
  holder::ai::AiMessageRepo(host.db, &host.fts).trash("message-1", 4);
  REQUIRE(contract::list_card_links(host.context, "proj-1", card, false).size() == 2);
  REQUIRE(contract::list_card_links(host.context, "proj-1", card, true).size() == 3);
  auto again = to(Type::AiMessage, "message-1");
  again.kind = "see_also";
  REQUIRE_THROWS_AS(contract::add_card_link(host.context, "proj-1", card, again), contract::NotFound);
}

TEST_CASE("A card's backlinks come from cards and AI messages, here and in other projects", "[contract][links]") {
  Host host;
  const auto card = host.card("proj-1", "Target");
  const auto from_card = host.card("proj-1", "Source");
  contract::add_card_link(host.context, "proj-1", from_card, to(contract::NewLinkTargetType::Card, card));
  host.message("message-1", "thread-proj-1", 2);
  host.message_link("proj-1", "message-1", card, "proj-1", 5);
  host.message("message-2", "thread-proj-1", 3);
  host.message_link("proj-1", "message-2", card, "proj-1", 6);
  holder::ai::AiMessageRepo(host.db, &host.fts).trash("message-2", 7);
  host.message("message-3", "thread-proj-2", 4);
  host.message_link("proj-2", "message-3", card, "proj-1", 8);
  // A link whose source is gone is never listed.
  host.message_link("proj-1", "ghost-source", card, "proj-1", 9);

  const auto local =
      contract::list_card_backlinks(host.context, "proj-1", card, contract::ListCardBacklinksScope::Local, false);
  // Newest first: the card's link was made now.
  REQUIRE(local.size() == 2);
  REQUIRE(local[0].source_type == contract::LinkSourceType::Card);
  REQUIRE(local[0].source_id == from_card);
  REQUIRE(local[1].source_type == contract::LinkSourceType::AiMessage);
  REQUIRE(local[1].source_id == "message-1");
  // A message in the trash is a source only when asked for.
  REQUIRE(contract::list_card_backlinks(host.context, "proj-1", card, contract::ListCardBacklinksScope::Local, true)
              .size() == 3);

  const auto external = contract::list_card_backlinks(host.context, "proj-1", card,
                                                      contract::ListCardBacklinksScope::External, false);
  REQUIRE(external.size() == 1);
  REQUIRE(external[0].source_project_id == "proj-2");
  REQUIRE(external[0].source_id == "message-3");
  REQUIRE(external[0].target_project_id == "proj-1");
  REQUIRE(contract::list_card_backlinks(host.context, "proj-1", card,
                                        contract::ListCardBacklinksScope::AllAvailable, false)
              .size() == 3);

  // A project not on this device is not a source.
  std::filesystem::remove_all(host.dir / "proj-2");
  REQUIRE(contract::list_card_backlinks(host.context, "proj-1", card, contract::ListCardBacklinksScope::External,
                                        false)
              .empty());
}
