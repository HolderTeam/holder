#include "api/Contract.h"

#include "api/CardAccess.h"
#include "api/CoreErrors.h"
#include "capi/Context.h"
#include "card/CardStore.h"
#include "privacy/ProjectPrivacy.h"
#include "project/ProjectRepo.h"

#include <optional>
#include <string>
#include <utility>

namespace holder::contract {

ProjectEncryptionCheck check_project_encryption(holder_context& context, std::string project_id) {
  const auto project = holder::project::ProjectRepo(context.db).get(project_id);
  if (!project) throw NotFound("Project not found.");

  ProjectEncryptionCheck result;
  result.project_id = std::move(project_id);
  if (project->privacy_mode == "plain") {
    result.privacy_mode = ProjectEncryptionCheckPrivacyMode::Plain;
    result.check = {true, 0, 0, {}, "Project is plain mode; privacy check not required."};
    return result;
  }
  if (project->privacy_mode != "encrypted_git") throw Error("Unknown project privacy mode.");
  result.privacy_mode = ProjectEncryptionCheckPrivacyMode::EncryptedGit;
  auto checked = holder::privacy::run_encryption_safety_check(project->root_path);
  result.check = {checked.ok, static_cast<std::int32_t>(checked.checked_files),
                  static_cast<std::int32_t>(checked.unsafe_paths.size()),
                  std::move(checked.unsafe_paths), std::move(checked.message)};
  return result;
}

HierarchyRepair repair_project_hierarchy(holder_context& context, std::string project_id,
                                         const HierarchyRepairRequest& request) {
  if (!holder::project::ProjectRepo(context.db).get(project_id))
    throw NotFound("Project not found.");

  holder::card::CardStore cards(context.db, &context.fts, nullptr, context.git);
  const auto plan = with_contract_errors([&] {
    return request.apply ? cards.repair_hierarchy(project_id, now_epoch_seconds())
                         : cards.plan_hierarchy_repair(project_id);
  });
  HierarchyRepair result;
  result.applied = request.apply && !plan.placements.empty();
  result.changed_card_count = static_cast<std::int32_t>(plan.placements.size());
  result.cycle_card_ids = plan.cycle_card_ids;
  for (const auto& card_id : plan.moved_card_ids) {
    std::optional<std::string> parent;
    for (const auto& placement : plan.placements) {
      if (placement.card_id == card_id && placement.parent_card_id)
        parent = *placement.parent_card_id;
    }
    result.moved.push_back({card_id, std::move(parent)});
  }
  return result;
}

} // namespace holder::contract
