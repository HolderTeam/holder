#pragma once

#include "api/ApiService.h"
#include "api/HttpError.h"
#include "api/RigAnnotations.h"

#include <nlohmann/json.hpp>

#include <optional>
#include <string>
#include <vector>

namespace holder::api {

// Declared for the contract; the existing route handlers still write these responses.

struct RIG_OPEN_RECORD AiProviderCatalogProvider {
  std::string id;
  std::string display_name;
  bool enabled;
  bool configured;
  std::optional<nlohmann::json> api RIG_NULLABLE;
  std::optional<nlohmann::json> auth RIG_NULLABLE;
  std::vector<nlohmann::json> models;
  std::optional<std::optional<std::string>> setup_url RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> docs_url RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> api_key_label RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> api_key_hint RIG_OMIT_EMPTY RIG_NULLABLE;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD AiProviderCatalogResponseData {
  std::optional<std::optional<std::string>> default_provider RIG_OMIT_EMPTY RIG_NULLABLE;
  std::vector<AiProviderCatalogProvider> providers;
};

struct RIG_OPEN_RECORD AiProviderCatalogResponse {
  bool ok RIG_BOOL_VALUE(true);
  AiProviderCatalogResponseData data;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD AiProviderCredentialDeleteResponseData {
  std::string provider;
};

struct RIG_OPEN_RECORD AiProviderCredentialDeleteResponse {
  bool ok RIG_BOOL_VALUE(true);
  AiProviderCredentialDeleteResponseData data;
};

struct RIG_OPEN_RECORD LocalModel {
  std::string name;
  std::optional<std::string> digest RIG_OMIT_EMPTY;
  std::optional<long long> size RIG_OMIT_EMPTY;
  std::optional<std::string> modified_at RIG_OMIT_EMPTY;
};

struct RIG_OPEN_RECORD AiRunnerRuntime {
  bool configured;
  bool available;
  bool spawn_attempted;
  long long last_checked;
  std::optional<std::string> version RIG_NULLABLE;
  std::optional<std::string> error RIG_NULLABLE;
  std::vector<LocalModel> models;
  std::vector<holder::api::RunnerPullJob> pulls;
};

struct RIG_OPEN_RECORD AiRunner {
  std::string runner_id;
  std::string name;
  std::string kind;
  std::optional<std::string> base_url RIG_NULLABLE;
  std::string source;
  bool enabled;
  long long created_at;
  long long updated_at;
  AiRunnerRuntime runtime;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD AiRunnerListResponseData {
  std::vector<AiRunner> runners;
};

struct RIG_OPEN_RECORD AiRunnerListResponse {
  bool ok RIG_BOOL_VALUE(true);
  AiRunnerListResponseData data;
};

struct RIG_OPEN_RECORD AiRunnerResponse {
  bool ok RIG_BOOL_VALUE(true);
  AiRunner data;
};

struct RIG_OPEN_RECORD AiRunnerCreateRequest {
  std::string name;
  std::string kind RIG_STRING_VALUES("ollama");
  std::string base_url;
  std::optional<bool> enabled RIG_OMIT_EMPTY;
};

struct RIG_OPEN_RECORD AiRunnerUpdateRequest {
  std::optional<std::string> name RIG_OMIT_EMPTY;
  std::optional<std::optional<std::string>> base_url RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<bool> enabled RIG_OMIT_EMPTY;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD AiRunnerDeleteResponseData {
  std::string runner_id;
};

struct RIG_OPEN_RECORD AiRunnerDeleteResponse {
  bool ok RIG_BOOL_VALUE(true);
  AiRunnerDeleteResponseData data;
};

struct RIG_OPEN_RECORD AiLocalModelConfigData {
  std::optional<std::string> fast_model RIG_NULLABLE;
  std::optional<std::string> strong_model RIG_NULLABLE;
  std::optional<std::string> deep_model RIG_NULLABLE;
  std::optional<long long> updated_at RIG_NULLABLE;
};

struct RIG_OPEN_RECORD AiLocalModelConfigResponse {
  bool ok RIG_BOOL_VALUE(true);
  AiLocalModelConfigData data;
};

struct RIG_OPEN_RECORD AiLocalModelConfigUpdateRequest {
  std::optional<std::optional<std::string>> fast_model RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> strong_model RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> deep_model RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<long long>> updated_at RIG_OMIT_EMPTY RIG_NULLABLE;
};

struct RIG_OPEN_RECORD AiRunItem {
  std::string run_id;
  std::optional<std::optional<std::string>> project_id RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> thread_id RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> message_id RIG_OMIT_EMPTY RIG_NULLABLE;
  std::string mode;
  std::string prompt;
  std::optional<std::optional<std::string>> context_json RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> router_model RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> ranked_json RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> policy_trace_json RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<nlohmann::json>> policy_trace RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> chosen_model RIG_OMIT_EMPTY RIG_NULLABLE;
  std::string status;
  std::optional<std::optional<std::string>> error RIG_OMIT_EMPTY RIG_NULLABLE;
  long long created_at;
  long long updated_at;
};

struct RIG_OPEN_RECORD AiRunListResponse {
  bool ok RIG_BOOL_VALUE(true);
  std::vector<AiRunItem> data;
};

struct RIG_OPEN_RECORD AiRunGetResponse {
  bool ok RIG_BOOL_VALUE(true);
  AiRunItem data;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD RunnerPullStartResponseData {
  std::string job_id;
  std::optional<std::string> runner_id RIG_OMIT_EMPTY;
  std::optional<std::string> model_ref RIG_OMIT_EMPTY;
  std::string model;
  std::string status;
};

struct RIG_OPEN_RECORD RunnerPullStartResponse {
  bool ok RIG_BOOL_VALUE(true);
  RunnerPullStartResponseData data;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD PostAiRunnerPullRequest {
  std::string model;
  std::optional<std::string> runner_id RIG_OMIT_EMPTY;
};

struct RIG_OPEN_RECORD AiMessage {
  std::string message_id;
  std::string thread_id;
  std::string role;
  std::string source;
  std::optional<std::optional<std::string>> provider RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> model RIG_OMIT_EMPTY RIG_NULLABLE;
  std::string content;
  long long created_at;
  std::optional<std::optional<long long>> deleted_at RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> prompt_hash RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> meta_json RIG_OMIT_EMPTY RIG_NULLABLE;
};

struct RIG_OPEN_RECORD AiMessageListResponse {
  bool ok RIG_BOOL_VALUE(true);
  std::vector<AiMessage> data;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD AiMessageCreateResponseData {
  std::string message_id;
};

struct RIG_OPEN_RECORD AiMessageCreateResponse {
  bool ok RIG_BOOL_VALUE(true);
  AiMessageCreateResponseData data;
};

struct RIG_OPEN_RECORD AiMessageCreateRequest {
  std::optional<std::string> message_id RIG_FIELD_DESCRIPTION("Optional; server generates if omitted.") RIG_OMIT_EMPTY;
  std::string thread_id;
  std::string role;
  std::string source;
  std::optional<std::optional<std::string>> provider RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> model RIG_OMIT_EMPTY RIG_NULLABLE;
  std::string content;
  std::optional<long long> created_at RIG_FIELD_DESCRIPTION("Optional; server uses current time if omitted or 0.") RIG_OMIT_EMPTY;
  std::optional<std::optional<std::string>> prompt_hash RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> meta_json RIG_OMIT_EMPTY RIG_NULLABLE;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD AiMessageCaptureResponseData {
  std::string thread_id;
  std::string user_message_id;
  std::string assistant_message_id;
};

struct RIG_OPEN_RECORD AiMessageCaptureResponse {
  bool ok RIG_BOOL_VALUE(true);
  AiMessageCaptureResponseData data;
};

struct RIG_OPEN_RECORD AiMessageCaptureRequest {
  std::string project_id;
  std::optional<std::optional<std::string>> thread_id RIG_OMIT_EMPTY RIG_NULLABLE;
  std::string prompt;
  std::string response;
  std::optional<std::optional<std::string>> source RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> provider RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> model RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> url RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<nlohmann::json>> context RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<long long>> created_at RIG_OMIT_EMPTY RIG_NULLABLE;
};

struct RIG_OPEN_RECORD AiThread {
  std::string thread_id;
  std::string project_id;
  std::optional<std::optional<std::string>> card_id RIG_OMIT_EMPTY RIG_NULLABLE;
  std::string title;
  long long created_at;
  long long updated_at;
};

struct RIG_OPEN_RECORD AiThreadListResponse {
  bool ok RIG_BOOL_VALUE(true);
  std::vector<AiThread> data;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD AiThreadCreateResponseData {
  std::string thread_id;
};

struct RIG_OPEN_RECORD AiThreadCreateResponse {
  bool ok RIG_BOOL_VALUE(true);
  AiThreadCreateResponseData data;
};

struct RIG_OPEN_RECORD AiThreadCreateRequest {
  std::optional<std::string> thread_id RIG_FIELD_DESCRIPTION("Optional; server generates if omitted.") RIG_OMIT_EMPTY;
  std::string project_id;
  std::string title;
  std::optional<std::optional<std::string>> card_id RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<long long> created_at RIG_FIELD_DESCRIPTION("Optional; server uses current time if omitted or 0.") RIG_OMIT_EMPTY;
  std::optional<long long> updated_at RIG_FIELD_DESCRIPTION("Optional; server uses created_at if omitted or 0.") RIG_OMIT_EMPTY;
};

struct RIG_OPEN_RECORD AiThreadGetResponse {
  bool ok RIG_BOOL_VALUE(true);
  AiThread data;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD AiThreadPatchResponseData {
  std::string thread_id;
};

struct RIG_OPEN_RECORD AiThreadPatchResponse {
  bool ok RIG_BOOL_VALUE(true);
  AiThreadPatchResponseData data;
};

struct RIG_OPEN_RECORD AiThreadUpdateRequest {
  std::optional<std::optional<std::string>> title RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> card_id RIG_OMIT_EMPTY RIG_NULLABLE;
  long long updated_at;
};

struct RIG_OPEN_RECORD AiMessageGetResponse {
  bool ok RIG_BOOL_VALUE(true);
  AiMessage data;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD AiMessageUpdateResponseData {
  std::string message_id;
};

struct RIG_OPEN_RECORD AiMessageUpdateResponse {
  bool ok RIG_BOOL_VALUE(true);
  AiMessageUpdateResponseData data;
};

struct RIG_OPEN_RECORD AiMessageUpdateRequest {
  std::optional<std::optional<std::string>> role RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> source RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> provider RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> model RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> content RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<long long> created_at RIG_OMIT_EMPTY;
  std::optional<std::optional<std::string>> prompt_hash RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> meta_json RIG_OMIT_EMPTY RIG_NULLABLE;
};

RIG_ROUTE(GET, "/ai/providers/catalog")
RIG_SUMMARY("Get provider-agnostic cloud catalog with configured flags")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request")
AiProviderCatalogResponse get_ai_providers_catalog();

RIG_ROUTE(DELETE, "/ai/providers/credentials/{provider}")
RIG_SUMMARY("Delete a configured cloud provider API key")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request")
AiProviderCredentialDeleteResponse delete_ai_providers_credentials_provider(std::string provider);

RIG_ROUTE(GET, "/ai/runners")
RIG_SUMMARY("List configured local model runners")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
AiRunnerListResponse get_ai_runners();

RIG_ROUTE(POST, "/ai/runners")
RIG_SUMMARY("Configure a local model runner")
RIG_RESPONSE_DESCRIPTION("Created")
RIG_HOST_RESPONSE("application/json")
RIG_SUCCESS_STATUS(201)
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request")
AiRunnerResponse post_ai_runners(AiRunnerCreateRequest request);

RIG_ROUTE(GET, "/ai/runners/{runner_id}")
RIG_SUMMARY("Get a local model runner")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found")
AiRunnerResponse get_ai_runners_runner_id(std::string runner_id);

RIG_ROUTE(PATCH, "/ai/runners/{runner_id}")
RIG_SUMMARY("Update a local model runner")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found")
AiRunnerResponse patch_ai_runners_runner_id(std::string runner_id, AiRunnerUpdateRequest request);

RIG_ROUTE(DELETE, "/ai/runners/{runner_id}")
RIG_SUMMARY("Remove a configured local model runner")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found")
AiRunnerDeleteResponse delete_ai_runners_runner_id(std::string runner_id);

RIG_ROUTE(POST, "/ai/runners/{runner_id}/retry")
RIG_SUMMARY("Retry one local model runner")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found")
AiRunnerResponse post_ai_runners_runner_id_retry(std::string runner_id);

RIG_ROUTE(GET, "/ai/local-models/config")
RIG_SUMMARY("Get local model configuration")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
AiLocalModelConfigResponse get_ai_local_models_config();

RIG_ROUTE(PUT, "/ai/local-models/config")
RIG_SUMMARY("Set or clear local model configuration")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request")
AiLocalModelConfigResponse put_ai_local_models_config(AiLocalModelConfigUpdateRequest request);

RIG_ROUTE(GET, "/ai/runs")
RIG_SUMMARY("List AI runs by project or thread")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request")
AiRunListResponse get_ai_runs(
    std::optional<std::string> project_id RIG_QUERY,
    std::optional<std::string> thread_id RIG_QUERY);

RIG_ROUTE(GET, "/ai/runs/{run_id}")
RIG_SUMMARY("Fetch an AI run")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found")
AiRunGetResponse get_ai_runs_run_id(std::string run_id);

RIG_ROUTE(POST, "/ai/runner/pull")
RIG_SUMMARY("Start pulling a local model")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request")
RIG_ERROR_RESPONSE(501, ErrorResponse, "Not implemented")
RIG_ERROR_RESPONSE(503, ErrorResponse, "Runner unavailable")
RunnerPullStartResponse post_ai_runner_pull(PostAiRunnerPullRequest request);

RIG_ROUTE(GET, "/ai/messages")
RIG_SUMMARY("List AI messages by thread")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request")
AiMessageListResponse get_ai_messages(
    std::string thread_id RIG_QUERY,
    std::optional<int> include_deleted RIG_QUERY RIG_PARAMETER_DESCRIPTION("Set to 1 to include deleted messages."));

RIG_ROUTE(POST, "/ai/messages")
RIG_SUMMARY("Create an AI message")
RIG_RESPONSE_DESCRIPTION("Created")
RIG_HOST_RESPONSE("application/json")
RIG_SUCCESS_STATUS(201)
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request")
RIG_ERROR_RESPONSE(409, ErrorResponse, "Conflict")
AiMessageCreateResponse post_ai_messages(AiMessageCreateRequest request);

RIG_ROUTE(POST, "/ai/messages/capture")
RIG_SUMMARY("Capture prompt/response pair with provenance")
RIG_RESPONSE_DESCRIPTION("Created")
RIG_HOST_RESPONSE("application/json")
RIG_SUCCESS_STATUS(201)
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found")
RIG_ERROR_RESPONSE(409, ErrorResponse, "Conflict")
AiMessageCaptureResponse post_ai_messages_capture(AiMessageCaptureRequest request);

RIG_ROUTE(GET, "/ai/threads")
RIG_SUMMARY("List AI threads by project")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request")
AiThreadListResponse get_ai_threads(std::string project_id RIG_QUERY);

RIG_ROUTE(POST, "/ai/threads")
RIG_SUMMARY("Create an AI thread")
RIG_RESPONSE_DESCRIPTION("Created")
RIG_HOST_RESPONSE("application/json")
RIG_SUCCESS_STATUS(201)
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request")
RIG_ERROR_RESPONSE(409, ErrorResponse, "Conflict")
AiThreadCreateResponse post_ai_threads(AiThreadCreateRequest request);

RIG_ROUTE(GET, "/ai/threads/{thread_id}")
RIG_SUMMARY("Fetch an AI thread")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found")
AiThreadGetResponse get_ai_threads_thread_id(std::string thread_id);

RIG_ROUTE(PATCH, "/ai/threads/{thread_id}")
RIG_SUMMARY("Update AI thread metadata")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found")
AiThreadPatchResponse patch_ai_threads_thread_id(
    std::string thread_id,
    AiThreadUpdateRequest request);

RIG_ROUTE(DELETE, "/ai/threads/{thread_id}")
RIG_SUMMARY("Delete an AI thread")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found")
AiThreadPatchResponse delete_ai_threads_thread_id(std::string thread_id);

RIG_ROUTE(GET, "/ai/messages/{message_id}")
RIG_SUMMARY("Fetch an AI message")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found")
AiMessageGetResponse get_ai_messages_message_id(std::string message_id);

RIG_ROUTE(PATCH, "/ai/messages/{message_id}")
RIG_SUMMARY("Update an AI message")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found")
AiMessageUpdateResponse patch_ai_messages_message_id(
    std::string message_id,
    AiMessageUpdateRequest request);

RIG_ROUTE(DELETE, "/ai/messages/{message_id}")
RIG_SUMMARY("Move an AI message to trash")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found")
AiMessageUpdateResponse delete_ai_messages_message_id(std::string message_id);

RIG_ROUTE(POST, "/ai/messages/{message_id}/restore")
RIG_SUMMARY("Restore an AI message from trash")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found")
AiMessageUpdateResponse post_ai_messages_message_id_restore(std::string message_id);

RIG_ROUTE(GET, "/ai/messages/{message_id}/links")
RIG_SUMMARY("List outgoing links for an AI message")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found")
holder::api::routes::CardLinkListResponse get_ai_messages_message_id_links(
    std::string message_id,
    std::optional<int> include_deleted RIG_QUERY RIG_INT_VALUES(0, 1) RIG_PARAMETER_DESCRIPTION("Include links to deleted local targets. External card links remain listed even when their targets are unavailable or trashed."));

RIG_ROUTE(POST, "/ai/messages/{message_id}/links")
RIG_SUMMARY("Create or update a link from an AI message")
RIG_RESPONSE_DESCRIPTION("Created")
RIG_HOST_RESPONSE("application/json")
RIG_SUCCESS_STATUS(201)
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found")
holder::api::routes::CardLinkCreateResponse post_ai_messages_message_id_links(
    std::string message_id,
    holder::api::routes::CardLinkCreateRequest request);

RIG_ROUTE(GET, "/ai/messages/{message_id}/backlinks")
RIG_SUMMARY("List backlinks to an AI message")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found")
holder::api::routes::CardLinkListResponse get_ai_messages_message_id_backlinks(
    std::string message_id,
    std::optional<int> include_deleted RIG_QUERY RIG_INT_VALUES(0, 1) RIG_PARAMETER_DESCRIPTION("Include backlinks from deleted sources."));

} // namespace holder::api
