namespace HolderLinux {

public class ApiClientCardsEndpoints : Object { // LCOV_EXCL_BR_LINE: declaration branch artifact
    public static async ProjectHistoryPage list_project_history(ApiClient client,
                                                                 string project_id,
                                                                 int limit = 50,
                                                                 string? cursor = null,
                                                                 string? kind = null) throws Error {
        var query = new HashTable<string, string>(str_hash, str_equal);
        query.insert("limit", limit.to_string());
        if (cursor != null && ((!) cursor).strip().length > 0) query.insert("cursor", (!) cursor);
        if (kind != null && ((!) kind).strip().length > 0) query.insert("kind", (!) kind);
        var root = yield client.request_json(
            "GET", "/projects/%s/history".printf(Uri.escape_string(project_id)), null, query
        );
        return ApiParsersCards.parse_project_history_page(root);
    }

    public static async Gee.ArrayList<CardSummary> list_cards(ApiClient client, // LCOV_EXCL_BR_LINE: async declaration branch artifact
                                                              string project_id,
                                                              string view = "tree",
                                                              string? parent_card_id = null,
                                                              int limit = 0) throws Error {
        var query = new HashTable<string, string>(str_hash, str_equal);
        if (view == "tree") query.insert("parent", parent_card_id != null && ((!) parent_card_id).strip().length > 0
            ? (!) parent_card_id : "roots");
        query.insert("order", view == "recent" ? "updated_desc" : "sort_key_asc");
        if (limit > 0) {
            query.insert("limit", limit.to_string());
        }
        var path = "/projects/%s/cards".printf(Uri.escape_string(project_id));
        var cards = new Gee.ArrayList<CardSummary>();
        do {
            var root = yield client.request_json("GET", path, null, query);
            cards.add_all(ApiParsersCards.parse_cards(root));
            var page = root.get_object_member("data");
            var cursor = ApiParsersCommon.nullable_string_member_or_null(page, "next_cursor");
            if (view == "recent" || cursor == null) break;
            query.insert("cursor", (!) cursor);
        } while (true);
        return cards;
    }

    public static async CardContextData get_card_context(ApiClient client, // LCOV_EXCL_BR_LINE: async declaration branch artifact
                                                         string project_id,
                                                         string? parent_card_id = null) throws Error {
        string? parent = parent_card_id != null && ((!) parent_card_id).strip().length > 0
            ? ((!) parent_card_id).strip() : null;
        var project_path = "/projects/%s".printf(Uri.escape_string(project_id));
        var project_root = yield client.request_json("GET", project_path, null, null);
        if (!project_root.has_member("data")) throw new ApiError.PROTOCOL("Missing project data");
        var project_data = project_root.get_object_member("data");
        if (!project_data.has_member("project_id") || !project_data.has_member("name")) {
            throw new ApiError.PROTOCOL("Missing project for cards context response");
        }
        var project = new CardContextProject(
            project_data.get_string_member("project_id"), project_data.get_string_member("name")
        );
        var breadcrumbs = new Gee.ArrayList<CardContextBreadcrumb>();
        if (parent != null) {
            var root = yield client.request_json(
                "GET", card_path(project_id, (!) parent) + "/breadcrumbs", null, null
            );
            if (!root.has_member("data")) throw new ApiError.PROTOCOL("Missing breadcrumbs data");
            var crumbs = root.get_array_member("data");
            for (uint i = 0; i < crumbs.get_length(); i++) {
                var crumb = crumbs.get_object_element(i);
                var kind = crumb.get_string_member("kind");
                var id = crumb.get_string_member("id");
                breadcrumbs.add(new CardContextBreadcrumb(
                    kind, crumb.get_string_member("title"), kind == "project" ? id : null,
                    kind == "card" ? id : null
                ));
            }
        } else {
            breadcrumbs.add(new CardContextBreadcrumb("project", project.name, project.project_id));
        }

        var cards = new Gee.ArrayList<CardContextCard>();
        var query = new HashTable<string, string>(str_hash, str_equal);
        query.insert("parent", parent != null ? (!) parent : "roots");
        query.insert("order", "sort_key_asc");
        query.insert("include_child_count", "true");
        do {
            var root = yield client.request_json("GET", project_path + "/cards", null, query);
            if (!root.has_member("data")) throw new ApiError.PROTOCOL("Missing cards data");
            var page = root.get_object_member("data");
            var items = page.get_array_member("items");
            for (uint i = 0; i < items.get_length(); i++) {
                var item = items.get_object_element(i);
                cards.add(new CardContextCard(
                    item.get_string_member("card_id"), item.get_string_member("project_id"),
                    item.get_string_member("title"), item.get_string_member("rel_path"),
                    item.get_double_member("sort_key"),
                    ApiParsersCommon.nullable_string_member_or_null(item, "parent_card_id"),
                    item.get_int_member("created_at"), item.get_int_member("updated_at"),
                    (int) item.get_int_member("child_count")
                ));
            }
            var cursor = ApiParsersCommon.nullable_string_member_or_null(page, "next_cursor");
            if (cursor == null) break;
            query.insert("cursor", (!) cursor);
        } while (true);
        return new CardContextData(project, parent, breadcrumbs, cards);
    }

    // A card's path, under its project.
    internal static string card_path(string project_id, string card_id) {
        return "/projects/%s/cards/%s".printf(Uri.escape_string(project_id), Uri.escape_string(card_id));
    }

    public static async CardDetail get_card(ApiClient client, string project_id, string card_id) throws Error { // LCOV_EXCL_BR_LINE: async declaration branch artifact
        var root = yield client.request_json("GET", card_path(project_id, card_id), null, null); // LCOV_EXCL_BR_LINE: yield resume edge artifact
        return ApiParsersCards.parse_card_detail(root); // LCOV_EXCL_BR_LINE: call/return branch artifact
    }

    public static async CardHistoryPage list_card_history(ApiClient client,
                                                          string project_id,
                                                          string card_id,
                                                          int limit = 50,
                                                          string? cursor = null) throws Error {
        var query = new HashTable<string, string>(str_hash, str_equal);
        query.insert("limit", limit.to_string());
        if (cursor != null && ((!) cursor).strip().length > 0) {
            query.insert("cursor", (!) cursor);
        }
        var root = yield client.request_json(
            "GET",
            "/projects/%s/history/cards/%s".printf(
                Uri.escape_string(project_id), Uri.escape_string(card_id)
            ),
            null,
            query
        );
        return ApiParsersCards.parse_card_history_page(root);
    }

    public static async CardHistoryComparison compare_card_history(ApiClient client,
                                                                   string project_id,
                                                                   string card_id,
                                                                   string? from_oid,
                                                                   string to_oid,
                                                                   string mode = "since") throws Error {
        var query = new HashTable<string, string>(str_hash, str_equal);
        if (from_oid != null && ((!) from_oid).strip().length > 0) {
            query.insert("from_oid", (!) from_oid);
        }
        query.insert("to_oid", to_oid);
        query.insert("mode", mode);
        var root = yield client.request_json(
            "GET",
            "/projects/%s/history/cards/%s/compare".printf(
                Uri.escape_string(project_id), Uri.escape_string(card_id)
            ),
            null,
            query
        );
        return ApiParsersCards.parse_card_history_comparison(root);
    }

    public static async bool restore_card_history(ApiClient client,
                                                  string project_id,
                                                  string card_id,
                                                  string oid) throws Error {
        var query = new HashTable<string, string>(str_hash, str_equal);
        query.insert("oid", oid);
        yield client.request_json(
            "POST",
            "/projects/%s/history/cards/%s/restore".printf(
                Uri.escape_string(project_id), Uri.escape_string(card_id)
            ),
            null,
            query
        );
        return true;
    }

    // The window as instants, and as the local days they fall on for milestones on calendar days.
    public static async ProjectCalendar get_project_calendar(ApiClient client,
                                                             string project_id,
                                                             int64 from_epoch,
                                                             int64 to_epoch) throws Error {
        var query = new HashTable<string, string>(str_hash, str_equal);
        query.insert("first_instant", from_epoch.to_string());
        query.insert("last_instant", to_epoch.to_string());
        query.insert("first_day", new DateTime.from_unix_local(from_epoch).format("%Y-%m-%d"));
        query.insert("last_day", new DateTime.from_unix_local(to_epoch).format("%Y-%m-%d"));
        var root = yield client.request_json(
            "GET", "/projects/%s/calendar".printf(Uri.escape_string(project_id)), null, query
        );
        return ApiParsersCards.parse_project_calendar(root, project_id, from_epoch, to_epoch);
    }

    private static string milestones_path(string project_id, string card_id) {
        return "/projects/%s/cards/%s/milestones".printf(
            Uri.escape_string(project_id), Uri.escape_string(card_id)
        );
    }

    public static async Gee.ArrayList<Milestone> list_card_milestones(ApiClient client,
                                                                      string project_id,
                                                                      string card_id) throws Error {
        var root = yield client.request_json(
            "GET", milestones_path(project_id, card_id), null, null
        );
        return ApiParsersCards.parse_milestones_response(root);
    }

    // A calendar day, as the local date of its midnight, or an instant in local time.
    private static void add_date(Json.Builder body, string role, int64 at, bool all_day) {
        var local = new DateTime.from_unix_local(at);
        body.begin_object();
        body.set_member_name("role"); body.add_string_value(role);
        body.set_member_name(all_day ? "date" : "datetime");
        body.add_string_value(local.format(all_day ? "%Y-%m-%d" : "%Y-%m-%dT%H:%M:%S%:z"));
        body.end_object();
    }

    // The milestone's dates with its start and end replaced, keeping every other date in place.
    private static void add_dates(Json.Builder body,
                                  Json.Array? kept,
                                  int64 start_at,
                                  int64? end_at,
                                  bool all_day) {
        body.set_member_name("dates");
        body.begin_array();
        add_date(body, "start", start_at, all_day);
        if (end_at != null) add_date(body, "end", (!) end_at, all_day);
        if (kept != null) {
            for (uint i = 0; i < ((!) kept).get_length(); i++) {
                var date = ((!) kept).get_element(i);
                var role = date.get_object().get_string_member("role");
                if (role != "start" && role != "end") body.add_value(date.copy());
            }
        }
        body.end_array();
    }

    private static void add_text(Json.Builder body, string name, string? value) {
        body.set_member_name(name);
        if (value == null || ((!) value).strip().length == 0) body.add_null_value();
        else body.add_string_value(((!) value).strip());
    }

    // A milestone to add, or a change to one whose dates are kept: its start, its end if it has
    // one, the kept dates other than a start or end, and its kind and description.
    public static string milestone_body(Json.Array? kept,
                                        int64 start_at,
                                        int64? end_at,
                                        bool all_day,
                                        string? kind,
                                        string? description) {
        var body = new Json.Builder();
        body.begin_object();
        add_dates(body, kept, start_at, end_at, all_day);
        add_text(body, "kind", kind);
        add_text(body, "description", description);
        body.end_object();
        var generator = new Json.Generator();
        generator.set_root(body.get_root());
        return generator.to_data(null);
    }

    public static async Milestone add_card_milestone(ApiClient client,
                                                     string project_id,
                                                     string card_id,
                                                     int64 start_at,
                                                     int64? end_at,
                                                     bool all_day,
                                                     string? kind,
                                                     string? description) throws Error {
        var root = yield client.request_json(
            "POST",
            milestones_path(project_id, card_id),
            milestone_body(null, start_at, end_at, all_day, kind, description),
            null
        );
        return ApiParsersCards.parse_milestone_response(root);
    }

    // Removing a milestone that is already gone changes nothing.
    public static async void remove_card_milestone(ApiClient client,
                                                   string project_id,
                                                   string card_id,
                                                   string milestone_id) throws Error {
        var root = yield client.request_json(
            "DELETE",
            "%s/%s".printf(milestones_path(project_id, card_id), Uri.escape_string(milestone_id)),
            null,
            null
        );
        if (!root.has_member("data")) {
            throw new ApiError.PROTOCOL("Missing data for milestone delete response");
        }
    }

    private static Json.Array? kept_dates(Milestone milestone) throws Error {
        if (milestone.dates_json == null) return null;
        var node = Json.from_string((!) milestone.dates_json);
        if (node == null || ((!) node).get_node_type() != Json.NodeType.ARRAY) return null;
        return ((!) node).get_array();
    }

    public static async Milestone update_card_milestone(ApiClient client,
                                                        Milestone milestone,
                                                        int64 start_at,
                                                        int64? end_at,
                                                        bool all_day,
                                                        string? kind,
                                                        string? description) throws Error {
        var root = yield client.request_json(
            "PATCH",
            "%s/%s".printf(
                milestones_path(milestone.project_id, milestone.card_id),
                Uri.escape_string(milestone.milestone_id)
            ),
            milestone_body(kept_dates(milestone), start_at, end_at, all_day, kind, description),
            null
        );
        return ApiParsersCards.parse_milestone_response(root);
    }

    public static async Gee.ArrayList<TagCount> list_project_tags(ApiClient client,
                                                                  string project_id) throws Error {
        var root = yield client.request_json(
            "GET",
            "/projects/%s/tags".printf(Uri.escape_string(project_id)),
            null,
            null
        );
        return ApiParsersCards.parse_project_tags(root);
    }

    public static async Gee.ArrayList<CardSummary> list_cards_with_tag(ApiClient client,
                                                                       string project_id,
                                                                       string tag) throws Error {
        var query = new HashTable<string, string>(str_hash, str_equal);
        query.insert("tag", tag);
        query.insert("order", "updated_desc");
        var cards = new Gee.ArrayList<CardSummary>();
        var path = "/projects/%s/cards".printf(Uri.escape_string(project_id));
        do {
            var root = yield client.request_json("GET", path, null, query);
            cards.add_all(ApiParsersCards.parse_cards(root));
            var cursor = ApiParsersCommon.nullable_string_member_or_null(
                root.get_object_member("data"), "next_cursor"
            );
            if (cursor == null) break;
            query.insert("cursor", (!) cursor);
        } while (true);
        return cards;
    }

    public static async Gee.ArrayList<CardLink> list_card_links(ApiClient client, // LCOV_EXCL_BR_LINE: async declaration branch artifact
                                                                string project_id,
                                                                string card_id) throws Error {
        var root = yield client.request_json( // LCOV_EXCL_BR_LINE: yield resume edge artifact
            "GET",
            card_path(project_id, card_id) + "/links",
            null,
            null
        );
        return ApiParsersCards.parse_card_links(root); // LCOV_EXCL_BR_LINE: call/return branch artifact
    }

    public static async Gee.ArrayList<CardLink> list_card_backlinks(ApiClient client, // LCOV_EXCL_BR_LINE: async declaration branch artifact
                                                                    string project_id,
                                                                    string card_id) throws Error {
        var root = yield client.request_json( // LCOV_EXCL_BR_LINE: yield resume edge artifact
            "GET",
            card_path(project_id, card_id) + "/backlinks",
            null,
            null
        );
        return ApiParsersCards.parse_card_links(root); // LCOV_EXCL_BR_LINE: call/return branch artifact
    }

    public static async CardLink create_card_link(ApiClient client, // LCOV_EXCL_BR_LINE: async declaration branch artifact
                                                  string project_id,
                                                  string from_card_id,
                                                  string to_card_id,
                                                  string kind = "ref",
                                                  string? label = null,
                                                  string to_type = "card") throws Error {
        var body = new Json.Builder();
        body.begin_object();
        body.set_member_name("target_type");
        body.add_string_value(to_type != null && to_type.length > 0 ? to_type : "card"); // LCOV_EXCL_BR_LINE: short-circuit branch artifact
        body.set_member_name("target_id");
        body.add_string_value(to_card_id);
        if (kind != null && kind.strip().length > 0) { // LCOV_EXCL_BR_LINE: short-circuit branch artifact
            body.set_member_name("kind");
            body.add_string_value(kind.strip());
        }
        if (label != null && label.strip().length > 0) {
            body.set_member_name("label");
            body.add_string_value(label.strip());
        }
        body.end_object();

        var root = yield client.request_json( // LCOV_EXCL_BR_LINE: yield resume edge artifact
            "POST",
            card_path(project_id, from_card_id) + "/links",
            client.json_string_from_builder(body),
            null
        );
        if (!root.has_member("data")) {
            throw new ApiError.PROTOCOL("Missing data for card link create response"); // LCOV_EXCL_BR_LINE: throw edge artifact
        }
        return ApiParsersCards.parse_card_link(root.get_object_member("data")); // LCOV_EXCL_BR_LINE: call/return branch artifact
    }

    // Removes the one link to that target, of that type in that project, with that kind. Without a
    // target project, the target is in the card's own project.
    public static async void delete_card_link(ApiClient client, // LCOV_EXCL_BR_LINE: async declaration branch artifact
                                              string project_id,
                                              string from_card_id,
                                              string to_card_id,
                                              string kind,
                                              string to_type = "card",
                                              string? target_project_id = null) throws Error {
        var query = new HashTable<string, string>(str_hash, str_equal);
        query.insert("target_type", to_type != null && to_type.length > 0 ? to_type : "card"); // LCOV_EXCL_BR_LINE: short-circuit branch artifact
        query.insert("target_id", to_card_id);
        if (kind != null && kind.strip().length > 0) { // LCOV_EXCL_BR_LINE: short-circuit branch artifact
            query.insert("kind", kind.strip());
        }
        if (target_project_id != null && target_project_id.length > 0 && target_project_id != project_id) { // LCOV_EXCL_BR_LINE: short-circuit branch artifact
            query.insert("target_project_id", target_project_id);
        }
        yield client.request_json("DELETE", card_path(project_id, from_card_id) + "/links", null, query); // LCOV_EXCL_BR_LINE: yield resume edge artifact
    }

    public static async string create_card(ApiClient client, // LCOV_EXCL_BR_LINE: async declaration branch artifact
                                           string project_id,
                                           string title,
                                           string content,
                                           string? parent_card_id = null) throws Error {
        var body = new Json.Builder();
        body.begin_object();
        body.set_member_name("title");
        body.add_string_value(title);
        body.set_member_name("content");
        body.add_string_value(content);
        if (parent_card_id != null && parent_card_id.strip().length > 0) {
            body.set_member_name("parent_card_id");
            body.add_string_value(parent_card_id);
        }
        body.end_object();

        var root = yield client.request_json( // LCOV_EXCL_BR_LINE: yield resume edge artifact
            "POST",
            "/projects/%s/cards".printf(Uri.escape_string(project_id)),
            client.json_string_from_builder(body),
            null
        );
        var data = root.get_object_member("data"); // LCOV_EXCL_BR_LINE: invalid-type branch artifact
        return data.get_string_member("card_id"); // LCOV_EXCL_BR_LINE: call/return branch artifact
    }

    public static async void update_card(ApiClient client, // LCOV_EXCL_BR_LINE: async declaration branch artifact
                                         string project_id,
                                         string card_id,
                                         string title,
                                         string content,
                                         int64 updated_at) throws Error {
        var body = new Json.Builder();
        body.begin_object();
        body.set_member_name("title");
        body.add_string_value(title);
        body.set_member_name("content");
        body.add_string_value(content);
        body.set_member_name("updated_at");
        body.add_int_value(updated_at);
        body.end_object();

        yield client.request_json( // LCOV_EXCL_BR_LINE: yield resume edge artifact
            "PATCH",
            card_path(project_id, card_id),
            client.json_string_from_builder(body),
            null
        );
    }

    public static async void update_card_position(ApiClient client, // LCOV_EXCL_BR_LINE: async declaration branch artifact
                                                  string project_id,
                                                  string card_id,
                                                  string? parent_card_id,
                                                  double sort_key,
                                                  int64 updated_at) throws Error {
        var body = new Json.Builder();
        body.begin_object();
        body.set_member_name("parent_card_id");
        if (parent_card_id == null || parent_card_id.length == 0) {
            body.add_null_value();
        } else {
            body.add_string_value(parent_card_id);
        }
        body.set_member_name("sort_key");
        body.add_double_value(sort_key);
        body.set_member_name("updated_at");
        body.add_int_value(updated_at);
        body.end_object();

        yield client.request_json( // LCOV_EXCL_BR_LINE: yield resume edge artifact
            "PATCH",
            card_path(project_id, card_id),
            client.json_string_from_builder(body),
            null
        );
    }

    internal static string delete_card_path(string project_id, string card_id, bool trash_subtree) {
        var path = card_path(project_id, card_id);
        return trash_subtree ? path + "?children=trash_subtree" : path;
    }

    public static async void delete_card(ApiClient client, // LCOV_EXCL_BR_LINE: async declaration branch artifact
                                         string project_id,
                                         string card_id,
                                         bool trash_subtree = false) throws Error {
        yield client.request_json(
            "DELETE",
            delete_card_path(project_id, card_id, trash_subtree),
            null,
            null
        );
    }

    // Without a parent_card_id the daemon keeps the card's current parent. The field is left out
    // rather than sent as null, which the daemon reads as the project's top level.
    internal static string build_move_card_body_text(string intent,
                                                     string? target_card_id,
                                                     string? parent_card_id) {
        var body = new Json.Builder();
        body.begin_object();
        body.set_member_name("intent");
        body.add_string_value(intent);
        if (target_card_id != null && target_card_id.strip().length > 0) {
            body.set_member_name("target_card_id");
            body.add_string_value(target_card_id);
        }
        if ((intent == "to_start" || intent == "to_end") &&
            parent_card_id != null && parent_card_id.strip().length > 0) {
            body.set_member_name("parent_card_id");
            body.add_string_value(parent_card_id);
        }
        body.end_object();
        return ApiClientTransport.json_string_from_builder(body);
    }

    public static async CardMoveResult move_card(ApiClient client, // LCOV_EXCL_BR_LINE: async declaration branch artifact
                                                 string project_id,
                                                 string card_id,
                                                 string intent,
                                                 string? target_card_id = null,
                                                 string? parent_card_id = null) throws Error {
        var root = yield client.request_json( // LCOV_EXCL_BR_LINE: yield resume edge artifact
            "POST",
            card_path(project_id, card_id) + "/move",
            build_move_card_body_text(intent, target_card_id, parent_card_id),
            null
        );
        if (!root.has_member("data")) {
            throw new ApiError.PROTOCOL("Missing data for move response"); // LCOV_EXCL_BR_LINE: throw edge artifact
        }
        return ApiParsersCards.parse_card_move_result(root.get_object_member("data")); // LCOV_EXCL_BR_LINE: call/return branch artifact
    }
}

}
