#pragma once

#include "api/AiRunnerRead.h"
#include "api/HttpError.h"
#include "api/RigAnnotations.h"
#include "api/declared/Schemas.h"

#include <functional>
#include <stdexcept>
#include <string>

namespace holder::platform { class Db; }
namespace holder::llm { class RunnerRegistry; }

namespace holder::api {

struct RIG_OPEN_RECORD AiRunnerCreateRequest {
  std::string name;
  std::string kind RIG_STRING_VALUES("ollama");
  std::string base_url RIG_FIELD_DESCRIPTION("http://host:port");
  std::optional<bool> enabled RIG_FIELD_DESCRIPTION("Optional; defaults to true.") RIG_OMIT_EMPTY;
};

struct RIG_OPEN_RECORD AiRunnerUpdateRequest {
  std::optional<std::string> name RIG_OMIT_EMPTY;
  std::optional<std::optional<std::string>> base_url RIG_FIELD_DESCRIPTION("http://host:port, or null to clear it.") RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<bool> enabled RIG_OMIT_EMPTY;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD AiRunnerDeleteResponseData {
  std::string runner_id;
};

struct RIG_OPEN_RECORD AiRunnerDeleteResponse {
  bool ok RIG_BOOL_VALUE(true);
  AiRunnerDeleteResponseData data;
};

struct AiRunnerNotConfigured : std::runtime_error {
  AiRunnerNotConfigured() : std::runtime_error("Runner not configured.") {}
};

RIG_ROUTE(POST, "/ai/runners")
RIG_SUMMARY("Configure a local model runner")
RIG_RESPONSE_DESCRIPTION("Created")
RIG_SUCCESS_STATUS(201)
RIG_ERROR_RESPONSE(400, ErrorResponse, "Malformed request, an empty name, or a base_url that is not http://host:port", std::invalid_argument)
AiRunnerResponse post_ai_runners(holder::platform::Db& db RIG_CONTEXT,
                                 holder::llm::RunnerRegistry* registry RIG_CONTEXT,
                                 const std::function<std::string()>& uuid_v4 RIG_CONTEXT,
                                 AiRunnerCreateRequest request);

RIG_ROUTE(PATCH, "/ai/runners/{runner_id}")
RIG_SUMMARY("Update a local model runner")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Malformed request, an empty name, a base_url that is not http://host:port, or the auto-local runner", std::invalid_argument)
RIG_ERROR_RESPONSE(404, ErrorResponse, "Runner not found", AiRunnerNotFound)
AiRunnerResponse patch_ai_runners_runner_id(holder::platform::Db& db RIG_CONTEXT,
                                            holder::llm::RunnerRegistry* registry RIG_CONTEXT,
                                            std::string runner_id,
                                            AiRunnerUpdateRequest request);

RIG_ROUTE(DELETE, "/ai/runners/{runner_id}")
RIG_SUMMARY("Remove a configured local model runner")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_ERROR_RESPONSE(400, ErrorResponse, "The auto-local runner cannot be removed", std::invalid_argument)
RIG_ERROR_RESPONSE(404, ErrorResponse, "Runner not found", AiRunnerNotFound)
AiRunnerDeleteResponse delete_ai_runners_runner_id(holder::platform::Db& db RIG_CONTEXT,
                                                   holder::llm::RunnerRegistry* registry RIG_CONTEXT,
                                                   std::string runner_id);

RIG_ROUTE(POST, "/ai/runners/{runner_id}/retry")
RIG_SUMMARY("Retry one local model runner")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Runner not found or not configured", AiRunnerNotFound, AiRunnerNotConfigured)
AiRunnerResponse post_ai_runners_runner_id_retry(holder::llm::RunnerRegistry* registry RIG_CONTEXT,
                                                 std::string runner_id);

} // namespace holder::api
