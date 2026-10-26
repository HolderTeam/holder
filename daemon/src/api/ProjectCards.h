#pragma once

#include "api/HttpError.h"
#include "api/RigAnnotations.h"
#include "api/Schemas.h"
#include "api/support/PrivacyErrors.h"

#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace holder::platform { class Db; }
namespace holder::card { class CardStore; }

namespace holder::api {

struct RIG_OPEN_RECORD CardBreadcrumb {
  std::string kind RIG_STRING_VALUES("project", "card");
  std::string id;
  std::string title;
};

struct RIG_OPEN_RECORD CardBreadcrumbsResponse {
  bool ok RIG_BOOL_VALUE(true);
  std::vector<CardBreadcrumb> data RIG_FIELD_DESCRIPTION("Project first, then live ancestor cards from root to the requested card.");
};

struct RIG_OPEN_RECORD HierarchyRepairMove {
  std::string card_id RIG_FIELD_DESCRIPTION("A hidden branch's top card. Its descendants stay attached and move with it.");
  std::optional<std::string> parent_card_id RIG_FIELD_DESCRIPTION("Where it goes, or null for the project's top level.") RIG_NULLABLE;
};

struct RIG_OPEN_RECORD HierarchyRepair {
  bool applied RIG_FIELD_DESCRIPTION("True when changes were written.");
  std::vector<HierarchyRepairMove> moved;
  int changed_card_count RIG_FIELD_DESCRIPTION("Cards whose placement changes, counting siblings re-spaced to make room.") RIG_MINIMUM(0);
  std::vector<std::string> cycle_card_ids RIG_FIELD_DESCRIPTION("Live cards whose parent chain loops. They are left alone and need moving by hand.");
};

struct RIG_OPEN_RECORD HierarchyRepairResponse {
  bool ok RIG_BOOL_VALUE(true);
  HierarchyRepair data;
};

struct RIG_OPEN_RECORD HierarchyRepairRequest {
  bool apply RIG_FIELD_DESCRIPTION("Apply the repair. Without it, only report.") RIG_DEFAULT(false);
};

struct ProjectCardsNotFound : std::runtime_error {
  using std::runtime_error::runtime_error;
};

struct CardHierarchyInvalid : CodedError {
  explicit CardHierarchyInvalid(const std::string& message)
      : CodedError("hierarchy_invalid", message) {}
};


RIG_ROUTE(GET, "/projects/{project_id}/cards/{card_id}/breadcrumbs")
RIG_SUMMARY("Read a live card's project-to-card breadcrumb trail")
RIG_DESCRIPTION("Traverses only the requested card's live ancestors. A missing or trashed card returns 404; a broken or cyclic ancestor chain returns 409 rather than a partial trail.")
RIG_RESPONSE_DESCRIPTION("Project and live card path")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Project or card not found", ProjectCardsNotFound)
RIG_ERROR_RESPONSE(409, ErrorResponse, "Card's ancestor chain is broken or cyclic", CardHierarchyInvalid)
CardBreadcrumbsResponse get_projects_project_id_cards_card_id_breadcrumbs(
    holder::platform::Db& db RIG_CONTEXT,
    std::string project_id,
    std::string card_id);

RIG_ROUTE(POST, "/projects/{project_id}/repair-hierarchy")
RIG_SUMMARY("Find, and optionally fix, cards hidden under missing or trashed parents")
RIG_DESCRIPTION("A live card whose parent is missing or trashed cannot be reached by browsing from the project's top level. This can happen after an older client trashed a parent, a card file was removed by hand, or a sync brought either in. Without apply, this only reports, reading the database alone (so it works on a locked encrypted project). With apply, each hidden branch moves to its nearest live, reachable ancestor (or the top level), into the place its trashed ancestor held, with its descendants still attached, in one commit. Applying twice changes nothing the second time. Cards in parent cycles are reported and left alone. The daemon also repairs at startup and core repairs after every pull.")
RIG_RESPONSE_DESCRIPTION("The plan, and whether it was applied")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Invalid body, or a privacy error applying to an encrypted project", support::PrivacyRejected)
RIG_ERROR_RESPONSE(404, ErrorResponse, "Project not found", ProjectCardsNotFound)
RIG_ERROR_RESPONSE(501, ErrorResponse, "The daemon has no card store", CardStoreUnavailable)
RIG_ERROR_RESPONSE(503, ErrorResponse, "The keyring needed to write an encrypted project is unavailable", support::KeyringUnavailable)
HierarchyRepairResponse post_projects_project_id_repair_hierarchy(
    holder::platform::Db& db RIG_CONTEXT,
    holder::card::CardStore* card_store RIG_CONTEXT,
    std::string project_id,
    std::optional<HierarchyRepairRequest> request);

} // namespace holder::api
