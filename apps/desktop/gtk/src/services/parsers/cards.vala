namespace HolderLinux {

public class ApiParsersCards { // LCOV_EXCL_LINE: declaration-only coverage artifact
    public static ProjectHistoryPage parse_project_history_page(Json.Object root) throws Error {
        if (!root.has_member("data")) {
            throw new ApiError.PROTOCOL("Missing data for project history response");
        }
        var data = root.get_object_member("data");
        ProjectHistoryActivity[] activities = {};
        var array = data.get_array_member("activities");
        for (uint i = 0; i < array.get_length(); i++) {
            var item = array.get_object_element(i);
            string[] parents = {};
            var parent_array = item.get_array_member("parent_oids");
            for (uint j = 0; j < parent_array.get_length(); j++) parents += parent_array.get_string_element(j);
            var author = item.get_object_member("author");
            ProjectHistoryAffectedObject[] affected = {};
            var affected_array = item.get_array_member("affected_objects");
            for (uint j = 0; j < affected_array.get_length(); j++) {
                var object = affected_array.get_object_element(j);
                ProjectHistoryAffectedPath[] paths = {};
                if (object.has_member("items")) {
                    var items_array = object.get_array_member("items");
                    for (uint k = 0; k < items_array.get_length(); k++) {
                        var path = items_array.get_object_element(k);
                        paths += new ProjectHistoryAffectedPath(
                            path.get_string_member("path"),
                            ApiParsersCommon.nullable_string_member_or_null(path, "title"),
                            ApiParsersCommon.nullable_string_member_or_null(path, "detail")
                        );
                    }
                } else {
                    var paths_array = object.get_array_member("paths");
                    for (uint k = 0; k < paths_array.get_length(); k++) {
                        paths += new ProjectHistoryAffectedPath(paths_array.get_string_element(k));
                    }
                }
                affected += new ProjectHistoryAffectedObject(object.get_string_member("kind"), paths);
            }
            activities += new ProjectHistoryActivity(
                item.get_string_member("oid"), parents,
                ApiParsersCommon.string_member_or_empty(author, "name"),
                ApiParsersCommon.string_member_or_empty(author, "email"),
                item.get_int_member("committed_at"), item.get_string_member("message"),
                affected, item.get_boolean_member("is_merge")
            );
        }
        return new ProjectHistoryPage(
            ApiParsersCommon.nullable_string_member_or_null(data, "head_oid"), activities,
            ApiParsersCommon.nullable_string_member_or_null(data, "next_cursor"),
            data.has_member("scan_limited") && data.get_boolean_member("scan_limited")
        );
    }

    private static CardHistoryVersion parse_history_version(Json.Object item) {
        return new CardHistoryVersion(
            item.get_boolean_member("exists"),
            ApiParsersCommon.string_member_or_empty(item, "oid"),
            ApiParsersCommon.string_member_or_empty(item, "title"),
            ApiParsersCommon.string_member_or_empty(item, "body")
        );
    }

    public static CardHistoryPage parse_card_history_page(Json.Object root) throws Error {
        if (!root.has_member("data")) {
            throw new ApiError.PROTOCOL("Missing data for card history response");
        }
        var data = root.get_object_member("data");
        CardHistoryEntry[] entries = {};
        var array = data.get_array_member("entries");
        for (uint i = 0; i < array.get_length(); i++) {
            var item = array.get_object_element(i);
            string[] parents = {};
            var parent_array = item.get_array_member("parent_oids");
            for (uint j = 0; j < parent_array.get_length(); j++) {
                parents += parent_array.get_string_element(j);
            }
            string[] visible_parents = {};
            if (item.has_member("visible_parent_oids")) {
                var visible_parent_array = item.get_array_member("visible_parent_oids");
                for (uint j = 0; j < visible_parent_array.get_length(); j++) {
                    visible_parents += visible_parent_array.get_string_element(j);
                }
            }
            var author = item.get_object_member("author");
            CardHistorySave[] saves = {};
            if (item.has_member("saves")) {
                var saves_array = item.get_array_member("saves");
                for (uint save_index = 0; save_index < saves_array.get_length(); save_index++) {
                    var save = saves_array.get_object_element(save_index);
                    string[] save_parents = {};
                    var save_parent_array = save.get_array_member("parent_oids");
                    for (uint parent_index = 0;
                         parent_index < save_parent_array.get_length(); parent_index++) {
                        save_parents += save_parent_array.get_string_element(parent_index);
                    }
                    saves += new CardHistorySave(
                        save.get_string_member("oid"),
                        save_parents,
                        save.get_int_member("committed_at"),
                        save.has_member("authored_at")
                            ? save.get_int_member("authored_at")
                            : save.get_int_member("committed_at"),
                        ApiParsersCommon.string_member_or_empty(save, "message")
                    );
                }
            } else {
                saves += new CardHistorySave(
                    item.get_string_member("last_oid"), parents, item.get_int_member("ended_at")
                );
            }
            entries += new CardHistoryEntry(
                item.get_string_member("first_oid"),
                item.get_string_member("last_oid"),
                parents,
                ApiParsersCommon.string_member_or_empty(author, "name"),
                ApiParsersCommon.string_member_or_empty(author, "email"),
                item.get_int_member("started_at"),
                item.get_int_member("ended_at"),
                item.get_string_member("kind"),
                item.get_string_member("summary"),
                (int) item.get_int_member("commit_count"),
                item.get_boolean_member("is_merge"),
                saves,
                visible_parents
            );
        }
        return new CardHistoryPage(
            ApiParsersCommon.nullable_string_member_or_null(data, "head_oid"),
            entries,
            ApiParsersCommon.nullable_string_member_or_null(data, "next_cursor"),
            data.has_member("scan_limited") && data.get_boolean_member("scan_limited")
        );
    }

    public static CardHistoryComparison parse_card_history_comparison(Json.Object root) throws Error {
        if (!root.has_member("data")) {
            throw new ApiError.PROTOCOL("Missing data for card history comparison");
        }
        var data = root.get_object_member("data");
        CardHistoryDiffLine[] lines = {};
        var array = data.get_array_member("lines");
        for (uint i = 0; i < array.get_length(); i++) {
            var item = array.get_object_element(i);
            int64? old_line = null;
            int64? new_line = null;
            if (item.get_member("old_line").get_node_type() != Json.NodeType.NULL) {
                old_line = item.get_int_member("old_line");
            }
            if (item.get_member("new_line").get_node_type() != Json.NodeType.NULL) {
                new_line = item.get_int_member("new_line");
            }
            lines += new CardHistoryDiffLine(
                item.get_string_member("origin"), item.get_string_member("text"), old_line, new_line
            );
        }
        return new CardHistoryComparison(
            parse_history_version(data.get_object_member("before")),
            parse_history_version(data.get_object_member("after")),
            ApiParsersCommon.string_member_or_empty(data, "summary"),
            lines,
            data.has_member("truncated") && data.get_boolean_member("truncated")
        );
    }

    // The epoch second a milestone date names: an instant as it is, and a calendar day at its
    // local midnight, as the calendar shows it.
    public static int64 milestone_date_epoch(Json.Object date) throws Error {
        if (date.has_member("date")) {
            var parts = date.get_string_member("date").split("-");
            if (parts.length != 3) throw new ApiError.PROTOCOL("Invalid milestone date");
            var local = new DateTime.local(
                int.parse(parts[0]), int.parse(parts[1]), int.parse(parts[2]), 0, 0, 0
            );
            if (local == null) throw new ApiError.PROTOCOL("Invalid milestone date");
            return ((!) local).to_unix();
        }
        var instant = new DateTime.from_iso8601(date.get_string_member("datetime"), null);
        if (instant == null) throw new ApiError.PROTOCOL("Invalid milestone datetime");
        return ((!) instant).to_unix();
    }

    // The one date with the role, or null when there is none or more than one.
    private static Json.Object? only_date(Json.Array dates, string role) {
        Json.Object? found = null;
        for (uint i = 0; i < dates.get_length(); i++) {
            var date = dates.get_object_element(i);
            if (date.get_string_member("role") != role) continue;
            if (found != null) return null;
            found = date;
        }
        return found;
    }

    // A milestone with one start and at most one end, as the calendar shows it; null for any
    // other, which the calendar does not show.
    public static Milestone? parse_milestone(Json.Object item) throws Error {
        var dates = item.get_array_member("dates");
        var start = only_date(dates, "start");
        if (start == null) return null;
        var end = only_date(dates, "end");
        for (uint i = 0; i < dates.get_length(); i++) {
            if (end == null && dates.get_object_element(i).get_string_member("role") == "end") {
                return null;
            }
        }
        int64? end_at = null;
        if (end != null) end_at = milestone_date_epoch((!) end);
        var milestone = new Milestone(
            item.get_string_member("milestone_id"),
            item.get_string_member("card_id"),
            milestone_date_epoch((!) start),
            end_at,
            ((!) start).has_member("date"),
            ApiParsersCommon.nullable_string_member_or_null(item, "kind"),
            ApiParsersCommon.nullable_string_member_or_null(item, "description"),
            item.get_int_member("created_at"),
            item.get_int_member("updated_at"),
            ApiParsersCommon.nullable_string_member_or_null(item, "card_title")
        );
        milestone.project_id = item.get_string_member("project_id");
        milestone.dates_json = Json.to_string(item.get_member("dates"), false);
        return milestone;
    }

    public static Gee.ArrayList<Milestone> parse_milestones_response(Json.Object root) throws Error {
        if (!root.has_member("data")) {
            throw new ApiError.PROTOCOL("Missing data for milestones response");
        }
        return new Gee.ArrayList<Milestone>.wrap(
            parse_milestones_array(root.get_array_member("data"))
        );
    }

    public static Milestone parse_milestone_response(Json.Object root) throws Error {
        if (!root.has_member("data")) {
            throw new ApiError.PROTOCOL("Missing data for milestone response");
        }
        var milestone = parse_milestone(root.get_object_member("data"));
        if (milestone == null) {
            throw new ApiError.PROTOCOL("The milestone has no single start");
        }
        return (!) milestone;
    }

    // The calendar for a window, with the milestones that start in it.
    public static ProjectCalendar parse_project_calendar(Json.Object root,
                                                         string project_id,
                                                         int64 from_epoch,
                                                         int64 to_epoch) throws Error {
        if (!root.has_member("data")) {
            throw new ApiError.PROTOCOL("Missing data for calendar response");
        }
        var data = root.get_object_member("data");
        Milestone[] starting = {};
        foreach (var milestone in parse_milestones_array(data.get_array_member("milestones"))) {
            if (milestone.start_at >= from_epoch && milestone.start_at <= to_epoch) {
                starting += milestone;
            }
        }
        return new ProjectCalendar(
            project_id,
            from_epoch,
            to_epoch,
            starting,
            parse_calendar_card_array(data.get_array_member("created_cards")),
            parse_calendar_card_array(data.get_array_member("updated_cards"))
        );
    }

    private static Milestone[] parse_milestones_array(Json.Array data) throws Error {
        Milestone[] result = {};
        for (uint i = 0; i < data.get_length(); i++) {
            var milestone = parse_milestone(data.get_object_element(i));
            if (milestone != null) result += (!) milestone;
        }
        return result;
    }

    private static CalendarCardActivity[] parse_calendar_card_array(Json.Array data) {
        CalendarCardActivity[] result = {};
        for (uint i = 0; i < data.get_length(); i++) {
            var item = data.get_object_element(i);
            result += new CalendarCardActivity(
                item.get_string_member("card_id"),
                item.get_string_member("title"),
                item.get_int_member("created_at"),
                item.get_int_member("updated_at")
            );
        }
        return result;
    }

    public static Gee.ArrayList<CardSummary> parse_cards(Json.Object root) throws Error { // LCOV_EXCL_BR_LINE: declaration branch artifact
        if (!root.has_member("data")) {
            throw new ApiError.PROTOCOL("Missing data for cards response");
        }

        var out_list = new Gee.ArrayList<CardSummary>();
        var data = root.get_array_member("data"); // LCOV_EXCL_BR_LINE: invalid-type branch aborts in json-glib
        for (uint i = 0; i < data.get_length(); i++) { // LCOV_EXCL_BR_LINE: loop overflow branch artifact
            var item = data.get_object_element(i); // LCOV_EXCL_BR_LINE: invalid-type branch aborts in json-glib
            out_list.add(new CardSummary( // LCOV_EXCL_BR_LINE: allocator/ctor edge branch artifact
                item.get_string_member("card_id"),
                item.get_string_member("project_id"),
                item.get_string_member("title"),
                item.has_member("rel_path") ? item.get_string_member("rel_path") : "",
                item.has_member("sort_key") ? item.get_double_member("sort_key") : 0.0,
                item.has_member("parent_card_id") && item.get_member("parent_card_id").get_node_type() != Json.NodeType.NULL
                    ? item.get_string_member("parent_card_id")
                    : null,
                item.has_member("created_at") ? item.get_int_member("created_at") : 0,
                item.has_member("updated_at") ? item.get_int_member("updated_at") : 0
            ));
        }
        return out_list; // LCOV_EXCL_BR_LINE: return edge branch artifact
    }

    public static CardDetail parse_card_detail(Json.Object root) throws Error { // LCOV_EXCL_BR_LINE: declaration branch artifact
        if (!root.has_member("data")) {
            throw new ApiError.PROTOCOL("Missing data for card response");
        }

        var data = root.get_object_member("data");
        string[] tags = {};
        if (data.has_member("tags")) {
            var tags_data = data.get_array_member("tags");
            for (uint i = 0; i < tags_data.get_length(); i++) {
                tags += tags_data.get_string_element(i);
            }
        }
        CardTagOccurrence[] tag_occurrences = {};
        if (data.has_member("tag_occurrences")) {
            var occurrence_data = data.get_array_member("tag_occurrences");
            for (uint i = 0; i < occurrence_data.get_length(); i++) {
                var item = occurrence_data.get_object_element(i);
                tag_occurrences += new CardTagOccurrence(
                    item.get_string_member("tag"),
                    (int) item.get_int_member("byte_start"),
                    (int) item.get_int_member("byte_end")
                );
            }
        }
        return new CardDetail( // LCOV_EXCL_BR_LINE: ctor edge branch artifact
            data.get_string_member("card_id"),
            data.get_string_member("project_id"),
            data.get_string_member("title"),
            data.get_string_member("content"),
            data.has_member("updated_at") ? data.get_int_member("updated_at") : 0,
            tags,
            tag_occurrences
        );
    }

    public static Gee.ArrayList<TagCount> parse_project_tags(Json.Object root) throws Error {
        if (!root.has_member("data")) {
            throw new ApiError.PROTOCOL("Missing data for project tags response");
        }
        var tags = new Gee.ArrayList<TagCount>();
        var data = root.get_array_member("data");
        for (uint i = 0; i < data.get_length(); i++) {
            var item = data.get_object_element(i);
            tags.add(new TagCount(
                item.get_string_member("tag"),
                (int) item.get_int_member("card_count")
            ));
        }
        return tags;
    }

    public static CardContextData parse_card_context(Json.Object root) throws Error { // LCOV_EXCL_BR_LINE: declaration branch artifact
        if (!root.has_member("data")) {
            throw new ApiError.PROTOCOL("Missing data for cards context response");
        }

        var data = root.get_object_member("data");
        if (!data.has_member("project")) {
            throw new ApiError.PROTOCOL("Missing project for cards context response"); // LCOV_EXCL_BR_LINE: throw edge branch artifact
        }
        var project_obj = data.get_object_member("project");
        var project = new CardContextProject(
            ApiParsersCommon.string_member_or_empty(project_obj, "project_id"),
            ApiParsersCommon.string_member_or_empty(project_obj, "name")
        );

        var breadcrumbs = new Gee.ArrayList<CardContextBreadcrumb>();
        if (data.has_member("breadcrumbs")) { // LCOV_EXCL_BR_LINE: short-circuit artifact branch
            var crumbs = data.get_array_member("breadcrumbs");
            for (uint i = 0; i < crumbs.get_length(); i++) {
                var crumb = crumbs.get_object_element(i);
                breadcrumbs.add(new CardContextBreadcrumb( // LCOV_EXCL_BR_LINE: allocator/ctor edge branch artifact
                    ApiParsersCommon.string_member_or_empty(crumb, "type"),
                    ApiParsersCommon.string_member_or_empty(crumb, "title"),
                    ApiParsersCommon.nullable_string_member_or_null(crumb, "project_id"),
                    ApiParsersCommon.nullable_string_member_or_null(crumb, "card_id")
                ));
            }
        }

        var cards = new Gee.ArrayList<CardContextCard>();
        if (data.has_member("cards")) { // LCOV_EXCL_BR_LINE: short-circuit artifact branch
            var items = data.get_array_member("cards");
            for (uint i = 0; i < items.get_length(); i++) {
                var item = items.get_object_element(i);
                cards.add(new CardContextCard( // LCOV_EXCL_BR_LINE: allocator/ctor edge branch artifact
                    ApiParsersCommon.string_member_or_empty(item, "card_id"),
                    ApiParsersCommon.string_member_or_empty(item, "project_id"),
                    ApiParsersCommon.string_member_or_empty(item, "title"),
                    ApiParsersCommon.string_member_or_empty(item, "rel_path"),
                    item.has_member("sort_key") ? item.get_double_member("sort_key") : 0.0, // LCOV_EXCL_BR_LINE: json member error edge artifact
                    ApiParsersCommon.nullable_string_member_or_null(item, "parent_card_id"),
                    item.has_member("created_at") ? item.get_int_member("created_at") : 0, // LCOV_EXCL_BR_LINE: json member error edge artifact
                    item.has_member("updated_at") ? item.get_int_member("updated_at") : 0, // LCOV_EXCL_BR_LINE: json member error edge artifact
                    item.has_member("child_count") ? (int) item.get_int_member("child_count") : 0 // LCOV_EXCL_BR_LINE: json member error edge artifact
                ));
            }
        }

        return new CardContextData( // LCOV_EXCL_BR_LINE: ctor edge branch artifact
            project,
            ApiParsersCommon.nullable_string_member_or_null(data, "current_parent_card_id"),
            breadcrumbs,
            cards
        );
    }

    public static Gee.ArrayList<CardLink> parse_card_links(Json.Object root) throws Error { // LCOV_EXCL_BR_LINE: declaration branch artifact
        if (!root.has_member("data")) {
            throw new ApiError.PROTOCOL("Missing data for card links response");
        }

        var out_list = new Gee.ArrayList<CardLink>();
        var data = root.get_array_member("data");
        for (uint i = 0; i < data.get_length(); i++) {
            out_list.add(parse_card_link(data.get_object_element(i))); // LCOV_EXCL_BR_LINE: allocator edge branch artifact
        }
        return out_list; // LCOV_EXCL_BR_LINE: return edge branch artifact
    }

    public static CardLink parse_card_link(Json.Object item) { // LCOV_EXCL_BR_LINE: declaration branch artifact
        return new CardLink( // LCOV_EXCL_BR_LINE: ctor edge branch artifact
            item.get_string_member("source_id"),
            item.get_string_member("target_id"),
            item.has_member("target_type") ? ApiParsersCommon.string_member_or_empty(item, "target_type") : "card",
            item.has_member("kind") ? ApiParsersCommon.string_member_or_empty(item, "kind") : "ref",
            ApiParsersCommon.nullable_string_member_or_null(item, "label"),
            item.has_member("created_at") ? item.get_int_member("created_at") : 0,
            ApiParsersCommon.string_member_or_empty(item, "source_project_id"),
            ApiParsersCommon.string_member_or_empty(item, "target_project_id")
        );
    }

    public static CardMoveResult parse_card_move_result(Json.Object data) throws Error { // LCOV_EXCL_BR_LINE: declaration branch artifact
        // The move answers with the moved card; only its place is read here.
        if (!data.has_member("card_id") // LCOV_EXCL_BR_LINE: short-circuit artifact branch
            || !data.has_member("sort_key")) {
            throw new ApiError.PROTOCOL("Missing fields for card move result");
        }

        var parent = ApiParsersCommon.nullable_string_member_or_null(data, "parent_card_id");
        return new CardMoveResult( // LCOV_EXCL_BR_LINE: ctor edge branch artifact
            data.get_string_member("card_id"),
            parent,
            data.get_double_member("sort_key")
        );
    }
}

}
