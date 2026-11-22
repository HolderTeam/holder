#include "sync/PullConflictResolution.h"

#include "card/CardFrontMatter.h"
#include "card/CardPaths.h"
#include "card/CardStore.h"
#include "identity/Uuid.h"
#include "model/Card.h"
#include "model/CardLink.h"
#include "privacy/ProjectPrivacy.h"
#include "project/ProjectManifest.h"
#include "project/ProjectRepo.h"
#include "project/Rebuilder.h"
#include "sync/CardMerge.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <vector>

namespace holder::sync {
namespace {

holder::model::IdScheme recovery_manifest_scheme(const holder::model::Project& project) {
  const auto durable = holder::project::read_project_manifest(project.root_path);
  if (durable.project_id != project.project_id || durable.privacy_mode != project.privacy_mode ||
      durable.project_key_id != project.project_key_id) {
    throw std::runtime_error("pulled project metadata does not match the recovery token");
  }
  return durable.id_scheme;
}

} // namespace

void reconcile_index_after_pull(
    holder::platform::Db& db,
    holder::index::FtsIndexer* fts,
    const holder::model::Project& project
) {
  const auto root = std::filesystem::path(project.root_path);
  if (std::filesystem::exists(root / holder::project::kPendingRecoveryPath) &&
      std::filesystem::exists(root / holder::project::kProjectManifestPath)) {
    (void)recovery_manifest_scheme(project);
  }
  holder::store::Rebuilder(db, fts).rebuild_project(project);
}

void complete_recovery_manifest(
    holder::platform::Db& db,
    holder::git::GitOps& git,
    const holder::model::Project& project,
    long long now
) {
  const auto root = std::filesystem::path(project.root_path);
  const auto pending = root / holder::project::kPendingRecoveryPath;
  if (!std::filesystem::exists(pending)) return;
  const auto manifest = root / holder::project::kProjectManifestPath;
  // A manifest fetched from the remote owns the durable settings. Rewriting it from
  // the local projection could reintroduce a name or remote URL from an older token.
  if (!std::filesystem::exists(manifest)) {
    holder::project::write_project_manifest(git, project);
    git.commit("Restore encrypted project metadata");
    (void)recovery_manifest_scheme(project);
  } else if (const auto scheme = recovery_manifest_scheme(project); scheme != project.id_scheme) {
    holder::project::ProjectRepo(db).update_id_scheme(project.project_id, scheme, now);
  }
  std::filesystem::remove(pending);
}

int resolve_pull_conflicts(
    holder::platform::Db& db,
    holder::index::FtsIndexer* fts,
    const holder::model::Project& project,
    holder::git::GitOps& git,
    const holder::git::NonFastForwardPullError& diverged,
    long long now,
    int* preserved_count,
    holder::model::PullChanges* changes
) {
  const auto note = [&](std::vector<std::string> holder::model::PullChanges::*list,
                        const std::string& card_id) {
    if (changes != nullptr) (changes->*list).push_back(card_id);
  };
  const auto current_project = holder::project::ProjectRepo(db).get(project.project_id);
  if (!current_project.has_value()) {
    throw std::runtime_error("project not found: " + project.project_id);
  }

  const auto merge_result = git.merge_remote_taking_theirs_for_conflicts(
      "origin",
      diverged.local_oid_hex,
      diverged.remote_oid_hex
  );

  const bool encrypted = project.privacy_mode == "encrypted_git" &&
                         project.project_key_id.has_value();
  // A version that cannot be read or decrypted is treated as missing: the card keeps today's
  // behaviour instead of being merged.
  const auto read_version = [&](const std::string& commit, const std::string& path
                            ) -> std::optional<holder::core::ParsedCardFile> {
    if (commit.empty()) return std::nullopt;
    const auto blob = git.read_blob_at(commit, path);
    if (!blob.has_value()) return std::nullopt;
    try {
      const auto plain = encrypted ? holder::privacy::decrypt_project_blob(
                                         project.project_id,
                                         *project.project_key_id,
                                         *blob
                                     )
                                   : *blob;
      auto parsed = holder::core::parse_card_file(plain);
      if (!parsed.has_front_matter) return std::nullopt;
      return parsed;
    } catch (const std::exception&) {
      return std::nullopt;
    }
  };
  const auto render = [](const holder::core::ParsedCardFile& card) {
    return holder::core::render_card_front_matter(card.card, card.links, card.milestones) +
           card.body;
  };

  // This device moved a card to Trash while the other device edited it. The merge kept the
  // other device's edited file at the live path and also wrote this device's copy in Trash,
  // leaving the card twice. Keep the edited card live and drop the Trash move, so nothing
  // written on the other device is lost; the card can be moved to Trash again.
  std::vector<std::string> kept_live_paths;
  for (const auto& path : merge_result.conflicted_paths) {
    const auto id = std::filesystem::path(path).stem().string();
    if (!holder::identity::is_valid_uuid(id) || path != holder::core::card_rel_path(id)) continue;
    const auto trash_path = holder::core::card_trash_rel_path(id);
    if (git.read_blob_at(diverged.local_oid_hex, path).has_value() ||
        !git.read_blob_at(diverged.remote_oid_hex, path).has_value() ||
        !git.read_blob_at(diverged.local_oid_hex, trash_path).has_value() ||
        git.read_blob_at(diverged.remote_oid_hex, trash_path).has_value())
      continue;
    const auto trash_file = git.repo_dir() / trash_path;
    if (!std::filesystem::exists(trash_file)) continue;
    std::filesystem::remove(trash_file);
    git.remove_path(trash_path);
    kept_live_paths.push_back(path);
    note(&holder::model::PullChanges::kept_out_of_trash_card_ids, id);
  }
  if (!kept_live_paths.empty()) {
    git.commit(
        kept_live_paths.size() == 1 ? "Keep 1 card edited on another device out of Trash"
                                    : "Keep " + std::to_string(kept_live_paths.size()) +
                                          " cards edited on another device out of Trash"
    );
  }

  // The other device moved a card to Trash while this device edited it. The merge kept the
  // other device's side, so the card sits in Trash and the edit is only in this device's
  // history. Take the card back out of Trash with the edit, as Restore would, so nothing
  // written on this device is lost; it can be moved to Trash again. The two versions are
  // combined field by field, so a change the other device made before moving the card is kept
  // too. When those changes clash, the card stays in Trash and this device's version becomes a
  // conflict copy instead.
  struct Restoration {
    std::string path;
    std::string trash_path;
    holder::core::ParsedCardFile card;
  };
  std::vector<Restoration> restorations;
  std::set<std::string> handled_paths;
  for (const auto& path : merge_result.conflicted_paths) {
    const auto id = std::filesystem::path(path).stem().string();
    if (!holder::identity::is_valid_uuid(id) || path != holder::core::card_rel_path(id)) continue;
    const auto trash_path = holder::core::card_trash_rel_path(id);
    if (!git.read_blob_at(diverged.local_oid_hex, path).has_value() ||
        git.read_blob_at(diverged.remote_oid_hex, path).has_value() ||
        git.read_blob_at(diverged.local_oid_hex, trash_path).has_value() ||
        !std::filesystem::exists(git.repo_dir() / trash_path))
      continue;
    const auto base = read_version(merge_result.merge_base_oid_hex, path);
    const auto local = read_version(diverged.local_oid_hex, path);
    const auto trashed = read_version(diverged.remote_oid_hex, trash_path);
    if (!base || !local || !trashed || local->card.card_id != id || base->card.deleted_at ||
        local->card.deleted_at || !trashed->card.deleted_at)
      continue;
    auto remote = *trashed;
    remote.card.deleted_at.reset();
    remote.card.rel_path = path;
    auto merged = merge_card_versions(*base, *local, remote);
    if (!merged) continue;
    merged->card.updated_at = std::max(merged->card.updated_at, now);
    restorations.push_back({path, trash_path, std::move(*merged)});
    handled_paths.insert(path);
  }
  if (!restorations.empty()) {
    std::map<std::string, const holder::core::ParsedCardFile*> restored_by_id;
    for (const auto& restoration : restorations)
      restored_by_id.emplace(restoration.card.card.card_id, &restoration.card);
    // A card as it now stands in the working tree, live or in Trash; nullopt when missing.
    const auto current_card = [&](const std::string& card_id
                              ) -> std::optional<holder::core::ParsedCardFile> {
      if (const auto restored = restored_by_id.find(card_id); restored != restored_by_id.end())
        return *restored->second;
      for (const auto& rel :
           {holder::core::card_rel_path(card_id), holder::core::card_trash_rel_path(card_id)}) {
        std::ifstream input(git.repo_dir() / rel, std::ios::binary);
        if (!input.is_open()) continue;
        const std::string raw{
            std::istreambuf_iterator<char>(input),
            std::istreambuf_iterator<char>()
        };
        try {
          auto parsed = holder::core::parse_card_file(
              encrypted ? holder::privacy::decrypt_project_blob(
                              project.project_id,
                              *project.project_key_id,
                              raw
                          )
                        : raw
          );
          if (parsed.has_front_matter && parsed.card.card_id == card_id) return parsed;
        } catch (const std::exception&) {
        }
        return std::nullopt;
      }
      return std::nullopt;
    };
    // Like Restore: the card goes under its nearest ancestor that is not in Trash, or to the top
    // level, since the other device may have moved its parent to Trash too.
    const auto reachable_parent = [&](const holder::core::ParsedCardFile& card) {
      std::set<std::string> visited{card.card.card_id};
      std::optional<std::string> candidate;
      auto next = card.card.parent_card_id;
      while (next) {
        if (!visited.insert(*next).second) return std::optional<std::string>{};
        const auto ancestor = current_card(*next);
        if (!ancestor) return std::optional<std::string>{};
        if (ancestor->card.deleted_at) {
          candidate.reset();
        } else if (!candidate) {
          candidate = ancestor->card.card_id;
        }
        next = ancestor->card.parent_card_id;
      }
      return candidate;
    };
    std::vector<std::string> restored_paths;
    for (auto& restoration : restorations) {
      restoration.card.card.parent_card_id = reachable_parent(restoration.card);
      const auto plain = render(restoration.card);
      git.write_file(
          restoration.path,
          encrypted ? holder::privacy::encrypt_project_blob(
                          project.project_id,
                          *project.project_key_id,
                          plain
                      )
                    : plain
      );
      git.stage_path(restoration.path);
      std::filesystem::remove(git.repo_dir() / restoration.trash_path);
      git.remove_path(restoration.trash_path);
      restored_paths.push_back(restoration.path);
      note(
          &holder::model::PullChanges::restored_from_trash_card_ids,
          restoration.card.card.card_id
      );
    }
    if (encrypted)
      holder::privacy::assert_encryption_index_paths_safe(project.root_path, restored_paths);
    git.commit(
        restored_paths.size() == 1 ? "Restore 1 card edited here that another device moved to Trash"
                                   : "Restore " + std::to_string(restored_paths.size()) +
                                         " cards edited here that another device moved to Trash"
    );
  }

  // Cards whose two versions combine field by field are saved as one merged card instead of
  // a conflict copy, in a commit of their own so card history can undo the merge.
  std::vector<std::string> merged_paths;
  for (const auto& path : merge_result.conflicted_paths) {
    const auto id = std::filesystem::path(path).stem().string();
    if (!holder::identity::is_valid_uuid(id) || path != holder::core::card_rel_path(id)) continue;
    const auto base = read_version(merge_result.merge_base_oid_hex, path);
    const auto local = read_version(diverged.local_oid_hex, path);
    const auto remote = read_version(diverged.remote_oid_hex, path);
    if (!base || !local || !remote || local->card.card_id != id) continue;
    const auto merged = merge_card_versions(*base, *local, *remote);
    if (!merged) continue;
    handled_paths.insert(path);
    const auto plain = render(*merged);
    if (plain == render(*remote)) continue; // The remote version, now checked out, is the result.
    git.write_file(
        path,
        encrypted ? holder::privacy::encrypt_project_blob(
                        project.project_id,
                        *project.project_key_id,
                        plain
                    )
                  : plain
    );
    git.stage_path(path);
    merged_paths.push_back(path);
    note(&holder::model::PullChanges::merged_card_ids, id);
  }
  if (!merged_paths.empty()) {
    if (encrypted)
      holder::privacy::assert_encryption_index_paths_safe(project.root_path, merged_paths);
    git.commit(
        merged_paths.size() == 1 ? "Merge changes to 1 card made on two devices"
                                 : "Merge changes to " + std::to_string(merged_paths.size()) +
                                       " cards made on two devices"
    );
  }

  struct PendingCopy {
    std::string path;
    holder::core::ParsedCardFile snapshot;
    std::string new_card_id;
  };
  std::vector<PendingCopy> pending;
  std::map<std::string, std::string> copied_card_ids;
  for (const auto& path : merge_result.conflicted_paths) {
    if (handled_paths.contains(path)) continue;
    const auto id = std::filesystem::path(path).stem().string();
    if (!holder::identity::is_valid_uuid(id) ||
        (path != holder::core::card_rel_path(id) && path != holder::core::card_trash_rel_path(id)))
      continue;
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
    if (parsed.card.card_id != id)
      throw std::runtime_error("conflicted card ID does not match its path: " + path);
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
      // A copy of a copy points at the identity involved in this pull.
      std::erase_if(links, [](const auto& link) {
        return link.kind == holder::model::kConflictCopyLinkKind;
      });
      for (auto& link : links) {
        link.project_id = project.project_id;
        link.from_card_id = duplicate.card_id;
        if (link.to_type == "card" &&
            (!link.target_project_id || *link.target_project_id == project.project_id)) {
          const auto copied_target = copied_card_ids.find(link.to_card_id);
          if (copied_target != copied_card_ids.end()) link.to_card_id = copied_target->second;
        }
      }
      links.push_back(
          {project.project_id,
           duplicate.card_id,
           parsed.card.card_id,
           "card",
           holder::model::kConflictCopyLinkKind,
           std::nullopt,
           now,
           std::nullopt}
      );

      auto milestones = parsed.milestones;
      for (auto& milestone : milestones) {
        milestone.milestone_id = holder::identity::uuid_v4();
        milestone.project_id = project.project_id;
        milestone.card_id = duplicate.card_id;
      }

      try {
        store.create(duplicate, parsed.body, std::nullopt, links, milestones);
        resolved++;
        if (preserved_count) *preserved_count = resolved;
        created_card_ids.insert(duplicate.card_id);
        note(&holder::model::PullChanges::conflict_copy_ids, duplicate.card_id);
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
