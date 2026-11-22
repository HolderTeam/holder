namespace HolderLinux {

public class GitSyncNowOutcome : Object {
    public string toast_message { get; construct; }
    public string error_title { get; construct; }
    public string error_details { get; construct; }
    public bool pull_succeeded { get; construct; }

    public GitSyncNowOutcome(string toast_message,
                             string error_title = "",
                             string error_details = "",
                             bool pull_succeeded = false) {
        Object(
            toast_message: toast_message,
            error_title: error_title,
            error_details: error_details,
            pull_succeeded: pull_succeeded
        );
    }
}

public class GitSyncOutcomes : Object { // LCOV_EXCL_LINE: declaration-only coverage artifact
    public static GitSyncNowOutcome for_sync_result(GitSyncResult result) {
        if (!result.pull.attempted || result.pull.status != "succeeded") {
            var details = result.pull.error_message.strip();
            if (details.length == 0) {
                details = result.pull.status == "remote_unset"
                    ? "No remote repository is configured."
                    : "Pull status: %s.".printf(result.pull.status);
            }
            return new GitSyncNowOutcome("", "Git pull failed",
                                         details + "\nNo push was attempted.");
        }

        var reconciled = new StringBuilder();
        var conflicts = result.pull.conflicts_resolved;
        if (conflicts == 1) {
            reconciled.append(" One local version was preserved as a conflict copy.");
        } else if (conflicts > 1) {
            reconciled.append(" %d local versions were preserved as conflict copies.".printf(conflicts));
        }
        var repaired = result.pull.hidden_cards_repaired;
        if (repaired == 1) {
            reconciled.append(" One hidden card was made reachable.");
        } else if (repaired > 1) {
            reconciled.append(" %d hidden cards were made reachable.".printf(repaired));
        }

        if (result.push_attempted &&
            (result.push.status == "pushed" || result.push.status == "up_to_date")) {
            // A pull that changed something is announced once the project's last pull changes
            // are read back (see GitPullReportTracker), the same way as a background pull's, so
            // the toast here would only repeat it.
            var toast = result.pull.changes.is_empty() ? "Project synced." + reconciled.str : "";
            return new GitSyncNowOutcome(toast, "", "", true);
        }
        var details = result.push.error_message.strip();
        if (!result.push_attempted) {
            details = "No push was attempted.";
        } else if (details.length == 0) {
            details = "Push status: %s.".printf(result.push.status);
        }
        return new GitSyncNowOutcome("", "Git push failed",
                                     "Pull succeeded." + reconciled.str + "\n" + details, true);
    }
}


// A toast about what a pull changed, with an optional action: Compare opens the one preserved
// copy beside its original; Review opens the Git tool, which lists every preserved copy.
public class GitPullNotice : Object {
    public string message { get; construct; }
    public string action_label { get; construct; }
    public string copy_id { get; construct; }
    public string original_id { get; construct; }

    public GitPullNotice(string message, string action_label = "", string copy_id = "", string original_id = "") {
        Object(message: message, action_label: action_label, copy_id: copy_id, original_id: original_id);
    }

    public bool opens_comparison() {
        return action_label == "Compare";
    }

    public bool opens_git_tool() {
        return action_label == "Review";
    }
}

// Decides when a project's last pull changes are news. Each report is announced once, by when the
// pull finished, whether a background pull or Sync now made it; a report from before the app
// started was made while nobody was looking at this window and is left to the Git tool.
public class GitPullReportTracker : Object {
    private int64 since;
    private Gee.HashMap<string, int64?> announced = new Gee.HashMap<string, int64?>();

    public GitPullReportTracker(int64 since) {
        this.since = since;
    }

    public GitPullNotice? take(string project_id, ProjectSyncState state) {
        var changes = state.last_pull_changes;
        if (changes == null || ((!) changes).is_empty()) {
            return null;
        }
        var at = state.last_pull_changes_at;
        if (at < since) {
            return null;
        }
        var previous = announced.get(project_id);
        if (previous != null && (!) previous >= at) {
            return null;
        }
        announced.set(project_id, at);
        return notice_for((!) changes, state);
    }

    public static GitPullNotice notice_for(GitPullChanges changes, ProjectSyncState state) {
        var message = describe(changes);
        var copies = changes.conflict_copy_ids;
        if (copies.size == 1) {
            foreach (var copy in state.conflict_copies) {
                if (copy.copy_id == copies[0] && copy.original_title != null && !copy.original_trashed) {
                    return new GitPullNotice(message, "Compare", copy.copy_id, copy.original_id);
                }
            }
        }
        return new GitPullNotice(message, copies.is_empty ? "" : "Review");
    }

    public static string describe(GitPullChanges changes) {
        var parts = new Gee.ArrayList<string>();
        var merged = changes.merged_card_ids.size;
        if (merged > 0) {
            parts.add("merged edits into %s".printf(cards(merged)));
        }
        var kept = changes.kept_out_of_trash_card_ids.size;
        if (kept > 0) {
            parts.add("kept %s you edited out of Trash".printf(cards(kept)));
        }
        var restored = changes.restored_from_trash_card_ids.size;
        if (restored > 0) {
            parts.add("brought back %s you edited from Trash".printf(cards(restored)));
        }
        var copies = changes.conflict_copy_ids.size;
        if (copies == 1) {
            parts.add("saved your version of a card as a copy");
        } else if (copies > 1) {
            parts.add("saved your versions of %d cards as copies".printf(copies));
        }
        var repaired = changes.hidden_cards_repaired;
        if (repaired > 0) {
            parts.add("made %s reachable again".printf(repaired == 1 ? "a hidden card" : "%d hidden cards".printf(repaired)));
        }
        var text = new StringBuilder("Pulled changes from another device: ");
        for (int i = 0; i < parts.size; i++) {
            if (i > 0) {
                text.append(i == parts.size - 1 ? " and " : ", ");
            }
            text.append(parts[i]);
        }
        text.append(".");
        return text.str;
    }

    private static string cards(int count) {
        return count == 1 ? "1 card" : "%d cards".printf(count);
    }
}

}
