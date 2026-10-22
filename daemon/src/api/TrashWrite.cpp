#include "api/TrashWrite.h"

#include "ai/AiMessageRepo.h"
#include "ai/AiThreadRepo.h"
#include "card/CardRepo.h"
#include "card/CardStore.h"

#include <utility>

namespace holder::api {
namespace {

// Removes one trashed item; expected_project_id, when given, must own it.
TrashDeleteResponse delete_item(holder::platform::Db& db,
                                holder::index::FtsIndexer* fts,
                                holder::card::CardStore* card_store,
                                const std::optional<std::string>& expected_project_id,
                                const std::string& type,
                                std::string id) {
  if (type == "card") {
    if (card_store == nullptr) throw CardStoreUnavailable{};
    const auto card = holder::card::CardRepo(db).get(id);
    if (!card || (expected_project_id && card->project_id != *expected_project_id))
      throw TrashItemNotFound("Card not found.");
    if (!card->deleted_at) throw TrashItemNotTrashed("Card is not in Trash.");
    card_store->hard_delete(id, expected_project_id);
    return {true, {std::move(id), std::nullopt}};
  }
  holder::ai::AiMessageRepo messages(db, fts);
  const auto message = messages.get(id);
  const auto thread =
      message ? holder::ai::AiThreadRepo(db).get(message->thread_id) : std::nullopt;
  if (!thread || (expected_project_id && thread->project_id != *expected_project_id))
    throw TrashItemNotFound("AI message not found.");
  if (!message->deleted_at) throw TrashItemNotTrashed("AI message is not in Trash.");
  messages.remove_trashed(thread->project_id, id);
  return {true, {std::nullopt, std::move(id)}};
}

} // namespace

TrashEmptyResponse delete_trash(holder::platform::Db& db,
                                holder::index::FtsIndexer* fts,
                                holder::card::CardStore* card_store,
                                std::string project_id,
                                std::string type) {
  if (type != "ai_message") {
    if (card_store == nullptr) throw CardStoreUnavailable{};
    for (const auto& card : holder::card::CardRepo(db).list_all(project_id)) {
      if (card.deleted_at) card_store->hard_delete(card.card_id, project_id);
    }
  }
  if (type != "card") {
    holder::ai::AiMessageRepo messages(db, fts);
    for (const auto& message : messages.list_deleted_by_project(project_id))
      messages.remove_trashed(project_id, message.message_id);
  }
  return {true, {std::move(project_id)}};
}

TrashDeleteResponse delete_trash_type_id(holder::platform::Db& db,
                                         holder::index::FtsIndexer* fts,
                                         holder::card::CardStore* card_store,
                                         std::string type,
                                         std::string id) {
  return delete_item(db, fts, card_store, std::nullopt, type, std::move(id));
}

TrashDeleteResponse delete_projects_project_id_trash_type_id(holder::platform::Db& db,
                                                             holder::index::FtsIndexer* fts,
                                                             holder::card::CardStore* card_store,
                                                             std::string project_id,
                                                             std::string type,
                                                             std::string id) {
  return delete_item(db, fts, card_store, project_id, type, std::move(id));
}

} // namespace holder::api
