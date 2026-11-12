using GLib;

namespace HolderLinuxTests {

private class RecordingUriLauncher : Object, HolderLinux.IUriLauncher {
    public Gee.ArrayList<string> launched = new Gee.ArrayList<string>();
    public string? failure = null;

    public void launch(string uri) throws Error {
        if (failure != null) {
            throw new IOError.FAILED((!) failure);
        }
        launched.add(uri);
    }
}

// The navigator wired as the window wires it, around a controller backed by the fake API. Its card
// list holds two cards of the selected project and one of another project.
private class NavigatorFixture : WorkspaceFixture {
    public MainControllerFakeApi api = new MainControllerFakeApi();
    public MainControllerTestHarness harness;
    public GLib.ListStore card_store = new GLib.ListStore(typeof(HolderLinux.CardSummary));
    public RecordingUriLauncher launcher = new RecordingUriLauncher();
    public HolderLinux.WindowInternalLinkNavigator navigator;
    public Gee.ArrayList<string> previews = new Gee.ArrayList<string>();
    // Cards asked of the API. Opening a card goes through the selection, which the window keeps in
    // step with the controller's; here it is enough that the card was asked for.
    public Gee.ArrayList<string> requested_cards = new Gee.ArrayList<string>();

    public NavigatorFixture() {
        base();
        api.get_card_before_complete_hook = (card_id) => {
            requested_cards.add(card_id);
        };
        harness = new MainControllerTestHarness(api, new TestScheduler(), new FakeClock());
        var controller = harness.controller;
        card_store.append(new HolderLinux.CardSummary("c1", "p1", "Card 1", "c1.md", 1024.0, null, 20, 20));
        card_store.append(new HolderLinux.CardSummary("c2", "p1", "Card 2", "c2.md", 2048.0, null, 21, 21));
        card_store.append(new HolderLinux.CardSummary("x9", "p-other", "Elsewhere", "x9.md", 1.0, null, 1, 1));

        var project_store = new GLib.ListStore(typeof(HolderLinux.Project));
        var project_selection = new Gtk.SingleSelection(project_store);
        var card_selection = new Gtk.SingleSelection(card_store);
        var thread_selection = new Gtk.SingleSelection(new GLib.ListStore(typeof(HolderLinux.AiThreadSummary)));
        var search_selection = new Gtk.SingleSelection(search_store);
        var orchestrator = new HolderLinux.SelectionIntentOrchestrator(
            new HolderLinux.SelectionIntentController(),
            new HolderLinux.SelectionTransitionController(
                new HolderLinux.AppTransitionController(new HolderLinux.AppStateStore())
            ),
            new HolderLinux.SelectionController(controller),
            controller,
            new HolderLinux.FlowboardController(project_store, project_selection, card_store),
            project_selection,
            card_selection,
            thread_selection,
            search_selection,
            card_store,
            new HolderLinux.SearchSelectionController(search_store)
        );
        navigator = new HolderLinux.WindowInternalLinkNavigator(
            new HolderLinux.InternalLinkController(),
            new HolderLinux.TagNavigationController(),
            workspace.editor_buffer,
            workspace.editor_view,
            card_store,
            controller,
            orchestrator,
            new HolderLinux.CardActionDialogAdapter(window),
            workspace.toolbox,
            workspace,
            toast_overlay,
            null,
            null,
            launcher
        );
        navigator.resource_preview_requested.connect((resource_id) => {
            previews.add(resource_id);
        });

        controller.reload_everything.begin();
        assert(wait_for_condition(() => controller.selected_project_id() == "p1"));
    }

    // Puts text in the editor with the cursor just after the first occurrence of `at`.
    public override void destroy() {
        api.get_card_before_complete_hook = null;
        base.destroy();
    }

    public void type_with_cursor_in(string text, string at) {
        workspace.editor_buffer.set_text(text, -1);
        var offset = text.index_of(at);
        assert(offset >= 0);
        Gtk.TextIter cursor;
        workspace.editor_buffer.get_iter_at_offset(out cursor, text.substring(0, offset).char_count() + at.char_count());
        workspace.editor_buffer.place_cursor(cursor);
    }

    public bool ctrl_enter() {
        return navigator.handle_key(Gdk.Key.Return, 0, Gdk.ModifierType.CONTROL_MASK);
    }

    // The editor view's coordinates of the middle of the character just before the cursor.
    public void point_at_cursor(out double x, out double y) {
        Gtk.TextIter cursor;
        workspace.editor_buffer.get_iter_at_mark(out cursor, workspace.editor_buffer.get_insert());
        cursor.backward_char();
        Gdk.Rectangle location;
        workspace.editor_view.get_iter_location(cursor, out location);
        int window_x;
        int window_y;
        workspace.editor_view.buffer_to_window_coords(
            Gtk.TextWindowType.WIDGET,
            location.x + location.width / 2,
            location.y + location.height / 2,
            out window_x,
            out window_y
        );
        x = window_x;
        y = window_y;
    }
}

private void test_refreshing_connections_reads_the_links_in_the_editor() {
    var f = new NavigatorFixture();
    f.workspace.editor_buffer.set_text("[[Card 1]] then [[Card 2]]", -1);

    f.navigator.refresh_connections_from_editor();
    // The same links again leave the Connections tool as it is.
    f.navigator.refresh_connections_from_editor();

    assert(f.editor_text() == "[[Card 1]] then [[Card 2]]");
    f.destroy();
}

private void test_only_ctrl_enter_navigates() {
    var f = new NavigatorFixture();
    f.type_with_cursor_in("See [[Card 2]] here", "[[Card");

    assert(!f.navigator.handle_key(Gdk.Key.Return, 0, 0));
    assert(!f.navigator.handle_key(Gdk.Key.a, 0, Gdk.ModifierType.CONTROL_MASK));
    assert(f.api.get_card_calls == 0);
    f.destroy();
}

private void test_ctrl_enter_on_a_link_opens_its_card() {
    var f = new NavigatorFixture();
    f.type_with_cursor_in("See [[Card 2]] here", "[[Card");

    assert(f.ctrl_enter());

    assert(wait_for_condition(() => f.requested_cards.contains("c2")));
    f.destroy();
}

private void test_a_link_to_a_missing_card_offers_to_create_it() {
    var f = new NavigatorFixture();
    f.type_with_cursor_in("See [[Elsewhere]] here", "[[Else");

    // Elsewhere belongs to another project, so it does not count.
    assert(f.ctrl_enter());

    assert(f.has_toast("No card matches [[Elsewhere]]."));
    assert(wait_for_condition(() => f.window.get_visible_dialog() != null));
    var dialog = (Adw.AlertDialog) (!) f.window.get_visible_dialog();
    assert(dialog.get_heading() == "Create Linked Card?");
    dialog.response("create");
    assert(wait_for_condition(() => f.api.create_card_calls == 1));
    assert(f.api.last_created_title == "Elsewhere");
    f.destroy();
}

private void test_a_resource_image_asks_for_its_preview() {
    var f = new NavigatorFixture();
    f.type_with_cursor_in("Intro\n![Boiler](holder://resource/r-boiler)\n", "![Boi");

    assert(f.ctrl_enter());

    assert(f.previews.size == 1 && f.previews[0] == "r-boiler");
    f.destroy();
}

private void test_a_saved_tag_shows_the_tags_tool() {
    var f = new NavigatorFixture();
    var text = "Fix the #boiler soon";
    f.api.current_tag_occurrences = { new HolderLinux.CardTagOccurrence("boiler", 8, 15) };
    assert(wait_for_condition(() => f.harness.card_store.get_n_items() > 0));
    f.harness.card_selection.set_selected_index(0);
    f.harness.controller.load_card_by_id.begin("c1");
    assert(wait_for_condition(() => f.harness.controller.get_current_card() != null));
    f.type_with_cursor_in(text, "#boi");

    assert(f.ctrl_enter());

    assert(f.workspace.toolbox.is_showing_tool("tags"));
    f.destroy();
}

private void test_a_link_opens_in_the_browser_and_a_failure_is_shown() {
    var f = new NavigatorFixture();
    f.type_with_cursor_in("Read [the docs](https://holder.team/docs) first", "[the");

    assert(f.ctrl_enter());
    assert(f.launcher.launched.size == 1 && f.launcher.launched[0] == "https://holder.team/docs");

    f.launcher.failure = "no handler";
    assert(f.ctrl_enter());
    assert(f.has_toast("Could not open link: no handler"));
    f.destroy();
}

private void test_plain_text_leads_nowhere() {
    var f = new NavigatorFixture();
    f.type_with_cursor_in("Nothing to follow here", "Nothing");

    assert(!f.ctrl_enter());
    f.destroy();
}

private void test_a_ctrl_click_navigates_where_it_lands() {
    var f = new NavigatorFixture();
    f.present();
    f.type_with_cursor_in("See [[Card 2]] here", "[[Card");
    av_settle();
    double x;
    double y;
    f.point_at_cursor(out x, out y);

    // A plain click, a double click and a click below the text are left to the editor.
    assert(!f.navigator.navigate_at_point(1, false, x, y));
    assert(!f.navigator.navigate_at_point(2, true, x, y));
    assert(!f.navigator.navigate_at_point(1, true, x, f.workspace.editor_view.get_height() - 1));

    assert(f.navigator.navigate_at_point(1, true, x, y));
    assert(wait_for_condition(() => f.requested_cards.contains("c2")));
    f.destroy();
}

private void test_a_click_without_an_event_is_ignored() {
    var f = new NavigatorFixture();
    var gesture = new Gtk.GestureClick();

    assert(!f.navigator.handle_click(gesture, 1, 0, 0));
    f.destroy();
}

internal void register_internal_link_navigator_tests() {
    Test.add_func("/app_views/internal_link_navigator/refresh_connections", test_refreshing_connections_reads_the_links_in_the_editor);
    Test.add_func("/app_views/internal_link_navigator/only_ctrl_enter", test_only_ctrl_enter_navigates);
    Test.add_func("/app_views/internal_link_navigator/opens_linked_card", test_ctrl_enter_on_a_link_opens_its_card);
    Test.add_func("/app_views/internal_link_navigator/offers_to_create", test_a_link_to_a_missing_card_offers_to_create_it);
    Test.add_func("/app_views/internal_link_navigator/resource_preview", test_a_resource_image_asks_for_its_preview);
    Test.add_func("/app_views/internal_link_navigator/shows_tag", test_a_saved_tag_shows_the_tags_tool);
    Test.add_func("/app_views/internal_link_navigator/opens_uri", test_a_link_opens_in_the_browser_and_a_failure_is_shown);
    Test.add_func("/app_views/internal_link_navigator/plain_text", test_plain_text_leads_nowhere);
    Test.add_func("/app_views/internal_link_navigator/ctrl_click", test_a_ctrl_click_navigates_where_it_lands);
    Test.add_func("/app_views/internal_link_navigator/click_without_event", test_a_click_without_an_event_is_ignored);
}

}
