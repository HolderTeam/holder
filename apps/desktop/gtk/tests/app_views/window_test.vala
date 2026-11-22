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

private class RecordingBackendStarter : Object, HolderLinux.IBackendStarter {
    public int ensures = 0;

    public async HolderLinux.EnsureOutcome ensure() throws Error {
        ensures++;
        return new HolderLinux.EnsureOutcome(true, "running", "", "fake ensure");
    }
}

private const string WINDOW_PNG_BASE64 =
    "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR4nGP4z8DwHwAFAAH/iZk9HQAAAABJRU5ErkJggg==";

// The fake API with resource storage: a local Storage Location, imports that finish at once (or
// fail), and assets that download as a one-pixel PNG (or fail).
private class WindowStorageApi : MainControllerFakeApi, HolderLinux.IResourceStorageApi {
    public string? preferred_location = "loc1";
    public bool fail_import = false;
    public bool fail_download = false;
    public int imports = 0;
    public int downloads = 0;

    public async HolderLinux.StorageLocationList list_storage_locations(string project_id) throws Error {
        return make_location_list({ make_location("loc1", "local", true) }, preferred_location);
    }

    public async string create_storage_location(string project_id, string name, string provider,
                                                Gee.HashMap<string, string> configuration) throws Error {
        return "loc2";
    }

    public async void bind_storage_location(string location_id, Gee.HashMap<string, string> values,
                                            string preview) throws Error {
    }

    public async void prefer_storage_location(string project_id, string location_id) throws Error {
    }

    public async void test_storage_location(string location_id) throws Error {
    }

    public async void delete_storage_location(string location_id) throws Error {
    }

    public async string start_google_drive_oauth(string location_id) throws Error {
        return "https://auth.example/consent";
    }

    public async HolderLinux.AssetImportJob start_asset_import(string project_id, string card_id,
                                                               string location_id,
                                                               string source_path) throws Error {
        imports++;
        return new HolderLinux.AssetImportJob("job1", "queued");
    }

    public async HolderLinux.AssetImportJob get_asset_import_job(string job_id) throws Error {
        if (fail_import) {
            return new HolderLinux.AssetImportJob(job_id, "failed", null, null, false, false, "disk full");
        }
        return new HolderLinux.AssetImportJob(job_id, "completed", "r-new", "a-new", false, true);
    }

    public async void download_asset(string resource_id, string asset_id, string destination_path) throws Error {
        downloads++;
        if (fail_download) {
            throw new IOError.FAILED("offline");
        }
        FileUtils.set_data(destination_path, Base64.decode(WINDOW_PNG_BASE64));
    }
}

// Answers file choosers the way a test sets: a chosen file, nothing (cancelled), or an error.
private class ScriptedFilePicker : Object, HolderLinux.IFilePicker {
    public File? chosen = null;
    public int asks = 0;

    public async File? pick_folder(Gtk.Window parent, string title) throws Error {
        asks++;
        return chosen;
    }

    public async File? pick_file(Gtk.Window parent, string title, bool images_only) throws Error {
        asks++;
        return chosen;
    }

    public async File? save_file(Gtk.Window parent, string title, string initial_name) throws Error {
        asks++;
        return chosen;
    }
}

// Recovery key files as the app writes them, except that "emailing" one is only recorded.
private class RecordingRecoveryService : Object, HolderLinux.IRecoveryService {
    private HolderLinux.RecoveryService files = new HolderLinux.RecoveryService();
    public Gee.ArrayList<string> emailed = new Gee.ArrayList<string>();

    public string build_safe_filename(string project_name) {
        return files.build_safe_filename(project_name);
    }

    public string write_payload_to_temp_attachment(string project_name, string payload) throws Error {
        var path = Path.build_filename(av_temp_dir(), build_safe_filename(project_name));
        files.save_payload_to_path(path, payload);
        return path;
    }

    public void open_email_with_attachment(string attachment_path) throws Error {
        emailed.add(attachment_path);
    }

    public void save_payload_to_path(string path, string payload) throws Error {
        files.save_payload_to_path(path, payload);
    }

    public string load_payload_from_path(string path) throws Error {
        return files.load_payload_from_path(path);
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
    public HolderLinux.IApiFactory? api_factory = null;
    public FakeServerDiscovery discovery = new FakeServerDiscovery();
    public RecordingPresence presence = new RecordingPresence();
    public OfflineUpdateTransport updates = new OfflineUpdateTransport();
    public WindowUriLauncher launcher = new WindowUriLauncher();
    public ScriptedFilePicker picker = new ScriptedFilePicker();
    public RecordingRecoveryService recovery = new RecordingRecoveryService();
    public Settings settings;
    public bool without_settings = false;
    // Null with find_backend unset uses a daemon that is already running, as most tests do.
    public HolderLinux.IBackendStarter? backend_starter = null;
    public bool find_backend = false;
    // The size asked for on the command line; 0 uses the saved one.
    public int startup_width = 1400;
    public int startup_height = 900;
    public HolderLinux.MainWindow window;

    public WindowFixture() {
        // The editor writes recovery drafts under the user state directory a moment after an
        // edit. Give each window its own, so a draft one test leaves behind (the snapshot can
        // land before a slow test destroys its window) never offers itself to a later test.
        Environment.set_variable("XDG_STATE_HOME", av_temp_dir(), true);
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
        services.api_factory = api_factory ?? new MainControllerFakeApiFactory(api, api);
        services.server_discovery = discovery;
        services.find_backend_starter = find_backend;
        services.backend_starter = backend_starter;
        services.presence = presence;
        services.update_check = new HolderLinux.UpdateCheckService(updates);
        services.settings = without_settings ? null : settings;
        services.uri_launcher = launcher;
        services.file_picker = picker;
        services.recovery_service = recovery;
        window = new HolderLinux.MainWindow(app(), startup_width, startup_height, services);
        window.present();
    }

    // Opens the window and waits until the first project is loaded.
    public void open_loaded() {
        open();
        assert(wait_for_condition(() => window.controller_for_tests.selected_project_id() == "p1"));
        av_settle();
    }

    public HolderLinux.MainController controller() {
        return window.controller_for_tests;
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
        var buffer = window.workspace_for_tests.editor_buffer;
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

    public string debug_log() {
        var text = new StringBuilder();
        foreach (var widget in av_descendants(window)) {
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

    // Which page the asset preview shows: loading, image, document, error or empty.
    public string preview_state() {
        foreach (var widget in av_descendants(window.workspace_for_tests.asset_preview.widget)) {
            var stack = widget as Gtk.Stack;
            if (stack != null && ((!) stack).get_child_by_name("loading") != null) {
                return (!) ((!) stack).get_visible_child_name();
            }
        }
        assert_not_reached();
    }

    // Types a PIN into the open PIN dialog and continues.
    public void enter_pin(string pin, Adw.Dialog? previous = null) {
        var dialog = wait_for_dialog(previous);
        foreach (var widget in av_descendants(dialog)) {
            var entry = widget as Gtk.Entry;
            if (entry != null && ((!) entry).get_placeholder_text() == "PIN") {
                ((!) entry).set_text(pin);
            }
        }
        av_answer(dialog, "continue");
    }

    public bool has_toast(string title) {
        return shows_label(title);
    }

    // Errors reach the Debug tool immediately, even when their toasts are queued behind others.
    public bool reports_error(string title, string details) {
        return debug_contains("ERROR: %s | %s\n".printf(title, details));
    }

    public bool debug_contains(string text) {
        foreach (var widget in av_descendants(window.workspace_for_tests.toolbox.debug_tool.widget)) {
            var view = widget as Gtk.TextView;
            if (view != null) {
                Gtk.TextIter start;
                Gtk.TextIter end;
                ((!) view).buffer.get_bounds(out start, out end);
                if (((!) view).buffer.get_text(start, end, false).contains(text)) {
                    return true;
                }
            }
        }
        return false;
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
    av_answer(dialog, "create");
    assert(wait_for_condition(() => f.api.create_project_calls == 1));
    assert(f.api.last_created_project_name == "Garden");

    w.on_workspace_new_project_requested();
    var second = f.wait_for_dialog(dialog);
    // A blank name creates nothing.
    av_answer(second, "create");
    av_settle();
    assert(f.api.create_project_calls == 1);
    f.destroy();
}

private void test_the_window_menu_opens_its_pages_and_dialogs() {
    var f = new WindowFixture();
    f.open_loaded();
    var w = f.window;
    var workspace = w.workspace_for_tests;

    w.handle_toggle_toolbox_action();
    assert(workspace.toolbox.widget.get_visible());
    w.handle_find_replace_action();
    assert(workspace.get_find_text() == "");

    w.handle_show_about_action();
    var about = f.wait_for_dialog();
    assert(about.get_heading() == "Holder " + HolderLinux.VERSION);
    av_answer(about, "close");

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
    var buffer = w.workspace_for_tests.editor_buffer;
    buffer.set_text("# Card 1\n\nBody edited \n", -1);
    w.handle_save_action();
    assert(wait_for_condition(() => f.api.update_card_calls == 1));
    assert(f.api.last_updated_content == "# Card 1\n\nBody edited\n");

    w.handle_move_selected_card_to_trash_action();
    var dialog = f.wait_for_dialog();
    av_answer(dialog, "trash");
    assert(wait_for_condition(() => f.api.delete_card_calls == 1));

    w.on_sidebar_card_move_to_trash_requested("c1");
    var again = f.wait_for_dialog(dialog);
    av_answer(again, "cancel");
    av_settle();
    assert(f.api.delete_card_calls == 1);
    f.destroy();
}

private void test_closing_saves_the_window_and_says_goodbye() {
    var f = new WindowFixture();
    f.open_loaded();

    assert(!f.window.on_window_close_requested());

    assert(f.presence.leaves == 1);
    // The size is saved unless the window is maximized (macOS zooms a window larger than the
    // screen, so the CI runners may maximize this one).
    var maximized = f.window.is_maximized();
    assert(f.settings.get_boolean(HolderLinux.AppSettings.KEY_WINDOW_MAXIMIZED) == maximized);
    if (!maximized) {
        assert(f.settings.get_int(HolderLinux.AppSettings.KEY_WINDOW_WIDTH) == f.window.get_width());
        assert(f.settings.get_int(HolderLinux.AppSettings.KEY_WINDOW_HEIGHT) == f.window.get_height());
    }
    f.destroy();
}

private void test_a_recovery_draft_can_be_restored_or_discarded() {
    var f = new WindowFixture();
    var draft = new HolderLinux.EditorRecoveryDraft("c1", "p1", "Card 1", "# Card 1\n\nUnsaved words", 999);
    try {
        new HolderLinux.EditorRecoveryDraftService().save_draft(draft);
    } catch (Error e) {
        assert_not_reached();
    }
    f.open_loaded();
    f.open_card("c1");

    var dialog = f.wait_for_dialog();
    av_answer(dialog, "restore");
    assert(f.editor_text() == "# Card 1\n\nUnsaved words");
    assert(f.has_toast("Recovered unsaved changes."));

    f.window.show_recovery_draft(draft);
    var second = f.wait_for_dialog(dialog);
    av_answer(second, "discard");
    f.window.show_recovery_draft(draft);
    var third = f.wait_for_dialog(second);
    av_answer(third, "later");
    f.destroy();
}

private void test_the_workspace_panels_follow_their_toggles() {
    var f = new WindowFixture();
    f.open_loaded();
    var w = f.window;
    var workspace = w.workspace_for_tests;

    assert(f.shows_label("Projects"));
    // Hiding the explorer briefly measures the workspace narrower than its header needs, and GTK
    // warns ("Trying to measure GtkBox ... for width of 350, but it needs at least 621"). That is an
    // existing layout problem, not this test's, so that warning fails nothing for these two steps.
    av_tolerate_warning("Trying to measure GtkBox");
    w.on_workspace_explorer_panel_toggled(false);
    assert(!f.shows_label("Projects"));
    w.on_workspace_explorer_panel_toggled(true);
    assert(f.shows_label("Projects"));
    av_stop_tolerating();
    // Dragging the sidebar divider is remembered for the next start.
    w.on_root_paned_position_changed(300);

    w.on_workspace_ai_panel_toggled(true);
    assert(workspace.is_ai_panel_visible());
    // With the panel open, a selection or connection change refreshes it too.
    w.on_app_state_changed();
    w.on_api_client_connected(f.api);
    w.refresh_ai_status();
    w.on_workspace_ai_panel_toggled(false);
    assert(!workspace.is_ai_panel_visible());
    w.refresh_ai_status();

    w.on_workspace_toolbox_toggled(true);
    assert(workspace.toolbox.widget.get_visible());
    w.on_workspace_toolbox_toggled(false);
    assert(!workspace.toolbox.widget.get_visible());
    w.on_workspace_open_debug_panel_requested();
    assert(workspace.toolbox.is_showing_tool("debug"));

    w.on_navigation_loading_changed(true);
    w.on_navigation_loading_changed(false);

    assert(!w.on_window_close_requested());
    assert(f.settings.get_int(HolderLinux.AppSettings.KEY_SIDEBAR_WIDTH) == 300);
    f.destroy();
}

private void test_searching_runs_shows_and_clears_results() {
    var f = new WindowFixture();
    f.api.search_returns_card2 = true;
    f.api.include_card2 = true;
    f.open_loaded();
    var w = f.window;
    var workspace = w.workspace_for_tests;

    // With nothing found yet, there are no results to move to.
    w.on_workspace_search_focus_results_requested();
    assert(workspace.content_stack.get_visible_child_name() == "editor");

    workspace.search_entry.set_text("card");
    w.on_workspace_search_changed();
    assert(wait_for_condition(() => f.api.search_calls == 1));
    w.on_workspace_search_activated();
    assert(wait_for_condition(() => f.api.search_calls == 2));
    assert(wait_for_condition(() => f.window.search_store_for_tests.get_n_items() == 1));

    w.on_workspace_search_focus_results_requested();
    assert(workspace.content_stack.get_visible_child_name() == "search");
    assert(workspace.search_selection.get_selected() == 0);
    w.on_workspace_search_result_activated(0);
    assert(wait_for_condition(() => {
        var card = f.controller().get_current_card();
        return card != null && ((!) card).card_id == "c2";
    }));

    workspace.search_entry.set_text("  ");
    w.on_workspace_search_changed();
    assert(workspace.content_stack.get_visible_child_name() == "editor");
    workspace.search_entry.set_text("card");
    w.on_workspace_search_cleared();
    assert(workspace.search_entry.get_text() == "");
    assert(f.window.search_store_for_tests.get_n_items() == 0);
    f.destroy();
}

private void test_find_and_replace_edit_the_card() {
    var f = new WindowFixture();
    f.open_loaded();
    f.open_card("c1");
    var w = f.window;
    var workspace = w.workspace_for_tests;
    workspace.show_find_replace_bar(true);
    foreach (var widget in av_descendants(workspace.widget)) {
        var entry = widget as Gtk.Entry;
        if (entry != null && ((!) entry).get_placeholder_text() == "Search text") {
            ((!) entry).set_text("Body");
        } else if (entry != null && ((!) entry).get_placeholder_text() == "Replacement text") {
            ((!) entry).set_text("Text");
        }
    }

    w.on_workspace_find_next_requested();
    Gtk.TextIter a;
    Gtk.TextIter b;
    assert(workspace.editor_buffer.get_selection_bounds(out a, out b));
    w.on_workspace_replace_requested();
    assert(f.editor_text() == "# Card 1\n\nText");
    workspace.editor_buffer.set_text("Body and Body", -1);
    w.on_workspace_replace_all_requested();
    assert(f.editor_text() == "Text and Text");
    f.destroy();
}

private void test_the_window_shows_what_the_controller_reports() {
    var f = new WindowFixture();
    f.open_loaded();
    f.open_card("c1");
    var w = f.window;

    w.update_window_title("Renamed");
    assert(w.get_title() == "Renamed");
    w.set_editor_save_state_text("Saved");
    assert(f.shows_label("Saved"));
    w.set_search_summary_text("2 results");
    assert(w.workspace_for_tests.search_summary_label.get_text() == "2 results");
    w.set_ai_thread_title("Planning");
    assert(f.shows_label("Thread: Planning"));
    w.set_validated_tag_occurrences({});

    w.append_text_to_current_card("$ make test");
    assert(f.editor_text().has_suffix("$ make test"));

    w.show_tool_help_page("flowboard");
    assert(w.get_title() != "Renamed");

    var gets = f.api.get_card_calls;
    w.reload_card_after_history_restore("c1");
    assert(wait_for_condition(() => f.api.get_card_calls > gets));
    w.select_ai_thread_from_history("t1");
    w.refresh_milestones_after_card_save();
    w.refresh_history_after_card_save("p1", "c1");
    w.refresh_trash_tool();
    w.on_flowboard_project_overview_requested("p1");
    w.on_flowboard_context_load_requested("p1", null);

    var lists = f.api.list_cards_calls;
    var restored = false;
    w.on_trash_item_restored.begin("c1", (obj, res) => {
        w.on_trash_item_restored.end(res);
        restored = true;
    });
    assert(wait_for_condition(() => restored));
    assert(f.api.list_cards_calls > lists);

    w.on_project_create_error_reported("Could not create project", "Name taken");
    assert(wait_for_condition(() => f.reports_error("Could not create project", "Name taken")));
    f.destroy();
}

private class WindowSyncApi : MainControllerFakeApi {
    public string pulled_content = "# Card 1\n\nRemote edits";
    public string pulled_title = "Renamed on the other Desktop";
    public bool reject_push = false;
    public bool stall_next_card_read = false;
    public bool fail_stalled_card_read = false;
    private SourceFunc? stalled_card_read = null;

    public bool has_stalled_card_read() {
        return stalled_card_read != null;
    }

    public void release_stalled_card_read() {
        var resume = (owned) stalled_card_read;
        assert(resume != null);
        resume();
    }

    public override async HolderLinux.GitSyncResult sync_project_git(string project_id) throws Error {
        var result = yield base.sync_project_git(project_id);
        if (!reject_push) {
            return result;
        }
        var push = new HolderLinux.GitPushResult(
            project_id, "", "", "failed", 1, 0, "", "Remote rejected the push", "");
        return new HolderLinux.GitSyncResult(result.pull, true, push);
    }

    public override async HolderLinux.CardDetail get_card(string project_id, string card_id) throws Error {
        var card = yield base.get_card(project_id, card_id);
        if (sync_project_git_calls > 0) {
            card.content = pulled_content;
            card.title = pulled_title;
        }
        if (stall_next_card_read) {
            stall_next_card_read = false;
            var fail = fail_stalled_card_read;
            stalled_card_read = get_card.callback;
            yield;
            if (fail) {
                throw new IOError.FAILED("Older pulled card could not be read");
            }
        }
        return card;
    }
}

private Gtk.Button window_sync_button(WindowFixture f) {
    foreach (var widget in av_descendants(f.window.workspace_for_tests.toolbox.widget)) {
        var button = widget as Gtk.Button;
        if (button != null && button.get_label() == "Sync now") {
            return button;
        }
    }
    assert_not_reached();
}

private class WindowConflictApi : MainControllerFakeApi, HolderLinux.IConflictComparisonApi {
    public int resolution_calls;
    public int comparison_calls;
    public int trash_copy_calls;
    public bool reject_next_resolution;
    public bool reject_next_trash;
    public bool copy_trashed;
    public string? resolved_title;
    public string? resolved_content;

    public override async Gee.ArrayList<HolderLinux.Project> list_projects() throws Error {
        var projects = yield base.list_projects();
        if (!copy_trashed) {
            projects[0].sync.conflict_copies.add(new HolderLinux.GitConflictCopy(
                "c2", "Preserved edit", "c1", "Original", false));
        }
        return projects;
    }

    public override async HolderLinux.CardDetail get_card(string project_id, string card_id) throws Error {
        var card = yield base.get_card(project_id, card_id);
        if (card_id == "c2") {
            card.title = "Renamed preserved edit";
            card.content = "# Renamed preserved edit\n\nWords from the other version";
        } else if (card_id == "c1" && resolved_title != null && resolved_content != null) {
            card.title = resolved_title;
            card.content = resolved_content;
        }
        return card;
    }

    public async HolderLinux.ConflictComparison compare_conflict_copy(string project_id,
                                                                       string copy_id) throws Error {
        var original = yield get_card(project_id, "c1");
        var copy = yield get_card(project_id, copy_id);
        comparison_calls++;
        return new HolderLinux.ConflictComparison(original, copy, { 1, 3 }, { 1, 3 }, false,
                                                  "review-%d".printf(comparison_calls));
    }

    public async HolderLinux.ConflictResolutionResult resolve_conflict_copy(
        string project_id, string copy_id, string review_token, string title, string content) throws Error {
        resolution_calls++;
        assert(project_id == "p1");
        assert(copy_id == "c2");
        assert(review_token == "review-%d".printf(comparison_calls));
        if (reject_next_resolution) {
            reject_next_resolution = false;
            throw new HolderLinux.ApiError.HTTP("A reviewed version changed.");
        }
        resolved_title = title;
        resolved_content = content;
        var cleanup = new HolderLinux.ConflictCopyCleanupReview("cleanup-" + review_token);
        cleanup.preserves_original = true;
        cleanup.copy_tags = { "copy-only" };
        cleanup.copy_milestones = { "Ship Friday" };
        cleanup.children = { "Preserved checklist" };
        return new HolderLinux.ConflictResolutionResult(yield get_card(project_id, "c1"), cleanup);
    }

    public async void trash_conflict_copy(string project_id, string copy_id, string cleanup_token) throws Error {
        trash_copy_calls++;
        assert(project_id == "p1" && copy_id == "c2");
        assert(cleanup_token == "cleanup-review-%d".printf(comparison_calls));
        if (reject_next_trash) {
            reject_next_trash = false;
            throw new HolderLinux.ApiError.HTTP("The cleanup details changed.");
        }
        copy_trashed = true;
        include_card2 = false;
    }
}

// Answers the sync state with a pull report the test sets: the conflict copy c2 of c1 by
// default, as a pull that could not merge an edit records it.
private class WindowPullReportApi : WindowConflictApi, HolderLinux.IProjectSyncStatusApi {
    public int sync_status_calls;
    public string[] sync_status_projects = {};
    public HolderLinux.GitPullChanges? report = null;
    public int64 reported_at = new DateTime.now_utc().to_unix();
    // When the last pull finished, 0 for never.
    public int64 pulled_at = 0;
    public bool fail_sync_status;

    public WindowPullReportApi() {
        var changes = new HolderLinux.GitPullChanges();
        changes.merged_card_ids.add("c1");
        changes.conflict_copy_ids.add("c2");
        report = changes;
    }

    public async HolderLinux.ProjectSyncState get_project_sync_status(string project_id) throws Error {
        sync_status_calls++;
        sync_status_projects += project_id;
        if (fail_sync_status) {
            throw new HolderLinux.ApiError.HTTP("Sync state unavailable");
        }
        var projects = yield list_projects();
        var state = projects[0].sync;
        state.last_pull_changes = report;
        state.last_pull_changes_at = reported_at;
        state.has_last_pull_at = pulled_at > 0;
        state.last_pull_at = pulled_at;
        return state;
    }
}

private Gtk.Button? toast_button(WindowFixture f, string label) {
    av_settle();
    foreach (var widget in av_descendants(f.window)) {
        var button = widget as Gtk.Button;
        if (button != null && button.get_label() == label && button.is_drawable()) return button;
    }
    return null;
}

private const string PULL_REPORT_MESSAGE =
    "Pulled changes from another device: merged edits into 1 card and saved your version of a card as a copy.";

private Gtk.Button comparison_button(Adw.Dialog dialog, string label) {
    foreach (var widget in av_descendants(dialog)) {
        var button = widget as Gtk.Button;
        if (button != null && button.label == label) return button;
    }
    assert_not_reached();
}

private void wait_for_dialog_open_frames(Adw.Dialog dialog) {
    // Libadwaita 1.5 on Ubuntu 24.04 can lose a synthetic close request before
    // the sheet opens. The macOS CI display can skip these frame callbacks.
    if (Environment.get_variable("HOLDER_DESKTOP_TEST_PLATFORM") == "darwin") return;
    int frames = 0;
    dialog.add_tick_callback((widget, clock) => ++frames < 2);
    assert(wait_for_condition(() => frames == 2));
}

private Gtk.TextView conflict_result_editor(Adw.Dialog dialog) {
    foreach (var widget in av_descendants(dialog)) {
        var content = widget as Gtk.TextView;
        if (content != null && content.editable) return content;
    }
    assert_not_reached();
}

private bool comparison_contains_text(Adw.Dialog dialog, string text) {
    foreach (var widget in av_descendants(dialog)) {
        var content = widget as Gtk.TextView;
        if (content != null && content.get_buffer().text == text) {
            assert(!content.get_editable());
            var buffer = content.get_buffer();
            var changed = buffer.get_tag_table().lookup("changed-line");
            assert(changed != null);
            Gtk.TextIter start;
            buffer.get_start_iter(out start);
            assert(start.has_tag(changed));
            buffer.get_iter_at_line(out start, 1);
            assert(!start.has_tag(changed));
            buffer.get_iter_at_line(out start, 2);
            assert(start.has_tag(changed));
            return true;
        }
    }
    return false;
}

// Each conflict case uses a fresh window and runs in its own test process.
private class ConflictWindowFixture : WindowFixture {
    public WindowConflictApi conflict_api;
    public Gtk.Button copy;
    private Gtk.Button compare;

    public ConflictWindowFixture(string selected_card = "c1") {
        Gtk.Settings.get_default().gtk_enable_animations = false;
        conflict_api = new WindowConflictApi();
        api = conflict_api;
        api.include_card2 = true;
        open_loaded();
        open_card(selected_card);
        var toolbox = window.workspace_for_tests.toolbox;
        toolbox.show_tool("git");
        assert(wait_for_condition(() => {
            foreach (var widget in av_descendants(toolbox.widget)) {
                var button = widget as Gtk.Button;
                if (button != null && button.label == "Copy “Preserved edit”") {
                    copy = button;
                } else if (button != null && button.label == "Compare versions") {
                    compare = button;
                }
            }
            return copy != null && compare != null;
        }));
    }

    public Adw.Dialog open_comparison() {
        compare.clicked();
        assert(wait_for_condition(() => window.get_visible_dialog() != null));
        var dialog = (!) window.get_visible_dialog();
        wait_for_dialog_open_frames(dialog);
        return dialog;
    }

    public Adw.Dialog review_cleanup() {
        var comparison = open_comparison();
        assert(wait_for_condition(() => conflict_api.comparison_calls == 1));
        comparison_button(comparison, "Keep original").clicked();
        assert(wait_for_condition(() => av_find_label(comparison,
            "Saved to the original card. The preserved copy is still available.") != null));
        comparison_button(comparison, "Review preserved copy…").clicked();
        return comparison;
    }
}

private void test_discarding_a_conflict_draft() {
    var f = new ConflictWindowFixture();
    var api = f.conflict_api;
    Test.message("Comparing both versions and discarding a draft");
    var comparison = f.open_comparison();
    assert(wait_for_condition(() => {
        var dialog = f.window.get_visible_dialog();
        return dialog != null &&
            comparison_contains_text(dialog, "# Card 1\n\nBody") &&
            comparison_contains_text(dialog, "# Renamed preserved edit\n\nWords from the other version");
    }));
    assert(f.api.update_card_calls == 0);
    assert(f.controller().selected_card_id() == "c1");
    assert(f.editor_text() == "# Card 1\n\nBody");
    comparison_button(comparison, "Combine versions").clicked();
    var combined_editor = conflict_result_editor(comparison);
    assert(combined_editor.buffer.text == "# Card 1\n\nBody");
    combined_editor.buffer.text = "Discard this draft";
    assert(!comparison.close());
    var discard = f.wait_for_dialog(comparison);
    wait_for_dialog_open_frames(discard);
    Test.message("Confirming discard");
    av_answer(discard, "discard");
    assert(wait_for_condition(() => f.window.get_visible_dialog() == null));
    assert(api.resolution_calls == 0);
    assert(f.editor_text() == "# Card 1\n\nBody");
    f.destroy();
}

private void test_keeping_the_original_and_preserved_copy() {
    var f = new ConflictWindowFixture();
    var api = f.conflict_api;
    Test.message("Keeping the original and preserving the copy");
    var comparison = f.review_cleanup();
    assert(api.resolution_calls == 1);
    assert(api.resolved_title == "Card 1");
    assert(api.resolved_content == "# Card 1\n\nBody");
    assert(av_find_label(comparison, "Review preserved copy") != null);
    assert(av_find_label(comparison, "Ship Friday") != null);
    assert(av_find_label(comparison, "Preserved checklist") != null);
    comparison_button(comparison, "Keep copy").clicked();
    assert(api.trash_copy_calls == 0);
    comparison.close();
    assert(wait_for_condition(() => f.window.get_visible_dialog() == null));
    f.destroy();
}

private void test_saving_after_refreshing_a_conflict_review() {
    var f = new ConflictWindowFixture();
    var api = f.conflict_api;
    Test.message("Refreshing a stale review and saving the result");
    var comparison = f.open_comparison();
    comparison_button(comparison, "Start from copy").clicked();
    Gtk.Entry? result_title = null;
    foreach (var widget in av_descendants(comparison)) {
        var title = widget as Gtk.Entry;
        if (title != null) result_title = title;
    }
    var result_content = conflict_result_editor(comparison);
    assert(result_title != null && result_title.text == "Renamed preserved edit");
    assert(result_content.buffer.text == "# Renamed preserved edit\n\nWords from the other version");
    assert(api.resolution_calls == 0);
    string combined = "# Combined\n\nBoth versions";
    ((!) result_title).text = "Combined";
    result_content.buffer.text = combined;
    api.reject_next_resolution = true;
    comparison_button(comparison, "Save to original").clicked();
    assert(wait_for_condition(() => av_find_label(comparison,
        "Could not save the result.\nA reviewed version changed.") != null));
    assert(f.editor_text() == "# Card 1\n\nBody");
    comparison_button(comparison, "Refresh versions").clicked();
    assert(wait_for_condition(() => av_find_label(comparison,
        "Versions refreshed. Review both versions before saving.") != null));
    assert(result_content.buffer.text == combined);
    assert(((!) result_title).text == "Combined");
    comparison_button(comparison, "Save to original").clicked();
    assert(wait_for_condition(() => f.editor_text() == combined));
    assert(api.resolved_title == "Combined");
    assert(f.controller().selected_card_id() == "c1");
    assert(f.api.update_card_calls == 0);
    assert(av_find_label(comparison, "Saved to the original card. The preserved copy is still available.") != null);
    comparison.close();
    assert(wait_for_condition(() => f.window.get_visible_dialog() == null));
    f.destroy();
}

private void test_opening_the_preserved_card_after_a_failed_comparison() {
    var f = new ConflictWindowFixture();
    Test.message("Checking load failure and opening the preserved card");
    f.api.fail_get_card_once = true;
    var comparison = f.open_comparison();
    assert(wait_for_condition(() => av_find_label(comparison,
        "Could not load both card versions.\n" + f.api.get_card_failure_message) != null));
    comparison.close();
    assert(wait_for_condition(() => f.window.get_visible_dialog() == null));

    const string draft = "# Card 1\n\nSave before opening the conflict";
    f.window.workspace_for_tests.editor_buffer.set_text(draft, -1);
    f.api.fail_list_cards_once = true;
    f.copy.clicked();
    assert(wait_for_condition(() => !f.api.fail_list_cards_once));
    assert(f.controller().selected_card_id() == "c1");
    assert(f.api.last_updated_content == draft);

    f.copy.clicked();
    assert(wait_for_condition(() => {
        var card = f.controller().get_current_card();
        return card != null && card.card_id == "c2";
    }));
    f.destroy();
}

private void test_stale_conflict_cleanup_leaves_the_preserved_copy() {
    var f = new ConflictWindowFixture("c2");
    var api = f.conflict_api;
    Test.message("Confirming cleanup with a stale review");
    var comparison = f.review_cleanup();
    api.reject_next_trash = true;
    comparison_button(comparison, "Move copy to Trash").clicked();
    assert(wait_for_condition(() => av_find_label(comparison,
        "Could not move the copy to Trash.\nThe cleanup details changed.\n" +
        "Close this dialog and compare again for a fresh review.") != null));
    assert(f.controller().selected_card_id() == "c2");
    assert(!api.copy_trashed);
    comparison.close();
    assert(wait_for_condition(() => f.window.get_visible_dialog() == null));
    f.destroy();
}

private void test_confirmed_conflict_cleanup_removes_the_preserved_copy() {
    var f = new ConflictWindowFixture("c2");
    var api = f.conflict_api;
    Test.message("Confirming cleanup with a fresh review");
    var comparison = f.review_cleanup();
    comparison_button(comparison, "Move copy to Trash").clicked();
    assert(wait_for_condition(() => av_find_label(comparison,
        "The preserved copy was moved to Trash. You can restore it from there.") != null));
    assert(wait_for_condition(() => f.controller().selected_card_id() != "c2"));
    assert(api.copy_trashed);
    assert(api.resolved_content == "# Card 1\n\nBody");
    f.destroy();
}

private void test_pull_deletion_preserves_unsaved_editor_text() {
    var f = new WindowFixture();
    f.open_loaded();
    f.open_card("c1");
    var workspace = f.window.workspace_for_tests;
    const string draft = "# Card 1\n\nAn unsaved edit";
    workspace.editor_buffer.set_text(draft, -1);

    int reads_before_pull = f.api.get_card_calls;
    workspace.toolbox.git_project_pulled("p1");
    av_settle();
    assert(f.api.get_card_calls == reads_before_pull);
    assert(f.editor_text() == draft);

    f.api.list_cards_empty = true;
    workspace.toolbox.git_project_pulled("p1");
    assert(wait_for_condition(() => f.has_toast(
        "The pulled changes removed the open card. Your unsaved edits are still in the editor.")));
    assert(f.editor_text() == draft);
    assert(f.controller().selected_card_id() == "c1");
    f.destroy();
}

private void test_pull_refresh_handles_missing_selection_and_errors() {
    var f = new WindowFixture();
    f.open_loaded();
    var toolbox = f.window.workspace_for_tests.toolbox;
    int before = f.api.list_cards_calls;
    toolbox.git_project_pulled("p-other");
    assert(f.api.list_cards_calls == before);

    toolbox.git_project_pulled("p1");
    assert(f.controller().selected_card_id() == null);

    f.api.fail_list_cards_once = true;
    f.api.list_cards_failure_message = "Connection interrupted during refresh";
    toolbox.git_project_pulled("p1");
    assert(wait_for_condition(() => f.reports_error(
        "Could not refresh cards after pull", "Connection interrupted during refresh")));

    f.open_card("c1");
    f.api.list_cards_empty = true;
    f.api.list_cards_before_complete_hook = () => {
        f.api.list_cards_before_complete_hook = null;
        f.window.request_project_selection(null);
    };
    toolbox.git_project_pulled("p1");
    assert(wait_for_condition(() => f.controller().selected_project_id() == null));
    // The old, empty reply must not replace the cached list after the selection is cleared.
    assert(f.controller().has_card_summary("c1"));
    f.destroy();
}

private void test_sync_now_refreshes_remote_edits_and_deletions() {
    var f = new WindowFixture();
    var api = new WindowSyncApi();
    f.api = api;
    f.open_loaded();
    f.open_card("c1");

    var sync = window_sync_button(f);
    api.reject_push = true;
    sync.clicked();
    assert(wait_for_condition(() => f.editor_text() == api.pulled_content));
    assert(f.reports_error("Git push failed", "Pull succeeded.\nRemote rejected the push"));
    assert(f.window.get_title().contains(api.pulled_title));
    assert(f.controller().selected_card_id() == "c1");
    assert(api.sync_project_git_calls == 1);
    assert(api.push_project_git_calls == 0);
    assert(wait_for_condition(() => sync.get_sensitive()));

    // A draft typed while the refreshed card is being read must win over that response.
    var editor_buffer = f.window.workspace_for_tests.editor_buffer;
    api.reject_push = false;
    api.pulled_content = "# Card 1\n\nAnother remote edit";
    api.get_card_before_complete_hook = (card_id) => {
        if (card_id == "c1") {
            editor_buffer.set_text("# Card 1\n\nLocal draft", -1);
        }
    };
    sync.clicked();
    assert(wait_for_condition(() => f.editor_text() == "# Card 1\n\nLocal draft" && sync.get_sensitive()));
    assert(f.controller().has_unsaved_editor_changes());
    api.get_card_before_complete_hook = null;
    // Restore the committed editor state before testing deletion of a clean card.
    assert(wait_for_condition(() => !f.controller().is_editor_save_in_flight()));
    f.controller().set_loaded_card_editor_state((!) f.controller().get_current_card());
    assert(!f.controller().has_unsaved_editor_changes());

    api.list_cards_empty = true;
    sync.clicked();
    assert(wait_for_condition(() => f.controller().get_current_card() == null));
    assert(f.controller().selected_card_id() == null);
    assert(!f.controller().has_card_summary("c1"));
    assert(api.sync_project_git_calls == 3);
    f.destroy();
}

private void test_newer_pull_wins_over_an_older_card_reply_or_error() {
    var f = new WindowFixture();
    var api = new WindowSyncApi();
    f.api = api;
    f.open_loaded();
    f.open_card("c1");
    var sync = window_sync_button(f);

    // The Sync button can become ready again while the window's card refresh is still pending.
    for (int i = 0; i < 2; i++) {
        api.pulled_content = "# Card 1\n\nOld remote content %d".printf(i);
        api.stall_next_card_read = true;
        api.fail_stalled_card_read = i == 1;
        sync.clicked();
        assert(wait_for_condition(() => api.has_stalled_card_read() && sync.get_sensitive()));

        var latest = "# Card 1\n\nLatest remote content %d".printf(i);
        api.pulled_content = latest;
        sync.clicked();
        assert(wait_for_condition(() => f.editor_text() == latest && sync.get_sensitive()));

        api.release_stalled_card_read();
        av_settle();
        assert(f.editor_text() == latest);
        assert(f.controller().selected_card_id() == "c1");
        assert(!f.reports_error("Could not refresh cards after pull", "Older pulled card could not be read"));
    }
    assert(api.sync_project_git_calls == 4);
    f.destroy();
}

private class WindowFailureTransport : Object, HolderLinux.IApiHttpTransport {
    public async HolderLinux.ApiHttpBytesResponse send_and_read(Soup.Message message) throws Error {
        var path = message.get_uri().get_path();
        if (path == "/locations" && message.get_method() == "POST") {
            throw new IOError.TIMED_OUT("test-credential appeared in a raw socket error");
        }
        if (path == "/health") {
            return new HolderLinux.ApiHttpBytesResponse(200, new Bytes("test-credential".data));
        }
        return new HolderLinux.ApiHttpBytesResponse(200, new Bytes("{\"ok\":true,\"data\":[]}".data));
    }

    public async HolderLinux.ApiHttpStreamResponse send(Soup.Message message) throws Error {
        throw new IOError.NOT_SUPPORTED("unused");
    }
}

private class WindowFailureApiFactory : Object, HolderLinux.IApiFactory {
    public HolderLinux.IHolderApi create(string base_url, string auth_token) {
        return new HolderLinux.ApiClient(base_url, auth_token, new WindowFailureTransport());
    }
}

private void test_startup_health_error_shows_advice_and_logs_diagnostics() {
    var f = new WindowFixture();
    f.api_factory = new WindowFailureApiFactory();
    f.open();
    var advice = "Holder received an unexpected response. Reopen Holder and try again.";
    assert(wait_for_condition(() => f.editor_text().contains(advice)));
    assert(f.reports_error("Health check failed", advice));
    assert(f.debug_contains("API GET /health failed category=parse elapsed="));
    assert(f.debug_contains("status=200 invalid JSON object"));
    assert(!f.debug_contains("test-credential"));
    f.destroy();
}

private void test_reconnecting_disconnects_the_previous_api_diagnostics() {
    var f = new WindowFixture();
    f.open_loaded();
    var original = new HolderLinux.ApiClient(
        "http://127.0.0.1:8080", "test-credential", new WindowFailureTransport());
    var replacement = new HolderLinux.ApiClient(
        "http://127.0.0.1:8081", "test-credential", new WindowFailureTransport());
    f.window.on_api_client_connected(original);
    original.request_failed("first client before reconnect");
    assert(f.debug_contains("first client before reconnect"));

    f.window.on_api_client_connected(replacement);
    original.request_failed("first client after reconnect");
    replacement.request_failed("new client after reconnect");
    assert(!f.debug_contains("first client after reconnect"));
    assert(f.debug_contains("new client after reconnect"));
    f.destroy();
}

private void test_adding_a_storage_location_shows_timeout_advice() {
    var f = new WindowFixture();
    f.open_loaded();
    f.window.on_api_client_connected(new HolderLinux.ApiClient(
        "http://127.0.0.1:8080", "test-credential", new WindowFailureTransport()));
    var toolbox = f.window.workspace_for_tests.toolbox;
    toolbox.show_tool("resources");
    Gtk.Button? add_folder = null;
    foreach (var widget in av_descendants(toolbox.widget)) {
        var button = widget as Gtk.Button;
        if (button != null && button.get_label() == "Add Folder") {
            add_folder = button;
            break;
        }
    }
    assert(add_folder != null);
    add_folder.clicked();
    var dialog = f.wait_for_dialog();
    foreach (var widget in av_descendants(dialog)) {
        var entry = widget as Gtk.Entry;
        if (entry != null && entry.get_placeholder_text() == "Assets on this computer") {
            entry.set_text("Photos");
        } else if (entry != null && entry.get_placeholder_text() == "/path/to/assets") {
            entry.set_text("/srv/photos");
        }
    }
    av_answer(dialog, "save");
    var advice = "The Holder backend timed out. Check whether the action completed before trying again.";
    assert(wait_for_condition(() => f.reports_error("Failed to add storage location", advice)));
    assert(f.has_toast("Failed to add storage location"));
    assert(f.debug_contains("API POST /locations failed category=timeout elapsed="));
    assert(f.debug_contains("g-io-error-quark/"));
    assert(!f.debug_contains("test-credential"));
    f.destroy();
}

private void test_sharing_a_card_opens_an_email() {
    var f = new WindowFixture();
    f.open_loaded();
    f.open_card("c1");

    f.window.send_current_card_as_email();

    assert(f.launcher.launched.size == 1);
    assert(f.launcher.launched[0].has_prefix("mailto:"));
    f.destroy();
}

private void test_a_newer_release_is_offered_once() {
    var f = new WindowFixture();
    f.updates.body = "{\"schema\": 1, \"product\": \"Holder\", \"channel\": \"stable\", " +
        "\"version\": \"999.0.0\", \"release_url\": \"https://example.invalid/r\"}";
    f.open_loaded();

    var dialog = f.wait_for_dialog();
    assert(dialog.get_heading() == "Update Available");
    av_answer(dialog, "later");
    assert(f.settings.get_string(HolderLinux.AppSettings.KEY_UPDATE_LAST_PROMPT_VERSION) == "999.0.0");
    f.destroy();
}

private void test_saved_preferences_are_applied_at_startup() {
    var f = new WindowFixture();
    f.settings.set_string(HolderLinux.AppSettings.KEY_STYLE_SCHEME_ID, "classic");
    f.settings.set_boolean(HolderLinux.AppSettings.KEY_SHOW_LINE_NUMBERS, false);
    f.open_loaded();

    var workspace = f.window.workspace_for_tests;
    assert(!workspace.editor_view.get_show_line_numbers());
    var scheme = workspace.editor_buffer.get_style_scheme();
    assert(scheme != null && ((!) scheme).get_id() == "classic");
    f.destroy();

    // An unknown scheme leaves the editor's own.
    var g = new WindowFixture();
    g.settings.set_string(HolderLinux.AppSettings.KEY_STYLE_SCHEME_ID, "holder-no-such-scheme");
    g.open_loaded();
    var other = g.window.workspace_for_tests.editor_buffer.get_style_scheme();
    assert(other == null || ((!) other).get_id() != "holder-no-such-scheme");
    g.destroy();
}

private void test_the_window_works_without_saved_settings() {
    var f = new WindowFixture();
    f.without_settings = true;
    f.open_loaded();

    // Nothing is saved, and closing still says goodbye.
    assert(!f.window.on_window_close_requested());
    assert(f.presence.leaves == 1);
    f.destroy();
}

private void test_an_unsaveable_card_asks_before_closing() {
    var f = new WindowFixture();
    f.settings.set_boolean("no-plaintext-recovery-files", true);
    f.open_loaded();
    f.open_card("c1");
    f.api.fail_update_card = true;
    f.window.workspace_for_tests.editor_buffer.set_text("# Card 1\n\nUnsaved", -1);

    // Without a recovery copy, a failed save keeps the window open and asks.
    assert(f.window.on_window_close_requested());
    var dialog = f.wait_for_dialog();
    assert(dialog.get_heading() == HolderLinux.WindowCloseGuard.UNSAFE_DIALOG_TITLE);
    av_answer(dialog, "keep");
    assert(f.presence.leaves == 0);

    // Retrying fails again and asks again; quitting then closes without saving.
    assert(f.window.on_window_close_requested());
    var again = f.wait_for_dialog(dialog);
    av_answer(again, "retry");
    var third = f.wait_for_dialog(again);
    av_answer(third, "quit");
    assert(wait_for_condition(() => !f.window.get_visible()));
    assert(f.presence.leaves == 1);
    f.destroy();
}

private void test_an_injected_backend_starter_is_used() {
    var f = new WindowFixture();
    var starter = new RecordingBackendStarter();
    f.backend_starter = starter;
    f.open_loaded();

    assert(starter.ensures == 1);
    f.destroy();
}

private void test_holderctl_is_found_or_its_absence_logged() {
    if (Path.DIR_SEPARATOR_S != "/") {
        return;
    }
    var dir = av_temp_dir();
    var marker = Path.build_filename(dir, "ran");
    var holderctl = Path.build_filename(dir, "holderctl");
    try {
        FileUtils.set_contents(holderctl,
            "#!/bin/sh\ntouch '%s'\necho '{\"ok\":true,\"state\":\"running\"}'\n".printf(marker));
    } catch (FileError e) {
        assert_not_reached();
    }
    FileUtils.chmod(holderctl, 0755);
    var previous_ctl = Environment.get_variable("HOLDER_CTL");
    var previous_path = Environment.get_variable("PATH") ?? "";

    // HOLDER_CTL names the holderctl to run.
    Environment.set_variable("HOLDER_CTL", holderctl, true);
    var f = new WindowFixture();
    f.find_backend = true;
    f.open_loaded();
    assert(FileUtils.test(marker, FileTest.EXISTS));
    assert(f.debug_log().contains("BACKEND_ENSURE using " + holderctl));
    f.destroy();

    // Without one beside the program or on PATH, the window says so and uses a running daemon.
    Environment.set_variable("HOLDER_CTL", Path.build_filename(dir, "missing"), true);
    Environment.set_variable("PATH", dir + "-empty", true);
    var g = new WindowFixture();
    g.find_backend = true;
    g.open();
    Environment.set_variable("PATH", previous_path, true);
    assert(wait_for_condition(() => g.controller().selected_project_id() == "p1"));
    assert(g.debug_log().contains("BACKEND_ENSURE skipped: holderctl not found"));
    assert(g.debug_log().contains("is not an executable"));
    g.destroy();

    // Without HOLDER_CTL, the log says it can name one.
    Environment.unset_variable("HOLDER_CTL");
    Environment.set_variable("PATH", dir + "-empty", true);
    var h = new WindowFixture();
    h.find_backend = true;
    h.open();
    Environment.set_variable("PATH", previous_path, true);
    assert(wait_for_condition(() => h.controller().selected_project_id() == "p1"));
    assert(h.debug_log().contains("(HOLDER_CTL can name one)"));
    h.destroy();

    if (previous_ctl != null) {
        Environment.set_variable("HOLDER_CTL", (!) previous_ctl, true);
    } else {
        Environment.unset_variable("HOLDER_CTL");
    }
}

private void test_f9_toggles_the_toolbox_unless_a_terminal_has_focus() {
    var f = new WindowFixture();
    f.open_loaded();
    var workspace = f.window.workspace_for_tests;
    var root = (!) ((Adw.ToastOverlay) f.window.get_content()).get_child();

    assert(av_press(root, Gdk.Key.F9));
    assert(workspace.toolbox.widget.get_visible());
    assert(!av_press(root, Gdk.Key.F9, Gdk.ModifierType.CONTROL_MASK));
    assert(!av_press(root, Gdk.Key.F8));
#if HOLDER_TERMINAL_VTE
    workspace.toolbox.show_tool("terminals");
    Gtk.Widget? terminal = null;
    assert(wait_for_condition(() => {
        foreach (var widget in av_descendants(workspace.toolbox.widget)) {
            if (widget is Vte.Terminal && widget.get_mapped()) {
                terminal = widget;
                return true;
            }
        }
        return false;
    }));
    ((!) terminal).grab_focus();
    assert(wait_for_condition(() => f.window.get_focus() == terminal));
    // The terminal keeps F9 for the program running in it.
    assert(!av_press(root, Gdk.Key.F9));
    assert(workspace.toolbox.widget.get_visible());
#endif
    f.destroy();
}

private void test_an_app_without_an_id_uses_holders_icon() {
    var app = new Adw.Application(null, ApplicationFlags.NON_UNIQUE);
    try {
        app.register(null);
    } catch (Error e) {
        assert_not_reached();
    }
    var f = new WindowFixture();
    var services = new HolderLinux.MainWindowServices();
    services.api_factory = new MainControllerFakeApiFactory(f.api, f.api);
    services.server_discovery = f.discovery;
    services.find_backend_starter = false;
    services.presence = f.presence;
    services.update_check = new HolderLinux.UpdateCheckService(f.updates);
    services.settings = f.settings;
    services.uri_launcher = f.launcher;
    var window = new HolderLinux.MainWindow(app, 800, 600, services);

    assert(window.get_icon_name() == "team.holder.Holder");
    window.destroy();
    av_settle();
}

private void test_selection_and_editor_changes_reach_the_controller() {
    var f = new WindowFixture();
    f.api.include_card2 = true;
    f.open_loaded();
    var w = f.window;

    w.on_sidebar_card_context_selection_requested("c2");
    assert(wait_for_condition(() => {
        var card = f.controller().get_current_card();
        return card != null && ((!) card).card_id == "c2";
    }));

    // An edit to the open card is the user's: it is scheduled for saving.
    var buffer = w.workspace_for_tests.editor_buffer;
    buffer.set_text("# Card 2\n\nEdited", -1);
    w.on_editor_buffer_changed();
    assert(wait_for_condition(() =>
        f.api.update_card_calls >= 1 &&
        !f.controller().is_editor_save_in_flight() &&
        !f.controller().has_unsaved_editor_changes(), 8000));
    assert(f.api.last_updated_card_id == "c2");
    assert(f.api.last_updated_content == "# Card 2\n\nEdited");

    // Let the sidebar finish reloading the card before starting another navigation.
    var saved_card = f.controller().get_current_card();
    w.on_card_selection_changed();
    assert(wait_for_condition(() => {
        var card = f.controller().get_current_card();
        return card != null && card != saved_card && ((!) card).card_id == "c2";
    }));

    w.on_ai_thread_selection_changed();
    w.on_project_selection_changed();
    assert(wait_for_condition(() =>
        f.controller().get_current_card() == null && f.editor_text().has_prefix("# Project 1\n")));
    f.destroy();
}

private void test_editor_changes_without_a_card_are_ignored() {
    var f = new WindowFixture();
    f.open_loaded();

    f.window.on_editor_buffer_changed();

    assert(!f.controller().has_unsaved_editor_changes());
    f.destroy();
}

private void test_ctrl_enter_on_an_image_without_resources_says_so() {
    var f = new WindowFixture();
    f.open_loaded();
    f.open_card("c1");
    var buffer = f.window.workspace_for_tests.editor_buffer;
    buffer.set_text("![Boiler](holder://resource/r-boiler)\n", -1);
    Gtk.TextIter at;
    buffer.get_iter_at_offset(out at, 3);
    buffer.place_cursor(at);

    assert(f.window.on_internal_link_key_pressed(Gdk.Key.Return, 0, Gdk.ModifierType.CONTROL_MASK));

    assert(f.has_toast(HolderLinux.AttachmentSelection.NO_IMAGE_MESSAGE));
    f.destroy();
}

private HolderLinux.ProjectResource window_image_resource(string id = "r-boiler") {
    // The private cache checks a download's size and SHA-256 against the asset's.
    var png = Base64.decode(WINDOW_PNG_BASE64);
    var assets = new Gee.ArrayList<HolderLinux.ResourceAsset>();
    assets.add(new HolderLinux.ResourceAsset("a-" + id, id, "boiler.png", "image/png", png.length,
                                             Checksum.compute_for_data(ChecksumType.SHA256, png)));
    return new HolderLinux.ProjectResource(id, "p1", "file", "holder://resource/" + id, "Boiler",
                                           null, 1, 1, null, assets);
}

private string window_write_png() {
    var path = Path.build_filename(av_temp_dir(), "boiler.png");
    try {
        FileUtils.set_data(path, Base64.decode(WINDOW_PNG_BASE64));
    } catch (FileError e) {
        assert_not_reached();
    }
    return path;
}

private Gtk.TextMark window_mark_at_end(WindowFixture f) {
    var buffer = f.window.workspace_for_tests.editor_buffer;
    Gtk.TextIter end;
    buffer.get_end_iter(out end);
    return buffer.create_mark(null, end, false);
}

private void test_a_dropped_file_needs_a_card_storage_and_a_location() {
    // The plain fake API has no resource storage, so a drop is not ready.
    var f = new WindowFixture();
    f.open_loaded();
    f.open_card("c1");
    var file = File.new_for_path(window_write_png());
    f.window.on_workspace_file_dropped(file, window_mark_at_end(f));
    assert(wait_for_condition(() => f.has_toast(HolderLinux.AssetImportFlow.NOT_READY_MESSAGE)));
    f.destroy();

    // With storage but no preferred location, the Resources tool opens to set one up.
    var g = new WindowFixture();
    var api = new WindowStorageApi();
    api.preferred_location = null;
    g.api = api;
    g.open_loaded();
    g.open_card("c1");
    g.window.on_workspace_file_dropped(file, window_mark_at_end(g));
    assert(wait_for_condition(() => g.has_toast(HolderLinux.AssetImportFlow.NEEDS_LOCATION_MESSAGE)));
    var toolbox = g.window.workspace_for_tests.toolbox;
    assert(toolbox.widget.get_visible() && toolbox.is_showing_tool("resources"));
    g.destroy();
}

private void test_a_dropped_image_is_imported_and_placed_where_it_landed() {
    var f = new WindowFixture();
    var api = new WindowStorageApi();
    f.api = api;
    f.open_loaded();
    f.open_card("c1");

    f.window.on_workspace_file_dropped(File.new_for_path(window_write_png()), window_mark_at_end(f));

    assert(wait_for_condition(() => f.editor_text().contains("holder://resource/r-new")));
    assert(api.imports == 1);
    assert(f.window.workspace_for_tests.toolbox.is_showing_tool("resources"));

    // A failed import is reported and leaves the card as it was.
    api.fail_import = true;
    var before = f.editor_text();
    f.window.on_workspace_file_dropped(File.new_for_path(window_write_png()), window_mark_at_end(f));
    assert(wait_for_condition(() => f.reports_error(HolderLinux.AssetImportFlow.FAILED_TITLE, "disk full")));
    assert(f.editor_text() == before);
    f.destroy();
}

private void test_a_resource_preview_downloads_shows_and_opens_the_asset() {
    var f = new WindowFixture();
    var api = new WindowStorageApi();
    f.api = api;
    f.open_loaded();
    f.open_card("c1");
    var workspace = f.window.workspace_for_tests;
    var resource = window_image_resource();

    workspace.toolbox.asset_preview_requested(resource, resource.assets[0]);
    assert(workspace.is_asset_preview_visible());
    assert(wait_for_condition(() => f.preview_state() == "image"));
    assert(api.downloads == 1);

    // Opening it hands the cached copy to the desktop's default app.
    workspace.asset_preview.open_external_requested();
    assert(wait_for_condition(() => f.launcher.launched.size == 1));
    assert(f.launcher.launched[0].has_prefix("file://"));
    f.launcher.failure = "no viewer";
    workspace.asset_preview.open_external_requested();
    assert(wait_for_condition(() => f.shows_label("Failed to open Asset")));

    // A document is described instead of drawn.
    var doc_assets = new Gee.ArrayList<HolderLinux.ResourceAsset>();
    var png = Base64.decode(WINDOW_PNG_BASE64);
    doc_assets.add(new HolderLinux.ResourceAsset("a-doc", "r-doc", "manual.pdf", "application/pdf", png.length,
                                                 Checksum.compute_for_data(ChecksumType.SHA256, png)));
    var doc = new HolderLinux.ProjectResource("r-doc", "p1", "file", "holder://resource/r-doc", "Manual",
                                              null, 1, 1, null, doc_assets);
    workspace.asset_preview.select_attachment(0);
    f.window.workspace_for_tests.inline_resource_images.preview_requested(doc, doc_assets[0]);
    assert(wait_for_condition(() => f.preview_state() == "document"));
    f.destroy();
}

private void test_an_asset_that_cannot_be_fetched_shows_why() {
    var f = new WindowFixture();
    var api = new WindowStorageApi();
    api.fail_download = true;
    f.api = api;
    f.open_loaded();
    f.open_card("c1");
    var workspace = f.window.workspace_for_tests;
    // Asset IDs of its own: the private cache keeps what earlier tests downloaded.
    var resource = window_image_resource("r-offline");

    // Nothing selected yet: open and export do nothing.
    workspace.asset_preview.open_external_requested();
    workspace.asset_preview.export_requested();
    av_settle();
    assert(f.launcher.launched.size == 0);

    workspace.toolbox.asset_preview_requested(resource, resource.assets[0]);
    assert(wait_for_condition(() => f.preview_state() == "error"));
    workspace.asset_preview.export_requested();
    assert(wait_for_condition(() => f.shows_label("Failed to export Asset")));
    f.destroy();

    // Without resource storage at all, the preview says the Asset is not available.
    var g = new WindowFixture();
    g.open_loaded();
    g.open_card("c1");
    var other = window_image_resource("r-no-storage");
    g.window.workspace_for_tests.toolbox.asset_preview_requested(other, other.assets[0]);
    assert(wait_for_condition(() => g.preview_state() == "error"));
    g.destroy();
}

private void test_a_cards_attachments_fill_the_preview() {
    var f = new WindowFixture();
    var api = new WindowStorageApi();
    f.api = api;
    api.resources.add(window_image_resource("r-attached"));
    api.card_links.add(new HolderLinux.CardLink("c1", "r-attached", "resource", "attachment", null, 1));
    f.open_loaded();
    f.open_card("c1");
    var workspace = f.window.workspace_for_tests;

    // Opening the preview loads the selected attachment; choosing one loads that.
    workspace.set_asset_preview_visible(false);
    f.window.on_app_state_changed();
    av_settle();
    f.window.workspace_for_tests.asset_preview_toggled(true);
    assert(wait_for_condition(() => f.preview_state() == "image"));
    assert(api.downloads == 1);
    // Choosing the attachment again shows it from the private cache.
    workspace.asset_preview.attachment_selected(0);
    assert(wait_for_condition(() => f.preview_state() == "image"));
    assert(api.downloads == 1);

    // Ctrl+Enter on the image in the card previews it too.
    var buffer = workspace.editor_buffer;
    buffer.set_text("![Boiler](holder://resource/r-attached)\n", -1);
    Gtk.TextIter at;
    buffer.get_iter_at_offset(out at, 3);
    buffer.place_cursor(at);
    assert(f.window.on_internal_link_key_pressed(Gdk.Key.Return, 0, Gdk.ModifierType.CONTROL_MASK));
    assert(wait_for_condition(() => f.preview_state() == "image"));
    f.destroy();
}

private void test_attachments_that_fail_to_load_say_so_in_the_open_preview() {
    var f = new WindowFixture();
    f.open_loaded();
    f.open_card("c1");
    f.window.workspace_for_tests.set_asset_preview_visible(true);
    f.api.fail_list_card_links = true;

    f.window.on_app_state_changed();

    assert(wait_for_condition(() => f.preview_state() == "error"));
    assert(f.shows_label("Could not load this Card's attachments: list card links failed"));
    f.destroy();
}

private void test_an_asset_can_be_exported_where_the_user_chooses() {
    var f = new WindowFixture();
    var api = new WindowStorageApi();
    f.api = api;
    // Give attachment refreshes the same resource that the test will export.
    var resource = window_image_resource("r-export");
    api.resources.add(resource);
    api.card_links.add(new HolderLinux.CardLink("c1", resource.resource_id, "resource", "attachment", null, 1));
    f.open_loaded();
    f.open_card("c1");
    var workspace = f.window.workspace_for_tests;
    workspace.toolbox.asset_preview_requested(resource, resource.assets[0]);
    assert(wait_for_condition(() => f.preview_state() == "image"));

    // Cancelling the chooser exports nothing.
    workspace.asset_preview.export_requested();
    assert(wait_for_condition(() => f.picker.asks == 1));
    av_settle();

    var destination = Path.build_filename(av_temp_dir(), "exported.png");
    f.picker.chosen = File.new_for_path(destination);
    workspace.asset_preview.export_requested();
    assert(wait_for_condition(() => f.picker.asks == 2));
    // Wait for the complete PNG or the reported export error. A toast can be queued.
    bool exported_png = false;
    assert(wait_for_condition(() => {
        if (f.debug_contains("ERROR: Failed to export Asset | ")) return true;
        try {
            uint8[] exported;
            FileUtils.get_data(destination, out exported);
            exported_png = Base64.encode(exported) == WINDOW_PNG_BASE64;
            return exported_png;
        } catch (FileError e) {
            return false;
        }
    }));
    if (!exported_png) Test.message("%s", f.debug_log());
    assert(exported_png);
    f.destroy();
}

private void test_a_recovery_key_can_be_emailed_saved_and_imported() {
    var f = new WindowFixture();
    f.open_loaded();
    var w = f.window;

    w.request_send_recovery_key_as_email();
    f.enter_pin("2468");
    assert(wait_for_condition(() => f.recovery.emailed.size == 1));
    assert(f.has_toast("Opened default email app with recovery key attachment."));

    var saved = Path.build_filename(av_temp_dir(), "Project-1-recovery.hrk");
    f.picker.chosen = File.new_for_path(saved);
    w.request_save_recovery_key_to_usb();
    f.enter_pin("2468");
    assert(wait_for_condition(() => FileUtils.test(saved, FileTest.EXISTS)));
    string contents;
    try {
        FileUtils.get_contents(saved, out contents);
    } catch (FileError e) {
        assert_not_reached();
    }
    assert(contents == "{\"token\":\"fake\"}");

    // Importing reads the chosen file, asks for its PIN, and summarises what it restored.
    w.request_import_recovery_key();
    f.enter_pin("2468");
    var summary = f.wait_for_dialog();
    assert(summary.get_heading() != "Unlock Recovery Key");
    av_answer(summary, "ok");
    f.destroy();
}

private void test_closing_waits_for_the_save_and_then_closes() {
    var f = new WindowFixture();
    f.open_loaded();
    f.open_card("c1");
    // An open dialog, such as an offered recovery draft, would keep the window from closing.
    assert(f.window.get_visible_dialog() == null);
    f.window.workspace_for_tests.editor_buffer.set_text("# Card 1\n\nLast words", -1);

    // The card is saved first; the window closes once it is.
    assert(f.window.on_window_close_requested());
    assert(wait_for_condition(() => !f.window.get_visible()));
    assert(f.api.last_updated_content == "# Card 1\n\nLast words");
    assert(f.presence.leaves == 1);
    f.destroy();
}

private void test_inline_image_previews_appear_when_turned_on() {
    var f = new WindowFixture();
    var api = new WindowStorageApi();
    f.api = api;
    api.resources.add(window_image_resource("r-inline"));
    f.settings.set_boolean(HolderLinux.AppSettings.KEY_SHOW_INLINE_IMAGE_PREVIEWS, true);
    f.open_loaded();
    f.open_card("c1");
    var workspace = f.window.workspace_for_tests;
    f.window.on_app_state_changed();

    workspace.editor_buffer.set_text("# Card 1\n\n![Boiler](holder://resource/r-inline)\n", -1);
    f.window.on_editor_buffer_changed();

    assert(wait_for_condition(() => workspace.inline_resource_images.decoration_count() == 1));
    f.destroy();
}

private void test_the_intelligence_panel_follows_the_card() {
    var f = new WindowFixture();
    f.settings.set_int(HolderLinux.AppSettings.KEY_AI_PANEL_WIDTH, 720);
    f.open_loaded();
    f.open_card("c1");
    var w = f.window;
    w.on_workspace_ai_panel_toggled(true);

    // An autosave with the panel open asks for a title suggestion; accepting one renames the card.
    var panel = w.workspace_for_tests.ai_panel;
    w.workspace_for_tests.editor_buffer.set_text("# Card 1\n\nAutosaved", -1);
    w.on_editor_buffer_changed();
    assert(wait_for_condition(() => f.api.last_updated_content.has_suffix("Autosaved"), 8000));
    av_settle();
    panel.title_suggestion_apply_requested("n1", "c1", "Boiler service");
    av_settle();
    f.destroy();
}

private void test_a_window_saved_maximized_starts_maximized() {
    var f = new WindowFixture();
    f.settings.set_boolean(HolderLinux.AppSettings.KEY_WINDOW_MAXIMIZED, true);
    // A size given on the command line would win over the saved state.
    f.startup_width = 0;
    f.startup_height = 0;
    f.open_loaded();
    av_settle();

    assert(!f.window.on_window_close_requested());
    // Whether the window manager maximized it is what is saved again.
    assert(f.settings.get_boolean(HolderLinux.AppSettings.KEY_WINDOW_MAXIMIZED) == f.window.is_maximized());
    f.destroy();
}

private void test_window_edge_cases_change_nothing() {
    var f = new WindowFixture();
    f.api.search_returns_card2 = true;
    f.api.include_card2 = true;
    f.open_loaded();
    var w = f.window;
    var workspace = w.workspace_for_tests;

    // Nothing to append to, and no card id to trash.
    var before = f.editor_text();
    w.append_text_to_current_card("$ make");
    assert(f.editor_text() == before);
    w.confirm_move_card_to_trash("");
    av_settle();
    assert(w.get_visible_dialog() == null);

    // A press that is not a real click, and an attachment that is not there.
    w.on_internal_link_click_pressed(new Gtk.GestureClick(), 1, 0, 0);
    workspace.asset_preview.attachment_selected(5);

    // Clearing the project selection is a transient state the window ignores.
    w.request_project_selection(null);
    w.on_project_selection_changed();

    // Moving to search results with none selected selects the first.
    workspace.search_entry.set_text("card");
    w.on_workspace_search_activated();
    assert(wait_for_condition(() => w.search_store_for_tests.get_n_items() == 1));
    workspace.search_selection.set_autoselect(false);
    workspace.search_selection.set_can_unselect(true);
    workspace.search_selection.set_selected(Gtk.INVALID_LIST_POSITION);
    w.on_workspace_search_focus_results_requested();
    assert(workspace.search_selection.get_selected() == 0);

    // A restored trash item whose project cannot be reloaded is left alone.
    f.api.fail_list_cards = true;
    var done = false;
    w.on_trash_item_restored.begin("c1", (obj, res) => {
        w.on_trash_item_restored.end(res);
        done = true;
    });
    assert(wait_for_condition(() => done));
    f.destroy();
}

private void test_a_card_with_children_asks_what_to_trash() {
    var f = new WindowFixture();
    f.open_loaded();
    var w = f.window;
    w.card_store_for_tests.append(
        new HolderLinux.CardSummary("c1-child", "p1", "Child", "c1-child.md", 2.0, "c1", 30, 30)
    );

    w.confirm_move_card_to_trash("c1");
    var dialog = f.wait_for_dialog();
    av_answer(dialog, "subtree");
    assert(wait_for_condition(() => f.api.delete_card_calls == 1));
    f.destroy();
}

private void test_the_preferences_turn_inline_previews_on() {
    var f = new WindowFixture();
    f.open_loaded();
    var w = f.window;

    w.handle_show_preferences_action();
    assert(wait_for_condition(() => w.get_visible_dialog() is HolderLinux.PreferencesDialog));
    var dialog = (HolderLinux.PreferencesDialog) (!) w.get_visible_dialog();
    dialog.inline_image_previews_changed(true);
    dialog.force_close();
    f.destroy();
}

private void test_recovery_keys_need_a_project_and_a_daemon() {
    // With no project, there is nothing to export.
    var f = new WindowFixture();
    // Empty the project list after startup so first-run setup does not queue a creation toast.
    f.open_loaded();
    f.api.list_projects_empty = true;
    f.controller().reload_everything.begin();
    assert(wait_for_condition(() => f.controller().get_current_project() == null));
    f.window.request_save_recovery_key_to_usb();
    f.enter_pin("2468");
    assert(wait_for_condition(() => f.has_toast("Select a project first.")));
    assert(f.picker.asks == 0);
    f.destroy();

    // Without a daemon, an import fails and says so.
    var g = new WindowFixture();
    g.discovery.should_fail = true;
    var key = Path.build_filename(av_temp_dir(), "key.hrk");
    try {
        FileUtils.set_contents(key, "{\"token\":\"fake\"}");
    } catch (FileError e) {
        assert_not_reached();
    }
    g.picker.chosen = File.new_for_path(key);
    g.open();
    g.window.request_import_recovery_key();
    g.enter_pin("2468");
    assert(wait_for_condition(() => g.reports_error("Recovery key import failed", "API client not connected.")));
    g.destroy();
}

[CCode (cname = "g_object_run_dispose")]
private extern void window_run_dispose(Object object);

private void test_disposing_the_window_stops_intelligence_runs() {
    var f = new WindowFixture();
    f.open_loaded();
    var window = f.window;
    window.destroy();
    av_settle();

    // The window's own handlers keep it alive past destroy; dispose is what releases it.
    window_run_dispose(window);
}

private void test_a_background_pull_is_announced_once_with_its_copy_to_compare() {
    Gtk.Settings.get_default().gtk_enable_animations = false;
    var f = new WindowFixture();
    var api = new WindowPullReportApi();
    f.api = api;
    api.include_card2 = true;
    f.open_loaded();
    f.open_card("c1");
    var cards_before = api.list_cards_calls;

    // Another project's change is not this window's news.
    f.presence.project_changed("p9", "r1");
    av_settle();
    assert(api.sync_status_calls == 0);

    f.presence.project_changed("p1", "r1");
    assert(wait_for_condition(() => f.has_toast(PULL_REPORT_MESSAGE)));
    assert(api.sync_status_projects[0] == "p1");
    // The pulled cards are shown.
    assert(wait_for_condition(() => api.list_cards_calls > cards_before));

    // The same report again, as when the project changes for another reason: no new toast.
    var cards_after = api.list_cards_calls;
    f.presence.project_changed("p1", "r1");
    assert(wait_for_condition(() => api.sync_status_calls >= 2));
    av_settle();
    assert(api.list_cards_calls == cards_after);

    var compare = toast_button(f, "Compare");
    assert(compare != null);
    ((!) compare).clicked();
    assert(wait_for_condition(() => f.window.get_visible_dialog() != null));
    assert(wait_for_condition(() => api.comparison_calls == 1));
    ((!) f.window.get_visible_dialog()).force_close();
    f.destroy();
}

private void test_a_pull_report_from_sync_now_or_before_startup() {
    Gtk.Settings.get_default().gtk_enable_animations = false;
    var f = new WindowFixture();
    var api = new WindowPullReportApi();
    f.api = api;
    f.open_loaded();
    var toolbox = f.window.workspace_for_tests.toolbox;

    // A report from before the window opened was not made while anyone was looking.
    api.reported_at = 1;
    f.presence.project_changed("p1", "r1");
    assert(wait_for_condition(() => api.sync_status_calls == 1));
    av_settle();
    assert(!f.has_toast(PULL_REPORT_MESSAGE));

    // An unreadable sync state is only logged.
    api.fail_sync_status = true;
    toolbox.git_project_pulled("p1");
    assert(wait_for_condition(() => f.debug_contains("PULL_REPORT could not read the sync state of p1")));
    api.fail_sync_status = false;

    // Sync now leaves its changes to the report; two copies are reviewed in the Git tool.
    var changes = new HolderLinux.GitPullChanges();
    changes.conflict_copy_ids.add("c2");
    changes.conflict_copy_ids.add("c3");
    api.report = changes;
    api.reported_at = new DateTime.now_utc().to_unix() + 1;
    toolbox.git_project_pulled("p1");
    assert(wait_for_condition(() => f.has_toast(
        "Pulled changes from another device: saved your versions of 2 cards as copies.")));
    var review = toast_button(f, "Review");
    assert(review != null);
    ((!) review).clicked();
    assert(wait_for_condition(() => toolbox.is_showing_tool("git")));
    f.destroy();
}

private void test_a_background_pull_with_nothing_to_report_still_refreshes_the_cards() {
    var f = new WindowFixture();
    var api = new WindowPullReportApi();
    api.report = null;
    f.api = api;
    f.open_loaded();
    var now = new DateTime.now_utc().to_unix();

    // A local save moves the commit but is not a pull.
    var cards = api.list_cards_calls;
    f.presence.project_changed("p1", "local-1");
    assert(wait_for_condition(() => api.sync_status_calls == 1));
    av_settle();
    assert(api.list_cards_calls == cards);

    // A background fast-forward: a pull, and the commit moved.
    api.pulled_at = now;
    f.presence.project_changed("p1", "pulled-1");
    assert(wait_for_condition(() => api.list_cards_calls > cards));

    // The next pull brought nothing: the commit stayed.
    cards = api.list_cards_calls;
    api.pulled_at = now + 300;
    f.presence.project_changed("p1", "pulled-1");
    assert(wait_for_condition(() => api.sync_status_calls == 3));
    av_settle();
    assert(api.list_cards_calls == cards);

    // One that did, after Sync now saw the pull before it: Sync now refreshed by itself.
    api.pulled_at = now + 600;
    f.window.workspace_for_tests.toolbox.git_project_pulled("p1");
    assert(wait_for_condition(() => api.sync_status_calls == 4));
    var after_sync_now = api.list_cards_calls;
    f.presence.project_changed("p1", "pulled-2");
    assert(wait_for_condition(() => api.sync_status_calls == 5));
    av_settle();
    assert(api.list_cards_calls == after_sync_now);
    f.destroy();
}

internal void register_window_tests() {
    av_add_test("/app_views/window/starts_and_loads", test_the_window_starts_and_loads_the_first_project);
    av_add_test("/app_views/window/toolbar_actions", test_the_toolbar_actions_reach_the_controller);
    av_add_test("/app_views/window/menu_pages", test_the_window_menu_opens_its_pages_and_dialogs);
    av_add_test("/app_views/window/card_actions", test_card_actions_work_on_the_selected_card);
    av_add_test("/app_views/window/closing", test_closing_saves_the_window_and_says_goodbye);
    av_add_test("/app_views/window/recovery_draft", test_a_recovery_draft_can_be_restored_or_discarded);
    av_add_test("/app_views/window/panels", test_the_workspace_panels_follow_their_toggles);
    av_add_test("/app_views/window/search", test_searching_runs_shows_and_clears_results);
    av_add_test("/app_views/window/find_replace", test_find_and_replace_edit_the_card);
    av_add_test("/app_views/window/reported_state", test_the_window_shows_what_the_controller_reports);
    av_add_test("/app_views/window/sync_now_refresh", test_sync_now_refreshes_remote_edits_and_deletions);
    av_add_test("/app_views/window/background_pull_report",
                test_a_background_pull_is_announced_once_with_its_copy_to_compare);
    av_add_test("/app_views/window/pull_report_sync_now_and_startup", test_a_pull_report_from_sync_now_or_before_startup);
    av_add_test("/app_views/window/background_pull_refreshes_cards",
                test_a_background_pull_with_nothing_to_report_still_refreshes_the_cards);
    av_add_test("/app_views/window/sync_out_of_order_card_replies", test_newer_pull_wins_over_an_older_card_reply_or_error);
    av_add_test("/app_views/conflict_comparison/discard_draft", test_discarding_a_conflict_draft);
    av_add_test("/app_views/conflict_comparison/keep_original", test_keeping_the_original_and_preserved_copy);
    av_add_test("/app_views/conflict_comparison/save_result", test_saving_after_refreshing_a_conflict_review);
    av_add_test("/app_views/conflict_comparison/open_copy", test_opening_the_preserved_card_after_a_failed_comparison);
    av_add_test("/app_views/conflict_comparison/stale_cleanup", test_stale_conflict_cleanup_leaves_the_preserved_copy);
    av_add_test("/app_views/conflict_comparison/trash_copy", test_confirmed_conflict_cleanup_removes_the_preserved_copy);
    av_add_test("/app_views/window/sync_deletion_with_draft", test_pull_deletion_preserves_unsaved_editor_text);
    av_add_test("/app_views/window/sync_refresh_errors", test_pull_refresh_handles_missing_selection_and_errors);
    av_add_test("/app_views/window/startup_api_diagnostic", test_startup_health_error_shows_advice_and_logs_diagnostics);
    av_add_test("/app_views/window/reconnect_api_diagnostics", test_reconnecting_disconnects_the_previous_api_diagnostics);
    av_add_test("/app_views/window/storage_api_diagnostic", test_adding_a_storage_location_shows_timeout_advice);
    av_add_test("/app_views/window/email", test_sharing_a_card_opens_an_email);
    av_add_test("/app_views/window/update_offer", test_a_newer_release_is_offered_once);
    av_add_test("/app_views/window/preferences_at_startup", test_saved_preferences_are_applied_at_startup);
    av_add_test("/app_views/window/without_settings", test_the_window_works_without_saved_settings);
    av_add_test("/app_views/window/unsafe_close", test_an_unsaveable_card_asks_before_closing);
    av_add_test("/app_views/window/injected_backend", test_an_injected_backend_starter_is_used);
    av_add_test("/app_views/window/holderctl", test_holderctl_is_found_or_its_absence_logged);
    av_add_test("/app_views/window/f9", test_f9_toggles_the_toolbox_unless_a_terminal_has_focus);
    av_add_test("/app_views/window/icon_without_id", test_an_app_without_an_id_uses_holders_icon);
    av_add_test("/app_views/window/selection_and_edits", test_selection_and_editor_changes_reach_the_controller);
    av_add_test("/app_views/window/edits_without_card", test_editor_changes_without_a_card_are_ignored);
    av_add_test("/app_views/window/image_without_resources", test_ctrl_enter_on_an_image_without_resources_says_so);
    av_add_test("/app_views/window/drop_not_ready", test_a_dropped_file_needs_a_card_storage_and_a_location);
    av_add_test("/app_views/window/drop_imports", test_a_dropped_image_is_imported_and_placed_where_it_landed);
    av_add_test("/app_views/window/resource_preview", test_a_resource_preview_downloads_shows_and_opens_the_asset);
    av_add_test("/app_views/window/asset_unavailable", test_an_asset_that_cannot_be_fetched_shows_why);
    av_add_test("/app_views/window/card_attachments", test_a_cards_attachments_fill_the_preview);
    av_add_test("/app_views/window/attachments_fail", test_attachments_that_fail_to_load_say_so_in_the_open_preview);
    av_add_test("/app_views/window/export_asset", test_an_asset_can_be_exported_where_the_user_chooses);
    av_add_test("/app_views/window/recovery_key", test_a_recovery_key_can_be_emailed_saved_and_imported);
    av_add_test("/app_views/window/close_after_save", test_closing_waits_for_the_save_and_then_closes);
    av_add_test("/app_views/window/inline_previews", test_inline_image_previews_appear_when_turned_on);
    av_add_test("/app_views/window/intelligence", test_the_intelligence_panel_follows_the_card);
    av_add_test("/app_views/window/maximized", test_a_window_saved_maximized_starts_maximized);
    av_add_test("/app_views/window/edge_cases", test_window_edge_cases_change_nothing);
    av_add_test("/app_views/window/trash_children", test_a_card_with_children_asks_what_to_trash);
    av_add_test("/app_views/window/preferences_previews", test_the_preferences_turn_inline_previews_on);
    av_add_test("/app_views/window/recovery_needs", test_recovery_keys_need_a_project_and_a_daemon);
    av_add_test("/app_views/window/dispose", test_disposing_the_window_stops_intelligence_runs);
}

}
