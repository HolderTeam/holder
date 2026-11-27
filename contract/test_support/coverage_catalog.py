"""Reviewed behavior dispositions. Each operation must appear exactly once."""

# State-machine tests assert both result shape and an independent behavioral model.
STATE_MACHINE = [
    "get_card",
    "get_card_metadata",
    "list_cards",
    "create_card",
    "update_card",
    "trash_card",
    "restore_card",
    "move_card",
    "add_card_tag",
    "remove_card_tag",
    "list_project_tags",
    "list_card_links",
    "list_card_backlinks",
    "add_card_link",
    "remove_card_link",
    "list_card_milestones",
    "add_card_milestone",
    "update_card_milestone",
    "remove_card_milestone",
    "list_milestones_by_day",
    "list_milestones_by_instant",
    "get_calendar",
    "list_resources",
    "get_resource",
    "create_resource",
    "update_resource",
    "delete_resource",
    "list_resource_backlinks",
    "list_card_resources",
    "attach_card_resource",
    "detach_card_resource",
    "list_projects",
    "get_project",
    "create_project",
    "update_project",
    "delete_project",
    "get_project_sync_state",
    "pull_project",
    "push_project",
    "sync_project",
    "test_project_remote",
]

STORAGE_NATIVE = [
    "list_storage_locations",
    "get_storage_location",
    "create_storage_location",
    "update_storage_location",
    "delete_storage_location",
]

# Transport, test file, scenario. These links describe native coverage independently
# of the Python conformance run and do not imply that a native test ran in that job.
NATIVE = {
    "compare_conflict_copy": (
        "C",
        "core/tests/PullConflictResolution_test.cpp",
        "Compare renamed versions by their recorded pair and refuse trashed originals",
    ),
    "resolve_conflict_copy": (
        "C",
        "core/tests/PullConflictResolution_test.cpp",
        "Reject a stale review and preserve the original identity and copy on save",
    ),
    "trash_conflict_copy": (
        "C",
        "core/tests/PullConflictResolution_test.cpp",
        "Review copy-only metadata, reject stale cleanup and restore the trashed copy",
    ),
    "separate_conflict_copy": (
        "C",
        "core/tests/PullConflictResolution_test.cpp",
        "Reject a stale review, then keep the copy as its own card under a new title",
    ),
    "register_ssh_signer": (
        "C",
        "core/tests/CApiLegacyHostResource_test.cpp",
        "SSH signer validation, replacement and callback ownership",
    ),
    **{
        name: ("C", "core/tests/CApiLocations_test.cpp", "Portable declaration CRUD and provider validation")
        for name in STORAGE_NATIVE
    },
    **{
        name: (
            "HTTP",
            "daemon/tests/AiResourceRoutes_test.cpp",
            "Location binding, preference and declaration lifecycle",
        )
        for name in ("get_locations", "delete_location_binding", "test_location", "put_preferred_location")
    },
    "get_event_cursor": ("HTTP", "daemon/tests/HttpEvents_test.cpp", "Checkpoint and restart recovery"),
    "stream_changes": ("HTTP", "daemon/tests/HttpEvents_test.cpp", "Replay, authorization and restart/resync"),
    "post_ai_runs": ("HTTP", "daemon/tests/HttpAiRunEvents_test.cpp", "Failures before and after streaming begins"),
    "get_ai_runs_run_id_events": (
        "HTTP",
        "daemon/tests/HttpAiRunEvents_test.cpp",
        "Terminal events, reconnect and missing runs",
    ),
    **{
        name: ("HTTP", "daemon/tests/HttpAiRunnerPullRoutes_test.cpp", "Pull start and job lookup")
        for name in ("post_ai_runner_pull", "get_pull_status")
    },
}

GAPS = {
    "Add live ABI evidence for initialization, teardown, errors or host callback ownership as applicable.": """
version register_time_zone_provider register_keyring_provider register_storage_provider configure_git_home
context_open context_destroy error_message error_destroy
""",
    "Exercise rebuild and repair on a damaged or stale index while checking durable project data.": """
database_rebuild repair_project_hierarchy rebuild_project reindex_search
""",
    "Exercise valid device state, unavailable keys, and retry after interrupted recovery.": """
check_project_encryption read_recovery_snapshot restore_recovery_snapshot restore_recovery_bundle download_recovery_snapshot export_project_recovery_token
import_project_recovery_token import_recovery_token_global
""",
    "Exercise provider authorization and callback state with a controlled provider fixture.": """
authorize_google_drive_location finish_google_drive_location
""",
    "Exercise hierarchy or reference resolution across projects and Trash.": """
get_card_any_state get_card_breadcrumbs resolve_card_reference list_link_kinds
""",
    "Exercise local file or provider fixtures and check durable bytes, job outcomes and reported failures.": """
import_asset_file retrieve_asset_file start_asset_import get_asset_import_job get_resource_asset_content
""",
    "Exercise first startup and importing an existing project across reopen.": """
ensure_home_project import_project
""",
    "Exercise due and deferred sync phases, retry and resulting state using a disposable Git remote.": """
sync_project_if_due
""",
    "Exercise revision lookup, comparisons and restoration against independently observed Git history.": """
list_project_history list_card_history compare_card_history get_card_history_snapshot restore_card_history_snapshot
""",
    "Exercise deletion scope and durable results across reopen.": """
list_trash empty_trash delete_trash_item delete_project_trash_item
""",
    "Exercise known search matches and indexing after edits or deletes.": """
search_cards search_ai_messages
""",
    "Verify process readiness, health state and shutdown through a running daemon.": """
get_alive get_ping get_health post_bye
""",
    "Verify served content and missing files under a controlled static document root.": """
get_openapi_yaml get_openapi_json get_ai_catalog_yaml get_ai_catalog_json get_git_providers_yaml get_git_providers_json
get_docs get_docs_file
""",
    "Exercise configured providers and verify credential redaction, failure and persisted settings as applicable.": """
list_provider_credentials upsert_provider_credential delete_provider_credential get_local_model_config
put_local_model_config get_ai_provider_catalog list_provider_settings set_provider_setting delete_provider_setting
""",
    "Exercise seeded project/thread/message state and assert changes and scope independently.": """
get_ai_messages get_ai_message get_ai_message_links get_ai_message_backlinks post_ai_messages
patch_ai_messages_message_id delete_ai_messages_message_id post_ai_messages_message_id_restore
post_ai_messages_message_id_links delete_ai_messages_message_id_links post_ai_messages_capture
get_ai_threads get_ai_thread post_ai_threads patch_ai_threads_thread_id delete_ai_threads_thread_id
""",
    "Exercise seeded findings and verify evaluation and dismissal behavior.": """
get_ai_nudges post_ai_nudges_nudge_id_dismiss post_ai_nudges_evaluate
""",
    "Exercise configured and unavailable runners with an independent registry fixture.": """
get_ai_runners get_ai_runner post_ai_runners patch_ai_runners_runner_id delete_ai_runners_runner_id
post_ai_runners_runner_id_retry get_ai_capabilities get_ai_status
""",
    "Exercise seeded runs and verify project/thread scope and persisted results.": """
get_ai_runs get_ai_run
""",
    "Exercise a running pull job through SSE, including terminal delivery and resume.": """
get_ai_runner_pull_job_id_events
""",
}

# Explicit exceptions for claims that cover only one of an operation's transports.
OTHER_TRANSPORT_GAPS = {
    "compare_conflict_copy": "Exercise the comparison with a seeded conflict through live HTTP conformance.",
    "resolve_conflict_copy": "Exercise a stale review and successful save on a seeded conflict over HTTP.",
    "trash_conflict_copy": "Exercise guarded cleanup of a seeded conflict through live HTTP conformance.",
    "separate_conflict_copy": "Exercise keeping a seeded conflict copy as its own card over HTTP.",
    **{
        name: "Exercise portable declaration CRUD through the live HTTP interface."
        for name in STORAGE_NATIVE
        if name != "list_storage_locations"
    },
}

BEHAVIOR_LIMITS = {
    **{
        name: "The state machine exercises an unset remote. Add successful and conflicting sync with a local remote."
        for name in ("pull_project", "push_project", "sync_project", "get_project_sync_state", "test_project_remote")
    },
    "stream_changes": "Add systematic validation of each named SSE payload against its declared schema.",
    "post_ai_runs": "Add systematic named-event checks with a configured provider and successful run.",
    "get_ai_runs_run_id_events": "Add systematic validation of each named SSE payload against its declared schema.",
}
