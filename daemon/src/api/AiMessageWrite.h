#pragma once

#include "api/AiMessageRead.h"
#include "api/CardLinks.h"
#include "api/HttpError.h"
#include "api/AiRuntimeTypes.h"
#include "api/Schemas.h"
#include "model/ConflictError.h"

#include <functional>
#include <optional>
#include <stdexcept>
#include <string>

namespace holder::platform { class Db; }
namespace holder::index { class FtsIndexer; }

namespace holder::api {

struct AiMessageCreateResponseData {
  std::string message_id;
};

struct AiMessageCreateResponse {
  bool ok;
  AiMessageCreateResponseData data;
};

struct AiMessageCreateRequest {
  std::optional<std::string> message_id;
  std::string thread_id;
  std::string role;
  std::string source;
  std::optional<std::optional<std::string>> provider;
  std::optional<std::optional<std::string>> model;
  std::string content;
  std::optional<long long> created_at;
  std::optional<std::optional<std::string>> prompt_hash;
  std::optional<std::optional<std::string>> meta_json;
};

struct AiMessageUpdateResponseData {
  std::string message_id;
};

struct AiMessageUpdateResponse {
  bool ok;
  AiMessageUpdateResponseData data;
};

struct AiMessageUpdateRequest {
  std::optional<std::optional<std::string>> role;
  std::optional<std::optional<std::string>> source;
  std::optional<std::optional<std::string>> provider;
  std::optional<std::optional<std::string>> model;
  std::optional<std::optional<std::string>> content;
  std::optional<long long> created_at;
  std::optional<std::optional<std::string>> prompt_hash;
  std::optional<std::optional<std::string>> meta_json;
};

// The request names a thread that cannot be found.
struct AiMessageCaptureResponseData {
  std::string thread_id;
  std::string user_message_id;
  std::string assistant_message_id;
};

struct AiMessageCaptureResponse {
  bool ok;
  AiMessageCaptureResponseData data;
};

struct AiMessageCaptureRequest {
  std::string project_id;
  std::optional<std::optional<std::string>> thread_id;
  std::string prompt;
  std::string response;
  std::optional<std::optional<std::string>> source;
  std::optional<std::optional<std::string>> provider;
  std::optional<std::optional<std::string>> model;
  std::optional<std::optional<std::string>> url;
  std::optional<std::optional<nlohmann::json>> context;
  std::optional<std::optional<long long>> created_at;
};

struct AiCaptureNotFound : std::runtime_error {
  using std::runtime_error::runtime_error;
};

struct AiMessageThreadNotFound : std::invalid_argument {
  AiMessageThreadNotFound() : std::invalid_argument("Thread not found.") {}
};

// The message is in the trash (or, for restore, is not).
struct AiMessageWrongState : std::invalid_argument {
  using std::invalid_argument::invalid_argument;
};

AiMessageCreateResponse post_ai_messages(holder::platform::Db& db,
                                         holder::index::FtsIndexer* fts,
                                         const std::function<std::string()>& uuid_v4,
                                         AiMessageCreateRequest request);

AiMessageUpdateResponse patch_ai_messages_message_id(holder::platform::Db& db,
                                                     holder::index::FtsIndexer* fts,
                                                     std::string message_id,
                                                     AiMessageUpdateRequest request);

AiMessageUpdateResponse delete_ai_messages_message_id(holder::platform::Db& db,
                                                      holder::index::FtsIndexer* fts,
                                                      std::string message_id);

AiMessageUpdateResponse post_ai_messages_message_id_restore(
    holder::platform::Db& db,
    holder::index::FtsIndexer* fts,
    std::string message_id);

routes::CardLinkCreateResponse post_ai_messages_message_id_links(
    holder::platform::Db& db,
    holder::index::FtsIndexer* fts,
    std::string message_id,
    routes::CardLinkCreateRequest request);

AiMessageCreateResponse delete_ai_messages_message_id_links(
    holder::platform::Db& db,
    holder::index::FtsIndexer* fts,
    std::string message_id,
    std::optional<CardLinkDeleteRequest> request);

AiMessageCaptureResponse post_ai_messages_capture(holder::platform::Db& db,
                                                  holder::index::FtsIndexer* fts,
                                                  const std::function<std::string()>& uuid_v4,
                                                  AiMessageCaptureRequest request);

} // namespace holder::api
