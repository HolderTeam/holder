#pragma once

#include "api/CardLinks.h"
#include "api/AiRuntimeTypes.h"
#include "api/Schemas.h"

#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace holder::platform { class Db; }
namespace holder::index { class FtsIndexer; }

namespace holder::api {

struct AiMessage {
  std::string message_id;
  std::string thread_id;
  std::string role;
  std::string source;
  std::optional<std::string> provider;
  std::optional<std::string> model;
  std::string content;
  long long created_at;
  std::optional<long long> deleted_at;
  std::optional<std::string> prompt_hash;
  std::optional<std::string> meta_json;
};

struct AiMessageListResponse {
  bool ok;
  std::vector<AiMessage> data;
};

struct AiMessageGetResponse {
  bool ok;
  AiMessage data;
};

struct AiMessageNotFound : std::runtime_error {
  AiMessageNotFound() : std::runtime_error("AI message not found.") {}
};

AiMessageListResponse get_ai_messages(
    holder::platform::Db& db,
    holder::index::FtsIndexer* fts,
    std::string thread_id,
    std::optional<int> include_deleted);

AiMessageGetResponse get_ai_message(holder::platform::Db& db,
                                    holder::index::FtsIndexer* fts,
                                    std::string message_id);

holder::api::routes::CardLinkListResponse get_ai_message_links(
    holder::platform::Db& db,
    holder::index::FtsIndexer* fts,
    std::string message_id,
    std::optional<int> include_deleted);

holder::api::routes::CardLinkListResponse get_ai_message_backlinks(
    holder::platform::Db& db,
    holder::index::FtsIndexer* fts,
    std::string message_id,
    std::optional<int> include_deleted);

} // namespace holder::api
