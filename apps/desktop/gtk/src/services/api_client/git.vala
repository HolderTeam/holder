namespace HolderLinux {

public class ApiClientGitEndpoints : Object { // LCOV_EXCL_BR_LINE: declaration branch artifact
    public static async ConflictComparison compare_conflict_copy(ApiClient client, string project_id,
                                                                 string copy_id) throws Error {
        var root = yield client.request_json("GET",
            "/projects/%s/git/conflict-copies/%s/comparison".printf(
                Uri.escape_string(project_id), Uri.escape_string(copy_id)), null, null);
        if (!root.has_member("data")) {
            throw new ApiError.PROTOCOL("Missing data for conflict comparison");
        }
        var data = root.get_object_member("data");
        var review_token = data.get_string_member_with_default("review_token", "");
        if (review_token == "") {
            throw new ApiError.PROTOCOL("Missing review token for conflict comparison");
        }
        return new ConflictComparison(
            ApiParsersCards.parse_card_detail_data(data.get_object_member("original")),
            ApiParsersCards.parse_card_detail_data(data.get_object_member("copy_card")),
            comparison_lines(data.get_array_member("original_changed_lines")),
            comparison_lines(data.get_array_member("copy_changed_lines")),
            data.get_boolean_member("truncated"), review_token);
    }

    public static async ConflictResolutionResult resolve_conflict_copy(ApiClient client, string project_id,
                                                                       string copy_id, string review_token,
                                                                       string title, string content) throws Error {
        var body = new Json.Builder();
        body.begin_object();
        body.set_member_name("review_token").add_string_value(review_token);
        body.set_member_name("title").add_string_value(title);
        body.set_member_name("content").add_string_value(content);
        body.end_object();
        var root = yield client.request_json("POST",
            "/projects/%s/git/conflict-copies/%s/resolution".printf(
                Uri.escape_string(project_id), Uri.escape_string(copy_id)),
            client.json_string_from_builder(body), null);
        if (!root.has_member("data")) throw new ApiError.PROTOCOL("Missing conflict resolution result");
        var data = root.get_object_member("data");
        return new ConflictResolutionResult(
            ApiParsersCards.parse_card_detail_data(data.get_object_member("card")),
            parse_cleanup_review(data.get_object_member("cleanup")));
    }

    public static async void trash_conflict_copy(ApiClient client, string project_id, string copy_id,
                                                string cleanup_token) throws Error {
        var body = new Json.Builder();
        body.begin_object();
        body.set_member_name("cleanup_token").add_string_value(cleanup_token);
        body.end_object();
        yield client.request_json("POST",
            "/projects/%s/git/conflict-copies/%s/trash".printf(
                Uri.escape_string(project_id), Uri.escape_string(copy_id)),
            client.json_string_from_builder(body), null);
    }

    private static ConflictCopyCleanupReview parse_cleanup_review(Json.Object data) throws Error {
        var token = data.get_string_member_with_default("cleanup_token", "");
        if (token == "") throw new ApiError.PROTOCOL("Missing cleanup review token");
        var review = new ConflictCopyCleanupReview(token);
        review.placement_differs = data.get_boolean_member("placement_differs");
        review.preserves_original = data.get_boolean_member("preserves_original");
        var parent = data.get_object_member("promotion_parent");
        if (parent != null) review.promotion_parent = parent.get_string_member("title");
        review.copy_links = cleanup_links(data.get_array_member("copy_links"));
        review.backlinks = cleanup_links(data.get_array_member("backlinks"));
        var children = data.get_array_member("children");
        review.children = new string[children.get_length()];
        children.foreach_element((array, index, node) => {
            review.children[index] = node.get_object().get_string_member("title");
        });
        var tags = data.get_array_member("copy_tags");
        review.copy_tags = new string[tags.get_length()];
        tags.foreach_element((array, index, node) => {
            review.copy_tags[index] = node.get_string();
        });
        var milestones = data.get_array_member("copy_milestones");
        review.copy_milestones = new string[milestones.get_length()];
        milestones.foreach_element((array, index, node) => {
            var milestone = node.get_object();
            var description = ApiParsersCommon.nullable_string_member_or_null(milestone, "description");
            var kind = ApiParsersCommon.nullable_string_member_or_null(milestone, "kind");
            var detail = new StringBuilder(description ?? kind ?? "Milestone");
            if (description != null && kind != null) detail.append(" · " + kind);
            milestone.get_array_member("dates").foreach_element((dates, date_index, date_node) => {
                var date = date_node.get_object();
                var value = ApiParsersCommon.nullable_string_member_or_null(date, "date") ??
                            ApiParsersCommon.nullable_string_member_or_null(date, "datetime");
                if (value != null) {
                    detail.append("\n%s · %s".printf(date.get_string_member("role"), value));
                }
            });
            review.copy_milestones[index] = detail.str;
        });
        return review;
    }

    private static CardLink[] cleanup_links(Json.Array data) {
        var links = new CardLink[data.get_length()];
        for (uint i = 0; i < data.get_length(); i++) {
            links[i] = ApiParsersCards.parse_card_link(data.get_object_element(i));
        }
        return links;
    }

    private static int[] comparison_lines(Json.Array array) {
        var lines = new int[array.get_length()];
        for (uint i = 0; i < array.get_length(); i++) {
            lines[i] = (int) array.get_object_element(i).get_int_member("line");
        }
        return lines;
    }

    public static async Gee.ArrayList<GitProviderCatalogEntry> list_git_provider_catalog(ApiClient client) // LCOV_EXCL_BR_LINE: async signature branch artifact
        throws Error { // LCOV_EXCL_BR_LINE: throws edge artifact
        var root = yield client.request_json_unwrapped("GET", "/git_providers.json", null, null); // LCOV_EXCL_BR_LINE: async yield branch artifact
        return ApiParsersGit.parse_git_provider_catalog(root); // LCOV_EXCL_BR_LINE: return edge artifact
    }

    public static async void set_project_git_remote(ApiClient client, // LCOV_EXCL_BR_LINE: async signature branch artifact
                                                    string project_id,
                                                    string? git_remote_url) throws Error {
        var body = new Json.Builder();
        body.begin_object();
        body.set_member_name("git_remote_url");
        if (git_remote_url == null || git_remote_url.strip().length == 0) {
            body.add_null_value();
        } else {
            body.add_string_value(git_remote_url);
        }
        body.end_object();

        yield client.request_json( // LCOV_EXCL_BR_LINE: async yield branch artifact
            "PATCH",
            "/projects/%s".printf(Uri.escape_string(project_id)),
            client.json_string_from_builder(body),
            null
        );
    }

    public static async GitTestRemoteResult test_project_git_remote(ApiClient client, // LCOV_EXCL_BR_LINE: async signature branch artifact
                                                                    string project_id,
                                                                    string? remote_url = null) throws Error {
        // Without a URL, the project's own remote is tested.
        var body = new Json.Builder();
        body.begin_object();
        if (remote_url != null && remote_url.strip().length > 0) {
            body.set_member_name("remote_url");
            body.add_string_value(remote_url);
        }
        body.end_object();

        var root = yield client.request_json( // LCOV_EXCL_BR_LINE: async yield branch artifact
            "POST",
            "/projects/%s/git/test-remote".printf(Uri.escape_string(project_id)),
            client.json_string_from_builder(body),
            null
        );
        if (!root.has_member("data")) {
            throw new ApiError.PROTOCOL("Missing data for git test-remote response"); // LCOV_EXCL_BR_LINE: throw edge artifact
        }
        return ApiParsersGit.parse_git_test_remote_result(root.get_object_member("data")); // LCOV_EXCL_BR_LINE: return edge artifact
    }

    public static async GitSyncResult sync_project_git(ApiClient client, string project_id) throws Error {
        var root = yield client.request_json(
            "POST",
            "/projects/%s/git/sync".printf(Uri.escape_string(project_id)),
            "{}",
            null
        );
        return ApiParsersGit.parse_git_sync_result(root);
    }

    public static async GitPushResult push_project_git(ApiClient client, // LCOV_EXCL_BR_LINE: async signature branch artifact
                                                       string project_id,
                                                       string branch = "",
                                                       bool set_upstream = true) throws Error {
        var body = new Json.Builder();
        body.begin_object();
        if (branch.strip().length > 0) {
            body.set_member_name("branch");
            body.add_string_value(branch);
        }
        body.set_member_name("set_upstream");
        body.add_boolean_value(set_upstream);
        body.end_object();

        var root = yield client.request_json( // LCOV_EXCL_BR_LINE: async yield branch artifact
            "POST",
            "/projects/%s/git/push".printf(Uri.escape_string(project_id)),
            client.json_string_from_builder(body),
            null
        );
        if (!root.has_member("data")) {
            throw new ApiError.PROTOCOL("Missing data for git push response"); // LCOV_EXCL_BR_LINE: throw edge artifact
        }
        return ApiParsersGit.parse_git_push_result(root.get_object_member("data"), branch); // LCOV_EXCL_BR_LINE: return edge artifact
    }
}

}
