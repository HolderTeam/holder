using GLib;

namespace HolderLinuxTests {

private const string WT_COMPLETE_TRANSCRIPT =
    "**********************\r\n" +
    "PowerShell transcript start\r\n" +
    "PSVersion: 7.6.5\r\n" +
    "**********************\r\n" +
    "Transcript started, output file is C:\\Temp\\holder.txt\r\n" +
    "PS C:\\Work> echo hello\r\n" +
    "hello\r\n" +
    "PS C:\\Work> Stop-Transcript\r\n" +
    "**********************\r\n" +
    "PowerShell transcript end\r\n" +
    "End time: 20260901153424\r\n" +
    "**********************\r\n";

private const string WT_USEFUL_TEXT = "PS C:\\Work> echo hello\nhello";

private const string WT_OPEN_TRANSCRIPT =
    "**********************\r\n" +
    "PowerShell transcript start\r\n" +
    "**********************\r\n" +
    "PS C:\\Work> ping.exe 127.0.0.1\r\n";

// ---- fakes ---------------------------------------------------------------------------------------

private class FakeDiscovery : HolderLinux.PowerShellDiscoveryService {
    public HolderLinux.PowerShellPrerequisites next;
    public int calls = 0;
    public bool last_force_refresh = false;
    public bool stall = false;
    private SourceFunc? resume = null;

    public FakeDiscovery(HolderLinux.PowerShellPrerequisites next, bool stall) {
        base(null);
        this.next = next;
        this.stall = stall;
    }

    public override async HolderLinux.PowerShellPrerequisites discover(bool force_refresh = false) {
        calls++;
        last_force_refresh = force_refresh;
        if (stall) {
            resume = discover.callback;
            yield;
        }
        return next;
    }

    public void release() {
        stall = false;
        var callback = (owned) resume;
        resume = null;
        callback();
    }
}

private class FakeLauncher : HolderLinux.WindowsTerminalLauncher {
    public int session_launches = 0;
    public int install_launches = 0;
    public bool fail = false;
    public HolderLinux.TerminalSession? last_session = null;

    public override void launch_session(HolderLinux.PowerShellPrerequisites prerequisites,
                                        HolderLinux.TerminalSession session) throws Error {
        if (fail) {
            throw new IOError.FAILED("wt.exe vanished");
        }
        session_launches++;
        last_session = session;
    }

    public override void launch_install(HolderLinux.PowerShellPrerequisites prerequisites) throws Error {
        if (fail) {
            throw new IOError.FAILED("winget vanished");
        }
        install_launches++;
    }
}

private class FakeSessionStore : HolderLinux.TerminalSessionStore {
    public bool fail_load = false;
    public bool fail_read = false;
    private bool loading = false;

    public FakeSessionStore(string root_dir) {
        base(root_dir);
    }

    public override Gee.ArrayList<HolderLinux.TerminalSession> load_sessions(string? project_id = null) throws Error {
        if (fail_load) {
            throw new IOError.FAILED("session directory unreadable");
        }
        loading = true;
        try {
            return base.load_sessions(project_id);
        } finally {
            loading = false;
        }
    }

    public override HolderLinux.PowerShellTranscriptSnapshot read_transcript(HolderLinux.TerminalSession session) throws Error {
        // load_sessions reads each transcript itself; only the view's own reads fail.
        if (fail_read && !loading) {
            throw new IOError.FAILED("transcript locked");
        }
        return base.read_transcript(session);
    }
}

private HolderLinux.PowerShellPrerequisites wt_ready() {
    return new HolderLinux.PowerShellPrerequisites(
        HolderLinux.PowerShellPrerequisiteStatus.READY,
        "C:\\pwsh.exe", "7.4.6", "C:\\wt.exe", "C:\\winget.exe"
    );
}

private HolderLinux.PowerShellPrerequisites wt_missing(bool with_winget = true) {
    return new HolderLinux.PowerShellPrerequisites(
        HolderLinux.PowerShellPrerequisiteStatus.POWERSHELL_MISSING,
        null, null, "C:\\wt.exe", with_winget ? "C:\\winget.exe" : null
    );
}

private HolderLinux.PowerShellPrerequisites wt_terminal_missing() {
    return new HolderLinux.PowerShellPrerequisites(
        HolderLinux.PowerShellPrerequisiteStatus.WINDOWS_TERMINAL_MISSING,
        "C:\\pwsh.exe", "7.4.6", null, "C:\\winget.exe"
    );
}

// ---- widget helpers ------------------------------------------------------------------------------

private void wt_settle() {
    var context = MainContext.default();
    for (int i = 0; i < 20; i++) {
        while (context.iteration(false)) {}
    }
}

private void wt_collect(Gtk.Widget root, Gee.ArrayList<Gtk.Widget> found) {
    found.add(root);
    for (var child = root.get_first_child(); child != null; child = ((!) child).get_next_sibling()) {
        wt_collect((!) child, found);
    }
}

private Gee.ArrayList<Gtk.Widget> wt_descendants(Gtk.Widget root) {
    var found = new Gee.ArrayList<Gtk.Widget>();
    wt_collect(root, found);
    return found;
}

private Gtk.Button wt_button(Gtk.Widget root, string label) {
    foreach (var widget in wt_descendants(root)) {
        var button = widget as Gtk.Button;
        if (button != null && ((!) button).get_label() == label) {
            return (!) button;
        }
    }
    assert_not_reached();
}

private Gtk.Label? wt_label(Gtk.Widget root, string text) {
    foreach (var widget in wt_descendants(root)) {
        var label = widget as Gtk.Label;
        if (label != null && ((!) label).get_text() == text) {
            return label;
        }
    }
    return null;
}

private bool wt_shows_label(Gtk.Widget root, string text) {
    var label = wt_label(root, text);
    return label != null && ((!) label).get_visible();
}

private T wt_find<T>(Gtk.Widget root) {
    foreach (var widget in wt_descendants(root)) {
        if (widget.get_type().is_a(typeof(T))) {
            return (T) widget;
        }
    }
    assert_not_reached();
}

// Appends in place, as PowerShell's Start-Transcript does. (FileUtils.set_contents renames a
// temporary file into place, which the directory monitor reports under the temporary name.)
private void wt_append(string path, string text) {
    try {
        var stream = File.new_for_path(path).append_to(FileCreateFlags.NONE);
        stream.write_all(text.data, null);
        stream.close();
    } catch (Error e) {
        assert_not_reached();
    }
}

// ---- harness -------------------------------------------------------------------------------------

private class TerminalViewHarness : Object {
    public string root_dir;
    public FakeDiscovery discovery;
    public FakeSessionStore store;
    public FakeLauncher launcher = new FakeLauncher();
    public TestScheduler scheduler = new TestScheduler();
    public HolderLinux.TerminalToolView view;
    public Adw.Window window = new Adw.Window();
    public GLib.ListStore projects = new GLib.ListStore(typeof(HolderLinux.Project));
    public GLib.ListStore cards = new GLib.ListStore(typeof(HolderLinux.CardSummary));
    public Gtk.SingleSelection project_selection;
    public Gtk.SingleSelection card_selection;
    public Gee.ArrayList<string> toasts = new Gee.ArrayList<string>();
    public Gee.ArrayList<string> logs = new Gee.ArrayList<string>();
    public Gee.ArrayList<string> copies = new Gee.ArrayList<string>();

    public TerminalViewHarness(HolderLinux.PowerShellPrerequisites prerequisites,
                               bool stall = false,
                               bool attach_window = true) {
        try {
            root_dir = DirUtils.make_tmp("holder-terminal-view-XXXXXX");
        } catch (FileError e) {
            assert_not_reached();
        }
        discovery = new FakeDiscovery(prerequisites, stall);
        store = new FakeSessionStore(Path.build_filename(root_dir, "sessions"));
        projects.append(new HolderLinux.Project("p1", "Project One", "encrypted_git", root_dir, 10, 10));
        projects.append(new HolderLinux.Project("p2", "Project Two", "encrypted_git", root_dir, 10, 10));
        cards.append(new HolderLinux.CardSummary("c1", "p1", "Card One", "c1.md", 1.0, null, 10, 10));
        project_selection = unselected(projects);
        card_selection = unselected(cards);

        view = new HolderLinux.TerminalToolView(discovery, store, launcher, scheduler);
        view.toast_requested.connect((message) => toasts.add(message));
        view.debug_log_requested.connect((line) => logs.add(line));
        view.copy_to_card_requested.connect((text) => copies.add(text));
        if (attach_window) {
            window.set_content(view.widget);
        }
        settle();
    }

    private static Gtk.SingleSelection unselected(ListModel model) {
        var selection = new Gtk.SingleSelection(model);
        selection.set_autoselect(false);
        selection.set_can_unselect(true);
        selection.set_selected(Gtk.INVALID_LIST_POSITION);
        return selection;
    }

    public void bind(uint project_index = Gtk.INVALID_LIST_POSITION) {
        project_selection.set_selected(project_index);
        view.bind_context(project_selection, card_selection);
        settle();
    }

    public HolderLinux.TerminalSession add_session(string project_id,
                                                   string? transcript,
                                                   string? card_label = null) {
        try {
            var session = store.create_session(
                project_id, project_id == "p1" ? "Project One" : "Project Two",
                card_label != null ? "card" : null, card_label, root_dir
            );
            if (transcript != null) {
                FileUtils.set_contents(session.transcript_path, (!) transcript);
            }
            return session;
        } catch (Error e) {
            assert_not_reached();
        }
    }

    public void settle() {
        wt_settle();
    }

    public string page() {
        return ((Gtk.Stack) view.widget).get_visible_child_name() ?? "";
    }

    public Gtk.Button actions_button() {
        var actions = view.get_actions_widget();
        assert(actions != null);
        return (Gtk.Button) ((!) actions).get_first_child();
    }

    public Gtk.Button button(string label) {
        return wt_button(view.widget, label);
    }

    public Gtk.ListBox session_list() {
        return wt_find<Gtk.ListBox>(view.widget);
    }

    public int row_count() {
        int count = 0;
        var list = session_list();
        while (list.get_row_at_index(count) != null) {
            count++;
        }
        return count;
    }

    public Gtk.TextBuffer buffer() {
        return wt_find<Gtk.TextView>(view.widget).get_buffer();
    }

    public string transcript() {
        var buffer = buffer();
        Gtk.TextIter start;
        Gtk.TextIter end;
        buffer.get_bounds(out start, out end);
        return buffer.get_text(start, end, false);
    }

    public Gtk.ToggleButton raw_toggle() {
        foreach (var widget in wt_descendants(view.widget)) {
            var toggle = widget as Gtk.ToggleButton;
            if (toggle != null && ((!) toggle).get_label() == "Raw") {
                return (!) toggle;
            }
        }
        assert_not_reached();
    }

    public bool interrupted_notice_shown() {
        return wt_find<Gtk.Revealer>(view.widget).get_reveal_child();
    }

    public Adw.AlertDialog? dialog() {
        return window.get_visible_dialog() as Adw.AlertDialog;
    }

    public void cleanup() {
        try {
            var dir = File.new_for_path(root_dir);
            delete_tree(dir);
        } catch (Error e) {
            // A leftover temp directory must not fail the test.
        }
    }

    private static void delete_tree(File file) throws Error {
        if (file.query_file_type(FileQueryInfoFlags.NOFOLLOW_SYMLINKS) == FileType.DIRECTORY) {
            var children = file.enumerate_children("standard::name", FileQueryInfoFlags.NOFOLLOW_SYMLINKS);
            FileInfo? info;
            while ((info = children.next_file()) != null) {
                delete_tree(file.get_child(((!) info).get_name()));
            }
        }
        file.delete();
    }
}

// ---- adapter surface -----------------------------------------------------------------------------

private void test_adapter_identity_and_navigation() {
    var h = new TerminalViewHarness(wt_ready());
    assert(h.view.tool_id == "terminals");
    assert(h.view.tool_label == "Terminals");
    assert(h.view.get_content_widget() == h.view.widget);
    assert(h.view.get_actions_widget() != null);
    assert(!h.view.captures_application_shortcuts(null));
    assert(!h.view.captures_application_shortcuts(h.view.widget));

    int done = 0;
    h.view.navigate_to_projects_root.begin(null, (obj, res) => {
        assert(h.view.navigate_to_projects_root.end(res));
        done++;
    });
    h.view.navigate_to_project_root.begin("p1", (obj, res) => {
        assert(h.view.navigate_to_project_root.end(res));
        done++;
    });
    h.view.navigate_to_card.begin("c1", (obj, res) => {
        assert(h.view.navigate_to_card.end(res));
        done++;
    });
    assert(wait_for_condition(() => done == 3));
    h.cleanup();
}

private void test_scope_snapshot_reports_loading_while_discovery_runs() {
    var h = new TerminalViewHarness(wt_ready(), true);
    var project = (HolderLinux.Project) h.projects.get_item(0);
    var card = (HolderLinux.CardSummary) h.cards.get_item(0);

    var loading = h.view.get_scope_snapshot(project, card);
    assert(loading.is_loading);
    assert(loading.tool_id == "terminals");
    assert(loading.project_id == "p1");
    assert(loading.card_id == "c1");

    h.discovery.release();
    h.settle();
    assert(!h.view.get_scope_snapshot(project, null).is_loading);
    h.cleanup();
}

// ---- prerequisites -------------------------------------------------------------------------------

private void test_checking_page_shows_until_discovery_finishes() {
    var h = new TerminalViewHarness(wt_ready(), true);
    assert(h.page() == "checking");
    assert(h.discovery.calls == 1);
    assert(!h.discovery.last_force_refresh);

    h.discovery.release();
    h.settle();
    assert(h.page() == "sessions");
    h.cleanup();
}

private void test_missing_powershell_offers_install_and_manual_link() {
    var h = new TerminalViewHarness(wt_missing());
    assert(h.page() == "prerequisite");
    assert(wt_label(h.view.widget, "PowerShell 7 is required") != null);
    var install = h.button("Install PowerShell 7");
    assert(install.get_visible());
    assert(install.get_sensitive());
    assert(wt_find<Gtk.LinkButton>(h.view.widget).get_visible());
    h.cleanup();
}

private void test_missing_windows_terminal_hides_install_and_manual_link() {
    var h = new TerminalViewHarness(wt_terminal_missing());
    assert(h.page() == "prerequisite");
    assert(wt_label(h.view.widget, "Windows Terminal is required") != null);
    var install = h.button("Install PowerShell 7");
    assert(!install.get_visible());
    assert(!install.get_sensitive());
    assert(!wt_find<Gtk.LinkButton>(h.view.widget).get_visible());
    h.cleanup();
}

private void test_check_again_forces_a_fresh_discovery_and_ignores_repeat_clicks() {
    var h = new TerminalViewHarness(wt_missing());
    assert(h.page() == "prerequisite");

    h.discovery.next = wt_ready();
    h.discovery.stall = true;
    h.button("Check Again").clicked();
    assert(h.discovery.calls == 2);
    assert(h.discovery.last_force_refresh);
    // A known result keeps its page while the recheck runs.
    assert(h.page() == "prerequisite");

    h.button("Check Again").clicked();
    assert(h.discovery.calls == 2);

    h.discovery.release();
    h.settle();
    assert(h.page() == "sessions");
    h.cleanup();
}

// ---- opening terminals ---------------------------------------------------------------------------

private void test_new_terminal_is_blocked_until_ready_and_a_project_is_selected() {
    var h = new TerminalViewHarness(wt_missing());
    h.bind(0);
    assert(!h.actions_button().get_sensitive());
    h.actions_button().clicked();
    assert(h.toasts.size == 1);
    assert(h.toasts[0] == HolderLinux.TerminalPresenter.NOT_READY_MESSAGE);

    var ready = new TerminalViewHarness(wt_ready());
    ready.bind();
    assert(!ready.actions_button().get_sensitive());
    assert(wt_shows_label(ready.view.widget, "Select a project, then open a terminal."));
    assert(wt_label(ready.view.widget, HolderLinux.TerminalPresenter.NO_SESSION_TITLE) != null);
    ready.actions_button().clicked();
    assert(ready.toasts.size == 1);
    assert(ready.toasts[0] == HolderLinux.TerminalPresenter.SELECT_PROJECT_MESSAGE);
    assert(ready.launcher.session_launches == 0);
    h.cleanup();
    ready.cleanup();
}

private void test_new_terminal_for_a_project_opens_and_selects_a_recording_session() {
    var h = new TerminalViewHarness(wt_ready());
    h.bind(0);
    assert(h.actions_button().get_sensitive());
    assert(h.row_count() == 0);
    assert(wt_shows_label(h.view.widget, "No terminal sessions for this project yet."));

    h.actions_button().clicked();
    h.settle();

    assert(h.launcher.session_launches == 1);
    var session = (!) h.launcher.last_session;
    assert(session.project_id == "p1");
    assert(session.card_id == null);
    assert(h.row_count() == 1);
    assert(h.session_list().get_selected_row() == h.session_list().get_row_at_index(0));
    assert(!wt_shows_label(h.view.widget, "No terminal sessions for this project yet."));
    assert(wt_label(h.view.widget, "Project One") != null);
    assert(wt_label(h.view.widget, "Recording") != null);
    assert(!h.button("Copy All to Card").get_sensitive());
    assert(h.toasts.contains("Holder terminal opened in Windows Terminal."));
    assert(h.logs.contains("Windows terminal session opened: %s".printf(session.session_id)));
    h.cleanup();
}

private void test_new_terminal_with_a_card_selected_is_titled_after_the_card() {
    var h = new TerminalViewHarness(wt_ready());
    h.card_selection.set_selected(0);
    h.bind(0);

    h.actions_button().clicked();
    h.settle();

    var session = (!) h.launcher.last_session;
    assert(session.card_id == "c1");
    assert(session.card_label == "Card One");
    assert(session.working_directory == h.root_dir);
    assert(wt_label(h.view.widget, "Card One") != null);
    h.cleanup();
}

private void test_new_terminal_launch_failure_is_reported() {
    var h = new TerminalViewHarness(wt_ready());
    h.bind(0);
    h.launcher.fail = true;

    h.actions_button().clicked();
    h.settle();

    assert(h.row_count() == 0);
    assert(h.toasts.contains("Could not open Windows Terminal."));
    assert(h.logs.contains("Windows terminal launch failed: wt.exe vanished"));
    h.cleanup();
}

// ---- sessions and transcripts --------------------------------------------------------------------

private void test_project_sessions_load_and_the_newest_is_shown() {
    var h = new TerminalViewHarness(wt_ready());
    h.add_session("p1", WT_COMPLETE_TRANSCRIPT, "Card One");
    h.add_session("p2", WT_COMPLETE_TRANSCRIPT);
    h.bind(0);

    assert(h.row_count() == 1);
    assert(h.session_list().get_selected_row() != null);
    assert(!wt_shows_label(h.view.widget, "No terminal sessions for this project yet."));
    assert(wt_label(h.view.widget, "Card One") != null);
    assert(wt_label(h.view.widget, "Completed") != null);
    assert(h.transcript() == WT_USEFUL_TEXT);
    assert(!h.interrupted_notice_shown());
    assert(h.button("Copy All to Card").get_sensitive());
    assert(h.button("Copy Selection to Card").get_sensitive());

    // The row's subtitle carries the state too.
    bool row_subtitle = false;
    foreach (var widget in wt_descendants(h.session_list())) {
        var label = widget as Gtk.Label;
        if (label != null && ((!) label).get_text().has_prefix("Completed · ")) {
            row_subtitle = true;
        }
    }
    assert(row_subtitle);
    h.cleanup();
}

private void test_interrupted_session_shows_the_notice() {
    var h = new TerminalViewHarness(wt_ready());
    h.add_session("p1", WT_OPEN_TRANSCRIPT);
    h.bind(0);

    assert(wt_label(h.view.widget, "Interrupted") != null);
    assert(h.interrupted_notice_shown());
    assert(h.transcript() == "PS C:\\Work> ping.exe 127.0.0.1");
    h.cleanup();
}

private void test_raw_toggle_switches_between_useful_and_raw_text() {
    var h = new TerminalViewHarness(wt_ready());
    h.add_session("p1", WT_COMPLETE_TRANSCRIPT);
    h.bind(0);
    assert(h.transcript() == WT_USEFUL_TEXT);

    h.raw_toggle().set_active(true);
    assert(h.transcript().contains("PowerShell transcript start"));
    assert(h.transcript().contains("PowerShell transcript end"));

    h.raw_toggle().set_active(false);
    assert(h.transcript() == WT_USEFUL_TEXT);
    h.cleanup();
}

private void test_raw_toggle_without_a_session_shows_nothing() {
    var h = new TerminalViewHarness(wt_ready());
    h.add_session("p1", WT_COMPLETE_TRANSCRIPT);
    h.bind();

    h.raw_toggle().set_active(true);
    assert(h.transcript() == "");
    assert(!h.button("Copy All to Card").get_sensitive());

    // Selecting a session resets the toggle before its transcript is read.
    h.project_selection.set_selected(0);
    h.settle();
    assert(!h.raw_toggle().get_active());
    assert(h.transcript() == WT_USEFUL_TEXT);
    h.cleanup();
}

private void test_changing_or_clearing_the_project_reloads_the_sessions() {
    var h = new TerminalViewHarness(wt_ready());
    h.add_session("p2", WT_COMPLETE_TRANSCRIPT, "Other Card");
    h.bind(0);
    assert(h.row_count() == 0);
    assert(wt_label(h.view.widget, HolderLinux.TerminalPresenter.NO_SESSION_TITLE) != null);

    h.project_selection.set_selected(1);
    h.settle();
    assert(h.row_count() == 1);
    assert(wt_label(h.view.widget, "Other Card") != null);
    assert(h.button("Copy All to Card").get_sensitive());

    h.project_selection.set_selected(Gtk.INVALID_LIST_POSITION);
    h.settle();
    assert(h.row_count() == 0);
    assert(wt_shows_label(h.view.widget, "Select a project, then open a terminal."));
    assert(wt_label(h.view.widget, HolderLinux.TerminalPresenter.NO_SESSION_TITLE) != null);
    assert(h.transcript() == "");
    assert(!h.interrupted_notice_shown());
    assert(!h.button("Copy All to Card").get_sensitive());
    assert(!h.button("Copy Selection to Card").get_sensitive());
    h.cleanup();
}

private void test_session_load_failure_is_shown_and_logged() {
    var h = new TerminalViewHarness(wt_ready());
    h.store.fail_load = true;
    h.bind(0);

    assert(wt_shows_label(h.view.widget, "Could not load terminal sessions."));
    assert(h.logs.contains("Terminal session load failed: session directory unreadable"));
    h.cleanup();
}

private void test_transcript_read_failure_is_shown_and_logged() {
    var h = new TerminalViewHarness(wt_ready());
    h.add_session("p1", WT_COMPLETE_TRANSCRIPT);
    h.store.fail_read = true;
    h.bind(0);

    assert(wt_label(h.view.widget, "Could not read transcript") != null);
    assert(h.logs.contains("Terminal transcript read failed: transcript locked"));
    assert(h.transcript() == "");
    h.cleanup();
}

// ---- live transcript refresh ---------------------------------------------------------------------

private void test_transcript_changes_refresh_after_a_debounce() {
    var h = new TerminalViewHarness(wt_ready());
    h.bind(0);
    h.actions_button().clicked();
    h.settle();
    var session = (!) h.launcher.last_session;
    assert(h.transcript() == "");

    // A neighbouring file in the session directory is ignored.
    wt_append(Path.build_filename(Path.get_dirname(session.transcript_path), "scratch.txt"), "x");
    wt_append(session.transcript_path, WT_OPEN_TRANSCRIPT);
    assert(wait_for_condition(() => h.scheduler.pending_with_delay(100) == 1));
    h.settle();
    // Further events for the same file share the queued refresh.
    assert(h.scheduler.pending_one_shots() == 1);

    h.scheduler.run_due(100);
    assert(h.transcript() == "PS C:\\Work> ping.exe 127.0.0.1");
    assert(wt_label(h.view.widget, "Recording") != null);

    var footer = WT_COMPLETE_TRANSCRIPT.substring(
        WT_COMPLETE_TRANSCRIPT.index_of("PS C:\\Work> Stop-Transcript")
    );
    wt_append(session.transcript_path, footer);
    assert(wait_for_condition(() => h.scheduler.pending_with_delay(100) >= 1));
    h.scheduler.run_due(100);
    assert(h.transcript() == "PS C:\\Work> ping.exe 127.0.0.1");
    assert(session.state == HolderLinux.TerminalSessionState.COMPLETED);
    assert(wt_label(h.view.widget, "Completed") != null);
    h.cleanup();
}

private void test_switching_sessions_cancels_a_queued_refresh() {
    var h = new TerminalViewHarness(wt_ready());
    h.bind(0);
    h.actions_button().clicked();
    h.settle();
    var session = (!) h.launcher.last_session;

    wt_append(session.transcript_path, WT_OPEN_TRANSCRIPT);
    assert(wait_for_condition(() => h.scheduler.pending_with_delay(100) == 1));
    var cancels_before = h.scheduler.cancel_calls;

    h.project_selection.set_selected(1);
    h.settle();
    assert(h.scheduler.cancel_calls > cancels_before);
    assert(h.scheduler.pending_one_shots() == 0);
    h.cleanup();
}

// ---- copying to the card -------------------------------------------------------------------------

private void test_copy_all_sends_the_useful_or_raw_text() {
    var h = new TerminalViewHarness(wt_ready());
    h.add_session("p1", WT_COMPLETE_TRANSCRIPT);
    h.bind(0);

    h.button("Copy All to Card").clicked();
    assert(h.copies.size == 1);
    assert(h.copies[0] == WT_USEFUL_TEXT);

    h.raw_toggle().set_active(true);
    h.button("Copy All to Card").clicked();
    assert(h.copies.size == 2);
    assert(h.copies[1].contains("PowerShell transcript start"));
    h.cleanup();
}

private void test_copy_all_with_nothing_to_copy_says_so() {
    var h = new TerminalViewHarness(wt_ready());
    h.bind();
    h.button("Copy All to Card").clicked();
    assert(h.copies.size == 0);
    assert(h.toasts.contains(HolderLinux.TerminalPresenter.NOTHING_TO_COPY_MESSAGE));

    // A session whose transcript is still empty has nothing to copy either.
    h.project_selection.set_selected(0);
    h.settle();
    h.actions_button().clicked();
    h.settle();
    h.toasts.clear();
    h.button("Copy All to Card").clicked();
    assert(h.copies.size == 0);
    assert(h.toasts.contains(HolderLinux.TerminalPresenter.NOTHING_TO_COPY_MESSAGE));
    h.cleanup();
}

private void test_copy_selection_needs_a_non_blank_selection() {
    var h = new TerminalViewHarness(wt_ready());
    h.add_session("p1", WT_COMPLETE_TRANSCRIPT);
    h.bind(0);
    var buffer = h.buffer();
    var copy = h.button("Copy Selection to Card");

    copy.clicked();
    assert(h.copies.size == 0);
    assert(h.toasts.size == 1);
    assert(h.toasts[0] == HolderLinux.TerminalPresenter.SELECT_TEXT_MESSAGE);

    // Only the line break between the two lines.
    var newline = WT_USEFUL_TEXT.index_of_char('\n');
    var break_offset = WT_USEFUL_TEXT.substring(0, newline).char_count();
    Gtk.TextIter start;
    Gtk.TextIter end;
    buffer.get_iter_at_offset(out start, break_offset);
    buffer.get_iter_at_offset(out end, break_offset + 1);
    buffer.select_range(start, end);
    copy.clicked();
    assert(h.copies.size == 0);
    assert(h.toasts.size == 2);
    assert(h.toasts[1] == HolderLinux.TerminalPresenter.SELECT_TEXT_MESSAGE);

    buffer.get_end_iter(out end);
    buffer.get_iter_at_offset(out start, break_offset + 1);
    buffer.select_range(start, end);
    copy.clicked();
    assert(h.copies.size == 1);
    assert(h.copies[0] == "hello");
    h.cleanup();
}

// ---- installing PowerShell -----------------------------------------------------------------------

private void test_install_does_nothing_before_discovery_or_outside_a_window() {
    var pending = new TerminalViewHarness(wt_missing(), true);
    pending.button("Install PowerShell 7").clicked();
    pending.settle();
    assert(pending.dialog() == null);

    var detached = new TerminalViewHarness(wt_missing(), false, false);
    detached.button("Install PowerShell 7").clicked();
    detached.settle();
    assert(detached.dialog() == null);
    assert(detached.launcher.install_launches == 0);
    pending.discovery.release();
    pending.settle();
    pending.cleanup();
    detached.cleanup();
}

private void test_install_dialog_cancel_installs_nothing() {
    var h = new TerminalViewHarness(wt_missing());
    h.button("Install PowerShell 7").clicked();
    assert(wait_for_condition(() => h.dialog() != null));
    var dialog = (!) h.dialog();
    assert(dialog.get_heading() == "Install PowerShell 7?");

    wt_button(dialog, "Cancel").clicked();
    h.settle();
    assert(h.launcher.install_launches == 0);
    assert(h.scheduler.pending_repeating() == 0);
    assert(h.toasts.size == 0);
    h.cleanup();
}

private void test_install_opens_the_installer_and_polls_until_ready() {
    var h = new TerminalViewHarness(wt_missing());
    h.bind(0);
    h.button("Install PowerShell 7").clicked();
    assert(wait_for_condition(() => h.dialog() != null));
    wt_button((!) h.dialog(), "Install").clicked();
    h.settle();

    assert(h.launcher.install_launches == 1);
    assert(h.toasts.contains(
        "PowerShell installer opened. Holder will detect it when installation finishes."
    ));
    assert(h.scheduler.pending_repeating() == 1);

    // Still missing: keep polling.
    h.scheduler.tick_repeating();
    h.settle();
    assert(h.discovery.calls == 2);
    assert(h.page() == "prerequisite");
    assert(h.scheduler.pending_repeating() == 1);

    // Installed: the sessions page appears and polling stops.
    h.discovery.next = wt_ready();
    h.scheduler.tick_repeating();
    h.settle();
    assert(h.page() == "sessions");
    assert(h.scheduler.pending_repeating() == 0);
    assert(h.actions_button().get_sensitive());
    h.cleanup();
}

private void test_install_polling_gives_up_after_the_limit() {
    var h = new TerminalViewHarness(wt_missing());
    h.button("Install PowerShell 7").clicked();
    assert(wait_for_condition(() => h.dialog() != null));
    wt_button((!) h.dialog(), "Install").clicked();
    h.settle();

    for (int i = 1; i < HolderLinux.TerminalPresenter.INSTALL_POLL_MAX_COUNT; i++) {
        h.scheduler.tick_repeating();
        h.settle();
        assert(h.scheduler.pending_repeating() == 1);
    }
    h.scheduler.tick_repeating();
    h.settle();
    assert(h.scheduler.pending_repeating() == 0);
    assert(h.discovery.calls == 1 + HolderLinux.TerminalPresenter.INSTALL_POLL_MAX_COUNT);
    h.cleanup();
}

private void test_installing_again_restarts_polling() {
    var h = new TerminalViewHarness(wt_missing());
    for (int i = 0; i < 2; i++) {
        h.button("Install PowerShell 7").clicked();
        assert(wait_for_condition(() => h.dialog() != null));
        wt_button((!) h.dialog(), "Install").clicked();
        h.settle();
    }
    assert(h.launcher.install_launches == 2);
    assert(h.scheduler.pending_repeating() == 1);
    assert(h.scheduler.cancel_calls >= 1);
    h.cleanup();
}

private void test_installer_launch_failure_is_reported() {
    var h = new TerminalViewHarness(wt_missing());
    h.launcher.fail = true;
    h.button("Install PowerShell 7").clicked();
    assert(wait_for_condition(() => h.dialog() != null));
    wt_button((!) h.dialog(), "Install").clicked();
    h.settle();

    assert(h.toasts.contains("Could not open the PowerShell installer."));
    assert(h.logs.contains("PowerShell installer launch failed: winget vanished"));
    assert(h.scheduler.pending_repeating() == 0);
    h.cleanup();
}

public static int main(string[] args) {
    Test.init(ref args);
    if (!Gtk.init_check()) {
        stdout.printf("Skipping Windows terminal tool view tests: GTK display is unavailable.\n");
        return 0;
    }
    Adw.init();

    Test.add_func("/holder/windows-terminal-view/adapter", test_adapter_identity_and_navigation);
    Test.add_func("/holder/windows-terminal-view/scope-loading", test_scope_snapshot_reports_loading_while_discovery_runs);
    Test.add_func("/holder/windows-terminal-view/checking-page", test_checking_page_shows_until_discovery_finishes);
    Test.add_func("/holder/windows-terminal-view/powershell-missing", test_missing_powershell_offers_install_and_manual_link);
    Test.add_func("/holder/windows-terminal-view/terminal-missing", test_missing_windows_terminal_hides_install_and_manual_link);
    Test.add_func("/holder/windows-terminal-view/check-again", test_check_again_forces_a_fresh_discovery_and_ignores_repeat_clicks);
    Test.add_func("/holder/windows-terminal-view/new-terminal-blocked", test_new_terminal_is_blocked_until_ready_and_a_project_is_selected);
    Test.add_func("/holder/windows-terminal-view/new-terminal-project", test_new_terminal_for_a_project_opens_and_selects_a_recording_session);
    Test.add_func("/holder/windows-terminal-view/new-terminal-card", test_new_terminal_with_a_card_selected_is_titled_after_the_card);
    Test.add_func("/holder/windows-terminal-view/new-terminal-failure", test_new_terminal_launch_failure_is_reported);
    Test.add_func("/holder/windows-terminal-view/sessions-load", test_project_sessions_load_and_the_newest_is_shown);
    Test.add_func("/holder/windows-terminal-view/interrupted", test_interrupted_session_shows_the_notice);
    Test.add_func("/holder/windows-terminal-view/raw-toggle", test_raw_toggle_switches_between_useful_and_raw_text);
    Test.add_func("/holder/windows-terminal-view/raw-toggle-no-session", test_raw_toggle_without_a_session_shows_nothing);
    Test.add_func("/holder/windows-terminal-view/project-change", test_changing_or_clearing_the_project_reloads_the_sessions);
    Test.add_func("/holder/windows-terminal-view/load-failure", test_session_load_failure_is_shown_and_logged);
    Test.add_func("/holder/windows-terminal-view/read-failure", test_transcript_read_failure_is_shown_and_logged);
    Test.add_func("/holder/windows-terminal-view/live-refresh", test_transcript_changes_refresh_after_a_debounce);
    Test.add_func("/holder/windows-terminal-view/refresh-cancelled", test_switching_sessions_cancels_a_queued_refresh);
    Test.add_func("/holder/windows-terminal-view/copy-all", test_copy_all_sends_the_useful_or_raw_text);
    Test.add_func("/holder/windows-terminal-view/copy-all-empty", test_copy_all_with_nothing_to_copy_says_so);
    Test.add_func("/holder/windows-terminal-view/copy-selection", test_copy_selection_needs_a_non_blank_selection);
    Test.add_func("/holder/windows-terminal-view/install-guarded", test_install_does_nothing_before_discovery_or_outside_a_window);
    Test.add_func("/holder/windows-terminal-view/install-cancel", test_install_dialog_cancel_installs_nothing);
    Test.add_func("/holder/windows-terminal-view/install-poll", test_install_opens_the_installer_and_polls_until_ready);
    Test.add_func("/holder/windows-terminal-view/install-poll-limit", test_install_polling_gives_up_after_the_limit);
    Test.add_func("/holder/windows-terminal-view/install-again", test_installing_again_restarts_polling);
    Test.add_func("/holder/windows-terminal-view/install-failure", test_installer_launch_failure_is_reported);
    return Test.run();
}

}
