"""Ai operations."""

from typing import Annotated, Literal

from fastapi import Body, Header, Path, Query

from authoring import Contract, Stream
from authoring.routes import ErrorResponse

from .models import (
    AiLocalModelConfigResponse,
    AiLocalModelConfigUpdateRequest,
    AiMessageCaptureRequest,
    AiMessageCaptureResponse,
    AiMessageCreateRequest,
    AiMessageCreateResponse,
    AiMessageGetResponse,
    AiMessageListResponse,
    AiMessageUpdateRequest,
    AiMessageUpdateResponse,
    AiNudgeCandidate,
    AiNudgeDismissResponse,
    AiNudgeEvaluateResponse,
    AiNudgeListResponse,
    AiProviderCatalogResponse,
    AiProviderCredentialDeleteResponse,
    AiProviderCredentialsListResponse,
    AiProviderCredentialUpsertRequest,
    AiProviderCredentialUpsertResponse,
    AiProviderSettingDeleteResponse,
    AiProviderSettingResponse,
    AiProviderSettingsResponse,
    AiProviderSettingUpdateRequest,
    AiRunCreateRequest,
    AiRunEventData,
    AiRunGetResponse,
    AiRunListResponse,
    AiRunnerCreateRequest,
    AiRunnerDeleteResponse,
    AiRunnerListResponse,
    AiRunnerResponse,
    AiRunnerUpdateRequest,
    AiStatusResponse,
    AiStreamResyncData,
    AiThreadCreateRequest,
    AiThreadCreateResponse,
    AiThreadGetResponse,
    AiThreadListResponse,
    AiThreadPatchResponse,
    AiThreadUpdateRequest,
    CardLinkCreateRequest,
    CardLinkCreateResponse,
    CardLinkDeleteRequest,
    CardLinkListResponse,
    PostAiRunnerPullRequest,
    RunnerCapabilitiesResponse,
    RunnerPullJob,
    RunnerPullStartResponse,
    RunnerPullStatusResponse,
)


def register(contract: Contract) -> None:
    @contract.route('GET', '/ai/runner/pull/{job_id}', core=False, raw_media_type="application/json", responses={404: {"model": ErrorResponse, "description": 'Not found'}})
    def get_pull_status(
        job_id: Annotated[str, Path()],
        runner_id: Annotated[str | None, Query()] = None,
    ) -> RunnerPullStatusResponse:
        """Fetch pull job status"""


    @contract.route('POST', '/ai/runner/pull', core=False, raw_media_type="application/json", responses={404: {"model": ErrorResponse, "description": 'Runner not configured'}, 503: {"model": ErrorResponse, "description": 'Runner unavailable (runner_unavailable)'}})
    def post_ai_runner_pull(
        request: Annotated[PostAiRunnerPullRequest, Body()],
    ) -> RunnerPullStartResponse:
        """Start pulling a local model

        Pulls on runner_id, or the auto-local runner when it is omitted or empty. Follow progress with GET /ai/runner/pull/{job_id} or its events stream."""


    @contract.route('GET', '/ai/providers/credentials', core=False, raw_media_type="application/json")
    def list_provider_credentials(
    ) -> AiProviderCredentialsListResponse:
        """List configured cloud provider credentials"""


    @contract.route('PUT', '/ai/providers/credentials', core=False, raw_media_type="application/json", responses={503: {"model": ErrorResponse, "description": 'Secret store unavailable, or it failed storing the key'}})
    def upsert_provider_credential(
        request: Annotated[AiProviderCredentialUpsertRequest, Body()],
    ) -> AiProviderCredentialUpsertResponse:
        """Create or update a cloud provider API key"""


    @contract.route('DELETE', '/ai/providers/credentials/{provider}', core=False, raw_media_type="application/json", responses={503: {"model": ErrorResponse, "description": 'Secret store unavailable, or it failed removing the key'}})
    def delete_provider_credential(
        provider: Annotated[str, Path()],
    ) -> AiProviderCredentialDeleteResponse:
        """Delete a configured cloud provider API key

        Removes the stored key and disables the provider. Deleting a provider with no key succeeds."""


    @contract.route('GET', '/ai/local-models/config', core=False, raw_media_type="application/json")
    def get_local_model_config(
    ) -> AiLocalModelConfigResponse:
        """Get local model configuration"""


    @contract.route('PUT', '/ai/local-models/config', core=False, raw_media_type="application/json")
    def put_local_model_config(
        request: Annotated[AiLocalModelConfigUpdateRequest, Body()],
    ) -> AiLocalModelConfigResponse:
        """Set or clear local model configuration"""


    @contract.route('GET', '/ai/messages', core=False, raw_media_type="application/json")
    def get_ai_messages(
        thread_id: Annotated[str, Query(min_length=1)],
        # None means the query parameter is absent; a sent value is always 0 or 1.
        include_deleted: Annotated[Literal[0, 1], Query(description='Set to 1 to include deleted messages.')] = None,  # noqa: RUF013
    ) -> AiMessageListResponse:
        """List AI messages by thread"""


    @contract.route('GET', '/ai/messages/{message_id}', core=False, raw_media_type="application/json", responses={404: {"model": ErrorResponse, "description": 'Not found'}})
    def get_ai_message(
        message_id: Annotated[str, Path()],
    ) -> AiMessageGetResponse:
        """Fetch an AI message

        A message in the trash is not found."""


    @contract.route('GET', '/ai/messages/{message_id}/links', core=False, raw_media_type="application/json", responses={404: {"model": ErrorResponse, "description": 'Not found'}})
    def get_ai_message_links(
        message_id: Annotated[str, Path()],
        include_deleted: Annotated[Literal[0, 1], Query(description='Include links to deleted local targets. External card links remain listed even when their targets are unavailable or trashed.')] = None,  # noqa: RUF013
    ) -> CardLinkListResponse:
        """List outgoing links for an AI message"""


    @contract.route('GET', '/ai/messages/{message_id}/backlinks', core=False, raw_media_type="application/json", responses={404: {"model": ErrorResponse, "description": 'Not found'}})
    def get_ai_message_backlinks(
        message_id: Annotated[str, Path()],
        include_deleted: Annotated[Literal[0, 1], Query(description='Include backlinks from deleted sources.')] = None,  # noqa: RUF013
    ) -> CardLinkListResponse:
        """List backlinks to an AI message"""


    @contract.route('POST', '/ai/messages', core=False, raw_media_type="application/json", status=201, responses={409: {"model": ErrorResponse, "description": 'A message with this ID already exists'}})
    def post_ai_messages(
        request: Annotated[AiMessageCreateRequest, Body()],
    ) -> AiMessageCreateResponse:
        """Create an AI message"""


    @contract.route('PATCH', '/ai/messages/{message_id}', core=False, raw_media_type="application/json", responses={404: {"model": ErrorResponse, "description": 'Not found'}})
    def patch_ai_messages_message_id(
        message_id: Annotated[str, Path()],
        request: Annotated[AiMessageUpdateRequest, Body()],
    ) -> AiMessageUpdateResponse:
        """Update an AI message"""


    @contract.route('DELETE', '/ai/messages/{message_id}', core=False, raw_media_type="application/json", responses={404: {"model": ErrorResponse, "description": 'Not found'}})
    def delete_ai_messages_message_id(
        message_id: Annotated[str, Path()],
    ) -> AiMessageUpdateResponse:
        """Move an AI message to trash"""


    @contract.route('POST', '/ai/messages/{message_id}/restore', core=False, raw_media_type="application/json", responses={404: {"model": ErrorResponse, "description": 'Not found'}})
    def post_ai_messages_message_id_restore(
        message_id: Annotated[str, Path()],
    ) -> AiMessageUpdateResponse:
        """Restore an AI message from trash"""


    @contract.route('POST', '/ai/messages/{message_id}/links', core=False, raw_media_type="application/json", status=201, responses={404: {"model": ErrorResponse, "description": 'Not found'}})
    def post_ai_messages_message_id_links(
        message_id: Annotated[str, Path()],
        request: Annotated[CardLinkCreateRequest, Body()],
    ) -> CardLinkCreateResponse:
        """Create or update a link from an AI message"""


    @contract.route('DELETE', '/ai/messages/{message_id}/links', core=False, raw_media_type="application/json", responses={404: {"model": ErrorResponse, "description": 'Not found'}})
    def delete_ai_messages_message_id_links(
        message_id: Annotated[str, Path()],
        request: Annotated[CardLinkDeleteRequest, Body()] = None,
    ) -> AiMessageCreateResponse:
        """Delete links from an AI message

        Provide to_card_id (and optional to_type/kind) to delete matching links. A fully specified selector without target_project_id is rejected if more than one target project matches. Provide target_project_id together with to_card_id, to_type and kind to select one qualified target. If to_card_id is omitted, deletes all outgoing links."""


    @contract.route('POST', '/ai/messages/capture', core=False, raw_media_type="application/json", status=201, responses={404: {"model": ErrorResponse, "description": 'Project or thread not found'}, 409: {"model": ErrorResponse, "description": 'A generated ID already exists'}})
    def post_ai_messages_capture(
        request: Annotated[AiMessageCaptureRequest, Body()],
    ) -> AiMessageCaptureResponse:
        """Capture prompt/response pair with provenance

        Appends the prompt as a user message and the response as an assistant message carrying provider, model, url and context."""


    @contract.route('GET', '/ai/nudges', core=False, raw_media_type="application/json")
    def get_ai_nudges(
        project_id: Annotated[str, Query(min_length=1)],
        card_id: Annotated[str | None, Query(description="Only this card's nudges; empty means every card's.")] = None,
    ) -> AiNudgeListResponse:
        """List a project's current nudges

        Nudges whose evidence no longer holds are removed before listing, and dismissed nudges are left out."""


    @contract.route('POST', '/ai/nudges/{nudge_id}/dismiss', core=False, raw_media_type="application/json", responses={404: {"model": ErrorResponse, "description": 'Nudge not found'}})
    def post_ai_nudges_nudge_id_dismiss(
        nudge_id: Annotated[str, Path()],
    ) -> AiNudgeDismissResponse:
        """Dismiss a nudge"""


    @contract.route('POST', '/ai/nudges/evaluate', core=False, raw_media_type="application/json")
    def post_ai_nudges_evaluate(
        candidate: Annotated[AiNudgeCandidate, Body()],
    ) -> AiNudgeEvaluateResponse:
        """Evaluate a nudge candidate and record the nudge it warrants

        Answers whether the candidate is accepted and whether it warrants a nudge, with the reason. A candidate the daemon does not know is answered, not rejected."""


    @contract.route('GET', '/ai/providers/catalog', core=False, raw_media_type="application/json")
    def get_ai_provider_catalog(
    ) -> AiProviderCatalogResponse:
        """Get provider-agnostic cloud catalog with configured flags"""


    @contract.route('GET', '/ai/runners', core=False, raw_media_type="application/json")
    def get_ai_runners(
    ) -> AiRunnerListResponse:
        """List configured local model runners"""


    @contract.route('GET', '/ai/runners/{runner_id}', core=False, raw_media_type="application/json", responses={404: {"model": ErrorResponse, "description": 'Not found'}})
    def get_ai_runner(
        runner_id: Annotated[str, Path()],
    ) -> AiRunnerResponse:
        """Get a local model runner"""


    @contract.route('POST', '/ai/runners', core=False, raw_media_type="application/json", status=201)
    def post_ai_runners(
        request: Annotated[AiRunnerCreateRequest, Body()],
    ) -> AiRunnerResponse:
        """Configure a local model runner"""


    @contract.route('PATCH', '/ai/runners/{runner_id}', core=False, raw_media_type="application/json", responses={404: {"model": ErrorResponse, "description": 'Runner not found'}})
    def patch_ai_runners_runner_id(
        runner_id: Annotated[str, Path()],
        request: Annotated[AiRunnerUpdateRequest, Body()],
    ) -> AiRunnerResponse:
        """Update a local model runner"""


    @contract.route('DELETE', '/ai/runners/{runner_id}', core=False, raw_media_type="application/json", responses={404: {"model": ErrorResponse, "description": 'Runner not found'}})
    def delete_ai_runners_runner_id(
        runner_id: Annotated[str, Path()],
    ) -> AiRunnerDeleteResponse:
        """Remove a configured local model runner"""


    @contract.route('POST', '/ai/runners/{runner_id}/retry', core=False, raw_media_type="application/json", responses={404: {"model": ErrorResponse, "description": 'Runner not found or not configured'}})
    def post_ai_runners_runner_id_retry(
        runner_id: Annotated[str, Path()],
    ) -> AiRunnerResponse:
        """Retry one local model runner"""


    @contract.route('GET', '/ai/runs', core=False, raw_media_type="application/json")
    def get_ai_runs(
        project_id: Annotated[str | None, Query()] = None,
        thread_id: Annotated[str | None, Query()] = None,
    ) -> AiRunListResponse:
        """List AI runs by project or thread

        Needs project_id or thread_id; with both, thread_id is used."""


    @contract.route('GET', '/ai/runs/{run_id}', core=False, raw_media_type="application/json", responses={404: {"model": ErrorResponse, "description": 'Not found'}})
    def get_ai_run(
        run_id: Annotated[str, Path()],
    ) -> AiRunGetResponse:
        """Fetch an AI run"""


    @contract.route('GET', '/ai/capabilities', core=False, raw_media_type="application/json")
    def get_ai_capabilities(
        runner_id: Annotated[str | None, Query(description='The runner whose installed models decide recommended_install; defaults to auto-local.')] = None,
    ) -> RunnerCapabilitiesResponse:
        """Get local model runner capabilities"""


    @contract.route('GET', '/ai/status', core=False, raw_media_type="application/json")
    def get_ai_status(
    ) -> AiStatusResponse:
        """Get live AI runtime status"""


    @contract.route('GET', '/ai/threads', core=False, raw_media_type="application/json")
    def get_ai_threads(
        project_id: Annotated[str, Query(min_length=1)],
    ) -> AiThreadListResponse:
        """List AI threads by project"""


    @contract.route('GET', '/ai/threads/{thread_id}', core=False, raw_media_type="application/json", responses={404: {"model": ErrorResponse, "description": 'Not found'}})
    def get_ai_thread(
        thread_id: Annotated[str, Path()],
    ) -> AiThreadGetResponse:
        """Fetch an AI thread"""


    @contract.route('POST', '/ai/threads', core=False, raw_media_type="application/json", status=201, responses={409: {"model": ErrorResponse, "description": 'A thread with this ID already exists'}})
    def post_ai_threads(
        request: Annotated[AiThreadCreateRequest, Body()],
    ) -> AiThreadCreateResponse:
        """Create an AI thread"""


    @contract.route('PATCH', '/ai/threads/{thread_id}', core=False, raw_media_type="application/json", responses={404: {"model": ErrorResponse, "description": 'Not found'}})
    def patch_ai_threads_thread_id(
        thread_id: Annotated[str, Path()],
        request: Annotated[AiThreadUpdateRequest, Body()],
    ) -> AiThreadPatchResponse:
        """Update AI thread metadata"""


    @contract.route('DELETE', '/ai/threads/{thread_id}', core=False, raw_media_type="application/json", responses={404: {"model": ErrorResponse, "description": 'Not found'}})
    def delete_ai_threads_thread_id(
        thread_id: Annotated[str, Path()],
    ) -> AiThreadPatchResponse:
        """Delete an AI thread"""


    @contract.route('GET', '/ai/providers/settings', core=False, raw_media_type="application/json")
    def list_provider_settings(
    ) -> AiProviderSettingsResponse:
        """List cloud provider enabled settings"""


    @contract.route('PUT', '/ai/providers/settings', core=False, raw_media_type="application/json")
    def set_provider_setting(
        request: Annotated[AiProviderSettingUpdateRequest, Body()],
    ) -> AiProviderSettingResponse:
        """Set whether a cloud provider is enabled"""


    @contract.route('DELETE', '/ai/providers/settings/{provider}', core=False, raw_media_type="application/json")
    def delete_provider_setting(
        provider: Annotated[str, Path()],
    ) -> AiProviderSettingDeleteResponse:
        """Remove a cloud provider enabled override"""


    @contract.route('POST', '/ai/runs', core=False, sse_events={'run_started': AiRunEventData, 'progress': AiRunEventData, 'router': AiRunEventData, 'router_result': AiRunEventData, 'chunk': AiRunEventData, 'fallback': AiRunEventData, 'done': AiRunEventData, 'failed': AiRunEventData}, responses={503: {"model": ErrorResponse, "description": 'No local runner or configured cloud provider available, or stream limit reached'}})
    def post_ai_runs(
        request: Annotated[AiRunCreateRequest, Body()],
    ) -> Stream[AiRunEventData]:
        """Create and execute an AI run with routing and streaming

        Creates one run and streams run_started (with run_id), provider-dependent progress/router/fallback/chunk events, then done or failed. Every frame has an opaque replay ID. Reconnect via GET /ai/runs/{run_id}/events rather than repeating this POST. Disconnecting does not cancel execution; terminal events follow persisted status."""


    @contract.route('GET', '/ai/runs/{run_id}/events', core=False, sse_events={'run_started': AiRunEventData, 'progress': AiRunEventData, 'router': AiRunEventData, 'router_result': AiRunEventData, 'chunk': AiRunEventData, 'fallback': AiRunEventData, 'done': AiRunEventData, 'failed': AiRunEventData, 'resync_required': AiStreamResyncData}, responses={204: {"description": 'The run has finished and Last-Event-ID is past its last event: stop reconnecting'}, 404: {"model": ErrorResponse, "description": 'Not found'}, 503: {"model": ErrorResponse, "description": 'Observer unavailable or stream limit reached'}})
    def get_ai_runs_run_id_events(
        run_id: Annotated[str, Path()],
        last_event_id: Annotated[str | None, Header(alias='Last-Event-ID', description='Opaque SSE event ID; unavailable history emits resync_required and closes.')] = None,
    ) -> Stream[AiRunEventData]:
        """Stream run events (SSE)

        Replays bounded retained run events with absolute event IDs; done or failed closes the stream. Missing in-memory history can recover persisted terminal status. An expired or foreign cursor emits resync_required; fetch the run before reconnecting. A reconnect after the stream's last event is answered with 204, the event-stream standard's signal to stop reconnecting."""


    @contract.route('GET', '/ai/runner/pull/{job_id}/events', core=False, sse_events={'progress': RunnerPullJob, 'completed': RunnerPullJob, 'failed': RunnerPullJob}, responses={204: {"description": 'The pull has finished: stop reconnecting'}, 404: {"model": ErrorResponse, "description": 'Runner not configured, or pull job not found'}, 503: {"model": ErrorResponse, "description": 'Observer unavailable or stream limit reached'}})
    def get_ai_runner_pull_job_id_events(
        job_id: Annotated[str, Path()],
        runner_id: Annotated[str | None, Query()] = None,
        last_event_id: Annotated[str | None, Header(alias='Last-Event-ID', description='Opaque SSE event ID, sent by a reconnecting client.')] = None,
    ) -> Stream[RunnerPullJob]:
        """Stream pull job progress (SSE)

        Streams the job's progress snapshot whenever it changes; completed or failed closes the stream. A reconnect receives the current snapshot, and once the pull has finished it is answered with 204, the event-stream standard's signal to stop reconnecting. Disconnecting does not cancel the pull."""
