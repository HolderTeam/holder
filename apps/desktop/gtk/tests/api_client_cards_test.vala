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

private HolderLinux.ApiClient make_client(FakeApiHttpTransport transport) {
    return new HolderLinux.ApiClient("http://127.0.0.1:8080", "token-123", transport);
}

private void test_list_cards_and_context_and_get_card() {
    var transport = new FakeApiHttpTransport();
    transport.enqueue_read(
        200,
        "{\"ok\":true,\"data\":{\"items\":[{\"card_id\":\"c1\",\"project_id\":\"p1\",\"title\":\"Card One\",\"rel_path\":\"cards/c1.md\",\"sort_key\":1.0,\"parent_card_id\":null,\"created_at\":1,\"updated_at\":2}],\"next_cursor\":null}}"
    );
    transport.enqueue_read(
        200,
        "{\"ok\":true,\"data\":{\"project_id\":\"p1\",\"name\":\"Project One\"}}"
    );
    transport.enqueue_read(
        200,
        "{\"ok\":true,\"data\":{\"items\":[{\"card_id\":\"c1\",\"project_id\":\"p1\",\"title\":\"Card One\",\"rel_path\":\"cards/c1.md\",\"sort_key\":1.0,\"parent_card_id\":null,\"created_at\":1,\"updated_at\":2,\"child_count\":0}],\"next_cursor\":null}}"
    );
    transport.enqueue_read(
        200,
        "{\"ok\":true,\"data\":{\"card_id\":\"c1\",\"project_id\":\"p1\",\"title\":\"Card One\",\"content\":\"Hello\",\"updated_at\":10}}"
    );
    var client = make_client(transport);

    bool done_cards = false;
    Gee.ArrayList<HolderLinux.CardSummary>? cards = null;
    client.list_cards.begin("p1", "tree", null, 10, (obj, res) => {
        try { cards = client.list_cards.end(res); } catch (Error e) { cards = null; }
        done_cards = true;
    });
    assert(wait_for_condition(() => done_cards));
    assert(cards != null);
    assert(cards.size == 1);
    assert(cards[0].card_id == "c1");
    assert(transport.last_uri.contains("/projects/p1/cards"));
    assert(transport.last_uri.contains("parent=roots"));
    assert(transport.last_uri.contains("limit=10"));

    bool done_ctx = false;
    HolderLinux.CardContextData? ctx = null;
    client.get_card_context.begin("p1", null, (obj, res) => {
        try { ctx = client.get_card_context.end(res); } catch (Error e) { ctx = null; }
        done_ctx = true;
    });
    assert(wait_for_condition(() => done_ctx));
    assert(ctx != null);
    assert(ctx.project.project_id == "p1");
    assert(ctx.cards.size == 1);
    assert(transport.last_uri.contains("/projects/p1/cards"));
    assert(transport.last_uri.contains("include_child_count=true"));

    bool done_card = false;
    HolderLinux.CardDetail? card = null;
    client.get_card.begin("p1", "c1", (obj, res) => {
        try { card = client.get_card.end(res); } catch (Error e) { card = null; }
        done_card = true;
    });
    assert(wait_for_condition(() => done_card));
    assert(card != null);
    assert(transport.last_uri.contains("/projects/p1/cards/c1"));
    assert(card.card_id == "c1");
    assert(card.content == "Hello");
}

private void test_tree_listing_follows_cursor() {
    var transport = new FakeApiHttpTransport();
    transport.enqueue_read(200,
        "{\"ok\":true,\"data\":{\"items\":[{\"card_id\":\"c1\",\"project_id\":\"p1\",\"title\":\"One\"}],\"next_cursor\":\"page two\"}}");
    transport.enqueue_read(200,
        "{\"ok\":true,\"data\":{\"items\":[{\"card_id\":\"c2\",\"project_id\":\"p1\",\"title\":\"Two\"}],\"next_cursor\":null}}");
    var client = make_client(transport);
    bool done = false;
    Gee.ArrayList<HolderLinux.CardSummary>? cards = null;
    client.list_cards.begin("p1", "tree", null, 0, (obj, res) => {
        try { cards = client.list_cards.end(res); } catch (Error e) { cards = null; }
        done = true;
    });
    assert(wait_for_condition(() => done));
    assert(cards != null && cards.size == 2);
    assert(cards[1].card_id == "c2");
    assert(transport.last_uri.contains("cursor=page%20two"));
}

private void test_links_create_delete_and_list() {
    var transport = new FakeApiHttpTransport();
    transport.enqueue_read(
        200,
        "{\"ok\":true,\"data\":[{\"source_project_id\":\"p1\",\"source_id\":\"c1\",\"target_project_id\":\"p1\",\"target_id\":\"c2\",\"target_type\":\"card\",\"kind\":\"ref\",\"label\":\"L\",\"created_at\":1}]}"
    );
    transport.enqueue_read(
        200,
        "{\"ok\":true,\"data\":[{\"source_project_id\":\"p1\",\"source_id\":\"c2\",\"target_project_id\":\"p1\",\"target_id\":\"c1\",\"target_type\":\"card\",\"kind\":\"ref\",\"label\":\"B\",\"created_at\":2}]}"
    );
    transport.enqueue_read(
        200,
        "{\"ok\":true,\"data\":{\"source_project_id\":\"p1\",\"source_id\":\"c1\",\"target_project_id\":\"p1\",\"target_id\":\"r1\",\"target_type\":\"resource\",\"kind\":\"depends_on\",\"label\":\"critical\",\"created_at\":3}}"
    );
    transport.enqueue_read(200, "{\"ok\":true,\"data\":{}}");
    var client = make_client(transport);

    bool done_links = false;
    Gee.ArrayList<HolderLinux.CardLink>? links = null;
    client.list_card_links.begin("p1", "c1", (obj, res) => {
        try { links = client.list_card_links.end(res); } catch (Error e) { links = null; }
        done_links = true;
    });
    assert(wait_for_condition(() => done_links));
    assert(links != null);
    assert(links.size == 1);
    assert(links[0].to_card_id == "c2");

    bool done_back = false;
    Gee.ArrayList<HolderLinux.CardLink>? backlinks = null;
    client.list_card_backlinks.begin("p1", "c1", (obj, res) => {
        try { backlinks = client.list_card_backlinks.end(res); } catch (Error e) { backlinks = null; }
        done_back = true;
    });
    assert(wait_for_condition(() => done_back));
    assert(backlinks != null);
    assert(backlinks.size == 1);
    assert(backlinks[0].from_card_id == "c2");

    bool done_create = false;
    HolderLinux.CardLink? created = null;
    client.create_card_link.begin("p1", "c1", "r1", "depends_on", " critical ", "resource", (obj, res) => {
        try { created = client.create_card_link.end(res); } catch (Error e) { created = null; }
        done_create = true;
    });
    assert(wait_for_condition(() => done_create));
    assert(created != null);
    assert(created.to_type == "resource");
    assert(created.kind == "depends_on");
    assert(created.label == "critical");
    assert(transport.last_method == "POST");
    assert(transport.last_uri.contains("/projects/p1/cards/c1/links"));

    bool done_delete = false;
    bool ok_delete = false;
    client.delete_card_link.begin("p1", "c1", "r1", " depends_on ", "resource", null, (obj, res) => {
        try { client.delete_card_link.end(res); ok_delete = true; } catch (Error e) { ok_delete = false; }
        done_delete = true;
    });
    assert(wait_for_condition(() => done_delete));
    assert(ok_delete);
    assert(transport.last_method == "DELETE");
}

private void test_create_link_missing_data_and_move_missing_data_are_protocol_errors() {
    var transport = new FakeApiHttpTransport();
    transport.enqueue_read(200, "{\"ok\":true}");
    transport.enqueue_read(200, "{\"ok\":true}");
    var client = make_client(transport);

    bool done_link = false;
    bool got_link_protocol = false;
    client.create_card_link.begin("p1", "c1", "c2", "ref", null, "card", (obj, res) => {
        try { client.create_card_link.end(res); } catch (Error e) { got_link_protocol = (e is HolderLinux.ApiError.PROTOCOL); }
        done_link = true;
    });
    assert(wait_for_condition(() => done_link));
    assert(got_link_protocol);

    bool done_move = false;
    bool got_move_protocol = false;
    client.move_card.begin("p1", "c1", "before", "c2", null, (obj, res) => {
        try { client.move_card.end(res); } catch (Error e) { got_move_protocol = (e is HolderLinux.ApiError.PROTOCOL); }
        done_move = true;
    });
    assert(wait_for_condition(() => done_move));
    assert(got_move_protocol);
}

private void test_create_update_position_and_move_card_success() {
    var transport = new FakeApiHttpTransport();
    transport.enqueue_read(200, "{\"ok\":true,\"data\":{\"card_id\":\"c-new\"}}");
    transport.enqueue_read(200, "{\"ok\":true,\"data\":{}}");
    transport.enqueue_read(200, "{\"ok\":true,\"data\":{}}");
    transport.enqueue_read(
        200,
        "{\"ok\":true,\"data\":{\"card_id\":\"c1\",\"parent_card_id\":null,\"sort_key\":10.5}}"
    );
    var client = make_client(transport);

    bool done_create = false;
    string created = "";
    client.create_card.begin("p1", "Title", "Body", "parent-1", (obj, res) => {
        try { created = client.create_card.end(res); } catch (Error e) { created = ""; }
        done_create = true;
    });
    assert(wait_for_condition(() => done_create));
    assert(created == "c-new");
    assert(transport.last_uri.contains("/projects/p1/cards"));

    bool done_update = false;
    bool ok_update = false;
    client.update_card.begin("p1", "c1", "New Title", "New Body", 100, (obj, res) => {
        try { client.update_card.end(res); ok_update = true; } catch (Error e) { ok_update = false; }
        done_update = true;
    });
    assert(wait_for_condition(() => done_update));
    assert(ok_update);
    assert(transport.last_method == "PATCH");
    assert(transport.last_uri.contains("/projects/p1/cards/c1"));

    bool done_pos = false;
    bool ok_pos = false;
    client.update_card_position.begin("p1", "c1", null, 1.25, 101, (obj, res) => {
        try { client.update_card_position.end(res); ok_pos = true; } catch (Error e) { ok_pos = false; }
        done_pos = true;
    });
    assert(wait_for_condition(() => done_pos));
    assert(ok_pos);
    assert(transport.last_method == "PATCH");

    bool done_move = false;
    HolderLinux.CardMoveResult? moved = null;
    client.move_card.begin("p1", "c1", "to_start", null, null, (obj, res) => {
        try { moved = client.move_card.end(res); } catch (Error e) { moved = null; }
        done_move = true;
    });
    assert(wait_for_condition(() => done_move));
    assert(moved != null);
    assert(transport.last_uri.contains("/projects/p1/cards/c1/move"));
    assert(moved.card_id == "c1");
    assert(moved.sort_key == 10.5);
}

private void test_delete_card_sends_delete_to_card_path() {
    var transport = new FakeApiHttpTransport();
    transport.enqueue_read(200, "{\"ok\":true,\"data\":{}}");
    var client = make_client(transport);

    bool done_delete = false;
    bool ok_delete = false;
    client.delete_card.begin("p 1", "c 1/2", false, (obj, res) => {
        try { client.delete_card.end(res); ok_delete = true; } catch (Error e) { ok_delete = false; }
        done_delete = true;
    });

    assert(wait_for_condition(() => done_delete));
    assert(ok_delete);
    assert(transport.last_method == "DELETE");
    assert(transport.last_uri.contains("/projects/p%201/cards/c%201%2F2"));
    assert(!transport.last_uri.contains("children="));

    transport.enqueue_read(200, "{\"ok\":true,\"data\":{}}");
    done_delete = false;
    client.delete_card.begin("p 1", "c 1/2", true, (obj, res) => {
        try { client.delete_card.end(res); ok_delete = true; } catch (Error e) { ok_delete = false; }
        done_delete = true;
    });
    assert(wait_for_condition(() => done_delete));
    assert(ok_delete);
    assert(transport.last_uri.contains("/projects/p%201/cards/c%201%2F2?children=trash_subtree"));
}

private void test_project_tags_and_cards_with_tag() {
    var transport = new FakeApiHttpTransport();
    transport.enqueue_read(
        200,
        "{\"ok\":true,\"data\":[{\"tag\":\"android\",\"card_count\":2}]}"
    );
    transport.enqueue_read(
        200,
        "{\"ok\":true,\"data\":{\"items\":[{\"card_id\":\"c1\",\"project_id\":\"p1\",\"title\":\"Tagged\",\"updated_at\":2}],\"next_cursor\":null}}"
    );
    var client = make_client(transport);

    bool tags_done = false;
    Gee.ArrayList<HolderLinux.TagCount>? tags = null;
    client.list_project_tags.begin("p 1", (obj, res) => {
        try { tags = client.list_project_tags.end(res); } catch (Error e) { tags = null; }
        tags_done = true;
    });
    assert(wait_for_condition(() => tags_done));
    assert(tags != null && tags.size == 1);
    assert(tags[0].tag == "android");
    assert(transport.last_uri.contains("/projects/p%201/tags"));

    bool cards_done = false;
    Gee.ArrayList<HolderLinux.CardSummary>? cards = null;
    client.list_cards_with_tag.begin("p1", "android/mobile", (obj, res) => {
        try { cards = client.list_cards_with_tag.end(res); } catch (Error e) { cards = null; }
        cards_done = true;
    });
    assert(wait_for_condition(() => cards_done));
    assert(cards != null && cards.size == 1);
    assert(cards[0].title == "Tagged");
    assert(transport.last_uri.contains("/projects/p1/cards?"));
    assert(transport.last_uri.contains("tag=android%2Fmobile"));
}

private void test_calendar_and_milestone_endpoints() {
    var transport = new FakeApiHttpTransport();
    transport.enqueue_read(200,
        "{\"ok\":true,\"data\":{\"milestones\":[],\"created_cards\":[],\"updated_cards\":[]}}");
    transport.enqueue_read(200,
        "{\"ok\":true,\"data\":[{\"milestone_id\":\"m1\",\"project_id\":\"p1\",\"card_id\":\"c1\"," +
        "\"dates\":[{\"role\":\"start\",\"date\":\"2026-06-01\"}],\"kind\":null," +
        "\"description\":null,\"created_at\":1,\"updated_at\":1}]}");
    transport.enqueue_read(201,
        "{\"ok\":true,\"data\":{\"milestone_id\":\"m2\",\"project_id\":\"p1\",\"card_id\":\"c1\"," +
        "\"dates\":[{\"role\":\"start\",\"datetime\":\"1970-01-01T00:05:00Z\"}," +
        "{\"role\":\"end\",\"datetime\":\"1970-01-01T00:06:00Z\"}],\"kind\":\"Service\"," +
        "\"description\":\"Boiler\",\"created_at\":2,\"updated_at\":2}}");
    transport.enqueue_read(200,
        "{\"ok\":true,\"data\":{\"milestone_id\":\"m2\",\"project_id\":\"p1\",\"card_id\":\"c1\"," +
        "\"dates\":[{\"role\":\"start\",\"date\":\"1970-01-01\"}],\"kind\":null," +
        "\"description\":null,\"created_at\":2,\"updated_at\":3}}");
    transport.enqueue_read(200, "{\"ok\":true,\"data\":[]}");
    var client = make_client(transport);

    bool calendar_done = false;
    HolderLinux.ProjectCalendar? calendar = null;
    client.get_project_calendar.begin("p1", 100, 500, (obj, res) => {
        try { calendar = client.get_project_calendar.end(res); } catch (Error e) { calendar = null; }
        calendar_done = true;
    });
    assert(wait_for_condition(() => calendar_done));
    assert(calendar != null && calendar.project_id == "p1");
    assert(calendar.from_epoch == 100 && calendar.to_epoch == 500);
    assert(transport.last_uri.contains("/projects/p1/calendar"));
    assert(transport.last_uri.contains("first_instant=100"));
    assert(transport.last_uri.contains("last_instant=500"));
    // The window's days are where the app is.
    assert(transport.last_uri.contains(
        "first_day=" + new DateTime.from_unix_local(100).format("%Y-%m-%d")));
    assert(transport.last_uri.contains(
        "last_day=" + new DateTime.from_unix_local(500).format("%Y-%m-%d")));

    bool list_done = false;
    Gee.ArrayList<HolderLinux.Milestone>? milestones = null;
    client.list_card_milestones.begin("p1", "c1", (obj, res) => {
        try { milestones = client.list_card_milestones.end(res); } catch (Error e) { milestones = null; }
        list_done = true;
    });
    assert(wait_for_condition(() => list_done));
    assert(milestones != null && milestones.size == 1);
    assert(transport.last_uri.contains("/projects/p1/cards/c1/milestones"));

    bool add_done = false;
    HolderLinux.Milestone? added = null;
    client.add_card_milestone.begin("p1", "c1", 300, 360, false, " Service ", " Boiler ", (obj, res) => {
        try { added = client.add_card_milestone.end(res); } catch (Error e) { added = null; }
        add_done = true;
    });
    assert(wait_for_condition(() => add_done));
    assert(added != null && added.kind == "Service");
    assert(added.start_at == 300 && added.end_at == 360);
    assert(transport.last_method == "POST");
    assert(transport.last_uri.contains("/projects/p1/cards/c1/milestones"));

    bool update_done = false;
    HolderLinux.Milestone? updated = null;
    client.update_card_milestone.begin(
        (!) added, 320, null, true, null, null,
        (obj, res) => {
            try { updated = client.update_card_milestone.end(res); }
            catch (Error e) { updated = null; }
            update_done = true;
        }
    );
    assert(wait_for_condition(() => update_done));
    assert(updated != null);
    assert(updated.milestone_id == "m2");
    assert(updated.created_at == 2);
    assert(updated.updated_at == 3);
    assert(updated.end_at == null);
    assert(updated.all_day);
    assert(updated.kind == null);
    assert(transport.last_method == "PATCH");
    assert(transport.last_uri.contains("/projects/p1/cards/c1/milestones/m2"));

    bool delete_done = false;
    bool failed = true;
    client.remove_card_milestone.begin("p1", "c1", "m2", (obj, res) => {
        try { client.remove_card_milestone.end(res); failed = false; } catch (Error e) { failed = true; }
        delete_done = true;
    });
    assert(wait_for_condition(() => delete_done));
    assert(!failed);
    assert(transport.last_method == "DELETE");
    assert(transport.last_uri.contains("/projects/p1/cards/c1/milestones/m2"));
}

// A change to a milestone's start and end keeps its other dates.
private void test_milestone_body_keeps_other_dates() {
    var kept = parse_json_object(
        "{\"dates\":[{\"role\":\"start\",\"date\":\"2026-01-01\"},{\"role\":\"due\",\"date\":\"2026-02-01\"}," +
        "{\"role\":\"end\",\"date\":\"2026-01-03\"},{\"role\":\"done\",\"datetime\":\"2026-02-02T10:00:00Z\"}]}"
    ).get_array_member("dates");
    var start = new DateTime.local(2026, 3, 1, 0, 0, 0).to_unix();
    var body = parse_json_object(
        HolderLinux.ApiClientCardsEndpoints.milestone_body(kept, start, null, true, " Exam ", "")
    );
    var dates = body.get_array_member("dates");
    assert(dates.get_length() == 3);
    assert(dates.get_object_element(0).get_string_member("role") == "start");
    assert(dates.get_object_element(0).get_string_member("date") == "2026-03-01");
    assert(dates.get_object_element(1).get_string_member("role") == "due");
    assert(dates.get_object_element(2).get_string_member("datetime") == "2026-02-02T10:00:00Z");
    assert(body.get_string_member("kind") == "Exam");
    assert(body.get_member("description").get_node_type() == Json.NodeType.NULL);

    // An instant is written in local time, with its offset.
    var timed = parse_json_object(
        HolderLinux.ApiClientCardsEndpoints.milestone_body(null, 300, 360, false, null, null)
    ).get_array_member("dates");
    assert(timed.get_length() == 2);
    var start_value = new DateTime.from_iso8601(timed.get_object_element(0).get_string_member("datetime"), null);
    assert(start_value != null && ((!) start_value).to_unix() == 300);
    assert(timed.get_object_element(1).get_string_member("role") == "end");
}

private void test_card_history_endpoints() {
    var transport = new FakeApiHttpTransport();
    transport.enqueue_read(200,
        "{\"ok\":true,\"data\":{\"head_oid\":\"project-head\",\"next_cursor\":null," +
        "\"scan_limited\":false,\"activities\":[{\"oid\":\"project-head\"," +
        "\"parent_oids\":[],\"author\":{\"name\":\"Ezra\",\"email\":\"e@test\"}," +
        "\"authored_at\":1,\"committed_at\":2,\"message\":\"Attach\"," +
        "\"affected_objects\":[{\"kind\":\"resource\",\"paths\":[\"resources/a.json\"]," +
        "\"items\":[{\"path\":\"resources/a.json\",\"title\":\"Example resource\",\"detail\":\"Attachment: a.pdf\"}]},{\"kind\":\"ai_data\",\"paths\":[\"ai_messages/a.md\"]," +
        "\"items\":[{\"path\":\"ai_messages/a.md\",\"title\":\"Release review\",\"detail\":\"user: Can you review?\"}]},{\"kind\":\"project_settings\",\"paths\":[\".holder/privacy.json\"]," +
        "\"items\":[{\"path\":\".holder/privacy.json\",\"title\":\"Privacy settings\",\"detail\":\"Mode: plain Git\"}]}]," +
        "\"is_merge\":false}]}}");
    transport.enqueue_read(200,
        "{\"ok\":true,\"data\":{\"head_oid\":\"head\",\"entries\":[{" +
        "\"first_oid\":\"old\",\"last_oid\":\"head\",\"parent_oids\":[\"parent\"]," +
        "\"visible_parent_oids\":[\"parent\"]," +
        "\"author\":{\"name\":\"Ezra\",\"email\":\"ezra@example.test\"}," +
        "\"started_at\":1,\"ended_at\":2,\"kind\":\"updated\",\"summary\":\"Changed\"," +
        "\"commit_count\":2,\"is_merge\":false,\"saves\":[{" +
        "\"oid\":\"old\",\"parent_oids\":[\"parent\"],\"authored_at\":1," +
        "\"committed_at\":1,\"message\":\"First save\"},{" +
        "\"oid\":\"head\",\"parent_oids\":[\"old\"],\"authored_at\":2," +
        "\"committed_at\":2,\"message\":\"Final save\"}]}]," +
        "\"next_cursor\":\"older\"}}");
    transport.enqueue_read(200,
        "{\"ok\":true,\"data\":{\"before\":{\"exists\":true,\"oid\":\"old\"," +
        "\"title\":\"Card\",\"body\":\"Before\"},\"after\":{\"exists\":true," +
        "\"oid\":\"head\",\"title\":\"Card\",\"body\":\"After\"}," +
        "\"summary\":\"Changed card\",\"lines\":[],\"truncated\":false}}");
    transport.enqueue_read(200,
        "{\"ok\":true,\"data\":{\"before\":{\"exists\":false,\"oid\":\"\"," +
        "\"title\":\"\",\"body\":\"\"},\"after\":{\"exists\":true," +
        "\"oid\":\"created\",\"title\":\"Card\",\"body\":\"After\"}," +
        "\"summary\":\"Card created\",\"lines\":[],\"truncated\":false}}");
    transport.enqueue_read(200, "{\"ok\":true,\"data\":{\"card_id\":\"card/one\"}}");
    var client = make_client(transport);

    bool project_done = false;
    HolderLinux.ProjectHistoryPage? project_page = null;
    client.list_project_history.begin("project one", 25, "cursor oid", "resource", (obj, res) => {
        try { project_page = client.list_project_history.end(res); } catch (Error e) { project_page = null; }
        project_done = true;
    });
    assert(wait_for_condition(() => project_done));
    assert(project_page != null && ((!) project_page).activities.length == 1);
    assert(((!) project_page).activities[0].affected_objects[0].kind == "resource");
    assert(((!) project_page).activities[0].affected_objects[0].items[0].title == "Example resource");
    assert(((!) project_page).activities[0].affected_objects[0].items[0].detail == "Attachment: a.pdf");
    assert(((!) project_page).activities[0].affected_objects[1].kind == "ai_data");
    assert(((!) project_page).activities[0].affected_objects[1].items[0].title == "Release review");
    assert(((!) project_page).activities[0].affected_objects[2].kind == "project_settings");
    assert(((!) project_page).activities[0].affected_objects[2].items[0].detail == "Mode: plain Git");
    assert(transport.last_uri.contains("/projects/project%20one/history"));
    assert(transport.last_uri.contains("kind=resource"));

    bool list_done = false;
    HolderLinux.CardHistoryPage? page = null;
    client.list_card_history.begin("project one", "card/one", 25, "cursor oid", (obj, res) => {
        try { page = client.list_card_history.end(res); } catch (Error e) { page = null; }
        list_done = true;
    });
    assert(wait_for_condition(() => list_done));
    assert(page != null && page.next_cursor == "older");
    assert(((!) page).entries[0].visible_parent_oids.length == 1);
    assert(((!) page).entries[0].visible_parent_oids[0] == "parent");
    assert(((!) page).entries[0].saves.length == 2);
    assert(((!) page).entries[0].saves[0].oid == "old");
    assert(((!) page).entries[0].saves[1].message == "Final save");
    assert(transport.last_uri.contains("/projects/project%20one/history/cards/card%2Fone"));
    assert(transport.last_uri.contains("limit=25"));
    assert(transport.last_uri.contains("cursor=cursor%20oid"));

    bool compare_done = false;
    HolderLinux.CardHistoryComparison? comparison = null;
    client.compare_card_history.begin(
        "project one", "card/one", "old", "head", "since", (obj, res) => {
            try { comparison = client.compare_card_history.end(res); }
            catch (Error e) { comparison = null; }
            compare_done = true;
        }
    );
    assert(wait_for_condition(() => compare_done));
    assert(comparison != null && ((!) comparison).to_version.body == "After");
    assert(transport.last_uri.contains("/projects/project%20one/history/cards/card%2Fone/compare"));
    assert(transport.last_uri.contains("from_oid=old"));
    assert(transport.last_uri.contains("to_oid=head"));
    assert(transport.last_uri.contains("mode=since"));

    compare_done = false;
    client.compare_card_history.begin(
        "project one", "card/one", null, "created", "change", (obj, res) => {
            try { comparison = client.compare_card_history.end(res); }
            catch (Error e) { comparison = null; }
            compare_done = true;
        }
    );
    assert(wait_for_condition(() => compare_done));
    assert(comparison != null);
    assert(!transport.last_uri.contains("from_oid="));
    assert(transport.last_uri.contains("to_oid=created"));
    assert(transport.last_uri.contains("mode=change"));

    bool restore_done = false;
    bool restored = false;
    client.restore_card_history.begin("project one", "card/one", "restore oid", (obj, res) => {
        try { restored = client.restore_card_history.end(res); } catch (Error e) { restored = false; }
        restore_done = true;
    });
    assert(wait_for_condition(() => restore_done));
    assert(restored);
    assert(transport.last_method == "POST");
    assert(transport.last_uri.contains("/projects/project%20one/history/cards/card%2Fone/restore"));
    assert(transport.last_uri.contains("oid=restore%20oid"));
}

private void test_milestone_endpoints_report_missing_data_and_trim_update_text() {
    var transport = new FakeApiHttpTransport();
    transport.enqueue_read(200, "{\"ok\":true}");
    transport.enqueue_read(200, "{\"ok\":true}");
    transport.enqueue_read(200, "{\"ok\":true}");
    transport.enqueue_read(
        200,
        "{\"ok\":true,\"data\":{\"milestone_id\":\"m3\",\"project_id\":\"p1\",\"card_id\":\"c1\"," +
        "\"dates\":[{\"role\":\"start\",\"datetime\":\"1970-01-01T00:00:10Z\"}],\"kind\":\"Service\"," +
        "\"description\":\"Boiler\",\"created_at\":1,\"updated_at\":2}}"
    );
    // A milestone the calendar cannot show is not a valid answer to a change made here.
    transport.enqueue_read(
        200,
        "{\"ok\":true,\"data\":{\"milestone_id\":\"m3\",\"project_id\":\"p1\",\"card_id\":\"c1\"," +
        "\"dates\":[],\"kind\":null,\"description\":null,\"created_at\":1,\"updated_at\":2}}"
    );
    var client = make_client(transport);
    var milestone = new HolderLinux.Milestone("m1", "c1", 10, null, false, null, null, 1, 1);
    milestone.project_id = "p1";

    bool create_done = false;
    bool create_protocol = false;
    client.add_card_milestone.begin("p1", "c1", 10, null, false, null, null, (obj, res) => {
        try { client.add_card_milestone.end(res); }
        catch (Error e) { create_protocol = (e is HolderLinux.ApiError.PROTOCOL); }
        create_done = true;
    });
    assert(wait_for_condition(() => create_done));
    assert(create_protocol);

    bool remove_done = false;
    bool remove_protocol = false;
    client.remove_card_milestone.begin("p1", "c1", "m1", (obj, res) => {
        try { client.remove_card_milestone.end(res); }
        catch (Error e) { remove_protocol = (e is HolderLinux.ApiError.PROTOCOL); }
        remove_done = true;
    });
    assert(wait_for_condition(() => remove_done));
    assert(remove_protocol);

    bool update_done = false;
    bool update_protocol = false;
    client.update_card_milestone.begin(milestone, 10, null, false, null, null, (obj, res) => {
        try { client.update_card_milestone.end(res); }
        catch (Error e) { update_protocol = (e is HolderLinux.ApiError.PROTOCOL); }
        update_done = true;
    });
    assert(wait_for_condition(() => update_done));
    assert(update_protocol);

    bool trimmed_done = false;
    HolderLinux.Milestone? updated = null;
    client.update_card_milestone.begin(milestone, 10, null, false, " Service ", " Boiler ", (obj, res) => {
        try { updated = client.update_card_milestone.end(res); }
        catch (Error e) { updated = null; }
        trimmed_done = true;
    });
    assert(wait_for_condition(() => trimmed_done));
    assert(updated != null);
    assert(updated.milestone_id == "m3");
    assert(transport.last_method == "PATCH");

    bool undated_done = false;
    bool undated_protocol = false;
    client.update_card_milestone.begin(milestone, 10, null, false, null, null, (obj, res) => {
        try { client.update_card_milestone.end(res); }
        catch (Error e) { undated_protocol = (e is HolderLinux.ApiError.PROTOCOL); }
        undated_done = true;
    });
    assert(wait_for_condition(() => undated_done));
    assert(undated_protocol);
}

public static int main(string[] args) {
    Test.init(ref args);

    Test.add_func("/api_client_cards/milestone_body_keeps_other_dates", test_milestone_body_keeps_other_dates);
    Test.add_func("/api_client_cards/list_cards_and_context_and_get_card",
                  test_list_cards_and_context_and_get_card);
    Test.add_func("/api_client_cards/tree_listing_follows_cursor", test_tree_listing_follows_cursor);
    Test.add_func("/api_client_cards/links_create_delete_and_list",
                  test_links_create_delete_and_list);
    Test.add_func("/api_client_cards/create_link_missing_data_and_move_missing_data_are_protocol_errors",
                  test_create_link_missing_data_and_move_missing_data_are_protocol_errors);
    Test.add_func("/api_client_cards/create_update_position_and_move_card_success",
                  test_create_update_position_and_move_card_success);
    Test.add_func("/api_client_cards/delete_card_sends_delete_to_card_path",
                  test_delete_card_sends_delete_to_card_path);
    Test.add_func("/api_client_cards/project_tags_and_cards_with_tag",
                  test_project_tags_and_cards_with_tag);
    Test.add_func("/api_client_cards/calendar_and_milestone_endpoints",
                  test_calendar_and_milestone_endpoints);
    Test.add_func("/api_client_cards/card_history_endpoints",
                  test_card_history_endpoints);
    Test.add_func("/api_client_cards/milestone_endpoints_report_missing_data_and_trim_update_text",
                  test_milestone_endpoints_report_missing_data_and_trim_update_text);

    return Test.run();
}

}
