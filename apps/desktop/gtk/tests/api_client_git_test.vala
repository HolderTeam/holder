using GLib;

namespace HolderLinuxTests {

private HolderLinux.ApiClient make_client(FakeApiHttpTransport transport) {
    return new HolderLinux.ApiClient("http://127.0.0.1:8080", "token-123", transport);
}

private void test_list_git_provider_catalog_parses_providers() {
    var transport = new FakeApiHttpTransport();
    transport.enqueue_read(
        200,
        "{\"providers\":[{\"id\":\"github\",\"name\":\"GitHub\",\"kind\":\"public\",\"defaults\":{\"preferred_transport\":\"https\"},\"git\":{\"transports\":[\"https\",\"ssh\"],\"examples\":{\"ssh\":\"git@github.com:owner/repo.git\",\"https\":\"https://github.com/owner/repo.git\"}}}]}"
    );
    var client = make_client(transport);

    bool done = false;
    Gee.ArrayList<HolderLinux.GitProviderCatalogEntry>? providers = null;
    client.list_git_provider_catalog.begin((obj, res) => {
        try { providers = client.list_git_provider_catalog.end(res); } catch (Error e) { providers = null; }
        done = true;
    });

    assert(wait_for_condition(() => done));
    assert(providers != null);
    assert(providers.size == 1);
    assert(providers[0].id == "github");
    assert(providers[0].name == "GitHub");
    assert(transport.last_uri.contains("/git_providers.json"));
}

private void test_set_project_git_remote_handles_null_and_non_empty_url() {
    var transport = new FakeApiHttpTransport();
    transport.enqueue_read(200, "{\"ok\":true,\"data\":{}}");
    transport.enqueue_read(200, "{\"ok\":true,\"data\":{}}");
    var client = make_client(transport);

    bool done_null = false;
    bool ok_null = false;
    client.set_project_git_remote.begin("p1", null, (obj, res) => {
        try { client.set_project_git_remote.end(res); ok_null = true; } catch (Error e) { ok_null = false; }
        done_null = true;
    });
    assert(wait_for_condition(() => done_null));
    assert(ok_null);
    assert(transport.last_method == "PATCH");
    assert(transport.last_uri.contains("/projects/p1"));

    bool done_url = false;
    bool ok_url = false;
    client.set_project_git_remote.begin("p1", "git@github.com:me/repo.git", (obj, res) => {
        try { client.set_project_git_remote.end(res); ok_url = true; } catch (Error e) { ok_url = false; }
        done_url = true;
    });
    assert(wait_for_condition(() => done_url));
    assert(ok_url);
}

private void test_test_remote_and_push_success_paths() {
    var transport = new FakeApiHttpTransport();
    transport.enqueue_read(
        200,
        "{\"ok\":true,\"data\":{\"project_id\":\"p1\",\"remote_url\":\"git@github.com:me/repo.git\",\"status\":\"reachable\",\"remote_has_head\":true,\"error_message\":null}}"
    );
    transport.enqueue_read(
        200,
        "{\"ok\":true,\"data\":{\"project_id\":\"p1\",\"remote_url\":\"git@github.com:me/repo.git\",\"status\":\"succeeded\",\"pull\":{\"attempted\":false,\"status\":\"not_attempted\",\"conflicts_resolved\":0,\"hidden_cards_repaired\":0,\"error_message\":null},\"push\":{\"attempted\":true,\"status\":\"pushed\",\"ahead_count\":0,\"behind_count\":0,\"local_head_commit\":\"abc\",\"error_message\":null}}}"
    );
    var client = make_client(transport);

    bool done_test = false;
    HolderLinux.GitTestRemoteResult? test_result = null;
    client.test_project_git_remote.begin("p1", "git@github.com:me/repo.git", (obj, res) => {
        try { test_result = client.test_project_git_remote.end(res); } catch (Error e) { test_result = null; }
        done_test = true;
    });
    assert(wait_for_condition(() => done_test));
    assert(test_result != null);
    assert(test_result.status == "reachable");
    assert(transport.last_uri.contains("/projects/p1/git/test-remote"));

    bool done_push = false;
    HolderLinux.GitPushResult? push_result = null;
    client.push_project_git.begin("p1", "cards", true, (obj, res) => {
        try { push_result = client.push_project_git.end(res); } catch (Error e) { push_result = null; }
        done_push = true;
    });
    assert(wait_for_condition(() => done_push));
    assert(push_result != null);
    assert(push_result.status == "pushed");
    assert(push_result.local_head_commit == "abc");
    assert(push_result.next_action == "");
    assert(push_result.branch == "cards");
    assert(transport.last_uri.contains("/projects/p1/git/push"));
}

private void test_test_remote_whitespace_remote_and_push_without_branch() {
    var transport = new FakeApiHttpTransport();
    transport.enqueue_read(
        200,
        "{\"ok\":true,\"data\":{\"project_id\":\"p1\",\"remote_url\":null,\"status\":\"remote_unset\",\"remote_has_head\":false,\"error_message\":\"unset\"}}"
    );
    transport.enqueue_read(
        200,
        "{\"ok\":true,\"data\":{\"project_id\":\"p1\",\"remote_url\":null,\"status\":\"failed\",\"pull\":{\"attempted\":false,\"status\":\"not_attempted\",\"conflicts_resolved\":0,\"hidden_cards_repaired\":0,\"error_message\":null},\"push\":{\"attempted\":true,\"status\":\"non_fast_forward\",\"ahead_count\":1,\"behind_count\":2,\"local_head_commit\":null,\"error_message\":\"pull first\"}}}"
    );
    var client = make_client(transport);

    bool done_test = false;
    HolderLinux.GitTestRemoteResult? test_result = null;
    client.test_project_git_remote.begin("p1", "   ", (obj, res) => {
        try { test_result = client.test_project_git_remote.end(res); } catch (Error e) { test_result = null; }
        done_test = true;
    });
    assert(wait_for_condition(() => done_test));
    assert(test_result != null);
    assert(test_result.status == "remote_unset");
    assert(test_result.remote_url == "");

    bool done_push = false;
    HolderLinux.GitPushResult? push_result = null;
    client.push_project_git.begin("p1", "", false, (obj, res) => {
        try { push_result = client.push_project_git.end(res); } catch (Error e) { push_result = null; }
        done_push = true;
    });
    assert(wait_for_condition(() => done_push));
    assert(push_result != null);
    assert(push_result.status == "non_fast_forward");
    assert(push_result.behind_count == 2);
    assert(push_result.error_message == "pull first");
    assert(push_result.next_action == "pull_then_retry");
}

private void test_missing_data_errors_for_test_remote_and_push() {
    var transport = new FakeApiHttpTransport();
    transport.enqueue_read(200, "{\"ok\":true}");
    transport.enqueue_read(200, "{\"ok\":true}");
    var client = make_client(transport);

    bool done_test = false;
    bool got_test_protocol = false;
    client.test_project_git_remote.begin("p1", null, (obj, res) => {
        try { client.test_project_git_remote.end(res); } catch (Error e) { got_test_protocol = (e is HolderLinux.ApiError.PROTOCOL); }
        done_test = true;
    });
    assert(wait_for_condition(() => done_test));
    assert(got_test_protocol);

    bool done_push = false;
    bool got_push_protocol = false;
    client.push_project_git.begin("p1", "", true, (obj, res) => {
        try { client.push_project_git.end(res); } catch (Error e) { got_push_protocol = (e is HolderLinux.ApiError.PROTOCOL); }
        done_push = true;
    });
    assert(wait_for_condition(() => done_push));
    assert(got_push_protocol);
}

private void test_forced_sync_reads_both_phases() {
    var transport = new FakeApiHttpTransport();
    transport.enqueue_read(200,
        "{\"ok\":true,\"data\":{\"project_id\":\"p1\",\"status\":\"failed\",\"remote_url\":null," +
        "\"pull\":{\"attempted\":true,\"status\":\"succeeded\",\"conflicts_resolved\":2," +
        "\"hidden_cards_repaired\":1,\"error_message\":null," +
        "\"changes\":{\"merged_card_ids\":[\"m1\"],\"kept_out_of_trash_card_ids\":[]," +
        "\"restored_from_trash_card_ids\":[\"r1\"],\"conflict_copy_ids\":[\"c1\",\"c2\"]," +
        "\"hidden_cards_repaired\":1}}," +
        "\"push\":{\"attempted\":true,\"status\":\"auth_failed\",\"error_message\":\"remote rejected push\"}}}");
    var client = make_client(transport);
    HolderLinux.GitSyncResult? result = null;
    bool done = false;
    client.sync_project_git.begin("p1", (obj, res) => {
        try { result = client.sync_project_git.end(res); } catch (Error e) { assert_not_reached(); }
        done = true;
    });

    assert(wait_for_condition(() => done));
    assert(result != null);
    assert(transport.last_method == "POST");
    assert(transport.last_uri.has_suffix("/projects/p1/git/sync"));
    assert(result.pull.attempted && result.pull.status == "succeeded");
    assert(result.pull.conflicts_resolved == 2 && result.pull.hidden_cards_repaired == 1);
    assert(result.pull.changes.merged_card_ids.size == 1 && result.pull.changes.merged_card_ids[0] == "m1");
    assert(result.pull.changes.restored_from_trash_card_ids[0] == "r1");
    assert(result.pull.changes.conflict_copy_ids.size == 2);
    assert(result.pull.changes.hidden_cards_repaired == 1);
    assert(result.push_attempted && result.push.status == "auth_failed");
    assert(result.push.error_message == "remote rejected push");
}

private void test_sync_result_without_changes_reads_as_empty() {
    var transport = new FakeApiHttpTransport();
    transport.enqueue_read(200,
        "{\"ok\":true,\"data\":{\"project_id\":\"p1\",\"status\":\"succeeded\",\"remote_url\":null," +
        "\"pull\":{\"attempted\":true,\"status\":\"succeeded\",\"conflicts_resolved\":1}," +
        "\"push\":{\"attempted\":true,\"status\":\"pushed\"}}}");
    var client = make_client(transport);
    HolderLinux.GitSyncResult? result = null;
    client.sync_project_git.begin("p1", (obj, res) => {
        try { result = client.sync_project_git.end(res); } catch (Error e) { assert_not_reached(); }
    });
    assert(wait_for_condition(() => result != null));
    assert(result.pull.changes.is_empty());
}

private const string CARD_C1 = "{\"card_id\":\"c1\",\"project_id\":\"p1\",\"title\":\"One\",\"content\":\"a\",\"updated_at\":10}";
private const string CARD_C2 = "{\"card_id\":\"c2\",\"project_id\":\"p1\",\"title\":\"Two\",\"content\":\"b\",\"updated_at\":11}";

// The comparison says which device made each version; an older daemon that does not is unknown.
private void test_comparison_reads_which_device_edited_each_version() {
    var transport = new FakeApiHttpTransport();
    transport.enqueue_read(200, "{\"ok\":true,\"data\":{\"original\":" + CARD_C1 + ",\"copy_card\":" + CARD_C2 +
        ",\"original_changed_lines\":[{\"line\":1}],\"copy_changed_lines\":[{\"line\":1}],\"truncated\":false," +
        "\"original_edited_on\":\"another_device\",\"copy_edited_on\":\"this_device\",\"review_token\":\"r1\"}}");
    transport.enqueue_read(200, "{\"ok\":true,\"data\":{\"original\":" + CARD_C1 + ",\"copy_card\":" + CARD_C2 +
        ",\"original_changed_lines\":[],\"copy_changed_lines\":[],\"truncated\":false,\"review_token\":\"r2\"}}");
    var client = make_client(transport);
    HolderLinux.ConflictComparison? first = null;
    HolderLinux.ConflictComparison? second = null;
    bool done = false;
    client.compare_conflict_copy.begin("p1", "c2", (obj, res) => {
        try { first = client.compare_conflict_copy.end(res); } catch (Error e) { first = null; }
        client.compare_conflict_copy.begin("p1", "c2", (obj2, res2) => {
            try { second = client.compare_conflict_copy.end(res2); } catch (Error e) { second = null; }
            done = true;
        });
    });
    assert(wait_for_condition(() => done));
    assert(first != null && second != null);
    assert(first.original_edited_on == HolderLinux.ConflictEditedOn.ANOTHER_DEVICE);
    assert(first.copy_edited_on == HolderLinux.ConflictEditedOn.THIS_DEVICE);
    assert(second.original_edited_on == HolderLinux.ConflictEditedOn.UNKNOWN);
    assert(second.copy_edited_on == HolderLinux.ConflictEditedOn.UNKNOWN);
}

private void test_separate_conflict_copy_posts_the_review_and_title() {
    var transport = new FakeApiHttpTransport();
    transport.enqueue_read(200, "{\"ok\":true,\"data\":" + CARD_C2 + "}");
    var client = make_client(transport);
    HolderLinux.CardDetail? kept = null;
    bool done = false;
    client.separate_conflict_copy.begin("p1", "c2", "r1", "Two", (obj, res) => {
        try { kept = client.separate_conflict_copy.end(res); } catch (Error e) { kept = null; }
        done = true;
    });
    assert(wait_for_condition(() => done));
    assert(kept != null && kept.card_id == "c2");
    assert(transport.last_method == "POST");
    assert(transport.last_uri.has_suffix("/projects/p1/git/conflict-copies/c2/separation"));
}

// Nothing chosen sends only the token; each chosen detail goes in its own list.
private void test_trash_request_names_the_details_to_carry_over() {
    var client = make_client(new FakeApiHttpTransport());
    assert(HolderLinux.ApiClientGitEndpoints.trash_request_body(client, "t1", null) == "{\"cleanup_token\":\"t1\"}");
    assert(HolderLinux.ApiClientGitEndpoints.trash_request_body(client, "t1", new HolderLinux.ConflictCarryOver()) ==
           "{\"cleanup_token\":\"t1\"}");
    var carry = new HolderLinux.ConflictCarryOver();
    carry.links = { new HolderLinux.CardLink("c2", "c5", "card", "see", null, 0, "p1", "p1") };
    carry.milestone_ids = { "m1" };
    carry.tags = { "draft" };
    carry.child_ids = { "c9" };
    carry.backlinks = { new HolderLinux.CardLink("c7", "c2", "card", "ref", null, 0, "p1", "p1") };
    var parser = new Json.Parser();
    try {
        parser.load_from_data(HolderLinux.ApiClientGitEndpoints.trash_request_body(client, "t1", carry));
    } catch (Error e) {
        assert_not_reached();
    }
    var body = parser.get_root().get_object();
    assert(body.get_string_member("cleanup_token") == "t1");
    var link = body.get_array_member("carry_links").get_object_element(0);
    assert(link.get_string_member("target_id") == "c5" && link.get_string_member("target_type") == "card");
    assert(link.get_string_member("target_project_id") == "p1" && link.get_string_member("kind") == "see");
    assert(body.get_array_member("carry_milestones").get_object_element(0).get_string_member("milestone_id") == "m1");
    assert(body.get_array_member("carry_tags").get_object_element(0).get_string_member("tag") == "draft");
    assert(body.get_array_member("adopt_children").get_object_element(0).get_string_member("card_id") == "c9");
    var backlink = body.get_array_member("retarget_backlinks").get_object_element(0);
    assert(backlink.get_string_member("source_id") == "c7" && backlink.get_string_member("kind") == "ref");
    assert(backlink.get_string_member("source_project_id") == "p1");
}

// The cleanup review keeps the API's structure; the dialog does the wording.
private void test_cleanup_review_keeps_milestones_and_cards_whole() {
    var parser = new Json.Parser();
    try {
        parser.load_from_data(
            "{\"cleanup_token\":\"t1\",\"placement_differs\":true,\"preserves_original\":true," +
            "\"promotion_parent\":{\"card_id\":\"p9\",\"title\":\"Parent\",\"deleted_at\":null}," +
            "\"copy_links\":[],\"backlinks\":[],\"copy_tags\":[\"draft\"]," +
            "\"children\":[{\"card_id\":\"k1\",\"title\":\"Child\",\"deleted_at\":null}]," +
            "\"copy_milestones\":[{\"milestone_id\":\"m1\",\"kind\":\"deadline\",\"description\":\"Launch\"," +
            "\"dates\":[{\"role\":\"due\",\"date\":\"2026-12-01\",\"datetime\":null}," +
            "{\"role\":\"review\",\"date\":null,\"datetime\":\"2026-11-30T09:00:00Z\"}]}]}");
    } catch (Error e) {
        assert_not_reached();
    }
    HolderLinux.ConflictCopyCleanupReview review;
    try {
        review = HolderLinux.ApiClientGitEndpoints.parse_cleanup_review(parser.get_root().get_object());
    } catch (Error e) {
        assert_not_reached();
    }
    assert(review.cleanup_token == "t1" && review.placement_differs && review.preserves_original);
    assert(review.promotion_parent != null);
    assert(((!) review.promotion_parent).card_id == "p9" && ((!) review.promotion_parent).title == "Parent");
    assert(review.children.length == 1 && review.children[0].card_id == "k1");
    assert(review.copy_tags.length == 1 && review.copy_tags[0] == "draft");
    assert(review.copy_milestones.length == 1);
    var milestone = review.copy_milestones[0];
    assert(milestone.milestone_id == "m1" && milestone.kind == "deadline" && milestone.description == "Launch");
    assert(milestone.dates.length == 2);
    assert(milestone.dates[0].role == "due" && milestone.dates[0].date == "2026-12-01");
    assert(milestone.dates[0].datetime == null);
    assert(milestone.dates[1].date == null && milestone.dates[1].datetime == "2026-11-30T09:00:00Z");
}

public static int main(string[] args) {
    Test.init(ref args);

    Test.add_func("/api_client_git/list_git_provider_catalog_parses_providers",
                  test_list_git_provider_catalog_parses_providers);
    Test.add_func("/api_client_git/set_project_git_remote_handles_null_and_non_empty_url",
                  test_set_project_git_remote_handles_null_and_non_empty_url);
    Test.add_func("/api_client_git/test_remote_and_push_success_paths",
                  test_test_remote_and_push_success_paths);
    Test.add_func("/api_client_git/test_remote_whitespace_remote_and_push_without_branch",
                  test_test_remote_whitespace_remote_and_push_without_branch);
    Test.add_func("/api_client_git/missing_data_errors_for_test_remote_and_push",
                  test_missing_data_errors_for_test_remote_and_push);
    Test.add_func("/api_client_git/forced_sync_reads_both_phases", test_forced_sync_reads_both_phases);
    Test.add_func("/api_client_git/comparison_reads_which_device_edited_each_version",
                  test_comparison_reads_which_device_edited_each_version);
    Test.add_func("/api_client_git/separate_conflict_copy_posts_the_review_and_title",
                  test_separate_conflict_copy_posts_the_review_and_title);
    Test.add_func("/api_client_git/trash_request_names_the_details_to_carry_over",
                  test_trash_request_names_the_details_to_carry_over);
    Test.add_func("/api_client_git/cleanup_review_keeps_milestones_and_cards_whole",
                  test_cleanup_review_keeps_milestones_and_cards_whole);
    Test.add_func("/api_client_git/sync_result_without_changes_reads_as_empty",
                  test_sync_result_without_changes_reads_as_empty);

    return Test.run();
}

}
