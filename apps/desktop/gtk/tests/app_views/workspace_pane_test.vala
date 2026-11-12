using GLib;

namespace HolderLinuxTests {

private const Gdk.ModifierType WS_CTRL = Gdk.ModifierType.CONTROL_MASK;

// A workspace recording every signal it raises.
private class RecordingWorkspace : WorkspaceFixture {
    public Gee.ArrayList<string> events = new Gee.ArrayList<string>();
    public Gee.ArrayList<Gtk.TextMark> dropped_marks = new Gee.ArrayList<Gtk.TextMark>();

    public RecordingWorkspace() {
        base();
        var w = workspace;
        w.refresh_requested.connect(() => { events.add("refresh"); });
        w.new_project_requested.connect(() => { events.add("new_project"); });
        w.new_card_requested.connect(() => { events.add("new_card"); });
        w.explorer_panel_toggled.connect((visible) => { events.add("explorer|" + visible.to_string()); });
        w.ai_panel_toggled.connect((visible) => { events.add("ai|" + visible.to_string()); });
        w.asset_preview_toggled.connect((visible) => { events.add("asset_preview|" + visible.to_string()); });
        w.toolbox_toggled.connect((visible) => { events.add("toolbox|" + visible.to_string()); });
        w.open_debug_panel_requested.connect(() => { events.add("open_debug_panel"); });
        w.search_activated.connect(() => { events.add("search_activated"); });
        w.search_changed.connect(() => { events.add("search_changed"); });
        w.search_cleared.connect(() => { events.add("search_cleared"); });
        w.search_focus_results_requested.connect(() => { events.add("search_focus_results"); });
        w.search_result_activated.connect((position) => { events.add("search_result|%u".printf(position)); });
        w.find_next_requested.connect(() => { events.add("find_next"); });
        w.replace_requested.connect(() => { events.add("replace"); });
        w.replace_all_requested.connect(() => { events.add("replace_all"); });
        w.file_dropped.connect((file, mark) => {
            events.add("file_dropped|" + (file.get_basename() ?? ""));
            dropped_marks.add(mark);
        });
    }

    public Gtk.Button tooltip(string text) {
        return av_button_with_tooltip(workspace.widget, text);
    }

    public Gtk.ToggleButton toggle(string text) {
        return (Gtk.ToggleButton) tooltip(text);
    }

    public Gtk.Revealer find_bar() {
        var entry = find_entry();
        return (Gtk.Revealer) (!) entry.get_ancestor(typeof(Gtk.Revealer));
    }

    public Gtk.Entry find_entry() {
        return entry_with_placeholder("Search text");
    }

    public Gtk.Entry replace_entry() {
        return entry_with_placeholder("Replacement text");
    }

    private Gtk.Entry entry_with_placeholder(string placeholder) {
        foreach (var widget in av_descendants(workspace.widget)) {
            var entry = widget as Gtk.Entry;
            if (entry != null && ((!) entry).get_placeholder_text() == placeholder) {
                return (!) entry;
            }
        }
        assert_not_reached();
    }

    public void set_text_and_select(string text, int start, int end) {
        workspace.editor_buffer.set_text(text, -1);
        Gtk.TextIter a;
        Gtk.TextIter b;
        workspace.editor_buffer.get_iter_at_offset(out a, start);
        workspace.editor_buffer.get_iter_at_offset(out b, end);
        workspace.editor_buffer.select_range(a, b);
    }

    public void set_text_with_cursor(string text, int offset) {
        set_text_and_select(text, offset, offset);
    }

    public bool key(uint keyval, Gdk.ModifierType state = 0) {
        return av_press(workspace.editor_view, keyval, state);
    }

    public string selection() {
        Gtk.TextIter a;
        Gtk.TextIter b;
        workspace.editor_buffer.get_selection_bounds(out a, out b);
        return workspace.editor_buffer.get_text(a, b, false);
    }

    public int cursor_offset() {
        Gtk.TextIter cursor;
        workspace.editor_buffer.get_iter_at_mark(out cursor, workspace.editor_buffer.get_insert());
        return cursor.get_offset();
    }

    // Editor view coordinates of the middle of the character at this offset.
    public void point_at(int offset, out double x, out double y) {
        Gtk.TextIter iter;
        workspace.editor_buffer.get_iter_at_offset(out iter, offset);
        Gdk.Rectangle location;
        workspace.editor_view.get_iter_location(iter, out location);
        int wx;
        int wy;
        workspace.editor_view.buffer_to_window_coords(
            Gtk.TextWindowType.WIDGET,
            location.x + location.width / 2,
            location.y + location.height / 2,
            out wx,
            out wy
        );
        x = wx;
        y = wy;
    }
}

private void test_the_header_buttons_raise_their_requests() {
    var f = new RecordingWorkspace();

    f.tooltip("Refresh projects and cards").clicked();
    f.tooltip("Create a new project").clicked();
    f.tooltip("Create a new card").clicked();
    f.toggle("Toggle explorer panel").set_active(false);
    f.toggle("Toggle Intelligence panel").set_active(true);
    f.toggle("Toggle Asset Preview").set_active(true);
    f.toggle("Toggle project toolbox panel").set_active(true);
    f.workspace.toggle_toolbox();

    string[] expected = {
        "refresh", "new_project", "new_card", "explorer|false", "ai|true", "asset_preview|true",
        "toolbox|true", "toolbox|false"
    };
    assert(f.events.size == expected.length);
    for (int i = 0; i < expected.length; i++) {
        assert(f.events[i] == expected[i]);
    }
    f.destroy();
}

private void test_the_search_and_editor_toggles_hide_their_parts() {
    var f = new RecordingWorkspace();
    var search_row = (!) f.workspace.search_entry.get_parent();

    assert(search_row.get_visible());
    f.toggle("Toggle search panel").set_active(false);
    assert(!search_row.get_visible());
    f.toggle("Toggle search panel").set_active(true);
    assert(search_row.get_visible());

    f.toggle("Toggle card editor").set_active(false);
    assert(!f.workspace.content_stack.get_visible());
    f.toggle("Toggle card editor").set_active(true);
    assert(f.workspace.content_stack.get_visible());
    f.destroy();
}

private void test_an_app_message_offers_the_debug_panel() {
    var f = new RecordingWorkspace();
    var debug_button = av_button_with_label(f.workspace.widget, "Open Debug Panel");
    var revealer = (Gtk.Revealer) (!) debug_button.get_ancestor(typeof(Gtk.Revealer));

    f.workspace.set_app_message("Cannot reach the daemon");
    assert(av_find_label(f.workspace.widget, "Cannot reach the daemon") != null);
    assert(debug_button.get_visible());
    assert(revealer.get_reveal_child());
    debug_button.clicked();
    assert(f.events.contains("open_debug_panel"));

    f.workspace.set_app_message("  ");
    assert(!debug_button.get_visible());
    assert(!revealer.get_reveal_child());
    f.workspace.set_app_message(null);
    assert(!revealer.get_reveal_child());
    f.destroy();
}

private void test_the_search_entry_raises_its_requests() {
    var f = new RecordingWorkspace();
    var entry = f.workspace.search_entry;

    entry.activate();
    entry.search_changed();
    assert(av_press(entry, Gdk.Key.Down));
    assert(!av_press(entry, Gdk.Key.Up));
    f.tooltip("Clear search").clicked();

    assert(f.events.size == 4);
    assert(f.events[0] == "search_activated");
    assert(f.events[1] == "search_changed");
    assert(f.events[2] == "search_focus_results");
    assert(f.events[3] == "search_cleared");
    f.destroy();
}

private void test_find_and_replace_shows_hides_and_asks() {
    var f = new RecordingWorkspace();
    var bar = f.find_bar();
    var replace_row = (!) f.replace_entry().get_parent();

    f.workspace.show_find_replace_bar(false);
    assert(bar.get_reveal_child());
    assert(!replace_row.get_visible());
    f.workspace.toggle_find_replace_bar(true);
    assert(!bar.get_reveal_child());
    f.tooltip("Find and replace").clicked();
    assert(bar.get_reveal_child());
    assert(replace_row.get_visible());

    f.find_entry().set_text("boiler");
    f.replace_entry().set_text("heater");
    assert(f.workspace.get_find_text() == "boiler");
    assert(f.workspace.get_replace_text() == "heater");

    f.find_entry().activate();
    av_button_with_label(f.workspace.widget, "Find Next").clicked();
    f.replace_entry().activate();
    av_button_with_label(f.workspace.widget, "Replace").clicked();
    av_button_with_label(f.workspace.widget, "Replace All").clicked();
    assert(f.events.size == 5);
    assert(f.events[0] == "find_next" && f.events[1] == "find_next");
    assert(f.events[2] == "replace" && f.events[3] == "replace" && f.events[4] == "replace_all");

    // Escape in either entry closes the bar; other keys are the entry's.
    assert(!av_press(f.find_entry(), Gdk.Key.a));
    assert(av_press(f.find_entry(), Gdk.Key.Escape));
    assert(!bar.get_reveal_child());
    f.workspace.show_find_replace_bar(true);
    assert(!av_press(f.replace_entry(), Gdk.Key.a));
    assert(av_press(f.replace_entry(), Gdk.Key.Escape));
    assert(!bar.get_reveal_child());
    f.workspace.show_find_replace_bar(true);
    f.tooltip("Close find and replace").clicked();
    assert(!bar.get_reveal_child());
    f.destroy();
}

private void test_dropped_images_are_inserted_at_their_mark() {
    var f = new RecordingWorkspace();
    var buffer = f.workspace.editor_buffer;
    buffer.set_text("Before after", -1);
    Gtk.TextIter at;
    buffer.get_iter_at_offset(out at, 7);
    var mark = buffer.create_mark(null, at, false);

    f.workspace.insert_resource_image_markdown(mark, "boiler_room.png", "r-boiler");

    assert(f.editor_text() == "Before \n![boiler room](holder://resource/r-boiler)\nafter");
    assert(mark.get_deleted());
    // A mark already gone, because its text was deleted, inserts nothing.
    f.workspace.insert_resource_image_markdown(mark, "other.png", "r-other");
    assert(!f.editor_text().contains("r-other"));

    buffer.get_iter_at_offset(out at, 0);
    var unused = buffer.create_mark(null, at, false);
    f.workspace.discard_file_drop_mark(unused);
    assert(unused.get_deleted());
    f.workspace.discard_file_drop_mark(unused);
    f.destroy();
}

private void test_saved_tags_are_highlighted_until_cleared() {
    var f = new RecordingWorkspace();
    var buffer = f.workspace.editor_buffer;
    buffer.set_text("Fix the #boiler soon", -1);
    var tag = (!) buffer.get_tag_table().lookup("holder-validated-tag");

    f.workspace.set_validated_tag_occurrences({ new HolderLinux.CardTagOccurrence("boiler", 8, 15) });
    Gtk.TextIter inside;
    buffer.get_iter_at_offset(out inside, 10);
    Gtk.TextIter outside;
    buffer.get_iter_at_offset(out outside, 2);
    assert(inside.has_tag(tag));
    assert(!outside.has_tag(tag));

    f.workspace.clear_validated_tag_highlights();
    buffer.get_iter_at_offset(out inside, 10);
    assert(!inside.has_tag(tag));
    f.destroy();
}

private void test_the_tag_style_follows_the_colour_scheme() {
    var f = new RecordingWorkspace();
    var buffer = f.workspace.editor_buffer;
    var tag = (!) buffer.get_tag_table().lookup("holder-validated-tag");
    var dir = av_temp_dir();
    av_write_scheme(dir, "holder-test-plain", "");
    av_write_scheme(dir, "holder-test-links", "<style name=\"def:link-text\" foreground=\"#123456\"/>");
    var manager = GtkSource.StyleSchemeManager.get_default();
    manager.prepend_search_path(dir);
    manager.force_rescan();

    // Without a scheme, or with one that colours neither preprocessor nor link text, tags are only
    // bold.
    buffer.set_style_scheme(null);
    assert(!tag.foreground_set);
    buffer.set_style_scheme(manager.get_scheme("holder-test-plain"));
    assert(!tag.foreground_set);
    // A scheme without a preprocessor colour lends its link colour.
    buffer.set_style_scheme(manager.get_scheme("holder-test-links"));
    assert(tag.foreground_set);
    assert(tag.foreground_rgba.to_string() == "rgb(18,52,86)");
    assert(tag.weight == Pango.Weight.SEMIBOLD);
    f.destroy();
}

private void test_tidying_and_spell_checking_reach_the_editor() {
    var f = new RecordingWorkspace();
    f.workspace.editor_buffer.set_text("Line  \n", -1);

    assert(f.workspace.tidy_editor_text("Line\n"));
    assert(f.editor_text() == "Line\n");
    assert(!f.workspace.tidy_editor_text("Line\n"));

    f.workspace.set_spell_check_enabled(false);
    assert(!f.workspace.editor_spellcheck.requested_enabled);
    f.workspace.set_spell_check_enabled(true);
    assert(f.workspace.editor_spellcheck.requested_enabled);
    f.destroy();
}

private void test_panels_open_at_their_remembered_widths() {
    var f = new RecordingWorkspace();
    // Opened before the window has a size, each panel waits for one, then gives up.
    f.workspace.set_toolbox_visible(true);
    f.workspace.set_ai_panel_visible(true);
    f.workspace.set_asset_preview_visible(true);
    av_settle();
    f.workspace.set_toolbox_visible(false);
    f.workspace.set_ai_panel_visible(false);
    f.workspace.set_asset_preview_visible(false);
    f.present();

    f.workspace.set_ai_panel_width(400);
    f.workspace.set_ai_panel_visible(true);
    assert(f.workspace.is_ai_panel_visible());
    assert(f.workspace.ai_panel.widget.get_visible());
    av_settle();
    var ai_split = f.workspace.ai_split;
    assert(wait_for_condition(() => ai_split.get_width() - ai_split.get_position() == 400));
    assert(f.workspace.get_ai_panel_width_for_persist() == 400);
    // Dragging the divider afterwards is remembered.
    ai_split.set_position(ai_split.get_width() - 450);
    assert(f.workspace.get_ai_panel_width_for_persist() == 450);
    f.workspace.set_ai_panel_visible(false);
    var asset_split = (Gtk.Paned) (!) f.workspace.asset_preview.widget.get_parent();
    // With the Intelligence panel closed, the asset split takes the whole width on the next layout.
    assert(wait_for_condition(() => asset_split.get_width() == ai_split.get_width()));

    f.workspace.set_asset_preview_width(400);
    f.workspace.set_asset_preview_visible(true);
    assert(f.workspace.is_asset_preview_visible());
    assert(f.toggle("Toggle Asset Preview").get_active());
    av_settle();
    assert(wait_for_condition(() => asset_split.get_width() - asset_split.get_position() == 400));
    assert(f.workspace.get_asset_preview_width_for_persist() == 400);
    asset_split.set_position(asset_split.get_width() - 420);
    assert(f.workspace.get_asset_preview_width_for_persist() == 420);
    // Closing the preview from inside it turns its header toggle off.
    f.workspace.asset_preview.close_requested();
    assert(!f.toggle("Toggle Asset Preview").get_active());

    f.workspace.set_toolbox_visible(true);
    assert(f.workspace.toolbox.widget.get_visible());
    assert(f.workspace.toolbox.widget.get_reveal_child());
    var content_paned = (Gtk.Paned) (!) f.workspace.toolbox.widget.get_parent();
    assert(wait_for_condition(() => content_paned.get_position() > 0));
    f.workspace.set_toolbox_visible(true);
    f.workspace.set_toolbox_visible(false);
    assert(!f.workspace.toolbox.widget.get_visible());

    f.workspace.show_tag("boiler");
    assert(f.toggle("Toggle project toolbox panel").get_active());
    assert(f.workspace.toolbox.is_showing_tool("tags"));
    f.destroy();
}

private void test_a_context_click_moves_the_cursor_unless_text_is_selected() {
    var f = new RecordingWorkspace();
    f.present();
    f.set_text_with_cursor("First line\nSecond line", 0);
    av_settle();
    double x;
    double y;
    f.point_at(14, out x, out y);

    // Only a single press that opens the menu counts.
    f.workspace.move_cursor_for_context_click(2, true, x, y);
    f.workspace.move_cursor_for_context_click(1, false, x, y);
    assert(f.cursor_offset() == 0);
    // Below the text there is nowhere to go.
    f.workspace.move_cursor_for_context_click(1, true, x, f.workspace.editor_view.get_height() - 1);
    assert(f.cursor_offset() == 0);

    f.workspace.move_cursor_for_context_click(1, true, x, y);
    assert(f.cursor_offset() == 14 || f.cursor_offset() == 15);

    f.set_text_and_select("First line\nSecond line", 0, 5);
    f.workspace.move_cursor_for_context_click(1, true, x, y);
    assert(f.selection() == "First");

    // A press without an event behind it, as when the gesture is driven directly, is ignored.
    var click = (Gtk.GestureClick) av_newest_controller(f.workspace.editor_view, typeof(Gtk.GestureClick));
    click.pressed(1, x, y);
    assert(f.selection() == "First");
    f.destroy();
}

private bool ws_drop(RecordingWorkspace f, Gdk.FileList? files, double x, double y) {
    var target = (Gtk.DropTarget) av_newest_controller(f.workspace.editor_view, typeof(Gtk.DropTarget));
    var value = Value(typeof(Gdk.FileList));
    value.set_boxed(files);
    bool accepted = false;
    Signal.emit_by_name(target, "drop", value, x, y, out accepted);
    return accepted;
}

private void test_dropped_files_are_handed_on_with_a_mark_where_they_landed() {
    var f = new RecordingWorkspace();
    f.present();
    f.set_text_with_cursor("First line\nSecond line", 3);
    av_settle();
    double x;
    double y;
    f.point_at(14, out x, out y);
    File[] files = { File.new_for_path("/tmp/one.png"), File.new_for_path("/tmp/two.png") };

    assert(ws_drop(f, new Gdk.FileList.from_array(files), x, y));
    assert(f.events.size == 2);
    assert(f.events[0] == "file_dropped|one.png" && f.events[1] == "file_dropped|two.png");
    Gtk.TextIter at;
    f.workspace.editor_buffer.get_iter_at_mark(out at, f.dropped_marks[0]);
    assert(at.get_line() == 1);

    // Dropped below the text, the files go where the cursor is.
    assert(ws_drop(f, new Gdk.FileList.from_array({ File.new_for_path("/tmp/three.png") }),
                   x, f.workspace.editor_view.get_height() - 1));
    f.workspace.editor_buffer.get_iter_at_mark(out at, f.dropped_marks[2]);
    assert(at.get_offset() == 3);

    assert(!ws_drop(f, null, x, y));
    assert(!ws_drop(f, new Gdk.FileList.from_array({}), x, y));
    f.destroy();
}

private void test_enter_continues_and_ends_lists() {
    var f = new RecordingWorkspace();
    f.workspace.editor_view.set_editable(true);

    f.set_text_with_cursor("- milk", 6);
    assert(f.key(Gdk.Key.Return));
    assert(f.editor_text() == "- milk\n- ");

    // Enter on an empty item ends the list.
    assert(f.key(Gdk.Key.Return));
    assert(f.editor_text() == "- milk\n\n");

    // Plain text, a selection or a modifier leaves Enter to the editor.
    f.set_text_with_cursor("plain", 5);
    assert(!f.key(Gdk.Key.Return));
    f.set_text_and_select("- milk", 2, 6);
    assert(!f.key(Gdk.Key.Return));
    f.set_text_with_cursor("- milk", 6);
    assert(!f.key(Gdk.Key.Return, Gdk.ModifierType.SHIFT_MASK));

    // A read-only editor leaves every key alone.
    f.workspace.editor_view.set_editable(false);
    assert(!f.key(Gdk.Key.Return));
    assert(!f.key(Gdk.Key.b, WS_CTRL));
    f.destroy();
}

private void test_formatting_shortcuts_wrap_and_unwrap_text() {
    var f = new RecordingWorkspace();
    f.workspace.editor_view.set_editable(true);

    f.set_text_and_select("make word bold", 5, 9);
    assert(f.key(Gdk.Key.b, WS_CTRL));
    assert(f.editor_text() == "make **word** bold");
    assert(f.selection() == "word");

    // The same shortcut on the marked word takes the markers away again.
    assert(f.key(Gdk.Key.b, WS_CTRL));
    assert(f.editor_text() == "make word bold");

    // With no selection the markers go around the cursor.
    f.set_text_with_cursor("ab", 1);
    assert(f.key(Gdk.Key.i, WS_CTRL));
    assert(f.editor_text() == "a**b");
    assert(f.cursor_offset() == 2);

    // A selection across lines is wrapped as it is.
    f.set_text_and_select("**one\ntwo**", 2, 9);
    assert(f.key(Gdk.Key.b, WS_CTRL));
    assert(f.editor_text() == "****one\ntwo****");

    // A selection that is already a link is left as it is.
    f.set_text_and_select("see [[Card 1]] here", 4, 14);
    assert(f.key(Gdk.Key.l, WS_CTRL));
    assert(f.editor_text() == "see [[Card 1]] here");

    // Keys that are not shortcuts are the editor's.
    f.set_text_with_cursor("ab", 1);
    assert(!f.key(Gdk.Key.q, WS_CTRL));
    assert(!f.key(Gdk.Key.b, WS_CTRL | Gdk.ModifierType.ALT_MASK));
    f.destroy();
}

private void test_line_shortcuts_change_whole_lines() {
    var f = new RecordingWorkspace();
    f.workspace.editor_view.set_editable(true);

    f.set_text_with_cursor("Title", 2);
    assert(f.key(Gdk.Key.slash, WS_CTRL));
    assert(f.editor_text() == "# Title");

    f.set_text_and_select("- one\n- two", 0, 11);
    assert(f.key(Gdk.Key.bracketright, WS_CTRL));
    assert(f.editor_text() == "    - one\n    - two");
    // The selection still covers both lines.
    assert(f.selection() == "    - one\n    - two");

    // Outdenting leaves a line that has nothing to take away as it is.
    f.set_text_and_select("    - one\n- two", 0, 15);
    assert(f.key(Gdk.Key.bracketleft, WS_CTRL));
    assert(f.editor_text() == "- one\n- two");
    f.destroy();
}

private void test_clearing_formatting_handles_selections_and_lines() {
    var f = new RecordingWorkspace();
    f.workspace.editor_view.set_editable(true);

    f.set_text_and_select("a **b** c", 2, 7);
    assert(f.key(Gdk.Key.backslash, WS_CTRL));
    assert(f.editor_text() == "a b c");

    // Plain selected text has nothing to clear.
    f.set_text_and_select("plain", 0, 5);
    assert(f.key(Gdk.Key.backslash, WS_CTRL));
    assert(f.editor_text() == "plain");

    // Without a selection, the line's structure goes.
    f.set_text_with_cursor("# Title", 3);
    assert(f.key(Gdk.Key.backslash, WS_CTRL));
    assert(f.editor_text() == "Title");
    f.destroy();
}

private void test_search_results_list_their_titles_and_snippets() {
    var f = new RecordingWorkspace();
    f.search_store.append(new HolderLinux.SearchCardResult("c1", "Boiler service", 2, 1, "booked for Tuesday", 1.0));
    f.workspace.show_search_mode();
    f.present();

    assert(wait_for_condition(() => av_find_label(f.workspace.widget, "Boiler service") != null));
    assert(av_find_label(f.workspace.widget, "booked for Tuesday") != null);
    f.workspace.search_list.activate(0);
    assert(f.events.contains("search_result|0"));
    f.destroy();
}

internal void register_workspace_pane_tests() {
    Test.add_func("/app_views/workspace/header_buttons", test_the_header_buttons_raise_their_requests);
    Test.add_func("/app_views/workspace/search_and_editor_toggles", test_the_search_and_editor_toggles_hide_their_parts);
    Test.add_func("/app_views/workspace/app_message", test_an_app_message_offers_the_debug_panel);
    Test.add_func("/app_views/workspace/search_entry", test_the_search_entry_raises_its_requests);
    Test.add_func("/app_views/workspace/find_replace", test_find_and_replace_shows_hides_and_asks);
    Test.add_func("/app_views/workspace/dropped_images", test_dropped_images_are_inserted_at_their_mark);
    Test.add_func("/app_views/workspace/tag_highlights", test_saved_tags_are_highlighted_until_cleared);
    Test.add_func("/app_views/workspace/tag_style", test_the_tag_style_follows_the_colour_scheme);
    Test.add_func("/app_views/workspace/tidy_and_spelling", test_tidying_and_spell_checking_reach_the_editor);
    Test.add_func("/app_views/workspace/panels", test_panels_open_at_their_remembered_widths);
    Test.add_func("/app_views/workspace/context_click", test_a_context_click_moves_the_cursor_unless_text_is_selected);
    Test.add_func("/app_views/workspace/file_drop", test_dropped_files_are_handed_on_with_a_mark_where_they_landed);
    Test.add_func("/app_views/workspace/list_enter", test_enter_continues_and_ends_lists);
    Test.add_func("/app_views/workspace/inline_shortcuts", test_formatting_shortcuts_wrap_and_unwrap_text);
    Test.add_func("/app_views/workspace/line_shortcuts", test_line_shortcuts_change_whole_lines);
    Test.add_func("/app_views/workspace/clear_formatting", test_clearing_formatting_handles_selections_and_lines);
    Test.add_func("/app_views/workspace/search_results", test_search_results_list_their_titles_and_snippets);
}

}
