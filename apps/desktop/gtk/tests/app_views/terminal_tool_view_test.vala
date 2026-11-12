using GLib;

namespace HolderLinuxTests {

// A terminal view whose tabs run `shell` instead of the user's shell. `cat` prints nothing on its
// own, so a terminal shows only what a test feeds it.
private class TerminalFixture : Object {
    public HolderLinux.TerminalToolView view;
    public Adw.Window window = new Adw.Window();
    public Gee.ArrayList<string> logs = new Gee.ArrayList<string>();
    public Gee.ArrayList<string> toasts = new Gee.ArrayList<string>();
    public Gee.ArrayList<string> copied = new Gee.ArrayList<string>();
    private string? previous_shell;

    public TerminalFixture(string shell = "cat") {
        previous_shell = Environment.get_variable("SHELL");
        Environment.set_variable("SHELL", shell, true);
        view = new HolderLinux.TerminalToolView();
        view.debug_log_requested.connect((line) => { logs.add(line); });
        view.toast_requested.connect((message) => { toasts.add(message); });
        view.copy_to_card_requested.connect((text) => { copied.add(text); });
        window.set_content(view.widget);
    }

    // VTE sets up its clipboard when a terminal is realized, and selecting or copying text needs it,
    // so tests that do either show the window first, as a user would see the terminal.
    public void present() {
        window.set_default_size(800, 500);
        window.present();
        assert(wait_for_condition(() => terminal().get_realized()));
    }

    public Gtk.Notebook notebook() {
        foreach (var widget in av_descendants(view.widget)) {
            if (widget is Gtk.Notebook) {
                return (Gtk.Notebook) widget;
            }
        }
        assert_not_reached();
    }

    public Vte.Terminal terminal(int page = -1) {
        var tabs = notebook();
        var index = page >= 0 ? page : tabs.get_current_page();
        return (Vte.Terminal) (!) tabs.get_nth_page(index);
    }

    public Gtk.Label tab_title(int page) {
        var tabs = notebook();
        var tab = (!) tabs.get_tab_label((!) tabs.get_nth_page(page));
        foreach (var widget in av_descendants(tab)) {
            if (widget is Gtk.Label) {
                return (Gtk.Label) widget;
            }
        }
        assert_not_reached();
    }

    public Gtk.Button button_with_tooltip(Gtk.Widget root, string tooltip) {
        foreach (var widget in av_descendants(root)) {
            var button = widget as Gtk.Button;
            if (button != null && ((!) button).get_tooltip_text() == tooltip) {
                return (!) button;
            }
        }
        assert_not_reached();
    }

    public void close_tab(int page) {
        var tabs = notebook();
        button_with_tooltip((!) tabs.get_tab_label((!) tabs.get_nth_page(page)), "Close terminal").clicked();
    }

    // VTE processes what it is fed on a later main loop turn; wait until the terminal shows it.
    public void feed(Vte.Terminal terminal, string text, string shows) {
        terminal.feed(text.data);
        assert(wait_for_condition(() => all_text(terminal).contains(shows)));
    }

    public string all_text(Vte.Terminal terminal) {
        terminal.select_all();
        var text = terminal.get_text_selected(Vte.Format.TEXT) ?? "";
        terminal.unselect_all();
        return text;
    }

    // Emits the view's own Ctrl+Shift key handler: the terminal's key controller that is not VTE's.
    public bool press(Vte.Terminal terminal, uint keyval, Gdk.ModifierType state) {
        Gtk.EventControllerKey? keys = null;
        var controllers = terminal.observe_controllers();
        for (uint i = 0; i < controllers.get_n_items(); i++) {
            var candidate = controllers.get_item(i) as Gtk.EventControllerKey;
            if (candidate != null && ((!) candidate).get_name() != "vte-key-controller") {
                keys = candidate;
            }
        }
        assert(keys != null);
        bool handled = false;
        Signal.emit_by_name((!) keys, "key-pressed", keyval, 0u, state, out handled);
        return handled;
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

private string clipboard_text() {
    string? text = null;
    var done = false;
    Gdk.Display.get_default().get_clipboard().read_text_async.begin(null, (obj, result) => {
        try {
            text = ((Gdk.Clipboard) obj).read_text_async.end(result);
        } catch (Error e) {
            text = null;
        }
        done = true;
    });
    assert(wait_for_condition(() => done));
    return text ?? "";
}

private void test_the_terminal_tool_describes_itself_and_ignores_navigation() {
    var f = new TerminalFixture();
    var view = f.view;

    assert(view.tool_id == "terminals");
    assert(view.tool_label == "Terminals");
    assert(view.get_content_widget() == view.widget);
    var actions = view.get_actions_widget();
    assert(actions != null);
    f.button_with_tooltip((!) actions, "New Terminal");
    var scope = view.get_scope_snapshot(null, null);
    assert(scope.tool_id == "terminals");

    var finished = 0;
    view.navigate_to_projects_root.begin(null, (obj, result) => {
        assert(view.navigate_to_projects_root.end(result));
        finished++;
    });
    view.navigate_to_project_root.begin("p1", (obj, result) => {
        assert(view.navigate_to_project_root.end(result));
        finished++;
    });
    view.navigate_to_card.begin("c1", (obj, result) => {
        assert(view.navigate_to_card.end(result));
        finished++;
    });
    assert(wait_for_condition(() => finished == 3));
    view.bind_context(
        new Gtk.SingleSelection(new GLib.ListStore(typeof(HolderLinux.Project))),
        new Gtk.SingleSelection(new GLib.ListStore(typeof(HolderLinux.CardSummary)))
    );
    f.destroy();
}

private void test_only_a_focused_terminal_keeps_the_application_shortcuts() {
    var f = new TerminalFixture();
    var terminal = f.terminal();

    assert(f.view.captures_application_shortcuts(terminal));
    assert(!f.view.captures_application_shortcuts(f.notebook()));
    assert(!f.view.captures_application_shortcuts(new Gtk.Button()));
    assert(!f.view.captures_application_shortcuts(null));
    f.destroy();
}

private void test_the_first_tab_starts_the_shell_and_reports_it() {
    var f = new TerminalFixture();

    assert(f.notebook().get_n_pages() == 1);
    assert(f.tab_title(0).get_text() == "Term 1");
    assert(wait_for_condition(() => f.logs.size > 0));
    assert(f.logs[0].has_prefix("Terminal spawned (pid="));
    f.destroy();
}

private void test_a_shell_that_cannot_start_is_reported() {
    var f = new TerminalFixture("holder-no-such-shell");

    assert(wait_for_condition(() => f.logs.size > 0));
    assert(f.logs[0].has_prefix("Terminal spawn failed: "));
    f.destroy();
}

private void test_new_tabs_are_numbered_and_closing_selects_a_neighbour() {
    var f = new TerminalFixture();
    var tabs = f.notebook();

    f.button_with_tooltip((!) f.view.get_actions_widget(), "New Terminal").clicked();
    f.button_with_tooltip((!) f.view.get_actions_widget(), "New Terminal").clicked();
    assert(tabs.get_n_pages() == 3);
    assert(tabs.get_current_page() == 2);
    assert(f.tab_title(2).get_text() == "Term 3");

    f.close_tab(1);
    assert(tabs.get_n_pages() == 2);
    assert(tabs.get_current_page() == 1);
    assert(f.logs.contains("Terminal tab closed"));
    f.destroy();
}

private void test_a_second_click_on_a_closed_tabs_button_is_ignored() {
    var f = new TerminalFixture();
    f.button_with_tooltip((!) f.view.get_actions_widget(), "New Terminal").clicked();
    var tabs = f.notebook();
    var close = f.button_with_tooltip((!) tabs.get_tab_label((!) tabs.get_nth_page(0)), "Close terminal");

    close.clicked();
    close.clicked();

    assert(tabs.get_n_pages() == 1);
    assert(f.logs.contains("Terminal tab closed"));
    var closed = 0;
    foreach (var line in f.logs) {
        if (line == "Terminal tab closed") {
            closed++;
        }
    }
    assert(closed == 1);
    f.destroy();
}

private void test_closing_the_last_tab_opens_a_fresh_one() {
    var f = new TerminalFixture();
    var first = f.terminal(0);

    f.close_tab(0);

    var tabs = f.notebook();
    assert(tabs.get_n_pages() == 1);
    assert(f.terminal(0) != first);
    assert(f.tab_title(0).get_text() == "Term 2");
    f.destroy();
}

private void test_the_tab_follows_the_title_the_program_sets() {
    var f = new TerminalFixture();
    var terminal = f.terminal();

    terminal.feed("\033]0;Build logs\007".data);
    assert(wait_for_condition(() => f.tab_title(0).get_text() == "Build logs"));
    assert(f.tab_title(0).get_tooltip_text() == "Build logs");

    terminal.feed("\033]0;\007".data);
    assert(wait_for_condition(() => f.tab_title(0).get_text() == "Term 1"));
    f.destroy();
}

private void test_copy_to_card_needs_a_selection() {
    var f = new TerminalFixture();
    f.present();
    var terminal = f.terminal();
    f.feed(terminal, "make test\r\nall passed\r\n", "all passed");

    terminal.activate_action("terminal.copy-to-card", null);
    assert(f.toasts.size == 1 && f.toasts[0] == "Select terminal text first.");

    terminal.activate_action("terminal.select-all", null);
    terminal.activate_action("terminal.copy-to-card", null);
    assert(f.copied.size == 1 && f.copied[0].contains("all passed"));
    f.destroy();
}

private void test_copy_all_to_card_copies_everything_or_says_there_is_nothing() {
    var f = new TerminalFixture();
    f.present();
    var terminal = f.terminal();

    terminal.activate_action("terminal.copy-all-to-card", null);
    assert(f.toasts.size == 1 && f.toasts[0] == "Terminal has no text to copy.");

    f.feed(terminal, "make test\r\nall passed\r\n", "all passed");
    terminal.activate_action("terminal.copy-all-to-card", null);
    assert(f.copied.size == 1);
    assert(f.copied[0].contains("make test") && f.copied[0].contains("all passed"));
    // Copying everything leaves no selection behind.
    assert(!terminal.get_has_selection());
    f.destroy();
}

private void test_the_menu_and_ctrl_shift_keys_copy_paste_and_select() {
    var f = new TerminalFixture();
    f.present();
    var terminal = f.terminal();
    assert(wait_for_condition(() => f.logs.size > 0));
    f.feed(terminal, "first output\r\n", "first output");

    terminal.activate_action("terminal.select-all", null);
    terminal.activate_action("terminal.copy", null);
    assert(clipboard_text().contains("first output"));

    terminal.unselect_all();
    assert(f.press(terminal, Gdk.Key.A, Gdk.ModifierType.CONTROL_MASK | Gdk.ModifierType.SHIFT_MASK));
    assert(terminal.get_has_selection());
    f.feed(terminal, "\r\nsecond output\r\n", "second output");
    terminal.select_all();
    assert(f.press(terminal, Gdk.Key.c, Gdk.ModifierType.CONTROL_MASK | Gdk.ModifierType.SHIFT_MASK));
    assert(clipboard_text().contains("second output"));

    // Pasting types into the program, and the terminal echoes what it was given.
    Gdk.Display.get_default().get_clipboard().set_text("pasted-by-menu");
    terminal.activate_action("terminal.paste", null);
    assert(wait_for_condition(() => f.all_text(terminal).contains("pasted-by-menu")));
    Gdk.Display.get_default().get_clipboard().set_text("pasted-by-key");
    assert(f.press(terminal, Gdk.Key.V, Gdk.ModifierType.CONTROL_MASK | Gdk.ModifierType.SHIFT_MASK));
    assert(wait_for_condition(() => f.all_text(terminal).contains("pasted-by-key")));

    // Anything else, or the keys without both Ctrl and Shift, is the terminal's.
    assert(!f.press(terminal, Gdk.Key.x, Gdk.ModifierType.CONTROL_MASK | Gdk.ModifierType.SHIFT_MASK));
    assert(!f.press(terminal, Gdk.Key.c, Gdk.ModifierType.CONTROL_MASK));
    f.destroy();
}

internal void register_terminal_tool_view_tests() {
    Test.add_func("/app_views/terminal/describes_itself", test_the_terminal_tool_describes_itself_and_ignores_navigation);
    Test.add_func("/app_views/terminal/captures_shortcuts", test_only_a_focused_terminal_keeps_the_application_shortcuts);
    Test.add_func("/app_views/terminal/starts_shell", test_the_first_tab_starts_the_shell_and_reports_it);
    Test.add_func("/app_views/terminal/shell_fails", test_a_shell_that_cannot_start_is_reported);
    Test.add_func("/app_views/terminal/new_and_close", test_new_tabs_are_numbered_and_closing_selects_a_neighbour);
    Test.add_func("/app_views/terminal/second_close_click", test_a_second_click_on_a_closed_tabs_button_is_ignored);
    Test.add_func("/app_views/terminal/close_last", test_closing_the_last_tab_opens_a_fresh_one);
    Test.add_func("/app_views/terminal/tab_title", test_the_tab_follows_the_title_the_program_sets);
    Test.add_func("/app_views/terminal/copy_to_card", test_copy_to_card_needs_a_selection);
    Test.add_func("/app_views/terminal/copy_all_to_card", test_copy_all_to_card_copies_everything_or_says_there_is_nothing);
    Test.add_func("/app_views/terminal/clipboard_and_keys", test_the_menu_and_ctrl_shift_keys_copy_paste_and_select);
}

}
