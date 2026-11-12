using GLib;

namespace HolderLinuxTests {

private class RecordingPresence : Object, HolderLinux.IPresence {
    public int starts = 0;
    public int stops = 0;
    public int leaves = 0;

    public void start() {
        starts++;
    }

    public void stop() {
        stops++;
    }

    public void leave(uint timeout_ms) {
        leaves++;
    }
}

// The update check's view of holder.team: it answers with `body` and counts what it was asked.
private class OfflineUpdateTransport : Object, HolderLinux.IUpdateMetadataTransport {
    public int fetches = 0;
    public string body = "{}";

    public async string fetch_text(string url) throws Error {
        fetches++;
        return body;
    }
}

private class WindowUriLauncher : Object, HolderLinux.IUriLauncher {
    public Gee.ArrayList<string> launched = new Gee.ArrayList<string>();
    public string? failure = null;

    public void launch(string uri) throws Error {
        if (failure != null) {
            throw new IOError.FAILED((!) failure);
        }
        launched.add(uri);
    }
}

// The main window, built with doubles for everything it reaches outside the app through: the fake
// API, a fixed daemon address, no daemon to start, a recorded presence, an offline update check and
// its own in-memory settings.
private class WindowFixture : Object {
    private static Adw.Application? shared_app = null;

    public MainControllerFakeApi api = new MainControllerFakeApi();
    public FakeServerDiscovery discovery = new FakeServerDiscovery();
    public RecordingPresence presence = new RecordingPresence();
    public OfflineUpdateTransport updates = new OfflineUpdateTransport();
    public WindowUriLauncher launcher = new WindowUriLauncher();
    public Settings settings;
    public HolderLinux.MainWindow window;

    public WindowFixture() {
        settings = new Settings.with_backend(
            HolderLinux.AppSettings.SCHEMA_ID,
            SettingsBackend.memory_settings_backend_new()
        );
    }

    public static Adw.Application app() {
        if (shared_app == null) {
            shared_app = new Adw.Application("team.holder.Holder.AppViewTests", ApplicationFlags.NON_UNIQUE);
            try {
                ((!) shared_app).register(null);
            } catch (Error e) {
                assert_not_reached();
            }
        }
        return (!) shared_app;
    }

    // Builds and shows the window; it starts talking to the fake API straight away.
    public void open() {
        var services = new HolderLinux.MainWindowServices();
        services.api_factory = new MainControllerFakeApiFactory(api, api);
        services.server_discovery = discovery;
        services.find_backend_starter = false;
        services.presence = presence;
        services.update_check = new HolderLinux.UpdateCheckService(updates);
        services.settings = settings;
        services.uri_launcher = launcher;
        window = new HolderLinux.MainWindow(app(), 1000, 700, services);
        window.present();
    }

    // Opens the window and waits until the first project is loaded.
    public void open_loaded() {
        open();
        assert(wait_for_condition(() => window.controller_for_tests().selected_project_id() == "p1"));
        av_settle();
    }

    public HolderLinux.MainController controller() {
        return window.controller_for_tests();
    }

    // Selects a card as the sidebar would, and waits until it is loaded into the editor.
    public void open_card(string card_id) {
        window.request_card_selection(card_id);
        assert(wait_for_condition(() => {
            var card = controller().get_current_card();
            return card != null && ((!) card).card_id == card_id;
        }));
        av_settle();
    }

    public string editor_text() {
        var buffer = window.workspace_for_tests().editor_buffer;
        Gtk.TextIter start;
        Gtk.TextIter end;
        buffer.get_bounds(out start, out end);
        return buffer.get_text(start, end, false);
    }

    public Adw.AlertDialog wait_for_dialog(Adw.Dialog? previous = null) {
        assert(wait_for_condition(() => {
            var current = window.get_visible_dialog();
            return current != null && current != previous && current is Adw.AlertDialog;
        }));
        return (Adw.AlertDialog) (!) window.get_visible_dialog();
    }

    public bool has_toast(string title) {
        return shows_label(title);
    }

    public bool shows_label(string text) {
        av_settle();
        return av_find_label(window, text) != null;
    }

    public void destroy() {
        window.destroy();
        av_settle();
    }
}

private void test_the_window_starts_and_loads_the_first_project() {
    var f = new WindowFixture();
    f.open();

    assert(wait_for_condition(() => f.shows_label("Project 1")));
    assert(f.api.list_projects_calls > 0);
    assert(f.presence.starts == 1);
    // The update check runs once it is due; nothing is newer here, so no dialog follows.
    assert(wait_for_condition(() => f.updates.fetches == 1));
    assert(f.window.get_visible_dialog() == null);
    f.destroy();
}

private void test_the_toolbar_actions_reach_the_controller() {
    var f = new WindowFixture();
    f.open_loaded();
    var w = f.window;

    var projects_before = f.api.list_projects_calls;
    w.handle_refresh_action();
    assert(wait_for_condition(() => f.api.list_projects_calls > projects_before));
    w.on_workspace_refresh_requested();
    assert(wait_for_condition(() => f.api.list_projects_calls > projects_before + 1));

    w.handle_new_card_action();
    assert(wait_for_condition(() => f.api.create_card_calls == 1));
    w.on_workspace_new_card_requested();
    assert(wait_for_condition(() => f.api.create_card_calls == 2));

    w.handle_new_project_action();
    var dialog = f.wait_for_dialog();
    assert(dialog.get_heading() == "New Project");
    foreach (var widget in av_descendants(dialog)) {
        if (widget is Gtk.Entry) {
            ((Gtk.Entry) widget).set_text("Garden");
        }
    }
    dialog.response("create");
    assert(wait_for_condition(() => f.api.create_project_calls == 1));
    assert(f.api.last_created_project_name == "Garden");

    w.on_workspace_new_project_requested();
    var second = f.wait_for_dialog(dialog);
    // A blank name creates nothing.
    second.response("create");
    av_settle();
    assert(f.api.create_project_calls == 1);
    f.destroy();
}

private void test_the_window_menu_opens_its_pages_and_dialogs() {
    var f = new WindowFixture();
    f.open_loaded();
    var w = f.window;
    var workspace = w.workspace_for_tests();

    w.handle_toggle_toolbox_action();
    assert(workspace.toolbox.widget.get_visible());
    w.handle_find_replace_action();
    assert(workspace.get_find_text() == "");

    w.handle_show_about_action();
    var about = f.wait_for_dialog();
    assert(about.get_heading() == "Holder " + HolderLinux.VERSION);
    about.response("close");

    w.handle_show_local_info_action();
    assert(wait_for_condition(() => f.editor_text().contains("Local")));

    w.handle_show_preferences_action();
    assert(wait_for_condition(() => w.get_visible_dialog() is HolderLinux.PreferencesDialog));
    ((!) w.get_visible_dialog()).force_close();

    // Printing an empty card says so instead of opening the print dialog.
    w.set_editor_state("", false);
    w.handle_print_action();
    assert(wait_for_condition(() => f.has_toast("Nothing to print.")));
    f.destroy();
}

private void test_card_actions_work_on_the_selected_card() {
    var f = new WindowFixture();
    f.open_loaded();
    var w = f.window;

    // With no card selected, the card shortcuts do nothing.
    w.handle_flowboard_new_child_card_action();
    w.handle_move_selected_card_to_trash_action();
    av_settle();
    assert(f.api.create_card_calls == 0);
    assert(w.get_visible_dialog() == null);

    f.open_card("c1");
    assert(f.editor_text() == "# Card 1\n\nBody");

    w.handle_flowboard_new_child_card_action();
    assert(wait_for_condition(() => f.api.create_card_calls == 1));
    assert(f.api.last_created_parent_card_id == "c1");
    w.on_sidebar_card_create_child_requested("c1");
    assert(wait_for_condition(() => f.api.create_card_calls == 2));

    // Typing in the editor marks the card changed; saving tidies a stray trailing space (two would
    // be a Markdown line break) and sends it.
    var buffer = w.workspace_for_tests().editor_buffer;
    buffer.set_text("# Card 1\n\nBody edited \n", -1);
    w.handle_save_action();
    assert(wait_for_condition(() => f.api.update_card_calls == 1));
    assert(f.api.last_updated_content == "# Card 1\n\nBody edited\n");

    w.handle_move_selected_card_to_trash_action();
    var dialog = f.wait_for_dialog();
    dialog.response("trash");
    assert(wait_for_condition(() => f.api.delete_card_calls == 1));

    w.on_sidebar_card_move_to_trash_requested("c1");
    var again = f.wait_for_dialog(dialog);
    again.response("cancel");
    av_settle();
    assert(f.api.delete_card_calls == 1);
    f.destroy();
}

private void test_closing_saves_the_window_and_says_goodbye() {
    var f = new WindowFixture();
    f.open_loaded();

    assert(!f.window.on_window_close_requested());

    assert(f.presence.leaves == 1);
    assert(f.settings.get_int(HolderLinux.AppSettings.KEY_WINDOW_WIDTH) == f.window.get_width());
    assert(f.settings.get_int(HolderLinux.AppSettings.KEY_WINDOW_HEIGHT) == f.window.get_height());
    assert(!f.settings.get_boolean(HolderLinux.AppSettings.KEY_WINDOW_MAXIMIZED));
    f.destroy();
}

private void test_a_recovery_draft_can_be_restored_or_discarded() {
    var f = new WindowFixture();
    f.open_loaded();
    f.open_card("c1");
    var draft = new HolderLinux.EditorRecoveryDraft("c1", "p1", "Card 1", "# Card 1\n\nUnsaved words", 999);

    f.window.show_recovery_draft(draft);
    var dialog = f.wait_for_dialog();
    dialog.response("restore");
    assert(f.editor_text() == "# Card 1\n\nUnsaved words");
    assert(f.has_toast("Recovered unsaved changes."));

    f.window.show_recovery_draft(draft);
    var second = f.wait_for_dialog(dialog);
    second.response("discard");
    f.window.show_recovery_draft(draft);
    var third = f.wait_for_dialog(second);
    third.response("later");
    f.destroy();
}

internal void register_window_tests() {
    Test.add_func("/app_views/window/starts_and_loads", test_the_window_starts_and_loads_the_first_project);
    Test.add_func("/app_views/window/toolbar_actions", test_the_toolbar_actions_reach_the_controller);
    Test.add_func("/app_views/window/menu_pages", test_the_window_menu_opens_its_pages_and_dialogs);
    Test.add_func("/app_views/window/card_actions", test_card_actions_work_on_the_selected_card);
    Test.add_func("/app_views/window/closing", test_closing_saves_the_window_and_says_goodbye);
    Test.add_func("/app_views/window/recovery_draft", test_a_recovery_draft_can_be_restored_or_discarded);
}

}
