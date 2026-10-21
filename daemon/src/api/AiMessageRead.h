#pragma once

#include "api/CardLinks.h"
#include "api/RigAnnotations.h"
#include "api/declared/Schemas.h"

#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace holder::platform { class Db; }
namespace holder::index { class FtsIndexer; }

namespace holder::api {

struct RIG_OPEN_RECORD AiMessage {
  std::string message_id;
  std::string thread_id;
  std::string role;
  std::string source;
  std::optional<std::string> provider RIG_NULLABLE;
  std::optional<std::string> model RIG_NULLABLE;
  std::string content;
  long long created_at;
  std::optional<long long> deleted_at RIG_NULLABLE;
  std::optional<std::string> prompt_hash RIG_NULLABLE;
  std::optional<std::string> meta_json RIG_NULLABLE;
};

struct RIG_OPEN_RECORD AiMessageListResponse {
  bool ok RIG_BOOL_VALUE(true);
  std::vector<AiMessage> data;
};

struct RIG_OPEN_RECORD AiMessageGetResponse {
  bool ok RIG_BOOL_VALUE(true);
  AiMessage data;
};

struct AiMessageNotFound : std::runtime_error {
  AiMessageNotFound() : std::runtime_error("AI message not found.") {}
};

RIG_ROUTE(GET, "/ai/messages")
RIG_SUMMARY("List AI messages by thread")
RIG_RESPONSE_DESCRIPTION("OK")
AiMessageListResponse get_ai_messages(
    holder::platform::Db& db RIG_CONTEXT,
    holder::index::FtsIndexer* fts RIG_CONTEXT,
    std::string thread_id RIG_QUERY RIG_MIN_LENGTH(1),
    std::optional<int> include_deleted RIG_QUERY RIG_INT_VALUES(0, 1) RIG_PARAMETER_DESCRIPTION("Set to 1 to include deleted messages."));

RIG_ROUTE(GET, "/ai/messages/{message_id}")
RIG_SUMMARY("Fetch an AI message")
RIG_DESCRIPTION("A message in the trash is not found.")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found", AiMessageNotFound)
AiMessageGetResponse get_ai_message(holder::platform::Db& db RIG_CONTEXT,
                                    holder::index::FtsIndexer* fts RIG_CONTEXT,
                                    std::string message_id);

RIG_ROUTE(GET, "/ai/messages/{message_id}/links")
RIG_SUMMARY("List outgoing links for an AI message")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found", AiMessageNotFound)
holder::api::routes::CardLinkListResponse get_ai_message_links(
    holder::platform::Db& db RIG_CONTEXT,
    holder::index::FtsIndexer* fts RIG_CONTEXT,
    std::string message_id,
    std::optional<int> include_deleted RIG_QUERY RIG_INT_VALUES(0, 1) RIG_PARAMETER_DESCRIPTION("Include links to deleted local targets. External card links remain listed even when their targets are unavailable or trashed."));

RIG_ROUTE(GET, "/ai/messages/{message_id}/backlinks")
RIG_SUMMARY("List backlinks to an AI message")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found", AiMessageNotFound)
holder::api::routes::CardLinkListResponse get_ai_message_backlinks(
    holder::platform::Db& db RIG_CONTEXT,
    holder::index::FtsIndexer* fts RIG_CONTEXT,
    std::string message_id,
    std::optional<int> include_deleted RIG_QUERY RIG_INT_VALUES(0, 1) RIG_PARAMETER_DESCRIPTION("Include backlinks from deleted sources."));

} // namespace holder::api
