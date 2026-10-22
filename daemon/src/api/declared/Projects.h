#pragma once

#include "api/ApiService.h"
#include "api/HttpError.h"
#include "api/RigAnnotations.h"
#include "api/declared/Schemas.h"

#include <optional>
#include <string>

namespace holder::api {

// Declared for the contract; the existing route handlers still write these responses.

RIG_ROUTE(POST, "/projects/{project_id}/repair-hierarchy")
RIG_SUMMARY("Find, and optionally fix, cards hidden under missing or trashed parents")
RIG_DESCRIPTION("A live card whose parent is missing or trashed cannot be reached by browsing from the project's top level. This can happen after an older client trashed a parent, a card file was removed by hand, or a sync brought either in. Without apply, this only reports, reading the database alone (so it works on a locked encrypted project). With apply, each hidden branch moves to its nearest live, reachable ancestor (or the top level), into the place its trashed ancestor held, with its descendants still attached, in one commit. Applying twice changes nothing the second time. Cards in parent cycles are reported and left alone. The daemon also repairs at startup and core repairs after every pull.")
RIG_RESPONSE_DESCRIPTION("The plan, and whether it was applied")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Invalid body, or a privacy error applying to an encrypted project")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Project not found")
RIG_ERROR_RESPONSE(501, ErrorResponse, "The daemon has no card store")
RIG_ERROR_RESPONSE(503, ErrorResponse, "The keyring needed to write an encrypted project is unavailable")
HierarchyRepairResponse post_projects_project_id_repair_hierarchy(
    std::string project_id,
    std::optional<HierarchyRepairRequest> request);

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

} // namespace holder::api
