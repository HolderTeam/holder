#include "api/AiMessageWrite.h"

#include "ai/AiMessageRepo.h"
#include "ai/AiThreadRepo.h"
#include "api/support/Time.h"
#include "card/LinkMutation.h"
#include "card/LinkRepo.h"

#include <stdexcept>
#include <utility>

namespace holder::api {
namespace {

holder::model::AiMessage find_message(holder::ai::AiMessageRepo& repo,
                                      const std::string& message_id) {
  auto message = repo.get(message_id);
  if (!message) throw AiMessageNotFound{};
  return std::move(*message);
}

// The project of the message's thread, which owns the message's links. A message without its
// thread is broken data, as the link reads treat it.
std::string message_project(holder::platform::Db& db, const holder::model::AiMessage& message) {
  const auto thread = holder::ai::AiThreadRepo(db).get(message.thread_id);
  if (!thread) throw std::runtime_error("Thread not found.");
  return thread->project_id;
}

// Applies a nullable patch field: absent leaves the value, null clears it.
void patch(std::optional<std::string>& value,
           const std::optional<std::optional<std::string>>& field) {
  if (field) value = *field;
}

// Applies a patch field that cannot be cleared: absent or null leaves the value.
void patch(std::string& value, const std::optional<std::optional<std::string>>& field) {
  if (field && *field) value = **field;
}

} // namespace

AiMessageCreateResponse post_ai_messages(holder::platform::Db& db,
                                         holder::index::FtsIndexer* fts,
                                         const std::function<std::string()>& uuid_v4,
                                         AiMessageCreateRequest request) {
  holder::model::AiMessage message;
  message.message_id = request.message_id.value_or("");
  if (message.message_id.empty()) message.message_id = uuid_v4();
  message.thread_id = std::move(request.thread_id);
  message.role = std::move(request.role);
  message.source = std::move(request.source);
  message.content = std::move(request.content);
  message.provider = request.provider.value_or(std::nullopt);
  message.model = request.model.value_or(std::nullopt);
  message.prompt_hash = request.prompt_hash.value_or(std::nullopt);
  message.meta_json = request.meta_json.value_or(std::nullopt);
  message.created_at = request.created_at.value_or(0);
  if (message.created_at <= 0) message.created_at = support::now_epoch_seconds();

  if (!holder::ai::AiThreadRepo(db).get(message.thread_id)) throw AiMessageThreadNotFound{};
  holder::ai::AiMessageRepo repo(db, fts);
  if (repo.get(message.message_id))
    throw holder::model::ConflictError("ai message already exists");
  repo.append(message);
  return {true, {std::move(message.message_id)}};
}

AiMessageUpdateResponse patch_ai_messages_message_id(holder::platform::Db& db,
                                                     holder::index::FtsIndexer* fts,
                                                     std::string message_id,
                                                     AiMessageUpdateRequest request) {
  holder::ai::AiMessageRepo repo(db, fts);
  auto message = find_message(repo, message_id);
  if (message.deleted_at) throw AiMessageWrongState("AI message is deleted.");
  patch(message.role, request.role);
  patch(message.source, request.source);
  patch(message.content, request.content);
  patch(message.provider, request.provider);
  patch(message.model, request.model);
  patch(message.prompt_hash, request.prompt_hash);
  patch(message.meta_json, request.meta_json);
  if (request.created_at) message.created_at = *request.created_at;
  repo.update(message);
  return {true, {std::move(message_id)}};
}

AiMessageUpdateResponse delete_ai_messages_message_id(holder::platform::Db& db,
                                                      holder::index::FtsIndexer* fts,
                                                      std::string message_id) {
  holder::ai::AiMessageRepo repo(db, fts);
  if (find_message(repo, message_id).deleted_at)
    throw AiMessageWrongState("AI message is already deleted.");
  repo.trash(message_id, support::now_epoch_seconds());
  return {true, {std::move(message_id)}};
}

AiMessageUpdateResponse post_ai_messages_message_id_restore(holder::platform::Db& db,
                                                            holder::index::FtsIndexer* fts,
                                                            std::string message_id) {
  holder::ai::AiMessageRepo repo(db, fts);
  if (!find_message(repo, message_id).deleted_at)
    throw AiMessageWrongState("AI message is not deleted.");
  repo.restore(message_id);
  return {true, {std::move(message_id)}};
}

routes::CardLinkCreateResponse post_ai_messages_message_id_links(
    holder::platform::Db& db,
    holder::index::FtsIndexer* fts,
    std::string message_id,
    routes::CardLinkCreateRequest request) {
  holder::ai::AiMessageRepo messages(db, fts);
  const auto project_id = message_project(db, find_message(messages, message_id));

  holder::model::CardLink link;
  link.project_id = project_id;
  link.from_card_id = message_id;
  link.to_card_id = std::move(request.to_card_id);
  link.to_type = request.to_type.value_or("");
  if (link.to_type.empty()) link.to_type = "card";
  link.kind = request.kind.value_or("");
  if (link.kind.empty()) link.kind = "ref";
  link.label = request.label.value_or(std::nullopt);
  if (request.target_project_id && *request.target_project_id != project_id)
    link.target_project_id = std::move(request.target_project_id);
  link.created_at = request.created_at.value_or(0);
  if (link.created_at <= 0) link.created_at = support::now_epoch_seconds();

  holder::card::validate_link_target(
      db, project_id, link.to_card_id, link.to_type, link.target_project_id);
  holder::card::LinkRepo(db).upsert_links(project_id, message_id, {link});
  messages.update_links(message_id);

  return {true,
          {link.from_card_id, link.to_card_id, link.target_project_id, std::nullopt, link.to_type,
           link.kind, std::optional<std::optional<std::string>>(link.label), link.created_at}};
}

AiMessageCreateResponse delete_ai_messages_message_id_links(
    holder::platform::Db& db,
    holder::index::FtsIndexer* fts,
    std::string message_id,
    std::optional<CardLinkDeleteRequest> request) {
  holder::ai::AiMessageRepo messages(db, fts);
  const auto project_id = message_project(db, find_message(messages, message_id));
  const auto selector = request.value_or(CardLinkDeleteRequest{});
  const auto& [to_card_id, target_project_id, to_type, kind] = selector;

  holder::card::LinkRepo links(db);
  if (target_project_id) {
    if (!to_card_id || !to_type || !kind)
      throw std::invalid_argument(
          "to_card_id, to_type and kind are required with target_project_id.");
    if (target_project_id->empty())
      throw std::invalid_argument("target_project_id must not be empty.");
    links.delete_link_exact(project_id, message_id, *to_card_id, *to_type, *kind,
                            *target_project_id);
  } else if (to_card_id && to_type && kind) {
    std::optional<std::string> matched_project_id;
    for (const auto& link : links.list_outgoing(project_id, message_id)) {
      if (link.to_card_id != *to_card_id || link.to_type != *to_type || link.kind != *kind)
        continue;
      if (matched_project_id)
        throw std::invalid_argument("target_project_id is required to disambiguate links.");
      matched_project_id = link.target_project_id.value_or(project_id);
    }
    if (matched_project_id)
      links.delete_link_exact(project_id, message_id, *to_card_id, *to_type, *kind,
                              *matched_project_id);
  } else if (to_card_id) {
    links.delete_link(project_id, message_id, *to_card_id, to_type, kind);
  } else {
    links.delete_links_from(project_id, message_id);
  }
  messages.update_links(message_id);
  return {true, {std::move(message_id)}};
}

} // namespace holder::api
