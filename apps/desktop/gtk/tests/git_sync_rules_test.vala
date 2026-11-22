using GLib;

namespace HolderLinuxTests {

private HolderLinux.Project make_project(string id, string? remote_url, string name = "Family Notes") {
    return new HolderLinux.Project(id, name, "encrypted_git", "/tmp/" + id, 10, 20, remote_url, null, 3, 2);
}

private Gee.ArrayList<HolderLinux.Project> project_list(HolderLinux.Project first, HolderLinux.Project? second = null) {
    var list = new Gee.ArrayList<HolderLinux.Project>();
    list.add(first);
    if (second != null) {
        list.add(second);
    }
    return list;
}

private HolderLinux.GitProviderCatalogEntry make_entry(string id, string name, string preferred, string transports) {
    return new HolderLinux.GitProviderCatalogEntry(id, name, "hosted", preferred, transports, "", "");
}

private HolderLinux.GitPushResult make_push(string status, string error_message = "") {
    return new HolderLinux.GitPushResult("p1", "git@x:y/z.git", "main", status, 0, 0, "abc", error_message, "");
}

// --- GitSyncProjectState -------------------------------------------------

private void test_project_state_refresh_generation_goes_stale() {
    var state = new HolderLinux.GitSyncProjectState();
    var first = state.begin_refresh();
    assert(state.is_current(first));
    var second = state.begin_refresh();
    assert(!state.is_current(first));
    assert(state.is_current(second));
}

private void test_project_state_resolve_refreshed_project() {
    var state = new HolderLinux.GitSyncProjectState();
    var p1 = make_project("p1", null);
    var p2 = make_project("p2", "git@github.com:a/b.git");
    var list = project_list(p1, p2);

    assert(state.resolve_refreshed_project(list, "p2") == p2);
    assert(state.resolve_refreshed_project(list, "missing") == null);

    // The local "disconnected" flag survives while the backend still reports a remote...
    state.locally_disconnected_project_id = "p2";
    assert(state.resolve_refreshed_project(list, "p2") == p2);
    assert(state.locally_disconnected_project_id == "p2");

    // ...and clears once the backend reports no (or a blank) remote.
    state.locally_disconnected_project_id = "p1";
    assert(state.resolve_refreshed_project(list, "p1") == p1);
    assert(state.locally_disconnected_project_id == "");

    state.locally_disconnected_project_id = "p3";
    var blank = make_project("p3", "   ");
    assert(state.resolve_refreshed_project(project_list(blank), "p3") == blank);
    assert(state.locally_disconnected_project_id == "");

    // A different project's flag is left alone.
    state.locally_disconnected_project_id = "other";
    state.resolve_refreshed_project(list, "p1");
    assert(state.locally_disconnected_project_id == "other");
}

private void test_project_state_select_page() {
    var state = new HolderLinux.GitSyncProjectState();

    var page = state.select_page(null);
    assert(page.show_setup);
    assert(!page.cancel_visible);

    page = state.select_page(make_project("p1", null));
    assert(page.show_setup);
    assert(state.configured_project == null);

    page = state.select_page(make_project("p1", "  "));
    assert(page.show_setup);

    var configured = make_project("p1", "git@github.com:a/b.git");
    page = state.select_page(configured);
    assert(!page.show_setup);
    assert(!page.cancel_visible);
    assert(state.configured_project == configured);

    // Editing forces the setup page, and Cancel is offered because a configured project exists.
    state.editing_remote = true;
    page = state.select_page(configured);
    assert(page.show_setup);
    assert(page.cancel_visible);

    // A locally disconnected project shows setup even though the snapshot still has a remote.
    state.editing_remote = false;
    state.locally_disconnected_project_id = "p1";
    page = state.select_page(configured);
    assert(page.show_setup);
    assert(!page.cancel_visible);
}

private void test_project_state_editing_without_configured_project_hides_cancel() {
    var state = new HolderLinux.GitSyncProjectState();
    state.editing_remote = true;
    var page = state.select_page(make_project("p1", null));
    assert(page.show_setup);
    assert(!page.cancel_visible);
}

private void test_project_state_connect_and_disconnect_transitions() {
    var state = new HolderLinux.GitSyncProjectState();
    var configured = make_project("p1", "git@github.com:a/b.git");
    state.select_page(configured);
    var generation = state.begin_refresh();
    state.editing_remote = true;

    state.mark_disconnected("p1");
    assert(!state.is_current(generation));
    assert(state.locally_disconnected_project_id == "p1");
    assert(state.configured_project == null);
    assert(!state.editing_remote);

    state.editing_remote = true;
    state.mark_connected();
    assert(state.locally_disconnected_project_id == "");
    assert(!state.editing_remote);
}

private void test_project_snapshot_with_remote_copies_everything_but_the_remote() {
    var source = make_project("p1", null, "Notes");
    var copy = HolderLinux.GitSyncProjectState.project_snapshot_with_remote(source, "git@github.com:a/b.git");
    assert(copy.project_id == "p1");
    assert(copy.name == "Notes");
    assert(copy.privacy_mode == "encrypted_git");
    assert(copy.root_path == "/tmp/p1");
    assert(copy.created_at == 10);
    assert(copy.updated_at == 20);
    assert(copy.git_remote_url == "git@github.com:a/b.git");
    assert(copy.sync == source.sync);
    assert(copy.card_count == 3);
    assert(copy.root_card_count == 2);
}

// --- GitSyncValidation ---------------------------------------------------

private void test_validation_project_and_api() {
    var missing_project = HolderLinux.GitSyncValidation.project_and_api(null, true);
    assert(!missing_project.ok && missing_project.is_toast);
    assert(missing_project.message == "Select a project first.");

    var missing_api = HolderLinux.GitSyncValidation.project_and_api(make_project("p1", null), false);
    assert(!missing_api.ok && !missing_api.is_toast);
    assert(missing_api.error_title == "Git sync failed");
    assert(missing_api.error_details == "Backend API client is not ready.");

    assert(HolderLinux.GitSyncValidation.project_and_api(make_project("p1", null), true).ok);
}

private void test_validation_auto_sync_inputs() {
    var project = make_project("p1", null);
    assert(HolderLinux.GitSyncValidation.auto_sync_inputs(null, true, true, "me").message == "Select a project first.");
    assert(HolderLinux.GitSyncValidation.auto_sync_inputs(project, false, true, "me").error_title == "Git sync failed");

    var unauthenticated = HolderLinux.GitSyncValidation.auto_sync_inputs(project, true, false, "me");
    assert(!unauthenticated.ok && unauthenticated.is_toast);
    assert(unauthenticated.message == "GitHub CLI is not authenticated. Run `gh auth login` first.");

    var blank_login = HolderLinux.GitSyncValidation.auto_sync_inputs(project, true, true, "   ");
    assert(!blank_login.ok && blank_login.is_toast);

    assert(HolderLinux.GitSyncValidation.auto_sync_inputs(project, true, true, "me").ok);
}

private void test_validation_guided_repo_inputs() {
    assert(HolderLinux.GitSyncValidation.cli_repo_inputs("me", "repo").ok);
    var cli_missing = HolderLinux.GitSyncValidation.cli_repo_inputs("", "repo");
    assert(!cli_missing.ok);
    assert(cli_missing.toast_message == "GitHub username and repository name are required.");
    assert(cli_missing.retreat == HolderLinux.GuidedRetreat.NONE);
    assert(!HolderLinux.GitSyncValidation.cli_repo_inputs("me", "").ok);

    assert(HolderLinux.GitSyncValidation.verify_repo_inputs("me", "repo").ok);
    var verify_no_user = HolderLinux.GitSyncValidation.verify_repo_inputs("", "repo");
    assert(!verify_no_user.ok);
    assert(verify_no_user.toast_message == "GitHub username is required.");
    assert(verify_no_user.retreat == HolderLinux.GuidedRetreat.USERNAME_PAGE);
    var verify_no_repo = HolderLinux.GitSyncValidation.verify_repo_inputs("me", "");
    assert(!verify_no_repo.ok);
    assert(verify_no_repo.toast_message == "Repository name is required.");
    assert(verify_no_repo.retreat == HolderLinux.GuidedRetreat.NONE);

    assert(HolderLinux.GitSyncValidation.push_inputs("me", "repo").ok);
    var push_no_user = HolderLinux.GitSyncValidation.push_inputs("", "repo");
    assert(!push_no_user.ok);
    assert(push_no_user.retreat == HolderLinux.GuidedRetreat.REPOSITORY_PAGE);
    assert(push_no_user.toast_message == "GitHub username and repository name are required.");
    assert(!HolderLinux.GitSyncValidation.push_inputs("me", "").ok);
}

// --- GitSyncGuided -------------------------------------------------------

private void test_guided_remote_and_messages() {
    assert(HolderLinux.GitSyncGuided.github_ssh_remote("me", "repo") == "git@github.com:me/repo.git");
    assert(HolderLinux.GitSyncGuided.push_intro_text("me", "repo")
           == "We'll now save this remote and push your cards.\nRemote: git@github.com:me/repo.git");
    assert(HolderLinux.GitSyncGuided.repo_create_status(true) == "Repository created with GitHub CLI and verified.");
    assert(HolderLinux.GitSyncGuided.repo_create_status(false) == "Repository available and verified.");
    assert(HolderLinux.GitSyncGuided.create_failure_details("  boom  ") == "boom");
    assert(HolderLinux.GitSyncGuided.create_failure_details("  ") == "Repository could not be created.");
    assert(HolderLinux.GitSyncGuided.auto_sync_progress("me", "repo")
           == "GitHub CLI: creating private repo `me/repo`...");
}

private void test_guided_defaults() {
    assert(HolderLinux.GitSyncGuided.noreply_email("  me  ") == "me@users.noreply.github.com");
    assert(HolderLinux.GitSyncGuided.noreply_email("   ") == "");

    assert(HolderLinux.GitSyncGuided.resolve_username("  typed ", "saved") == "typed");
    assert(HolderLinux.GitSyncGuided.resolve_username("  ", "saved") == "saved");

    assert(HolderLinux.GitSyncGuided.prefill_username("cli-user", "saved") == "cli-user");
    assert(HolderLinux.GitSyncGuided.prefill_username("  ", "saved") == "saved");

    assert(HolderLinux.GitSyncGuided.default_repo_name(make_project("p1", null, "My Notes")) == "My Notes");
    assert(HolderLinux.GitSyncGuided.default_repo_name(null) == "");

    assert(HolderLinux.GitSyncGuided.provider_namespace_default("", "me") == "me");
    assert(HolderLinux.GitSyncGuided.provider_namespace_default("  ", "me") == "me");
    assert(HolderLinux.GitSyncGuided.provider_namespace_default("existing", "me") == null);
    assert(HolderLinux.GitSyncGuided.provider_namespace_default("", "") == null);
}

// --- GitProviderOptions --------------------------------------------------

private void test_provider_transport_options_parse_and_prefer() {
    var options = HolderLinux.GitProviderOptions.transport_options(
        make_entry("gitlab", "GitLab", "https", " ssh , https ,, ")
    );
    assert(options.options.size == 2);
    assert(options.options[0] == "ssh");
    assert(options.options[1] == "https");
    assert(options.selected_index == 1);

    var unknown_preferred = HolderLinux.GitProviderOptions.transport_options(
        make_entry("x", "X", "carrier-pigeon", "ssh,https")
    );
    assert(unknown_preferred.selected_index == 0);

    var no_preferred = HolderLinux.GitProviderOptions.transport_options(
        make_entry("x", "X", "  ", "https,ssh")
    );
    assert(no_preferred.selected_index == 0);
}

private void test_provider_transport_options_defaults() {
    var no_provider = HolderLinux.GitProviderOptions.transport_options(null);
    assert(no_provider.options.size == 2);
    assert(no_provider.options[0] == "ssh");
    assert(no_provider.options[1] == "https");
    assert(no_provider.selected_index == 0);

    var blank_summary = HolderLinux.GitProviderOptions.transport_options(make_entry("x", "X", "https", "  "));
    assert(blank_summary.options.size == 2);
    assert(blank_summary.selected_index == 1);

    var only_separators = HolderLinux.GitProviderOptions.transport_options(make_entry("x", "X", "", " , ,"));
    assert(only_separators.options.size == 2);
}

private void test_provider_label_entry_at_and_resolve_transport() {
    var github = make_entry("github", "GitHub", "ssh", "ssh,https");
    assert(HolderLinux.GitProviderOptions.provider_label(github) == "GitHub (github)");

    var entries = new Gee.ArrayList<HolderLinux.GitProviderCatalogEntry>();
    entries.add(github);
    assert(HolderLinux.GitProviderOptions.entry_at(entries, 0) == github);
    assert(HolderLinux.GitProviderOptions.entry_at(entries, 1) == null);
    assert(HolderLinux.GitProviderOptions.entry_at(entries, uint.MAX) == null);

    var transports = new Gee.ArrayList<string>();
    assert(HolderLinux.GitProviderOptions.resolve_transport(transports, 0) == "ssh");
    transports.add("https");
    transports.add("ssh");
    assert(HolderLinux.GitProviderOptions.resolve_transport(transports, 1) == "ssh");
    assert(HolderLinux.GitProviderOptions.resolve_transport(transports, 7) == "https");
    assert(HolderLinux.GitProviderOptions.resolve_transport(transports, uint.MAX) == "https");
}

// --- GitSyncOutcomes -----------------------------------------------------

private void test_outcomes_explain_incomplete_sync_and_reconciliation() {
    foreach (var status in new string[] { "remote_unset", "failed" }) {
        var pull = new HolderLinux.GitPullPhase(true, status, 0, 0, "");
        var result = HolderLinux.GitSyncOutcomes.for_sync_result(
            new HolderLinux.GitSyncResult(pull, false, make_push("not_attempted")));
        var explanation = status == "remote_unset"
            ? "No remote repository is configured." : "Pull status: failed.";
        assert(result.error_details == explanation + "\nNo push was attempted.");
    }

    var pull = new HolderLinux.GitPullPhase(true, "succeeded", 1, 1, "");
    var interrupted = HolderLinux.GitSyncOutcomes.for_sync_result(
        new HolderLinux.GitSyncResult(pull, false, make_push("not_attempted")));
    assert(interrupted.pull_succeeded);
    assert(interrupted.error_title == "Git push failed");
    assert(interrupted.error_details == "Pull succeeded. One local version was preserved as a conflict copy." +
           " One hidden card was made reachable.\nNo push was attempted.");

    var repaired = new HolderLinux.GitPullPhase(true, "succeeded", 0, 2, "");
    var result = HolderLinux.GitSyncOutcomes.for_sync_result(
        new HolderLinux.GitSyncResult(repaired, true, make_push("up_to_date")));
    assert(result.toast_message == "Project synced. 2 hidden cards were made reachable.");
}

private HolderLinux.GitPullChanges make_changes(int merged, int kept, int restored, int copies, int repaired) {
    var changes = new HolderLinux.GitPullChanges();
    for (int i = 0; i < merged; i++) changes.merged_card_ids.add("m%d".printf(i));
    for (int i = 0; i < kept; i++) changes.kept_out_of_trash_card_ids.add("k%d".printf(i));
    for (int i = 0; i < restored; i++) changes.restored_from_trash_card_ids.add("r%d".printf(i));
    for (int i = 0; i < copies; i++) changes.conflict_copy_ids.add("c%d".printf(i));
    changes.hidden_cards_repaired = repaired;
    return changes;
}

private HolderLinux.ProjectSyncState reported_state(HolderLinux.GitPullChanges? changes, int64 at) {
    var state = new HolderLinux.ProjectSyncState();
    state.last_pull_changes = changes;
    state.last_pull_changes_at = at;
    return state;
}

private void test_outcomes_leave_a_pull_that_changed_something_to_its_report() {
    var pull = new HolderLinux.GitPullPhase(true, "succeeded", 1, 0, "");
    pull.changes = make_changes(0, 0, 0, 1, 0);
    var synced = HolderLinux.GitSyncOutcomes.for_sync_result(
        new HolderLinux.GitSyncResult(pull, true, make_push("pushed")));
    assert(synced.toast_message == "");
    assert(synced.error_title == "");
    assert(synced.pull_succeeded);

    // A failed push still explains the whole sync in its error.
    var failed = HolderLinux.GitSyncOutcomes.for_sync_result(
        new HolderLinux.GitSyncResult(pull, true, make_push("failed", "auth rejected")));
    assert(failed.error_details ==
           "Pull succeeded. One local version was preserved as a conflict copy.\nauth rejected");
}

private void test_pull_report_describes_every_change() {
    assert(HolderLinux.GitPullReportTracker.describe(make_changes(1, 0, 0, 0, 0)) ==
           "Pulled changes from another device: merged edits into 1 card.");
    assert(HolderLinux.GitPullReportTracker.describe(make_changes(2, 0, 0, 1, 0)) ==
           "Pulled changes from another device: merged edits into 2 cards and saved your version of a card as a copy.");
    assert(HolderLinux.GitPullReportTracker.describe(make_changes(0, 1, 2, 3, 1)) ==
           "Pulled changes from another device: kept 1 card you edited out of Trash, " +
           "brought back 2 cards you edited from Trash, saved your versions of 3 cards as copies " +
           "and made a hidden card reachable again.");
    assert(HolderLinux.GitPullReportTracker.describe(make_changes(0, 2, 1, 0, 4)) ==
           "Pulled changes from another device: kept 2 cards you edited out of Trash, " +
           "brought back 1 card you edited from Trash and made 4 hidden cards reachable again.");
}

private void test_pull_report_offers_compare_for_one_copy_and_review_for_more() {
    var state = new HolderLinux.ProjectSyncState();
    state.conflict_copies.add(new HolderLinux.GitConflictCopy("c0", "Copy", "o0", "Original", false));
    state.conflict_copies.add(new HolderLinux.GitConflictCopy("c1", "Copy", "o1", "Trashed", true));

    var one = HolderLinux.GitPullReportTracker.notice_for(make_changes(0, 0, 0, 1, 0), state);
    assert(one.opens_comparison() && !one.opens_git_tool());
    assert(one.action_label == "Compare" && one.copy_id == "c0" && one.original_id == "o0");

    var many = HolderLinux.GitPullReportTracker.notice_for(make_changes(0, 0, 0, 2, 0), state);
    assert(many.opens_git_tool() && many.action_label == "Review");

    // The one copy's original is in Trash, or the copy is no longer listed: nothing to compare.
    var trashed = new HolderLinux.GitPullChanges();
    trashed.conflict_copy_ids.add("c1");
    assert(HolderLinux.GitPullReportTracker.notice_for(trashed, state).opens_git_tool());
    var gone = new HolderLinux.GitPullChanges();
    gone.conflict_copy_ids.add("c9");
    assert(HolderLinux.GitPullReportTracker.notice_for(gone, state).opens_git_tool());

    var merged = HolderLinux.GitPullReportTracker.notice_for(make_changes(1, 0, 0, 0, 0), state);
    assert(merged.action_label == "" && !merged.opens_comparison() && !merged.opens_git_tool());
}

private void test_pull_report_tracker_announces_each_new_report_once() {
    var tracker = new HolderLinux.GitPullReportTracker(100);
    assert(tracker.take("p1", reported_state(null, 0)) == null);
    assert(tracker.take("p1", reported_state(new HolderLinux.GitPullChanges(), 150)) == null);
    // Made before the app started.
    assert(tracker.take("p1", reported_state(make_changes(1, 0, 0, 0, 0), 99)) == null);

    var first = tracker.take("p1", reported_state(make_changes(1, 0, 0, 0, 0), 100));
    assert(first != null && first.message == "Pulled changes from another device: merged edits into 1 card.");
    assert(tracker.take("p1", reported_state(make_changes(1, 0, 0, 0, 0), 100)) == null);
    // Another project's reports are its own.
    assert(tracker.take("p2", reported_state(make_changes(1, 0, 0, 0, 0), 100)) != null);
    assert(tracker.take("p1", reported_state(make_changes(0, 1, 0, 0, 0), 120)) != null);
    assert(tracker.take("p1", reported_state(make_changes(0, 1, 0, 0, 0), 110)) == null);
}

private HolderLinux.ProjectSyncState pulled_state(int64 at) {
    return new HolderLinux.ProjectSyncState(null, null, at);
}

private void test_pull_watch_notices_pulls_that_moved_the_commit() {
    var watch = new HolderLinux.GitPullWatch(100);
    // Never pulled, or pulled before the window opened.
    assert(!watch.pulled_new_commits("p1", new HolderLinux.ProjectSyncState(), "r1"));
    assert(!watch.pulled_new_commits("p1", pulled_state(99), "r1"));

    // The first pull seen has nothing to compare with, so it counts.
    assert(watch.pulled_new_commits("p1", pulled_state(100), "r1"));
    // The same pull again.
    assert(!watch.pulled_new_commits("p1", pulled_state(100), "r2"));
    // A later pull that left the commit where it was brought nothing.
    assert(!watch.pulled_new_commits("p1", pulled_state(400), "r1"));
    // A later pull that moved it.
    assert(watch.pulled_new_commits("p1", pulled_state(700), "r2"));
    // An unknown commit counts, and is not compared with next time.
    assert(watch.pulled_new_commits("p1", pulled_state(1000), null));
    assert(watch.pulled_new_commits("p1", pulled_state(1300), "r2"));
    // Each project on its own.
    assert(watch.pulled_new_commits("p2", pulled_state(1300), "r2"));
}

private void test_outcomes_for_sync_result() {
    var pull = new HolderLinux.GitPullPhase(true, "succeeded", 0, 0, "");
    var pushed = HolderLinux.GitSyncOutcomes.for_sync_result(
        new HolderLinux.GitSyncResult(pull, true, make_push("pushed")));
    assert(pushed.toast_message == "Project synced.");
    assert(pushed.error_title == "");
    assert(pushed.pull_succeeded);

    var up_to_date = HolderLinux.GitSyncOutcomes.for_sync_result(
        new HolderLinux.GitSyncResult(pull, true, make_push("up_to_date")));
    assert(up_to_date.toast_message == "Project synced.");
    assert(up_to_date.pull_succeeded);

    var failed = HolderLinux.GitSyncOutcomes.for_sync_result(
        new HolderLinux.GitSyncResult(pull, true, make_push("failed", "  auth rejected  ")));
    assert(failed.error_title == "Git push failed");
    assert(failed.error_details == "Pull succeeded.\nauth rejected");
    assert(failed.toast_message == "");
    assert(failed.pull_succeeded);

    var blocked = HolderLinux.GitSyncOutcomes.for_sync_result(
        new HolderLinux.GitSyncResult(
            new HolderLinux.GitPullPhase(true, "failed", 0, 0, "Cannot reach remote"),
            false, make_push("not_attempted")));
    assert(blocked.error_title == "Git pull failed");
    assert(blocked.error_details == "Cannot reach remote\nNo push was attempted.");
    assert(!blocked.pull_succeeded);
}

void assert_ranges(int[] actual, int[] expected) {
    assert(actual.length == expected.length);
    for (var i = 0; i < expected.length; i++) assert(actual[i] == expected[i]);
}

// Highlights must land on the lines core numbered, whatever line endings the card uses.
void test_conflict_line_ranges_follow_core_numbering() {
    assert_ranges(HolderLinux.ConflictLineRanges.of("one\ntwo\nthree", { 2, 3 }), { 4, 8, 8, 13 });
    // CRLF: the "\r" belongs to its line.
    assert_ranges(HolderLinux.ConflictLineRanges.of("one\r\ntwo\r\n", { 2, 3 }), { 5, 10, 10, 10 });
    // A lone "\r" and U+2029 do not end a line for core, though a GTK buffer breaks there.
    assert_ranges(HolderLinux.ConflictLineRanges.of("a\rb\nc", { 2 }), { 4, 5 });
    assert_ranges(HolderLinux.ConflictLineRanges.of("a\u2029b\nc", { 2 }), { 4, 5 });
    // Offsets count characters, not bytes.
    assert_ranges(HolderLinux.ConflictLineRanges.of("café\nnext", { 2 }), { 5, 9 });
    // Lines core could not have numbered are skipped.
    assert_ranges(HolderLinux.ConflictLineRanges.of("only", { 0, 2, 1 }), { 0, 4 });
}

public static int main(string[] args) {
    Test.init(ref args);

    Test.add_func("/git-sync-rules/project-state/generation", test_project_state_refresh_generation_goes_stale);
    Test.add_func("/git-sync-rules/project-state/resolve-refreshed", test_project_state_resolve_refreshed_project);
    Test.add_func("/git-sync-rules/project-state/select-page", test_project_state_select_page);
    Test.add_func("/git-sync-rules/project-state/editing-without-configured",
                  test_project_state_editing_without_configured_project_hides_cancel);
    Test.add_func("/git-sync-rules/project-state/transitions", test_project_state_connect_and_disconnect_transitions);
    Test.add_func("/git-sync-rules/project-state/snapshot-with-remote",
                  test_project_snapshot_with_remote_copies_everything_but_the_remote);
    Test.add_func("/git-sync-rules/validation/project-and-api", test_validation_project_and_api);
    Test.add_func("/git-sync-rules/validation/auto-sync", test_validation_auto_sync_inputs);
    Test.add_func("/git-sync-rules/validation/guided-repo", test_validation_guided_repo_inputs);
    Test.add_func("/git-sync-rules/guided/remote-and-messages", test_guided_remote_and_messages);
    Test.add_func("/git-sync-rules/guided/defaults", test_guided_defaults);
    Test.add_func("/git-sync-rules/provider/transport-parse", test_provider_transport_options_parse_and_prefer);
    Test.add_func("/git-sync-rules/provider/transport-defaults", test_provider_transport_options_defaults);
    Test.add_func("/git-sync-rules/provider/label-entry-resolve", test_provider_label_entry_at_and_resolve_transport);
    Test.add_func("/git-sync-rules/outcomes/sync-result", test_outcomes_for_sync_result);
    Test.add_func("/git-sync-rules/outcomes/pull-changes-left-to-report",
                  test_outcomes_leave_a_pull_that_changed_something_to_its_report);
    Test.add_func("/git-sync-rules/pull-report/describes-every-change", test_pull_report_describes_every_change);
    Test.add_func("/git-sync-rules/pull-report/compare-or-review",
                  test_pull_report_offers_compare_for_one_copy_and_review_for_more);
    Test.add_func("/git-sync-rules/pull-watch/moved-commit",
                  test_pull_watch_notices_pulls_that_moved_the_commit);
    Test.add_func("/git-sync-rules/pull-report/each-report-once",
                  test_pull_report_tracker_announces_each_new_report_once);
    Test.add_func("/git-sync-rules/conflict-line-ranges/core-numbering",
                  test_conflict_line_ranges_follow_core_numbering);
    Test.add_func("/git-sync-rules/outcomes/incomplete-and-reconciled",
                  test_outcomes_explain_incomplete_sync_and_reconciliation);

    return Test.run();
}

}
