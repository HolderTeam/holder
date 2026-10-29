"""AI route request and response records migrated from the daemon RIG declarations."""

from typing import Annotated, Literal

from holder_contract import MISSING, Field, Float64, Int64, JsonValue, OpenRecord


class RunnerPullProgress(OpenRecord):
    """RunnerPullProgress."""

    completed: Int64
    """The completed."""
    total: Int64
    """The total."""
    percent: Float64
    """The percent."""
    stage: str
    """The stage."""


class RunnerPullStartResponseData(OpenRecord):
    """RunnerPullStart responseData."""

    job_id: str
    """The job id."""
    runner_id: str | None | MISSING = MISSING
    """The runner id."""
    model_ref: str | None | MISSING = MISSING
    """The model ref."""
    model: str
    """The model."""
    status: str
    """The status."""


class PostAiRunnerPullRequest(OpenRecord):
    """PostAI RunnerPullRequest."""

    model: str
    """The model."""
    runner_id: str | None | MISSING = MISSING
    """The runner id."""


class AiProviderCredential(OpenRecord):
    """AI ProviderCredential."""

    provider: str
    """The provider."""
    configured: bool
    """The configured."""
    api_key_preview: str
    """The api key preview."""
    created_at: Int64
    """The created at."""
    updated_at: Int64
    """The updated at."""


class AiProviderCredentialUpsertRequest(OpenRecord):
    """AI ProviderCredentialUpsertRequest."""

    provider: str
    """The provider."""
    api_key: str
    """The api key."""
    updated_at: Int64 | None | MISSING = MISSING
    """The updated at."""


class AiProviderCredentialDeleteResponseData(OpenRecord):
    """AI ProviderCredentialDelete responseData."""

    provider: str
    """The provider."""


class CardLinkDeleteRequest(OpenRecord):
    """CardLinkDeleteRequest."""

    to_card_id: str | None | MISSING = MISSING
    """The to card id."""
    target_project_id: str | None | MISSING = MISSING
    """Selects the target project. Requires to_card_id, to_type and kind; use it to distinguish equal card IDs in different projects."""
    to_type: str | None | MISSING = MISSING
    """The to type."""
    kind: str | None | MISSING = MISSING
    """The kind."""


class AiLocalModelConfigData(OpenRecord):
    """AI LocalModelConfigData."""

    fast_model: str | None
    """The fast model."""
    strong_model: str | None
    """The strong model."""
    deep_model: str | None
    """The deep model."""
    updated_at: Int64 | None
    """The updated at."""


class LocalModel(OpenRecord):
    """LocalModel."""

    name: str
    """The name."""
    runner_id: str | None | MISSING = MISSING
    """The runner that has this model."""
    model_ref: str | None | MISSING = MISSING
    """The runner-qualified model reference to request it by."""
    digest: str | None | MISSING = MISSING
    """The digest."""
    size: Int64 | None | MISSING = MISSING
    """The size."""
    modified_at: str | None | MISSING = MISSING
    """The modified at."""


class AiRunEventData(OpenRecord):
    """Named run events share run_id. Fields vary by event and provider.."""

    run_id: str
    """The run id."""
    status: str | None | MISSING = MISSING
    """The status."""
    created_at: Int64 | None | MISSING = MISSING
    """The created at."""
    delta: str | None | MISSING = MISSING
    """The delta."""
    message: str | None | MISSING = MISSING
    """The message."""
    error: str | None | MISSING = MISSING
    """The error."""
    runner_id: str | None | MISSING = MISSING
    """The runner id."""
    model_ref: str | None | MISSING = MISSING
    """The model ref."""
    model: str | None | MISSING = MISSING
    """The model."""
    provider: str | None | MISSING = MISSING
    """The provider."""
    context_compacted: bool | None | MISSING = MISSING
    """On a cloud progress event: whether the thread was summarized to fit the model's context."""
    cooldown_until: Int64 | None | MISSING = MISSING
    """On a cloud fallback: when the failed model may be tried again, in epoch seconds."""
    router_model: str | None | MISSING = MISSING
    """On router_result: the model that ranked the candidates."""
    ranked: list[str] | None | MISSING = MISSING
    """On router_result: the candidate models, best first."""
    partial: bool | None | MISSING = MISSING
    """On a local fallback: whether the failed model had already streamed some output."""


class AiRunCreateRequest(OpenRecord):
    """AI RunCreateRequest."""

    prompt: str
    """The prompt."""
    mode: str | None | MISSING = MISSING
    """auto or model"""
    model: str | None | MISSING = MISSING
    """Optional explicit model id; interpreted for local or cloud selected provider."""
    provider: str | None | MISSING = MISSING
    """Optional cloud provider id when local runner is unavailable."""
    project_id: str | None | MISSING = MISSING
    """The project id."""
    thread_id: str | None | MISSING = MISSING
    """The thread id."""
    context: dict[str, JsonValue] | None | MISSING = MISSING
    """The context."""
    runner_id: str | None | MISSING = MISSING
    """The local runner to use; defaults to the automatic local runner."""


class AiThread(OpenRecord):
    """AI Thread."""

    thread_id: str
    """The thread id."""
    project_id: str
    """The project id."""
    card_id: str | None | MISSING = MISSING
    """The card id."""
    title: str
    """The title."""
    created_at: Int64
    """The created at."""
    updated_at: Int64
    """The updated at."""


class AiLocalModelConfigUpdateRequest(OpenRecord):
    """Replaces the whole configuration: an omitted, null or empty model clears it.."""

    fast_model: str | None | MISSING = MISSING
    """The fast model."""
    strong_model: str | None | MISSING = MISSING
    """The strong model."""
    deep_model: str | None | MISSING = MISSING
    """The deep model."""
    updated_at: Int64 | None | MISSING = MISSING
    """Optional; server uses current time if omitted or null."""


class AiMessage(OpenRecord):
    """AI Message."""

    message_id: str
    """The message id."""
    thread_id: str
    """The thread id."""
    role: str
    """The role."""
    source: str
    """The source."""
    provider: str | None
    """The provider."""
    model: str | None
    """The model."""
    content: str
    """The content."""
    created_at: Int64
    """The created at."""
    deleted_at: Int64 | None
    """The deleted at."""
    prompt_hash: str | None
    """The prompt hash."""
    meta_json: str | None
    """The meta json."""


class AiMessageCreateResponseData(OpenRecord):
    """AI MessageCreate responseData."""

    message_id: str
    """The message id."""


class AiMessageCreateRequest(OpenRecord):
    """AI MessageCreateRequest."""

    message_id: str | None | MISSING = MISSING
    """Optional; server generates if omitted."""
    thread_id: str
    """The thread id."""
    role: str
    """The role."""
    source: str
    """The source."""
    provider: str | None | MISSING = MISSING
    """The provider."""
    model: str | None | MISSING = MISSING
    """The model."""
    content: str
    """The content."""
    created_at: Int64 | None | MISSING = MISSING
    """Optional; server uses current time if omitted or 0."""
    prompt_hash: str | None | MISSING = MISSING
    """The prompt hash."""
    meta_json: str | None | MISSING = MISSING
    """The meta json."""


class AiMessageUpdateResponseData(OpenRecord):
    """AI MessageUpdate responseData."""

    message_id: str
    """The message id."""


class AiMessageUpdateRequest(OpenRecord):
    """AI MessageUpdateRequest."""

    role: str | None | MISSING = MISSING
    """Omit or send null to leave it unchanged."""
    source: str | None | MISSING = MISSING
    """Omit or send null to leave it unchanged."""
    provider: str | None | MISSING = MISSING
    """Omit to leave it unchanged; null clears it."""
    model: str | None | MISSING = MISSING
    """Omit to leave it unchanged; null clears it."""
    content: str | None | MISSING = MISSING
    """Omit or send null to leave it unchanged."""
    created_at: Int64 | None | MISSING = MISSING
    """Omit to leave it unchanged."""
    prompt_hash: str | None | MISSING = MISSING
    """Omit to leave it unchanged; null clears it."""
    meta_json: str | None | MISSING = MISSING
    """Omit to leave it unchanged; null clears it."""


class AiMessageCaptureResponseData(OpenRecord):
    """AI MessageCapture responseData."""

    thread_id: str
    """The thread id."""
    user_message_id: str
    """The user message id."""
    assistant_message_id: str
    """The assistant message id."""


class AiMessageCaptureRequest(OpenRecord):
    """AI MessageCaptureRequest."""

    project_id: str
    """The project id."""
    thread_id: str | None | MISSING = MISSING
    """An existing thread in the project; when omitted or null a new thread is created, titled from the prompt."""
    prompt: str
    """The prompt."""
    response: str
    """The response."""
    source: str | None | MISSING = MISSING
    """Optional; defaults to manual_paste."""
    provider: str | None | MISSING = MISSING
    """The provider."""
    model: str | None | MISSING = MISSING
    """The model."""
    url: str | None | MISSING = MISSING
    """The url."""
    context: dict[str, JsonValue] | None | MISSING = MISSING
    """The context."""
    created_at: Int64 | None | MISSING = MISSING
    """Optional; server uses current time if omitted or null."""


class AiNudge(OpenRecord):
    """AI Nudge."""

    nudge_id: str
    """The nudge id."""
    kind: str
    """The kind."""
    project_id: str
    """The project id."""
    card_id: str | None
    """The card id."""
    title: str
    """The title."""
    body: str
    """The body."""
    meta_json: dict[str, JsonValue]
    """The evidence and details the nudge was built from."""
    basis_fingerprint: str | None
    """The basis fingerprint."""
    basis_commit: str | None
    """The basis commit."""
    created_at: Int64
    """The created at."""
    suggestions: list[str] | None | MISSING = MISSING
    """Suggested titles, for a card.title_suggestion nudge."""


class AiNudgeDismissData(OpenRecord):
    """AI NudgeDismissData."""

    nudge_id: str
    """The nudge id."""
    dismissed: bool
    """The dismissed."""


class AiNudgeCandidate(OpenRecord):
    """AI NudgeCandidate."""

    kind: Annotated[str, Field(min_length=1)]
    """The candidate's kind, such as card.title_only; an unknown kind is not accepted."""
    project_id: Annotated[str, Field(min_length=1)]
    """The project id."""
    card_id: str | None | MISSING = MISSING
    """The card id."""
    created_at: Int64
    """The created at."""
    basis_fingerprint: str | None | MISSING = MISSING
    """The basis fingerprint."""
    basis_commit: str | None | MISSING = MISSING
    """The basis commit."""
    facts: dict[str, JsonValue]
    """The evidence for the candidate, which depends on its kind."""


class AiProviderCatalogModelLimits(OpenRecord):
    """AI ProviderCatalogModelLimits."""

    rpm: Int64
    """Requests per minute; 0 when unlimited or unknown."""
    tpm: Int64
    """Tokens per minute; 0 when unlimited or unknown."""
    rpd: Int64
    """Requests per day; 0 when unlimited or unknown."""


class AiProviderCatalogApi(OpenRecord):
    """AI ProviderCatalogApi."""

    base_url: str | None | MISSING = MISSING
    """The base url."""
    kind: str | None | MISSING = MISSING
    """The kind."""


class AiProviderCatalogAuth(OpenRecord):
    """AI ProviderCatalogAuth."""

    type: str | None | MISSING = MISSING
    """The type."""
    key_param: str | None | MISSING = MISSING
    """The key param."""
    header_name: str | None | MISSING = MISSING
    """The header name."""
    bearer_prefix: str | None | MISSING = MISSING
    """The bearer prefix."""


class AiRunnerCreateRequest(OpenRecord):
    """AI RunnerCreateRequest."""

    name: str
    """The name."""
    kind: Literal['ollama']
    """The kind."""
    base_url: str
    """http://host:port"""
    enabled: bool | None | MISSING = MISSING
    """Optional; defaults to true."""


class AiRunnerUpdateRequest(OpenRecord):
    """AI RunnerUpdateRequest."""

    name: str | None | MISSING = MISSING
    """The name."""
    base_url: str | None | MISSING = MISSING
    """http://host:port, or null to clear it."""
    enabled: bool | None | MISSING = MISSING
    """The enabled."""


class AiRunnerDeleteResponseData(OpenRecord):
    """AI RunnerDelete responseData."""

    runner_id: str
    """The runner id."""


class AiRunItem(OpenRecord):
    """AI RunItem."""

    run_id: str
    """The run id."""
    project_id: str | None | MISSING = MISSING
    """The project id."""
    thread_id: str | None | MISSING = MISSING
    """The thread id."""
    message_id: str | None | MISSING = MISSING
    """The message id."""
    mode: str
    """The mode."""
    prompt: str
    """The prompt."""
    context_json: str | None | MISSING = MISSING
    """The context json."""
    router_model: str | None | MISSING = MISSING
    """The router model."""
    router_runner_id: str | None | MISSING = MISSING
    """The runner named by router_model's reference, or null."""
    ranked_json: str | None | MISSING = MISSING
    """The ranked json."""
    policy_trace_json: str | None | MISSING = MISSING
    """The policy trace json."""
    policy_trace: dict[str, JsonValue] | None | MISSING = MISSING
    """The policy trace."""
    chosen_model: str | None | MISSING = MISSING
    """The chosen model."""
    chosen_runner_id: str | None | MISSING = MISSING
    """The runner named by chosen_model's reference, or null."""
    status: str
    """The status."""
    error: str | None | MISSING = MISSING
    """The error."""
    created_at: Int64
    """The created at."""
    updated_at: Int64
    """The updated at."""


class CasteInfo(OpenRecord):
    """CasteInfo."""

    name: Literal['mini', 'user', 'developer', 'workstation', 'rig']
    """The name."""
    reason: str | None
    """The reason."""


class CasteRecommendedModel(OpenRecord):
    """CasteRecommendedModel."""

    tag: str
    """The tag."""
    provider: str | None
    """The provider."""
    engine: str | None
    """The engine."""
    category: str | None
    """The category."""
    required_caste: Literal['mini', 'user', 'developer', 'workstation', 'rig']
    """The required caste."""
    installed: bool
    """The installed."""


class AiThreadCreateResponseData(OpenRecord):
    """AI ThreadCreate responseData."""

    thread_id: str
    """The thread id."""


class AiThreadCreateRequest(OpenRecord):
    """AI ThreadCreateRequest."""

    thread_id: str | None | MISSING = MISSING
    """Optional; server generates if omitted."""
    project_id: str
    """The project id."""
    title: str
    """The title."""
    card_id: str | None | MISSING = MISSING
    """The card id."""
    created_at: Int64 | None | MISSING = MISSING
    """Optional; server uses current time if omitted or 0."""
    updated_at: Int64 | None | MISSING = MISSING
    """Optional; server uses created_at if omitted or 0."""


class AiThreadPatchResponseData(OpenRecord):
    """AI ThreadPatch responseData."""

    thread_id: str
    """The thread id."""


class AiThreadUpdateRequest(OpenRecord):
    """AI ThreadUpdateRequest."""

    title: str | None | MISSING = MISSING
    """Omit or send null to leave it unchanged."""
    card_id: str | None | MISSING = MISSING
    """Omit to leave it unchanged; null detaches the thread from its card."""
    updated_at: Int64
    """The updated at."""


class AiProviderSetting(OpenRecord):
    """AI ProviderSetting."""

    provider: str
    """The provider."""
    enabled: bool
    """The enabled."""
    updated_at: Int64
    """The updated at."""


class AiProviderSettingUpdateRequest(OpenRecord):
    """AI ProviderSettingUpdateRequest."""

    provider: str
    """The provider."""
    enabled: bool
    """The enabled."""
    updated_at: Int64 | None | MISSING = MISSING
    """The updated at."""


class AiProviderSettingDeleteData(OpenRecord):
    """AI ProviderSettingDeleteData."""

    provider: str
    """The provider."""


class AiStreamResyncData(OpenRecord):
    """AI StreamResyncData."""

    reason: Literal['history_unavailable']
    """The stream's retained history no longer covers Last-Event-ID, for example after a daemon restart: fetch the run, then reconnect without Last-Event-ID."""


class CardLink(OpenRecord):
    """CardLink."""

    from_card_id: str
    """The from card id."""
    to_card_id: str
    """The to card id."""
    target_project_id: str | None | MISSING = MISSING
    """Present for a card target in another project; omitted for a local target. The link grants no access to the target."""
    source_project_id: str | None | MISSING = MISSING
    """Present on a cross-project backlink; identifies the available project that owns the link."""
    to_type: str
    """Target type (card, ai_message, ai_thread, resource)."""
    kind: str
    """The kind."""
    label: str | None | MISSING = MISSING
    """The label."""
    created_at: Int64
    """The created at."""


class CardLinkCreateRequest(OpenRecord):
    """CardLinkCreateRequest."""

    to_card_id: str
    """The to card id."""
    target_project_id: str | None | MISSING = MISSING
    """Optional for card targets. A named external target project need not be loaded locally."""
    to_type: str | None | MISSING = MISSING
    """Optional; defaults to "card"."""
    kind: str | None | MISSING = MISSING
    """Optional; defaults to "ref"."""
    label: str | None | MISSING = MISSING
    """The label."""
    created_at: Int64 | None | MISSING = MISSING
    """Optional; server uses current time if omitted or 0."""


class RunnerPullJob(OpenRecord):
    """RunnerPullJob."""

    job_id: str
    """The job id."""
    runner_id: str | None | MISSING = MISSING
    """The runner id."""
    model_ref: str | None | MISSING = MISSING
    """The model ref."""
    model: str
    """The model."""
    status: str
    """The status."""
    active: bool
    """Whether the pull is still queued, downloading or verifying."""
    updated_at: Int64
    """The updated at."""
    error: str | None | MISSING = MISSING
    """The error."""
    progress: RunnerPullProgress
    """The progress."""


class RunnerPullStartResponse(OpenRecord):
    """RunnerPullStart response."""

    ok: bool
    """The ok."""
    data: RunnerPullStartResponseData
    """The data."""


class AiProviderCredentialsListData(OpenRecord):
    """AI ProviderCredentialsListData."""

    providers: list[AiProviderCredential]
    """The providers."""


class AiProviderCredentialUpsertResponse(OpenRecord):
    """AI ProviderCredentialUpsert response."""

    ok: bool
    """The ok."""
    data: AiProviderCredential
    """The data."""


class AiProviderCredentialDeleteResponse(OpenRecord):
    """AI ProviderCredentialDelete response."""

    ok: bool
    """The ok."""
    data: AiProviderCredentialDeleteResponseData
    """The data."""


class AiLocalModelConfigResponse(OpenRecord):
    """AI LocalModelConfig response."""

    ok: bool
    """The ok."""
    data: AiLocalModelConfigData
    """The data."""


class AiMessageListResponse(OpenRecord):
    """AI MessageList response."""

    ok: bool
    """The ok."""
    data: list[AiMessage]
    """The data."""


class AiMessageGetResponse(OpenRecord):
    """AI MessageGet response."""

    ok: bool
    """The ok."""
    data: AiMessage
    """The data."""


class AiMessageCreateResponse(OpenRecord):
    """AI MessageCreate response."""

    ok: bool
    """The ok."""
    data: AiMessageCreateResponseData
    """The data."""


class AiMessageUpdateResponse(OpenRecord):
    """AI MessageUpdate response."""

    ok: bool
    """The ok."""
    data: AiMessageUpdateResponseData
    """The data."""


class AiMessageCaptureResponse(OpenRecord):
    """AI MessageCapture response."""

    ok: bool
    """The ok."""
    data: AiMessageCaptureResponseData
    """The data."""


class AiNudgeListData(OpenRecord):
    """AI NudgeListData."""

    nudges: list[AiNudge]
    """The nudges."""


class AiNudgeDismissResponse(OpenRecord):
    """AI NudgeDismiss response."""

    ok: bool
    """The ok."""
    data: AiNudgeDismissData
    """The data."""


class AiNudgeEvaluateData(OpenRecord):
    """AI NudgeEvaluateData."""

    kind: str
    """The kind."""
    accepted: bool
    """The accepted."""
    should_nudge: bool
    """The should nudge."""
    reason: str
    """The reason."""
    nudge: AiNudge | None
    """The recorded nudge, or null when there is nothing to show."""


class AiProviderCatalogModel(OpenRecord):
    """AI ProviderCatalogModel."""

    id: str
    """The id."""
    endpoint: str
    """The endpoint."""
    role: str | None
    """The role."""
    cost_tier: str | None
    """The cost tier."""
    default_for_low_budget: bool
    """The default for low budget."""
    limits: AiProviderCatalogModelLimits
    """The limits."""


class AiRunnerDeleteResponse(OpenRecord):
    """AI RunnerDelete response."""

    ok: bool
    """The ok."""
    data: AiRunnerDeleteResponseData
    """The data."""


class AiRunListResponse(OpenRecord):
    """AI RunList response."""

    ok: bool
    """The ok."""
    data: list[AiRunItem]
    """The data."""


class AiRunGetResponse(OpenRecord):
    """AI RunGet response."""

    ok: bool
    """The ok."""
    data: AiRunItem
    """The data."""


class AiThreadListResponse(OpenRecord):
    """AI ThreadList response."""

    ok: bool
    """The ok."""
    data: list[AiThread]
    """The data."""


class AiThreadGetResponse(OpenRecord):
    """AI ThreadGet response."""

    ok: bool
    """The ok."""
    data: AiThread
    """The data."""


class AiThreadCreateResponse(OpenRecord):
    """AI ThreadCreate response."""

    ok: bool
    """The ok."""
    data: AiThreadCreateResponseData
    """The data."""


class AiThreadPatchResponse(OpenRecord):
    """AI ThreadPatch response."""

    ok: bool
    """The ok."""
    data: AiThreadPatchResponseData
    """The data."""


class AiProviderSettingResponse(OpenRecord):
    """AI ProviderSetting response."""

    ok: bool
    """The ok."""
    data: AiProviderSetting
    """The data."""


class AiProviderSettingsData(OpenRecord):
    """AI ProviderSettingsData."""

    providers: list[AiProviderSetting]
    """The providers."""


class AiProviderSettingDeleteResponse(OpenRecord):
    """AI ProviderSettingDelete response."""

    ok: bool
    """The ok."""
    data: AiProviderSettingDeleteData
    """The data."""


class CardLinkListResponse(OpenRecord):
    """CardLinkList response."""

    ok: bool
    """The ok."""
    data: list[CardLink]
    """The data."""


class CardLinkCreateResponse(OpenRecord):
    """CardLinkCreate response."""

    ok: bool
    """The ok."""
    data: CardLink
    """The data."""


class RunnerPullStatusResponse(OpenRecord):
    """RunnerPullStatus response."""

    ok: bool
    """The ok."""
    data: RunnerPullJob
    """The data."""


class AiProviderCredentialsListResponse(OpenRecord):
    """AI ProviderCredentialsList response."""

    ok: bool
    """The ok."""
    data: AiProviderCredentialsListData
    """The data."""


class AiRunnerRuntime(OpenRecord):
    """AI RunnerRuntime."""

    configured: bool
    """The configured."""
    available: bool
    """The available."""
    spawn_attempted: bool
    """The spawn attempted."""
    last_checked: Int64
    """The last checked."""
    version: str | None
    """The version."""
    error: str | None
    """The error."""
    models: list[LocalModel]
    """The models."""
    pulls: list[RunnerPullJob]
    """The pulls."""


class AiNudgeListResponse(OpenRecord):
    """AI NudgeList response."""

    ok: bool
    """The ok."""
    data: AiNudgeListData
    """The data."""


class AiNudgeEvaluateResponse(OpenRecord):
    """AI NudgeEvaluate response."""

    ok: bool
    """The ok."""
    data: AiNudgeEvaluateData
    """The data."""


class AiProviderCatalogProvider(OpenRecord):
    """AI ProviderCatalogProvider."""

    id: str
    """The id."""
    display_name: str
    """The display name."""
    enabled: bool
    """This device's setting, or the catalog default without one."""
    configured: bool
    """Whether this device has an API key for the provider."""
    cost_tier: str | None
    """The cost tier."""
    setup_url: str | None
    """The setup url."""
    docs_url: str | None
    """The docs url."""
    api_key_label: str | None
    """The api key label."""
    api_key_hint: str | None
    """The api key hint."""
    api: AiProviderCatalogApi
    """The api."""
    auth: AiProviderCatalogAuth
    """The auth."""
    models: list[AiProviderCatalogModel]
    """The models."""


class AiProviderSettingsResponse(OpenRecord):
    """AI ProviderSettings response."""

    ok: bool
    """The ok."""
    data: AiProviderSettingsData
    """The data."""


class AiRunner(OpenRecord):
    """AI Runner."""

    runner_id: str
    """The runner id."""
    name: str
    """The name."""
    kind: str
    """The kind."""
    base_url: str | None
    """The base url."""
    source: str
    """The source."""
    enabled: bool
    """The enabled."""
    created_at: Int64
    """The created at."""
    updated_at: Int64
    """The updated at."""
    runtime: AiRunnerRuntime
    """The runtime."""


class AiProviderCatalogResponseData(OpenRecord):
    """AI ProviderCatalog responseData."""

    default_provider: str | None
    """The default provider."""
    providers: list[AiProviderCatalogProvider]
    """The providers."""


class AiRunnerListResponseData(OpenRecord):
    """AI RunnerList responseData."""

    runners: list[AiRunner]
    """The runners."""


class AiRunnerResponse(OpenRecord):
    """AI Runner response."""

    ok: bool
    """The ok."""
    data: AiRunner
    """The data."""


class AiProviderCatalogResponse(OpenRecord):
    """AI ProviderCatalog response."""

    ok: bool
    """The ok."""
    data: AiProviderCatalogResponseData
    """The data."""


class RunnerCapabilitiesData(OpenRecord):
    """RunnerCapabilitiesData."""

    local_model_config: AiLocalModelConfigData
    """The local model config."""
    caste: CasteInfo | None | MISSING = MISSING
    """This machine's hardware caste, or null when it cannot be detected."""
    runners: list[AiRunner]
    """The runners."""
    recommended_models: list[CasteRecommendedModel]
    """Catalog models this machine's caste can run, best first."""
    recommended_install: list[CasteRecommendedModel]
    """The recommended models that the selected runner has not installed."""


class AiStatusData(OpenRecord):
    """AI StatusData."""

    checked_at: Int64
    """The checked at."""
    active_runs: Int64
    """Runs still in progress across every project."""
    active_pull_jobs: Int64
    """Active pulls across every runner."""
    runners: list[AiRunner]
    """The runners."""
    cloud: list[AiProviderCredential]
    """The cloud."""
    cloud_configured_providers: Int64
    """The cloud configured providers."""


class AiRunnerListResponse(OpenRecord):
    """AI RunnerList response."""

    ok: bool
    """The ok."""
    data: AiRunnerListResponseData
    """The data."""


class RunnerCapabilitiesResponse(OpenRecord):
    """RunnerCapabilities response."""

    ok: bool
    """The ok."""
    data: RunnerCapabilitiesData
    """The data."""


class AiStatusResponse(OpenRecord):
    """AI Status response."""

    ok: bool
    """The ok."""
    data: AiStatusData
    """The data."""
