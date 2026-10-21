#pragma once

#include "api/ApiService.h"
#include "api/HttpError.h"
#include "api/RigAnnotations.h"
#include "api/declared/Schemas.h"

#include <optional>
#include <string>

namespace holder::api {

// Declared for the contract; the existing route handlers still write these responses.

RIG_ROUTE(GET, "/ai/capabilities")
RIG_SUMMARY("Get local model runner capabilities")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
RunnerCapabilitiesResponse get_ai_capabilities(
    std::optional<std::string> project_id RIG_QUERY RIG_PARAMETER_DESCRIPTION("Optional project id for project-scoped router config in response."));

RIG_ROUTE(GET, "/ai/status")
RIG_SUMMARY("Get live AI runtime status")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
AiStatusResponse get_ai_status();

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

RIG_ROUTE(POST, "/ai/runner/retry")
RIG_SUMMARY("Retry local model runner probe")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(501, ErrorResponse, "Not implemented")
RunnerCapabilitiesResponse post_ai_runner_retry();

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

RIG_ROUTE(POST, "/ai/runs")
RIG_SUMMARY("Create and execute an AI run with routing and streaming")
RIG_DESCRIPTION("Creates one run and streams run_started (with run_id), provider-dependent progress/router/fallback/chunk events, then done or failed. Every frame has an opaque replay ID. Reconnect via GET /ai/runs/{run_id}/events rather than repeating this POST. Disconnecting does not cancel execution; terminal events follow persisted status.")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("text/event-stream")
RIG_SSE_EVENT("run_started", AiRunEventData)
RIG_SSE_EVENT("chunk", AiRunEventData)
RIG_SSE_EVENT("done", AiRunEventData)
RIG_SSE_EVENT("failed", AiRunEventData)
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request")
RIG_ERROR_RESPONSE(503, ErrorResponse, "No local runner or configured cloud provider available")
std::string post_ai_runs(AiRunCreateRequest request);

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

RIG_ROUTE(GET, "/ai/runs/{run_id}/events")
RIG_SUMMARY("Stream run events (SSE)")
RIG_DESCRIPTION("Replays bounded retained run events with absolute event IDs; done or failed closes the stream. Missing in-memory history can recover persisted terminal status. An expired or foreign cursor emits resync_required; fetch the run before reconnecting.")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("text/event-stream")
RIG_SSE_EVENT("run_started", AiRunEventData)
RIG_SSE_EVENT("chunk", AiRunEventData)
RIG_SSE_EVENT("done", AiRunEventData)
RIG_SSE_EVENT("failed", AiRunEventData)
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Invalid query or cursor")
RIG_ERROR_RESPONSE(405, ErrorResponse, "Method not allowed")
RIG_ERROR_RESPONSE(503, ErrorResponse, "Observer unavailable or stream limit reached")
std::string get_ai_runs_run_id_events(
    std::optional<std::string> last_event_id RIG_HEADER("Last-Event-ID") RIG_PARAMETER_DESCRIPTION("Opaque SSE event ID; unavailable history emits resync_required and closes."),
    std::string run_id);

RIG_ROUTE(POST, "/ai/runner/pull")
RIG_SUMMARY("Start pulling a local model")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request")
RIG_ERROR_RESPONSE(501, ErrorResponse, "Not implemented")
RIG_ERROR_RESPONSE(503, ErrorResponse, "Runner unavailable")
RunnerPullStartResponse post_ai_runner_pull(PostAiRunnerPullRequest request);

RIG_ROUTE(GET, "/ai/runner/pull/{job_id}/events")
RIG_SUMMARY("Stream pull job progress (SSE)")
RIG_DESCRIPTION("Streams progress snapshots on status changes; completed or failed closes the stream. Pull event IDs are connection-local; reconnect without Last-Event-ID to fetch current progress after resync_required. Disconnecting does not cancel the pull.")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("text/event-stream")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found")
RIG_ERROR_RESPONSE(501, ErrorResponse, "Not implemented")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Invalid query or cursor")
RIG_ERROR_RESPONSE(405, ErrorResponse, "Method not allowed")
RIG_ERROR_RESPONSE(503, ErrorResponse, "Observer unavailable or stream limit reached")
std::string get_ai_runner_pull_job_id_events(
    std::optional<std::string> runner_id RIG_QUERY,
    std::optional<std::string> last_event_id RIG_HEADER("Last-Event-ID") RIG_PARAMETER_DESCRIPTION("Opaque SSE event ID; unavailable history emits resync_required and closes."),
    std::string job_id);

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

RIG_ROUTE(DELETE, "/ai/messages/{message_id}/links")
RIG_SUMMARY("Delete links from an AI message")
RIG_DESCRIPTION("Provide to_card_id (and optional to_type/kind) to delete matching links. A fully specified selector without target_project_id is rejected if more than one target project matches. Provide target_project_id together with to_card_id, to_type and kind to select one qualified target. If to_card_id is omitted, deletes all outgoing links.")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found")
AiMessageCreateResponse delete_ai_messages_message_id_links(
    std::string message_id,
    std::optional<CardLinkDeleteRequest> request);

RIG_ROUTE(GET, "/ai/messages/{message_id}/backlinks")
RIG_SUMMARY("List backlinks to an AI message")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found")
holder::api::routes::CardLinkListResponse get_ai_messages_message_id_backlinks(
    std::string message_id,
    std::optional<int> include_deleted RIG_QUERY RIG_INT_VALUES(0, 1) RIG_PARAMETER_DESCRIPTION("Include backlinks from deleted sources."));

} // namespace holder::api
