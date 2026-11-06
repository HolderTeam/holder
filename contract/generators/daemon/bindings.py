"""Daemon runtime bindings for operations implemented outside core.

The contract declares each operation's public shape. These bindings identify the daemon
services its implementation receives; they are validated against the extracted contract.
"""

from __future__ import annotations

from typing import Literal

from pydantic import BaseModel, ConfigDict

Service = Literal["db", "fts", "registry", "uuid_v4", "secret_store", "events", "socket"]


class RuntimeBinding(BaseModel):
    model_config = ConfigDict(extra="forbid", frozen=True)

    services: tuple[Service, ...]
    argument_order: tuple[str, ...] | None = None


# Operations with the same runtime dependencies share a binding declaration. The
# operation names are checked against the contract before any route is emitted.
GROUPS = {
    ("db", "fts"): "get_ai_messages get_ai_message get_ai_message_links get_ai_message_backlinks "
                   "patch_ai_messages_message_id delete_ai_messages_message_id "
                   "post_ai_messages_message_id_restore post_ai_messages_message_id_links "
                   "delete_ai_messages_message_id_links",
    ("db", "fts", "uuid_v4"): "post_ai_messages post_ai_messages_capture",
    ("db", "registry"): "get_ai_nudges post_ai_nudges_evaluate get_ai_capabilities get_ai_status "
                        "patch_ai_runners_runner_id delete_ai_runners_runner_id "
                        "post_ai_nudges_nudge_id_dismiss",
    ("registry",): "get_ai_runners get_ai_runner post_ai_runner_pull get_pull_status "
                   "post_ai_runners_runner_id_retry",
    ("db", "registry", "uuid_v4"): "post_ai_runners",
    ("db",): "get_ai_runs get_ai_threads list_provider_credentials get_local_model_config "
             "put_local_model_config get_ai_provider_catalog list_provider_settings "
             "set_provider_setting delete_provider_setting get_ai_run get_ai_thread "
             "patch_ai_threads_thread_id delete_ai_threads_thread_id",
    ("db", "uuid_v4"): "post_ai_threads",
    ("db", "secret_store"): "upsert_provider_credential delete_provider_credential",
    ("registry", "events"): "get_ai_runner_pull_job_id_events",
    ("db", "events"): "get_ai_runs_run_id_events",
    ("socket", "db", "fts", "secret_store", "registry", "uuid_v4", "events"): "post_ai_runs",
}

ARGUMENT_ORDER = {
    "get_ai_message_links": ("message_id", "include_deleted"),
    "get_ai_message_backlinks": ("message_id", "include_deleted"),
}


def bindings() -> dict[str, RuntimeBinding]:
    result = {}
    for services, names in GROUPS.items():
        for name in names.split():
            if name in result:
                raise ValueError(f"duplicate daemon binding for {name}")
            result[name] = RuntimeBinding(services=services, argument_order=ARGUMENT_ORDER.get(name))
    if unknown := ARGUMENT_ORDER.keys() - result.keys():
        raise ValueError(f"argument order without daemon binding: {sorted(unknown)}")
    return result


# Records the daemon declares in holder::api::routes rather than holder::api.
ROUTES_NAMESPACE = frozenset({"CardLink", "CardLinkCreateRequest", "CardLinkCreateResponse", "CardLinkListResponse"})

# The daemon's AI exception classes, in holder::api, by the HTTP status an AI route answers with.
# std::invalid_argument is 400 and holder::model::ConflictError 409 before these; anything else is 500.
ERROR_STATUSES = {
    400: ("AiProviderCatalogMissing", "AiRunsMissingScope", "AiMessageWrongState"),
    404: ("AiCaptureNotFound", "AiMessageNotFound", "AiNudgeNotFound", "AiRunNotFound", "AiRunnerNotConfigured",
          "AiRunnerNotFound", "AiThreadNotFound", "PullJobNotFound", "RunnerNotConfigured"),
    503: ("AiRunUnavailable", "ProviderCredentialSecretFailure", "RunnerUnavailable", "SecretStoreUnavailable"),
}


def cpp_name(record: str) -> str:
    """A daemon record's qualified C++ name."""
    return f"{'holder::api::routes' if record in ROUTES_NAMESPACE else 'holder::api'}::{record}"
