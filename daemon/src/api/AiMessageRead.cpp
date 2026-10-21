#include "api/AiMessageRead.h"

#include "ai/AiMessageRepo.h"
#include "ai/AiThreadRepo.h"
#include "card/LinkRepo.h"
#include "card/LinkVisibility.h"

#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace holder::api {
namespace {

AiMessage message_item(const holder::model::AiMessage& message) {
  return {
      message.message_id,
      message.thread_id,
      message.role,
      message.source,
      message.provider,
      message.model,
      message.content,
      message.created_at,
      message.deleted_at,
      message.prompt_hash,
      message.meta_json,
  };
}

routes::CardLink link_item(const holder::model::CardLink& link) {
  return {
      link.from_card_id,
      link.to_card_id,
      link.target_project_id,
      std::nullopt,
      link.to_type,
      link.kind,
      std::optional<std::optional<std::string>>{std::in_place, link.label},
      link.created_at,
  };
}

// The project that owns a message's links: its thread's project.
std::string message_project(holder::platform::Db& db,
                            holder::index::FtsIndexer* fts,
                            const std::string& message_id) {
  const auto message = holder::ai::AiMessageRepo(db, fts).get(message_id);
  if (!message) throw AiMessageNotFound{};
  // Messages cascade with their thread, so a missing thread is a storage fault.
  const auto thread = holder::ai::AiThreadRepo(db).get(message->thread_id);
  if (!thread) throw std::runtime_error("Thread not found.");
  return thread->project_id;
}

} // namespace

AiMessageListResponse get_ai_messages(holder::platform::Db& db,
                                      holder::index::FtsIndexer* fts,
                                      std::string thread_id,
                                      std::optional<int> include_deleted) {
  AiMessageListResponse response{true, {}};
  for (const auto& message : holder::ai::AiMessageRepo(db, fts).list_by_thread(thread_id)) {
    if (include_deleted.value_or(0) != 1 && message.deleted_at) continue;
    response.data.push_back(message_item(message));
  }
  return response;
}

AiMessageGetResponse get_ai_message(holder::platform::Db& db,
                                    holder::index::FtsIndexer* fts,
                                    std::string message_id) {
  const auto message = holder::ai::AiMessageRepo(db, fts).get(message_id);
  if (!message || message->deleted_at) throw AiMessageNotFound{};
  return {true, message_item(*message)};
}

routes::CardLinkListResponse get_ai_message_links(holder::platform::Db& db,
                                                  holder::index::FtsIndexer* fts,
                                                  std::string message_id,
                                                  std::optional<int> include_deleted) {
  const auto project_id = message_project(db, fts, message_id);
  routes::CardLinkListResponse response{true, {}};
  for (const auto& link : holder::card::LinkRepo(db).list_outgoing(project_id, message_id)) {
    if (include_deleted.value_or(0) != 1 && !holder::card::link_target_visible(db, link)) continue;
    response.data.push_back(link_item(link));
  }
  return response;
}

routes::CardLinkListResponse get_ai_message_backlinks(holder::platform::Db& db,
                                                      holder::index::FtsIndexer* fts,
                                                      std::string message_id,
                                                      std::optional<int> include_deleted) {
  const auto project_id = message_project(db, fts, message_id);
  routes::CardLinkListResponse response{true, {}};
  for (const auto& link :
       holder::card::LinkRepo(db).list_backlinks_typed(project_id, message_id, "ai_message")) {
    if (include_deleted.value_or(0) != 1 && !holder::card::backlink_source_visible(db, link)) {
      continue;
    }
    // A backlink to a message is local, so it names no target project.
    auto item = link_item(link);
    item.target_project_id.reset();
    response.data.push_back(std::move(item));
  }
  return response;
}

} // namespace holder::api
