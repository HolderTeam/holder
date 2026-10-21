#pragma once

#include "api/ApiService.h"
#include "api/HttpError.h"
#include "api/RigAnnotations.h"
#include "api/declared/Schemas.h"

#include <optional>
#include <string>
#include <variant>

namespace holder::api {

// Declared for the contract; the existing route handlers still write these responses.

RIG_ROUTE(POST, "/projects/{project_id}/repair-hierarchy")
RIG_SUMMARY("Find, and optionally fix, cards hidden under missing or trashed parents")
RIG_DESCRIPTION("A live card whose parent is missing or trashed cannot be reached by browsing from the project's top level. This can happen after an older client trashed a parent, a card file was removed by hand, or a sync brought either in. Without apply, this only reports, reading the database alone (so it works on a locked encrypted project). With apply, each hidden branch moves to its nearest live, reachable ancestor (or the top level), into the place its trashed ancestor held, with its descendants still attached, in one commit. Applying twice changes nothing the second time. Cards in parent cycles are reported and left alone. The daemon also repairs at startup and core repairs after every pull.")
RIG_RESPONSE_DESCRIPTION("The plan, and whether it was applied")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Invalid body, or a privacy error applying to an encrypted project")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Project not found")
RIG_ERROR_RESPONSE(405, ErrorResponse, "Method not allowed")
RIG_ERROR_RESPONSE(501, ErrorResponse, "The daemon has no card store")
RIG_ERROR_RESPONSE(503, ErrorResponse, "The keyring needed to write an encrypted project is unavailable")
HierarchyRepairResponse post_projects_project_id_repair_hierarchy(
    std::string project_id,
    std::optional<HierarchyRepairRequest> request);

RIG_ROUTE(GET, "/projects/{project_id}/cards")
RIG_SUMMARY("Page through a project's live cards")
RIG_DESCRIPTION("Filters combine. Each page reads current data; iteration is not a snapshot.")
RIG_RESPONSE_DESCRIPTION("A page of matching cards")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Invalid parameter or cursor, including a cursor from a different query, or a privacy error reading content")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Project not found")
RIG_ERROR_RESPONSE(405, ErrorResponse, "Method not allowed")
RIG_ERROR_RESPONSE(501, ErrorResponse, "Content pages unavailable because the daemon has no card store")
RIG_ERROR_RESPONSE(503, ErrorResponse, "The keyring needed to read an encrypted project's content is unavailable")
RIG_ERROR_RESPONSE(500, ErrorResponse, "Internal server error")
CardPageResponse get_projects_project_id_cards(
    std::string project_id,
    std::optional<std::string> tag RIG_QUERY RIG_MIN_LENGTH(1) RIG_PARAMETER_DESCRIPTION("Exact tag match, case-insensitive, without the leading"),
    std::optional<std::variant<ParentRoots, ParentUuid>> parent RIG_QUERY RIG_PARAMETER_DESCRIPTION("Omit for all hierarchy levels; roots selects root cards; a card ID selects its immediate children."),
    std::string order RIG_QUERY RIG_STRING_VALUES("card_id_asc", "updated_desc", "sort_key_asc") RIG_DEFAULT("card_id_asc") RIG_PARAMETER_DESCRIPTION("updated_desc breaks ties by descending card ID. sort_key_asc follows sibling order and breaks ties by ascending card ID; use it with parent=roots or a parent card ID."),
    int limit RIG_QUERY RIG_RANGE(1, 5000) RIG_DEFAULT(200),
    std::optional<std::string> cursor RIG_QUERY RIG_MIN_LENGTH(1) RIG_MAX_LENGTH(8192) RIG_PARAMETER_DESCRIPTION("Continuation from next_cursor. Keep the project, filters and ordering unchanged."),
    bool include_content RIG_QUERY RIG_DEFAULT(false) RIG_PARAMETER_DESCRIPTION("Include each card's body. The page is selected and every body read under one project lock, and the whole page fails if a body cannot be read (for example a locked encrypted project). limit is at most 1000 when true. The cursor is the same with or without content."),
    bool include_child_count RIG_QUERY RIG_DEFAULT(false) RIG_PARAMETER_DESCRIPTION("Include each returned live card's immediate live child count. The cursor is the same with or without counts."));

RIG_ROUTE(GET, "/projects/{project_id}/cards/{card_id}/breadcrumbs")
RIG_SUMMARY("Read a live card's project-to-card breadcrumb trail")
RIG_DESCRIPTION("Traverses only the requested card's live ancestors. A missing or trashed card returns 404; a broken or cyclic ancestor chain returns 409 rather than a partial trail.")
RIG_RESPONSE_DESCRIPTION("Project and live card path")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Project or card not found")
RIG_ERROR_RESPONSE(405, ErrorResponse, "Method not allowed")
RIG_ERROR_RESPONSE(409, ErrorResponse, "Card's ancestor chain is broken or cyclic")
RIG_ERROR_RESPONSE(500, ErrorResponse, "Internal server error")
CardBreadcrumbsResponse get_projects_project_id_cards_card_id_breadcrumbs(
    std::string project_id,
    std::string card_id);

RIG_ROUTE(GET, "/projects/{project_id}/history")
RIG_SUMMARY("List project activity history")
RIG_DESCRIPTION("Returns bounded, cursor-paginated Git activities for the project. Each activity groups its changed paths by recognised Holder object kind; unknown paths remain visible as unknown.")
RIG_RESPONSE_DESCRIPTION("Project activity history")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Invalid limit, cursor, or object-kind filter")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Project not found")
RIG_ERROR_RESPONSE(413, ErrorResponse, "The bounded History response would exceed 2 MiB")
RIG_ERROR_RESPONSE(503, ErrorResponse, "Project history is temporarily unavailable")
ProjectHistoryPageResponse get_projects_project_id_history(
    std::string project_id,
    int limit RIG_QUERY RIG_RANGE(1, 200) RIG_DEFAULT(50),
    std::optional<std::string> cursor RIG_QUERY,
    std::optional<std::string> kind RIG_QUERY RIG_STRING_VALUES("card", "resource", "location", "ai_data", "project_settings", "unknown") RIG_PARAMETER_DESCRIPTION("Return only activities affecting this Holder object kind."));

RIG_ROUTE(GET, "/projects/{project_id}/history/cards/{card_id}")
RIG_SUMMARY("List saved history for a card")
RIG_DESCRIPTION("Returns commits reachable from the captured project HEAD that changed the card. Adjacent autosaves may be grouped into one entry.")
RIG_RESPONSE_DESCRIPTION("Saved card history")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Invalid card identifier, limit, or cursor")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Project not found")
RIG_ERROR_RESPONSE(409, ErrorResponse, "The encrypted project key is unavailable on this device")
RIG_ERROR_RESPONSE(413, ErrorResponse, "The bounded History response would exceed 2 MiB")
RIG_ERROR_RESPONSE(503, ErrorResponse, "Project history is temporarily unavailable")
CardHistoryPageResponse get_projects_project_id_history_cards_card_id(
    std::string project_id,
    std::string card_id,
    int limit RIG_QUERY RIG_RANGE(1, 200) RIG_DEFAULT(50),
    std::optional<std::string> cursor RIG_QUERY);

RIG_ROUTE(GET, "/projects/{project_id}/history/cards/{card_id}/compare")
RIG_SUMMARY("Compare saved card versions or the change introduced by one revision")
RIG_DESCRIPTION("In since mode, compares the explicitly selected from and to snapshots. In change mode, compares the first parent of to with to; a root commit is compared with a previously nonexistent card. Merge commits use only their first parent, not a combined multi-parent diff.")
RIG_RESPONSE_DESCRIPTION("A line-oriented card comparison")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Invalid or missing comparison parameters")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Project or revision not found")
RIG_ERROR_RESPONSE(409, ErrorResponse, "A revision reference is ambiguous or the encrypted project key is unavailable")
RIG_ERROR_RESPONSE(413, ErrorResponse, "The bounded History response would exceed 2 MiB")
RIG_ERROR_RESPONSE(503, ErrorResponse, "Project history is temporarily unavailable")
CardHistoryComparisonResponse get_projects_project_id_history_cards_card_id_compare(
    std::string project_id,
    std::string card_id,
    std::optional<RevisionReference> from RIG_QUERY RIG_PARAMETER_DESCRIPTION("Required and used in since mode; change mode derives its base from to."),
    RevisionReference to RIG_QUERY RIG_PARAMETER_DESCRIPTION("The ending revision, or the revision whose introduced change is requested."),
    std::string mode RIG_QUERY RIG_STRING_VALUES("since", "change") RIG_DEFAULT("since") RIG_PARAMETER_DESCRIPTION("Since compares two explicit snapshots; change compares to with its first parent."));

RIG_ROUTE(GET, "/projects/{project_id}/history/cards/{card_id}/snapshot")
RIG_SUMMARY("Read a card snapshot at a selected revision")
RIG_RESPONSE_DESCRIPTION("The card snapshot, including an explicit absent state")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Missing revision reference or invalid card identifier")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Project or revision not found")
RIG_ERROR_RESPONSE(409, ErrorResponse, "A revision reference is ambiguous or the encrypted project key is unavailable")
RIG_ERROR_RESPONSE(413, ErrorResponse, "The bounded snapshot response would exceed 2 MiB")
RIG_ERROR_RESPONSE(503, ErrorResponse, "The historical snapshot is temporarily unavailable")
CardHistorySnapshotResponse get_projects_project_id_history_cards_card_id_snapshot(
    std::string project_id,
    std::string card_id,
    RevisionReference oid RIG_QUERY);

RIG_ROUTE(POST, "/projects/{project_id}/history/cards/{card_id}/restore")
RIG_SUMMARY("Restore a card to a saved snapshot")
RIG_DESCRIPTION("Restores the complete card snapshot at the selected commit and records the result as a new commit without rewriting history. Lifecycle rules still apply. Restoring a snapshot in which the card was trashed promotes its current live children, as trashing does. Restoring a live snapshot places the card under a reachable parent, as restoring from Trash does.")
RIG_RESPONSE_DESCRIPTION("Card snapshot restored")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Missing revision reference")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Project, card, or revision not found")
RIG_ERROR_RESPONSE(409, ErrorResponse, "A revision reference is ambiguous or the encrypted project key is unavailable")
RIG_ERROR_RESPONSE(503, ErrorResponse, "The historical snapshot cannot be read or restored")
CardHistoryRestoreResponse post_projects_project_id_history_cards_card_id_restore(
    std::string project_id,
    std::string card_id,
    RevisionReference oid RIG_QUERY RIG_PARAMETER_DESCRIPTION("Revision containing the snapshot to restore."));

RIG_ROUTE(POST, "/projects/{project_id}/git/test-remote")
RIG_SUMMARY("Test project remote reachability/auth")
RIG_DESCRIPTION("Connects and lists remote refs using daemon credentials. Does not fetch, create a project repository, or change stored project configuration or Git configuration.")
RIG_RESPONSE_DESCRIPTION("Probe completed with structured status.")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found")
ProjectGitTestRemoteResponse post_projects_project_id_git_test_remote(
    std::string project_id,
    std::optional<ProjectGitTestRemoteRequest> request);

RIG_ROUTE(POST, "/projects/{project_id}/git/push")
RIG_SUMMARY("Push project repository to remote")
RIG_RESPONSE_DESCRIPTION("Push attempted with structured status.")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found")
ProjectGitPushResponse post_projects_project_id_git_push(
    std::string project_id,
    std::optional<ProjectGitPushRequest> request);

RIG_ROUTE(POST, "/projects/{project_id}/git/pull")
RIG_SUMMARY("Pull and reconcile a project immediately")
RIG_DESCRIPTION("Fetches the configured remote, fast-forwards or applies Holder's card-level conflict resolution, then reconciles the SQLite index before returning. Concurrent Git operations for the same project are serialized.")
RIG_RESPONSE_DESCRIPTION("Pull completed with structured status.")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Project not found")
ProjectGitSyncOperationResponse post_projects_project_id_git_pull(
    std::string project_id,
    std::optional<ProjectGitSyncRequest> request);

RIG_ROUTE(POST, "/projects/{project_id}/git/sync")
RIG_SUMMARY("Pull, reconcile, and push a project immediately")
RIG_DESCRIPTION("Pulls and reconciles first. Push is attempted only when pull succeeds. Concurrent Git operations for the same project are serialized.")
RIG_RESPONSE_DESCRIPTION("Forced sync completed with structured phase statuses.")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Project not found")
ProjectGitSyncOperationResponse post_projects_project_id_git_sync(
    std::string project_id,
    std::optional<ProjectGitSyncRequest> request);

RIG_ROUTE(GET, "/projects/{project_id}/git/sync-status")
RIG_SUMMARY("Get persisted project Git sync status")
RIG_RESPONSE_DESCRIPTION("Current persisted sync state")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Sync status is unavailable")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Project not found")
ProjectGitSyncStatusResponse get_projects_project_id_git_sync_status(std::string project_id);

RIG_ROUTE(POST, "/projects/{project_id}/recovery-token/export")
RIG_SUMMARY("Export project recovery token")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found")
ProjectRecoveryTokenExportResponse post_projects_project_id_recovery_token_export(
    std::string project_id,
    ProjectRecoveryTokenExportRequest request);

RIG_ROUTE(POST, "/projects/{project_id}/recovery-token/import")
RIG_SUMMARY("Import project recovery token")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found")
ProjectRecoveryTokenImportResponse post_projects_project_id_recovery_token_import(
    std::string project_id,
    ProjectRecoveryTokenImportRequest request);

RIG_ROUTE(GET, "/projects/{project_id}/encryption-check")
RIG_SUMMARY("Check encrypted project card blobs before push/sync")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found")
ProjectEncryptionCheckResponse get_projects_project_id_encryption_check(std::string project_id);

RIG_ROUTE(DELETE, "/projects/{project_id}/trash/{type}/{id}")
RIG_SUMMARY("Permanently remove a trashed item from one project")
RIG_DESCRIPTION("The item must belong to the named project and still be in Trash. For cards, older Git revisions remain available; this operation does not erase repository history.")
RIG_RESPONSE_DESCRIPTION("Trashed item removed")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Invalid item type or identifier")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Item not found in the named project")
RIG_ERROR_RESPONSE(409, ErrorResponse, "Item is not in Trash")
RIG_ERROR_RESPONSE(501, ErrorResponse, "Card store unavailable")
TrashDeleteResponse delete_projects_project_id_trash_type_id(
    std::string project_id,
    std::string type RIG_STRING_VALUES("card", "ai_message"),
    std::string id);

} // namespace holder::api
