#include "api/TrashList.h"

#include "ai/AiMessageRepo.h"
#include "card/CardRepo.h"

#include <optional>
#include <string>
#include <utility>

namespace holder::api {
namespace {

std::optional<std::optional<std::string>> present(std::string value) {
  return std::optional<std::optional<std::string>>{std::in_place, std::move(value)};
}

} // namespace

TrashListResponse get_trash(holder::platform::Db& db,
                            holder::index::FtsIndexer* fts,
                            std::string project_id,
                            std::optional<std::string> type) {
  // An unknown type lists nothing, as before.
  const auto wants = [&](const char* item_type) {
    return !type || type->empty() || *type == item_type || *type == "all";
  };
  TrashListResponse response{true, {}};
  if (wants("card")) {
    for (const auto& card : holder::card::CardRepo(db).list_all(project_id)) {
      if (!card.deleted_at) continue;
      TrashItem item;
      item.type = "card";
      item.card_id = present(card.card_id);
      item.project_id = card.project_id;
      item.title = present(card.title);
      item.deleted_at = card.deleted_at;
      item.rel_path = present(card.rel_path);
      response.data.push_back(std::move(item));
    }
  }
  if (wants("ai_message")) {
    for (const auto& message :
         holder::ai::AiMessageRepo(db, fts).list_deleted_by_project(project_id)) {
      if (!message.deleted_at) continue;
      TrashItem item;
      item.type = "ai_message";
      item.message_id = present(message.message_id);
      item.thread_id = present(message.thread_id);
      item.project_id = project_id;
      item.role = present(message.role);
      item.deleted_at = message.deleted_at;
      item.created_at = message.created_at;
      response.data.push_back(std::move(item));
    }
  }
  return response;
}

} // namespace holder::api
