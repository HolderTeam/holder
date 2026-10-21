#pragma once

#include "api/ApiService.h"
#include "api/HttpError.h"
#include "api/RigAnnotations.h"
#include "api/declared/Schemas.h"

#include <optional>
#include <string>

namespace holder::api {

// Declared for the contract; the existing route handlers still write these responses.

RIG_ROUTE(GET, "/projects")
RIG_SUMMARY("List projects")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
ProjectListResponse get_projects(
    std::optional<std::string> name RIG_QUERY RIG_PARAMETER_DESCRIPTION("Substring match on project name."),
    std::optional<long long> updated_after RIG_QUERY RIG_PARAMETER_DESCRIPTION("Return projects with updated_at >= updated_after."),
    std::optional<long long> updated_before RIG_QUERY RIG_PARAMETER_DESCRIPTION("Return projects with updated_at <= updated_before."),
    std::optional<std::string> order RIG_QUERY RIG_STRING_VALUES("updated_at_desc", "updated_at_asc", "created_at_desc", "created_at_asc", "name_desc", "name_asc") RIG_PARAMETER_DESCRIPTION("Order by updated_at (default), created_at, or name."),
    std::optional<int> limit RIG_QUERY RIG_RANGE(0, 1000) RIG_FORMAT("int32") RIG_PARAMETER_DESCRIPTION("Max results (default 100, capped at 1000)."),
    std::optional<int> offset RIG_QUERY RIG_MINIMUM(0) RIG_FORMAT("int32") RIG_PARAMETER_DESCRIPTION("Offset into ordered results (default 0)."),
    bool count RIG_QUERY RIG_DEFAULT(false) RIG_PARAMETER_DESCRIPTION("Include count fields (card_count/root_card_count) per project."));

RIG_ROUTE(POST, "/projects")
RIG_SUMMARY("Create a project")
RIG_RESPONSE_DESCRIPTION("Created")
RIG_HOST_RESPONSE("application/json")
RIG_SUCCESS_STATUS(201)
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request")
ProjectCreateResponse post_projects(ProjectCreateRequest request);

RIG_ROUTE(GET, "/projects/{project_id}")
RIG_SUMMARY("Get a project")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found")
ProjectGetResponse get_projects_project_id(std::string project_id);

RIG_ROUTE(PATCH, "/projects/{project_id}")
RIG_SUMMARY("Update a project")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found")
ProjectUpdateResponse patch_projects_project_id(
    std::string project_id,
    ProjectUpdateRequest request);

RIG_ROUTE(DELETE, "/projects/{project_id}")
RIG_SUMMARY("Delete a project")
RIG_DESCRIPTION("Removes the project from this device's database and records its removal so startup discovery and database reconstruction do not automatically import it again. Project files and encryption keys are retained for explicit recovery. Creating or importing the project explicitly registers it again.")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(500, ErrorResponse, "Could not persist the project removal; the database deletion is rolled back.")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found")
ProjectDeleteResponse delete_projects_project_id(std::string project_id);

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

RIG_ROUTE(GET, "/projects/{project_id}/milestones-with-dates")
RIG_SUMMARY("Query project milestones by a labelled date role")
RIG_DESCRIPTION("Returns each matching milestone once with its complete ordered dates array. Calendar days and instants are queried separately; both range bounds are inclusive. Milestones on trashed cards are excluded. Roles are free-form, and this endpoint does not infer outstanding or completion state.")
RIG_RESPONSE_DESCRIPTION("Matching milestones")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Invalid role or range")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Project not found")
MilestoneWithDatesListResponse get_projects_project_id_milestones_with_dates(
    std::string project_id,
    std::string role RIG_QUERY RIG_MIN_LENGTH(1),
    std::string value_type RIG_QUERY RIG_STRING_VALUES("date", "datetime"),
    std::string from RIG_QUERY RIG_PARAMETER_DESCRIPTION("YYYY-MM-DD when value_type=date; Unix epoch seconds when value_type=datetime."),
    std::string to RIG_QUERY RIG_PARAMETER_DESCRIPTION("Inclusive upper bound in the same form as from."));

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
