#include "sync/PullConflictResolution.h"

#include "card/CardFrontMatter.h"
#include "card/CardStore.h"
#include "identity/Uuid.h"
#include "model/Card.h"
#include "privacy/ProjectPrivacy.h"
#include "project/ProjectRepo.h"
#include "project/Rebuilder.h"

#include <map>
#include <set>
#include <stdexcept>
#include <vector>

namespace holder::sync {

void reconcile_index_after_pull(
    holder::platform::Db& db,
    holder::index::FtsIndexer* fts,
    const holder::model::Project& project
) {
  holder::store::Rebuilder(db, fts).rebuild_project(project);
}

int resolve_pull_conflicts(
    holder::platform::Db& db,
    holder::index::FtsIndexer* fts,
    const holder::model::Project& project,
    holder::git::GitOps& git,
    const holder::git::NonFastForwardPullError& diverged,
    long long now
) {
  const auto current_project = holder::project::ProjectRepo(db).get(project.project_id);
  if (!current_project.has_value()) {
    throw std::runtime_error("project not found: " + project.project_id);
  }

  const auto merge_result = git.merge_remote_taking_theirs_for_conflicts(
      "origin",
      diverged.local_oid_hex,
      diverged.remote_oid_hex
  );

  struct PendingCopy {
    std::string path;
    holder::core::ParsedCardFile snapshot;
    std::string new_card_id;
  };
  std::vector<PendingCopy> pending;
  std::map<std::string, std::string> copied_card_ids;
  for (const auto& path : merge_result.conflicted_paths) {
    const auto blob = git.read_blob_at(diverged.local_oid_hex, path);
    if (!blob.has_value()) continue;

    std::string plain;
    try {
      plain = project.privacy_mode == "encrypted_git" && project.project_key_id.has_value()
                  ? holder::privacy::decrypt_project_blob(
                        project.project_id,
                        *project.project_key_id,
                        *blob
                    )
                  : *blob;
    } catch (const std::exception&) {
      continue;
    }

    const auto parsed = holder::core::parse_card_file(plain);
    if (!parsed.has_front_matter || parsed.card.card_id.empty() ||
        parsed.card.project_id != project.project_id)
      continue;

    const auto new_card_id = holder::identity::generate_id(current_project->id_scheme);
    if (!copied_card_ids.emplace(parsed.card.card_id, new_card_id).second)
      throw std::runtime_error("duplicate conflicted card ID: " + parsed.card.card_id);
    pending.push_back({path, parsed, new_card_id});
  }

  holder::card::CardStore store(db, fts);
  int resolved = 0;
  std::set<std::string> created_card_ids;
  while (created_card_ids.size() < pending.size()) {
    bool made_progress = false;
    for (const auto& copy : pending) {
      if (created_card_ids.contains(copy.new_card_id)) continue;
      const auto& parsed = copy.snapshot;
      auto parent_card_id = parsed.card.parent_card_id;
      if (parent_card_id) {
        const auto copied_parent = copied_card_ids.find(*parent_card_id);
        if (copied_parent != copied_card_ids.end()) {
          parent_card_id = copied_parent->second;
          if (!created_card_ids.contains(*parent_card_id)) continue;
        }
      }

      holder::model::Card duplicate;
      duplicate.card_id = copy.new_card_id;
      duplicate.project_id = project.project_id;
      duplicate.title = parsed.card.title + " (conflicted copy)";
      duplicate.parent_card_id = parent_card_id;
      duplicate.created_at = now;
      duplicate.updated_at = now;

      auto links = parsed.links;
      for (auto& link : links) {
        link.project_id = project.project_id;
        link.from_card_id = duplicate.card_id;
        if (link.to_type == "card" &&
            (!link.target_project_id || *link.target_project_id == project.project_id)) {
          const auto copied_target = copied_card_ids.find(link.to_card_id);
          if (copied_target != copied_card_ids.end()) link.to_card_id = copied_target->second;
        }
      }

      auto milestones = parsed.milestones;
      for (auto& milestone : milestones) {
        milestone.milestone_id = holder::identity::uuid_v4();
        milestone.project_id = project.project_id;
        milestone.card_id = duplicate.card_id;
      }

      try {
        store.create(duplicate, parsed.body, std::nullopt, links, milestones);
        resolved++;
        created_card_ids.insert(duplicate.card_id);
        made_progress = true;
      } catch (const std::exception& ex) {
        throw std::runtime_error(
            "failed to preserve conflicted card " + copy.path + " from commit " +
            diverged.local_oid_hex + ": " + ex.what()
        );
      }
    }
    if (!made_progress)
      throw std::runtime_error(
          "cannot preserve conflicted card parent cycle from commit " + diverged.local_oid_hex
      );
  }
  return resolved;
}

} // namespace holder::sync
