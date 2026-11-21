#pragma once

#include "git/GitOps.h"
#include "index/FtsIndexer.h"
#include "model/Project.h"
#include "platform/Db.h"

namespace holder::sync {

// A pull only fetches and checks out git's working tree; nothing else keeps the SQLite card
// index (what card listing/search actually read) in sync with content that arrived from a
// remote rather than through CardStore's own create/update calls. Reconciling it is exactly
// what Rebuilder already does for StartupRecovery, so every successful pull -- from any caller,
// C ABI or the daemon's own sync worker -- needs to run this too. Throws on failure by design:
// if pull's git state moved but the index didn't, the pull hasn't actually delivered on its
// purpose (new content becoming visible), so it should be reported failed, not silently wrong.
void reconcile_index_after_pull(
    holder::platform::Db& db,
    holder::index::FtsIndexer* fts,
    const holder::model::Project& project
);

// Older encrypted repositories can have a privacy bootstrap but no project manifest.
// Finish recovery metadata after the pull and index reconciliation succeeded.
void complete_recovery_manifest(
    holder::platform::Db& db,
    holder::git::GitOps& git,
    const holder::model::Project& project,
    long long now
);

// Called after GitRepo::merge_remote_taking_theirs_for_conflicts has already resolved a
// diverged pull at the git level (remote wins for any card touched on both sides since the
// merge-base). First, Trash moves are settled: a card this device moved to Trash while the other
// device edited it stays live with the edit, and a card the other device moved to Trash while
// this device edited it is restored with this device's edit. Then each such card whose two
// versions changed different fields is merged field by field (see merge_card_versions). Each step
// is saved in a commit of its own, so card history can undo it.
// Every other such card keeps its pre-merge LOCAL version as a brand new card titled
// "<original title> (conflicted copy)": both full versions kept, nothing silently lost. A card
// whose pre-merge content can't be read/decrypted/parsed is skipped. Failure to write a parsed
// copy is reported as an error, with the pre-merge local commit still available for recovery.
// Returns how many conflicts were preserved as new cards; merged cards are not counted.
int resolve_pull_conflicts(
    holder::platform::Db& db,
    holder::index::FtsIndexer* fts,
    const holder::model::Project& project,
    holder::git::GitOps& git,
    const holder::git::NonFastForwardPullError& diverged,
    long long now,
    int* preserved_count = nullptr
);

} // namespace holder::sync
