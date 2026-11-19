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
            return new GitSyncNowOutcome("Project synced." + reconciled.str, "", "", true);
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

}
