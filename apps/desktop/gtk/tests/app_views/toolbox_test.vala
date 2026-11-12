using GLib;

namespace HolderLinuxTests {

// A toolbox as the window builds it, recording every signal it raises. Its terminal runs `cat`, so
// no shell prompt appears.
private class ToolboxFixture : Object {
    public HolderLinux.ToolboxPane toolbox;
    public Adw.Window window = new Adw.Window();
    public Gee.ArrayList<string> events = new Gee.ArrayList<string>();
    public GLib.ListStore project_store = new GLib.ListStore(typeof(HolderLinux.Project));
    public GLib.ListStore card_store = new GLib.ListStore(typeof(HolderLinux.CardSummary));
    public Gtk.SingleSelection project_selection;
    public Gtk.SingleSelection card_selection;
    private string? previous_shell;

    public ToolboxFixture() {
        previous_shell = Environment.get_variable("SHELL");
        Environment.set_variable("SHELL", "cat", true);
        toolbox = new HolderLinux.ToolboxPane();
        window.set_content(toolbox.widget);
        project_selection = new Gtk.SingleSelection(project_store);
        project_selection.set_autoselect(false);
        project_selection.set_can_unselect(true);
        card_selection = new Gtk.SingleSelection(card_store);
        card_selection.set_autoselect(false);
        card_selection.set_can_unselect(true);
        record_signals();
    }

    private void record_signals() {
        var t = toolbox;
        t.error_reported.connect((title, details) => { events.add("error_reported|%s|%s".printf(title, details)); });
        t.toast_requested.connect((message) => { events.add("toast_requested|" + message); });
        t.flowboard_card_open_requested.connect((id) => { events.add("flowboard_card_open_requested|" + id); });
        t.connections_card_open_requested.connect((id) => { events.add("connections_card_open_requested|" + id); });
        t.tags_card_open_requested.connect((id) => { events.add("tags_card_open_requested|" + id); });
        t.resources_card_open_requested.connect((id) => { events.add("resources_card_open_requested|" + id); });
        t.milestones_card_open_requested.connect((id) => { events.add("milestones_card_open_requested|" + id); });
        t.resource_references_requested.connect((resource) => {
            events.add("resource_references_requested|" + resource.resource_id);
        });
        t.asset_preview_requested.connect((resource, asset) => {
            events.add("asset_preview_requested|%s|%s".printf(resource.resource_id, asset.asset_id));
        });
        t.project_resources_loaded.connect((project_id, resources) => {
            events.add("project_resources_loaded|%s|%d".printf(project_id, resources.size));
        });
        t.connections_card_create_child_requested.connect((id) => {
            events.add("connections_card_create_child_requested|" + id);
        });
        t.flowboard_card_move_to_trash_requested.connect((id) => {
            events.add("flowboard_card_move_to_trash_requested|" + id);
        });
        t.flowboard_move_intent_requested.connect((card_id, project_id, intent, target, parent) => {
            events.add("flowboard_move_intent_requested|%s|%s|%s|%s|%s".printf(
                card_id, project_id, intent, target ?? "-", parent ?? "-"
            ));
        });
        t.flowboard_new_card_requested.connect((parent) => { events.add("flowboard_new_card_requested|" + (parent ?? "-")); });
        t.history_copy_as_card_requested.connect((title, content) => {
            events.add("history_copy_as_card_requested|%s|%s".printf(title, content));
        });
        t.history_restore_succeeded.connect((project_id, card_id) => {
            events.add("history_restore_succeeded|%s|%s".printf(project_id, card_id));
        });
        t.history_card_open_requested.connect((id) => { events.add("history_card_open_requested|" + id); });
        t.history_ai_thread_open_requested.connect((id) => { events.add("history_ai_thread_open_requested|" + id); });
        t.send_card_as_email_requested.connect(() => { events.add("send_card_as_email_requested"); });
        t.send_recovery_key_as_email_requested.connect(() => { events.add("send_recovery_key_as_email_requested"); });
        t.save_recovery_key_to_usb_requested.connect(() => { events.add("save_recovery_key_to_usb_requested"); });
        t.import_recovery_key_requested.connect(() => { events.add("import_recovery_key_requested"); });
        t.terminal_copy_to_card_requested.connect((text) => { events.add("terminal_copy_to_card_requested|" + text); });
        t.activity_requested.connect((kind, message, project_id, card_id, details) => {
            events.add("activity_requested|%s|%s|%s|%s".printf(kind, message, project_id ?? "-", card_id ?? "-"));
        });
        t.breadcrumb_navigation_requested.connect((tool_id, index, project_id, card_id) => {
            events.add("breadcrumb_navigation_requested|%s|%d|%s|%s".printf(
                tool_id, index, project_id ?? "-", card_id ?? "-"
            ));
        });
    }

    // A project with two cards, bound as the window binds them; nothing is selected yet.
    public void bind_context() {
        project_store.append(new HolderLinux.Project("p1", "Project 1", "encrypted_git", "/tmp/p1", 10, 10));
        card_store.append(new HolderLinux.CardSummary("c1", "p1", "Card 1", "c1.md", 1024.0, null, 20, 20));
        card_store.append(new HolderLinux.CardSummary("c2", "p1", "Card 2", "c2.md", 2048.0, null, 21, 21));
        project_selection.set_selected(Gtk.INVALID_LIST_POSITION);
        card_selection.set_selected(Gtk.INVALID_LIST_POSITION);
        toolbox.bind_connections_context(project_selection, card_store, card_selection);
    }

    public bool saw(string event) {
        return events.contains(event);
    }

    public string debug_log() {
        var text = new StringBuilder();
        foreach (var widget in av_descendants(toolbox.widget)) {
            var view = widget as Gtk.TextView;
            if (view != null) {
                Gtk.TextIter start;
                Gtk.TextIter end;
                ((!) view).buffer.get_bounds(out start, out end);
                text.append(((!) view).buffer.get_text(start, end, false));
            }
        }
        return text.str;
    }

    // The breadcrumb buttons the toolbox shows for the current tool and selection.
    public Gtk.Button? breadcrumb(string label) {
        foreach (var widget in av_descendants(toolbox.widget)) {
            var button = widget as Gtk.Button;
            if (button != null && !(button is Gtk.ToggleButton) && ((!) button).get_label() == label) {
                return button;
            }
        }
        return null;
    }

    // The spinners showing now. Tools can show their own (the Windows terminal tool does while it
    // checks for PowerShell), so tests compare against the ones showing before.
    public Gee.ArrayList<Gtk.Widget> visible_spinners() {
        var spinners = new Gee.ArrayList<Gtk.Widget>();
        foreach (var widget in av_descendants(toolbox.widget)) {
            if (widget is Gtk.Spinner && widget.get_visible()) {
                spinners.add(widget);
            }
        }
        return spinners;
    }

    public void destroy() {
        window.destroy();
        av_settle();
        if (previous_shell != null) {
            Environment.set_variable("SHELL", (!) previous_shell, true);
        } else {
            Environment.unset_variable("SHELL");
        }
    }
}

private HolderLinux.ProjectResource toolbox_resource() {
    return new HolderLinux.ProjectResource("r1", "p1", "file", "holder://resource/r1", "Boiler", null, 1, 1);
}

private void test_the_toolbox_passes_each_tools_requests_on() {
    var f = new ToolboxFixture();
    var t = f.toolbox;
    var resource = toolbox_resource();
    var asset = new HolderLinux.ResourceAsset("a1", "r1", "boiler.png", "image/png", 10);
    var resources = new Gee.ArrayList<HolderLinux.ProjectResource>();
    resources.add(resource);

    t.flowboard_tool.card_open_requested("c1");
    t.flowboard_tool.card_move_to_trash_requested("c1");
    t.flowboard_tool.move_intent_requested("c1", "p1", "before", "c2", null);
    t.flowboard_tool.new_card_requested("c1");
    t.flowboard_tool.toast_requested("flowboard toast");
    t.connections_tool.error_reported("Connections failed", "why");
    t.connections_tool.toast_requested("connections toast");
    t.connections_tool.project_overview_requested("p1");
    t.connections_tool.projects_root_requested();
    t.connections_tool.card_open_requested("c2");
    t.connections_tool.card_create_child_requested("c2");
    t.tags_tool.error_reported("Tags failed", "why");
    t.tags_tool.card_open_requested("c3");
    t.resources_tool.error_reported("Resources failed", "why");
    t.resources_tool.toast_requested("resources toast");
    t.resources_tool.asset_preview_requested(resource, asset);
    t.resources_tool.card_open_requested("c4");
    t.resources_tool.resource_references_requested(resource);
    t.resources_tool.project_resources_loaded("p1", resources);
    t.resources_tool.activity_requested("resource", "Added Boiler", "p1", "r1", null);
    t.milestones_tool.error_reported("Milestones failed", "why");
    t.milestones_tool.toast_requested("milestones toast");
    t.milestones_tool.card_open_requested("c5");
    t.history_tool.error_reported("History failed", "why");
    t.history_tool.copy_as_card_requested("Old title", "Old text");
    t.history_tool.restore_succeeded("p1", "c1");
    t.history_tool.project_history_card_open_requested("c6");
    t.history_tool.project_history_ai_thread_open_requested("t1");
    t.sharing_tool.send_card_as_email_requested();
    t.terminal_tool.toast_requested("terminal toast");
    t.terminal_tool.copy_to_card_requested("$ make");
    t.git_sync_tool.error_reported("Git failed", "why");
    t.git_sync_tool.toast_requested("git toast");
    t.git_sync_tool.activity_requested("git", "Pushed", "p1", null, null);
    t.recovery_key_tool.send_recovery_key_as_email_requested();
    t.recovery_key_tool.save_recovery_key_to_usb_requested();
    t.recovery_key_tool.import_recovery_key_requested();
    t.trash_tool.error_reported("Trash failed", "why");
    t.trash_tool.toast_requested("trash toast");
    t.trash_tool.activity_requested("trash", "Emptied", "p1", null, null);

    string[] expected = {
        "flowboard_card_open_requested|c1",
        "flowboard_card_move_to_trash_requested|c1",
        "flowboard_move_intent_requested|c1|p1|before|c2|-",
        "flowboard_new_card_requested|c1",
        "toast_requested|flowboard toast",
        "error_reported|Connections failed|why",
        "toast_requested|connections toast",
        "breadcrumb_navigation_requested|connections|1|p1|-",
        "breadcrumb_navigation_requested|connections|0|-|-",
        "connections_card_open_requested|c2",
        "connections_card_create_child_requested|c2",
        "error_reported|Tags failed|why",
        "tags_card_open_requested|c3",
        "error_reported|Resources failed|why",
        "toast_requested|resources toast",
        "asset_preview_requested|r1|a1",
        "resources_card_open_requested|c4",
        "resource_references_requested|r1",
        "project_resources_loaded|p1|1",
        "activity_requested|resource|Added Boiler|p1|r1",
        "error_reported|Milestones failed|why",
        "toast_requested|milestones toast",
        "milestones_card_open_requested|c5",
        "error_reported|History failed|why",
        "history_copy_as_card_requested|Old title|Old text",
        "history_restore_succeeded|p1|c1",
        "history_card_open_requested|c6",
        "history_ai_thread_open_requested|t1",
        "send_card_as_email_requested",
        "toast_requested|terminal toast",
        "terminal_copy_to_card_requested|$ make",
        "error_reported|Git failed|why",
        "toast_requested|git toast",
        "activity_requested|git|Pushed|p1|-",
        "send_recovery_key_as_email_requested",
        "save_recovery_key_to_usb_requested",
        "import_recovery_key_requested",
        "error_reported|Trash failed|why",
        "toast_requested|trash toast",
        "activity_requested|trash|Emptied|p1|-",
    };
    foreach (var event in expected) {
        if (!f.saw(event)) {
            Test.message("missing: %s", event);
            assert_not_reached();
        }
    }
    assert(f.events.size == expected.length);
    f.destroy();
}

private void test_tool_debug_lines_reach_the_debug_tool() {
    var f = new ToolboxFixture();

    f.toolbox.connections_tool.debug_log_requested("connections line");
    f.toolbox.history_tool.debug_log_requested("history line");
    f.toolbox.terminal_tool.debug_log_requested("terminal line");
    f.toolbox.log_debug("toolbox line");

    var log = f.debug_log();
    assert(log.contains("connections line"));
    assert(log.contains("history line"));
    assert(log.contains("terminal line"));
    assert(log.contains("toolbox line"));
    f.destroy();
}

private void test_a_history_resource_opens_the_resources_tool() {
    var f = new ToolboxFixture();

    f.toolbox.history_tool.project_history_resource_open_requested("r1");

    assert(f.toolbox.is_showing_tool("resources"));
    f.destroy();
}

private void test_the_api_and_context_reach_the_tools() {
    var f = new ToolboxFixture();
    var api = new MainControllerFakeApi();
    f.toolbox.set_api_client(api);
    f.toolbox.set_settings(null);
    f.toolbox.set_activity_log_store(new HolderLinux.ActivityLogStore());
    f.bind_context();

    f.project_selection.set_selected(0);
    f.toolbox.show_tool("trash");
    assert(wait_for_condition(() => api.list_trash_calls > 0));
    f.toolbox.refresh_resources("r1");
    assert(wait_for_condition(() => api.list_resources_calls > 0));
    f.destroy();
}

private void test_the_breadcrumbs_follow_the_selection_and_lead_back() {
    var f = new ToolboxFixture();
    f.bind_context();
    f.toolbox.show_tool("connections");

    assert(f.breadcrumb("Project 1") == null);

    f.project_selection.set_selected(0);
    f.card_selection.set_selected(1);
    var project = f.breadcrumb("Project 1");
    assert(project != null);
    ((!) project).clicked();
    assert(f.saw("breadcrumb_navigation_requested|connections|1|p1|c2"));

    // A change to the card list rebuilds them too.
    f.card_store.append(new HolderLinux.CardSummary("c3", "p1", "Card 3", "c3.md", 4096.0, null, 22, 22));
    var rebuilt = f.breadcrumb("Project 1");
    assert(rebuilt != null && rebuilt != project);
    f.destroy();
}

private void test_sharing_needs_a_selected_card() {
    var f = new ToolboxFixture();
    f.bind_context();
    Gtk.Button? email = null;
    foreach (var widget in av_descendants(f.toolbox.sharing_tool.widget)) {
        if (widget is Gtk.Button) {
            email = (Gtk.Button) widget;
        }
    }
    assert(email != null);

    assert(!((!) email).get_sensitive());
    f.project_selection.set_selected(0);
    f.card_selection.set_selected(0);
    assert(((!) email).get_sensitive());
    f.card_selection.set_selected(Gtk.INVALID_LIST_POSITION);
    assert(!((!) email).get_sensitive());
    f.destroy();
}

private void test_new_card_goes_under_the_selected_card_or_to_the_flowboard() {
    var f = new ToolboxFixture();
    f.bind_context();
    var flowboard = new HolderLinux.FlowboardController(f.project_store, f.project_selection, f.card_store);
    f.toolbox.bind_flowboard_controller(flowboard);
    var asked_flowboard = false;
    flowboard.create_card_requested.connect((parent) => { asked_flowboard = parent == null; });

    f.project_selection.set_selected(0);
    f.card_selection.set_selected(1);
    f.toolbox.request_flowboard_child_or_current_level_card();
    assert(f.saw("flowboard_new_card_requested|c2"));

    // Without a selected card, the flowboard adds one at the level it shows.
    f.card_selection.set_selected(Gtk.INVALID_LIST_POSITION);
    f.toolbox.show_flowboard_project_root();
    f.toolbox.request_flowboard_child_or_current_level_card();
    assert(asked_flowboard);
    f.destroy();
}

private void test_showing_tools_and_roots() {
    var f = new ToolboxFixture();
    f.bind_context();
    var flowboard = new HolderLinux.FlowboardController(f.project_store, f.project_selection, f.card_store);
    f.toolbox.bind_flowboard_controller(flowboard);

    f.toolbox.show_tool("");
    assert(f.toolbox.is_showing_tool("flowboard"));
    // Before a project is chosen the flowboard can show the project list or go back from it.
    f.toolbox.show_flowboard_project_root();
    assert(!flowboard.is_showing_projects_root());
    f.toolbox.show_flowboard_projects_root();
    assert(flowboard.is_showing_projects_root());
    f.toolbox.show_tag("boiler");
    assert(f.toolbox.is_showing_tool("tags"));
    f.toolbox.show_tool("milestones");
    f.toolbox.refresh_milestones();
    f.toolbox.show_tool("history");
    f.project_selection.set_selected(0);
    f.card_selection.set_selected(0);
    f.toolbox.refresh_history("p1", "c1");
    f.toolbox.refresh_history("p1", "another-card");
    f.toolbox.git_sync_tool.repository_history_changed("p1");
    f.toolbox.show_connections_projects_root();
    f.toolbox.set_connections_internal_links(new Gee.ArrayList<string>());
    f.destroy();
}

private void test_loading_and_reveal_and_terminal_focus() {
    var f = new ToolboxFixture();

    var before = f.visible_spinners();
    f.toolbox.set_navigation_loading(true);
    var loading = f.visible_spinners();
    assert(loading.size == before.size + 1);
    f.toolbox.set_navigation_loading(false);
    var after = f.visible_spinners();
    assert(after.size == before.size && after.contains_all(before));

    f.toolbox.set_reveal_child(true);
    assert(f.toolbox.widget.get_reveal_child());

    assert(!f.toolbox.terminal_captures_application_shortcuts(null));
#if HOLDER_TERMINAL_VTE
    Vte.Terminal? terminal = null;
    foreach (var widget in av_descendants(f.toolbox.widget)) {
        if (widget is Vte.Terminal) {
            terminal = (Vte.Terminal) widget;
        }
    }
    assert(terminal != null);
    assert(f.toolbox.terminal_captures_application_shortcuts(terminal));
#endif
    f.destroy();
}

private void test_an_unbound_toolbox_asks_with_no_selection() {
    var f = new ToolboxFixture();

    // Before the window binds a selection, new cards go to the flowboard, which has no project yet.
    f.toolbox.request_flowboard_child_or_current_level_card();
    assert(f.events.size == 0);

    var flowboard_crumb = f.breadcrumb("Flowboard");
    assert(flowboard_crumb != null);
    ((!) flowboard_crumb).clicked();
    assert(f.saw("breadcrumb_navigation_requested|flowboard|0|-|-"));
    f.destroy();
}

internal void register_toolbox_tests() {
    Test.add_func("/app_views/toolbox/unbound", test_an_unbound_toolbox_asks_with_no_selection);
    Test.add_func("/app_views/toolbox/passes_requests_on", test_the_toolbox_passes_each_tools_requests_on);
    Test.add_func("/app_views/toolbox/debug_lines", test_tool_debug_lines_reach_the_debug_tool);
    Test.add_func("/app_views/toolbox/history_resource", test_a_history_resource_opens_the_resources_tool);
    Test.add_func("/app_views/toolbox/api_and_context", test_the_api_and_context_reach_the_tools);
    Test.add_func("/app_views/toolbox/breadcrumbs", test_the_breadcrumbs_follow_the_selection_and_lead_back);
    Test.add_func("/app_views/toolbox/sharing_needs_card", test_sharing_needs_a_selected_card);
    Test.add_func("/app_views/toolbox/new_card", test_new_card_goes_under_the_selected_card_or_to_the_flowboard);
    Test.add_func("/app_views/toolbox/tools_and_roots", test_showing_tools_and_roots);
    Test.add_func("/app_views/toolbox/loading_reveal_terminal", test_loading_and_reveal_and_terminal_focus);
}

}
