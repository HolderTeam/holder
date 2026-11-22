using GLib;

namespace HolderLinuxTests {

private Json.Object parse_json_object(string payload) {
    var parser = new Json.Parser();
    try {
        parser.load_from_data(payload, -1);
    } catch (Error e) {
        assert_not_reached();
    }
    return parser.get_root().get_object();
}

private void test_parse_projects_missing_data_is_protocol_error() {
    var root = parse_json_object("{\"ok\":true}");

    bool got_protocol = false;
    try {
        HolderLinux.ApiParsersProjects.parse_projects(root);
    } catch (Error e) {
        got_protocol = e.message.contains("Missing data for projects response");
    }

    assert(got_protocol);
}

private void test_parse_project_sync_status_missing_data_is_protocol_error() {
    bool got_protocol = false;
    try {
        HolderLinux.ApiParsersProjects.parse_project_sync_status(
            parse_json_object("{\"ok\":true}"));
    } catch (HolderLinux.ApiError.PROTOCOL e) {
        assert(e.message == "Missing data for project sync status response");
        got_protocol = true;
    } catch (Error e) {
        assert_not_reached();
    }
    assert(got_protocol);
}

private void test_parse_projects_full_and_defaults() {
    var root = parse_json_object(
        "{\"data\":[" +
        "{\"project_id\":\"p1\",\"name\":\"Project One\",\"privacy_mode\":\"encrypted_git\",\"root_path\":\"/tmp/p1\",\"created_at\":11,\"updated_at\":12,\"git_remote_url\":\"git@example/repo.git\",\"card_count\":7,\"root_card_count\":3}," +
        "{\"project_id\":\"p2\",\"name\":\"Project Two\"}," +
        "{\"project_id\":\"p3\",\"name\":\"Project Three\"}" +
        "]}"
    );

    Gee.ArrayList<HolderLinux.Project> projects;
    try {
        projects = HolderLinux.ApiParsersProjects.parse_projects(root);
    } catch (Error e) {
        assert_not_reached();
    }

    assert(projects.size == 3);

    var p1 = projects[0];
    assert(p1.project_id == "p1");
    assert(p1.name == "Project One");
    assert(p1.privacy_mode == "encrypted_git");
    assert(p1.root_path == "/tmp/p1");
    assert(p1.created_at == 11);
    assert(p1.updated_at == 12);
    assert(p1.git_remote_url == "git@example/repo.git");
    assert(p1.card_count == 7);
    assert(p1.root_card_count == 3);
    // A project's sync state is read on its own.
    assert(!p1.sync.has_last_commit_at);
    var status = parse_json_object(
        "{\"data\":{\"project_id\":\"p1\"," +
        "\"last_commit_at\":100,\"last_push_at\":101,\"last_pull_at\":102," +
        "\"uncommitted_changes_count\":1,\"unpushed_commits_count\":2," +
        "\"last_push_status\":\"ok\",\"last_pull_status\":\"ok\",\"last_sync_error\":\"\"," +
        "\"last_sync_error_at\":103,\"retry_count\":4,\"next_retry_at\":104," +
        "\"pull_retry_count\":5,\"next_pull_retry_at\":105,\"updated_at\":106}}"
    );
    try {
        p1.sync = HolderLinux.ApiParsersProjects.parse_project_sync_status(status);
    } catch (Error e) {
        assert_not_reached();
    }
    assert(p1.sync.has_last_commit_at);
    assert(p1.sync.last_commit_at == 100);
    assert(p1.sync.has_last_push_at);
    assert(p1.sync.last_push_at == 101);
    assert(p1.sync.has_last_pull_at);
    assert(p1.sync.last_pull_at == 102);
    assert(p1.sync.uncommitted_changes_count == 1);
    assert(p1.sync.unpushed_commits_count == 2);
    assert(p1.sync.last_push_status == "ok");
    assert(p1.sync.last_pull_status == "ok");
    assert(p1.sync.last_sync_error == "");
    assert(p1.sync.has_last_sync_error_at);
    assert(p1.sync.last_sync_error_at == 103);
    assert(p1.sync.retry_count == 4);
    assert(p1.sync.has_next_retry_at);
    assert(p1.sync.next_retry_at == 104);
    assert(p1.sync.pull_retry_count == 5);
    assert(p1.sync.has_next_pull_retry_at);
    assert(p1.sync.next_pull_retry_at == 105);
    assert(p1.sync.has_updated_at);
    assert(p1.sync.updated_at == 106);

    var p2 = projects[1];
    assert(p2.project_id == "p2");
    assert(p2.privacy_mode == "encrypted_git");
    assert(p2.root_path == "");
    assert(p2.created_at == 0);
    assert(p2.updated_at == 0);
    assert(p2.git_remote_url == null);
    assert(p2.card_count == 0);
    assert(p2.root_card_count == 0);
    assert(!p2.sync.has_last_commit_at);
    assert(!p2.sync.has_last_push_at);
    assert(!p2.sync.has_last_pull_at);
    assert(p2.sync.uncommitted_changes_count == 0);
    assert(p2.sync.unpushed_commits_count == 0);
    assert(!p2.sync.has_last_sync_error_at);
    assert(p2.sync.retry_count == 0);
    assert(!p2.sync.has_next_retry_at);
    assert(p2.sync.pull_retry_count == 0);
    assert(!p2.sync.has_next_pull_retry_at);
    assert(!p2.sync.has_updated_at);

    var p3 = projects[2];
    assert(p3.project_id == "p3");
    assert(p3.name == "Project Three");
    assert(p3.privacy_mode == "encrypted_git");
    assert(p3.root_path == "");
    assert(p3.created_at == 0);
    assert(p3.updated_at == 0);
    assert(p3.git_remote_url == null);
    assert(p3.card_count == 0);
    assert(p3.root_card_count == 0);
}

private void test_parse_conflict_copies() {
    try {
        var parser = new Json.Parser();
        parser.load_from_data(
            "{\"data\":{\"conflict_copies\":[" +
            "{\"original_card_id\":\"o1\",\"original\":null," +
            "\"copy_card\":{\"card_id\":\"c1\",\"title\":\"Renamed\",\"deleted_at\":null}}," +
            "{\"original_card_id\":\"o2\",\"original\":{\"card_id\":\"o2\",\"title\":\"Trashed\",\"deleted_at\":10}," +
            "\"copy_card\":{\"card_id\":\"c2\",\"title\":\"Second copy\",\"deleted_at\":null}}]}}"
        );
        var state = HolderLinux.ApiParsersProjects.parse_project_sync_status(parser.get_root().get_object());
        assert(state.conflict_copies.size == 2);
        assert(state.conflict_copies[0].original_id == "o1");
        assert(state.conflict_copies[0].original_title == null);
        assert(state.conflict_copies[0].copy_id == "c1");
        assert(state.conflict_copies[0].copy_title == "Renamed");
        assert(state.conflict_copies[1].original_trashed);
    } catch (Error e) {
        assert_not_reached();
    }
}

private void test_parse_last_pull_changes() {
    try {
        var parser = new Json.Parser();
        parser.load_from_data(
            "{\"data\":{\"last_pull_changes\":{\"pulled_at\":1700," +
            "\"merged_card_ids\":[\"m1\",\"m2\"],\"kept_out_of_trash_card_ids\":[\"k1\"]," +
            "\"restored_from_trash_card_ids\":[],\"conflict_copy_ids\":[\"c1\"]," +
            "\"hidden_cards_repaired\":3}}}"
        );
        var state = HolderLinux.ApiParsersProjects.parse_project_sync_status(parser.get_root().get_object());
        assert(state.last_pull_changes != null);
        var changes = (!) state.last_pull_changes;
        assert(state.last_pull_changes_at == 1700);
        assert(changes.merged_card_ids.size == 2 && changes.merged_card_ids[1] == "m2");
        assert(changes.kept_out_of_trash_card_ids.size == 1);
        assert(changes.restored_from_trash_card_ids.is_empty);
        assert(changes.conflict_copy_ids[0] == "c1");
        assert(changes.hidden_cards_repaired == 3);
        assert(!changes.is_empty());

        // No pull has changed anything yet, or an older daemon that does not report it.
        foreach (var data in new string[] { "{\"data\":{\"last_pull_changes\":null}}", "{\"data\":{}}" }) {
            parser.load_from_data(data);
            state = HolderLinux.ApiParsersProjects.parse_project_sync_status(parser.get_root().get_object());
            assert(state.last_pull_changes == null);
            assert(state.last_pull_changes_at == 0);
        }
    } catch (Error e) {
        assert_not_reached();
    }
}

public static int main(string[] args) {
    Test.init(ref args);

    Test.add_func("/parsers/projects/missing-data-protocol-error", test_parse_projects_missing_data_is_protocol_error);
    Test.add_func("/parsers/projects/sync-status-missing-data-protocol-error", test_parse_project_sync_status_missing_data_is_protocol_error);
    Test.add_func("/parsers/projects/full-and-defaults", test_parse_projects_full_and_defaults);
    Test.add_func("/parsers/projects/conflict-copies", test_parse_conflict_copies);
    Test.add_func("/parsers/projects/last-pull-changes", test_parse_last_pull_changes);

    return Test.run();
}

}
