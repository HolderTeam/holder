"""HTTP schema suite scope and documented exceptions."""

STREAM_EXCLUSIONS = {
    "stream_changes": "A successful change feed stays open. The native replay test exercises the stream over HTTP.",
}

SSE_SERVICE_ERRORS = {
    "post_ai_runner_pull",
    "post_ai_runs",
    "get_ai_runs_run_id_events",
    "get_ai_runner_pull_job_id_events",
}


def schema_routes(bindings):
    return [binding for binding in bindings if binding["operation"] not in STREAM_EXCLUSIONS]


def positive_acceptance_limits(operation, path, query, params, body):
    """Reasons a schema-valid request may still fail against a disposable daemon."""
    if path.startswith("/ai/"):
        yield "Requires configured providers/runners or seeded project, thread, message or run state."
    if "cursor" in query:
        yield "A cursor must have been issued by the daemon for the query."
    if "tag" in query or "tag" in params:
        yield "Tags must follow the contract's textual tag grammar."
    if "include_content" in query and "limit" in query:
        yield "Listings with content are limited to 1000 cards."
    if {"first_day", "last_day"} & query or "dates" in body:
        yield "Calendar day/instant formats and the milestone date-or-instant rule apply."
    if {"oid", "to_oid", "from_oid"} & query:
        yield "Revisions must resolve in the project's Git history."
    if "resource_id" in params or {"resource_id", "metadata"} & body:
        yield "Resource IDs and metadata names must follow the declared domain rules."
    if "recovery_token" in body:
        yield "A recovery token must have a valid format and recovery prerequisites."
    if path in {"/search/cards", "/search/ai"} and "q" in query:
        yield "The query must be valid SQLite FTS5 syntax."
    if operation == "start_asset_import":
        yield "The source must be an existing readable regular file on the daemon's host."
    if operation == "compare_conflict_copy":
        yield "Requires a live original and copy with a recorded conflict association."
    if operation in {"authorize_google_drive_location", "finish_google_drive_location"}:
        yield "Requires Google Drive provider configuration and daemon-issued OAuth state."
