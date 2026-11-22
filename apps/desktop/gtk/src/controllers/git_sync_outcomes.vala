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


// Decides when a pull may have brought cards that the window does not show yet. Every successful
// pull, background or not, records when it finished, even when there was nothing to pull, so a
// finished pull alone is not enough: the project's commit must also have moved since the last
// pull the window acted on. Local saves move the commit too, so a pull that brought nothing after
// local edits can still answer yes once; that only costs one refresh. Pulls finished before the
// window opened are already in the cards it loaded.
public class GitPullWatch : Object {
    private int64 since;
    private Gee.HashMap<string, int64?> pulls_seen = new Gee.HashMap<string, int64?>();
    private Gee.HashMap<string, string> revisions_at_pull = new Gee.HashMap<string, string>();

    public GitPullWatch(int64 since) {
        this.since = since;
    }

    // revision is the project's current commit, or null when it is not known.
    public bool pulled_new_commits(string project_id, ProjectSyncState state, string? revision) {
        if (!state.has_last_pull_at || state.last_pull_at < since) {
            return false;
        }
        var seen = pulls_seen.get(project_id);
        if (seen != null && state.last_pull_at <= (!) seen) {
            return false;
        }
        pulls_seen.set(project_id, state.last_pull_at);
        var before = revisions_at_pull.get(project_id);
        if (revision == null) {
            revisions_at_pull.unset(project_id);
            return true;
        }
        revisions_at_pull.set(project_id, (!) revision);
        return before == null || before != (!) revision;
    }
}

// Where core's numbered lines sit in a card's text. Core ends a line only at "\n", as Git does.
// A GTK text buffer also breaks lines at a lone "\r" and at U+2029, so its line numbers can
// drift from core's; these ranges are character offsets a buffer can use instead.
public class ConflictLineRanges : Object { // LCOV_EXCL_LINE: declaration-only coverage artifact
    // Start and end offsets, in pairs, of each 1-based line that exists in content. A range
    // includes the line's "\n".
    public static int[] of(string content, int[] lines) {
        int[] starts = { 0 };
        int offset = 0;
        int index = 0;
        unichar c;
        while (content.get_next_char(ref index, out c)) {
            offset++;
            if (c == '\n') starts += offset;
        }
        int[] ranges = {};
        foreach (var line in lines) {
            if (line < 1 || line > starts.length) continue;
            ranges += starts[line - 1];
            ranges += line < starts.length ? starts[line] : offset;
        }
        return ranges;
    }
}


// The starting text for "Combine changes": both versions, lines they share written once. Core's
// comparison numbers the lines each version changed, and the lines it did not mark are the ones
// the two share, in the same order on both sides. So walking both versions together, each run of
// changed lines is followed by the next shared line. A run appears first from version a, then
// from version b, either one after the other or between Git conflict markers. Lines end only at
// "\n", as core counts them, and are joined back the same way, so a "\r" stays where it was.
public class ConflictCombiner : Object { // LCOV_EXCL_LINE: declaration-only coverage artifact
    public static string combine(string a, int[] a_changed, string b, int[] b_changed, bool markers,
                                 string a_label, string b_label) {
        var a_lines = core_lines(a);
        var b_lines = core_lines(b);
        var a_marked = marked(a_changed);
        var b_marked = marked(b_changed);
        string[] out_lines = {};
        int i = 0;
        int j = 0;
        while (i < a_lines.length || j < b_lines.length) {
            string[] a_run = {};
            string[] b_run = {};
            while (i < a_lines.length && a_marked.contains(i + 1)) a_run += a_lines[i++];
            while (j < b_lines.length && b_marked.contains(j + 1)) b_run += b_lines[j++];
            if (a_run.length > 0 || b_run.length > 0) {
                if (markers) out_lines += "<<<<<<< " + a_label;
                foreach (var line in a_run) out_lines += line;
                if (markers) out_lines += "=======";
                foreach (var line in b_run) out_lines += line;
                if (markers) out_lines += ">>>>>>> " + b_label;
            }
            if (i < a_lines.length && j < b_lines.length) {
                out_lines += a_lines[i++];
                j++;
            } else {
                // The unmarked lines ran out on one side only, which a complete comparison never
                // does. Keep whatever is left rather than lose it.
                while (i < a_lines.length) out_lines += a_lines[i++];
                while (j < b_lines.length) out_lines += b_lines[j++];
            }
        }
        return string.joinv("\n", out_lines);
    }

    // As core's lines_of: split at "\n"; a trailing "\n" leaves an empty last line.
    private static string[] core_lines(string text) {
        if (text == "") return {};
        return text.split("\n");
    }

    private static Gee.HashSet<int> marked(int[] lines) {
        var set = new Gee.HashSet<int>();
        foreach (var line in lines) set.add(line);
        return set;
    }
}

}
