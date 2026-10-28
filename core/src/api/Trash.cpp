#include "api/Contract.h"

#include "api/CardAccess.h"
#include "api/CoreErrors.h"
#include "capi/Context.h"

#include "ai/AiMessageRepo.h"
#include "ai/AiThreadRepo.h"
#include "card/CardRepo.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace holder::contract {
namespace {

TrashDeleteResult remove_item(holder_context& context, const std::optional<std::string>& project_id,
                              const std::string& type, const std::string& id) {
  if (type == "card") {
    const auto card = holder::card::CardRepo(context.db).get(id);
    if (!card || (project_id && card->project_id != *project_id)) throw NotFound("Card not found.");
    if (!card->deleted_at) throw NotTrashed("Card is not in Trash.");
    auto store = card_store(context);
    store.hard_delete(id, project_id);
    return {id, std::nullopt};
  }
  if (type != "ai_message") throw InvalidArgument("type must be card or ai_message");
  holder::ai::AiMessageRepo messages(context.db, &context.fts);
  const auto message = messages.get(id);
  const auto thread = message ? holder::ai::AiThreadRepo(context.db).get(message->thread_id) : std::nullopt;
  if (!thread || (project_id && thread->project_id != *project_id))
    throw NotFound("AI message not found.");
  if (!message->deleted_at) throw NotTrashed("AI message is not in Trash.");
  messages.remove_trashed(thread->project_id, id);
  return {std::nullopt, id};
}

} // namespace

std::vector<TrashItem> list_trash(holder_context& context, std::string project_id,
                                  std::optional<std::string> type) {
  const auto wants = [&](const char* kind) {
    return !type || type->empty() || *type == kind || *type == "all";
  };
  std::vector<TrashItem> items;
  if (wants("card")) {
    for (const auto& card : holder::card::CardRepo(context.db).list_all(project_id)) {
      if (!card.deleted_at) continue;
      items.push_back({TrashItemType::Card, card.project_id, card.card_id, std::nullopt,
                       std::nullopt, card.title, std::nullopt, *card.deleted_at, std::nullopt,
                       card.rel_path});
    }
  }
  if (wants("ai_message")) {
    holder::ai::AiMessageRepo messages(context.db, &context.fts);
    for (const auto& message : messages.list_deleted_by_project(project_id)) {
      if (!message.deleted_at) continue;
      items.push_back({TrashItemType::AiMessage, project_id, std::nullopt, message.message_id,
                       message.thread_id, std::nullopt, message.role, *message.deleted_at,
                       message.created_at, std::nullopt});
    }
  }
  return items;
}

TrashEmptyResult empty_trash(holder_context& context, std::string project_id, EmptyTrashType type) {
  return with_contract_errors([&] {
    if (type != EmptyTrashType::AiMessage) {
      auto store = card_store(context);
      for (const auto& card : holder::card::CardRepo(context.db).list_all(project_id)) {
        if (card.deleted_at) store.hard_delete(card.card_id, project_id);
      }
    }
    if (type != EmptyTrashType::Card) {
      holder::ai::AiMessageRepo messages(context.db, &context.fts);
      for (const auto& message : messages.list_deleted_by_project(project_id))
        messages.remove_trashed(project_id, message.message_id);
    }
    return TrashEmptyResult{std::move(project_id)};
  });
}

TrashDeleteResult delete_trash_item(holder_context& context, std::string type, std::string id) {
  return with_contract_errors([&] { return remove_item(context, std::nullopt, type, id); });
}

TrashDeleteResult delete_project_trash_item(holder_context& context, std::string project_id,
                                             std::string type, std::string id) {
  return with_contract_errors([&] { return remove_item(context, project_id, type, id); });
}

} // namespace holder::contract
