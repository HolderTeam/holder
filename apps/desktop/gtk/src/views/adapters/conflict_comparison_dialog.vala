namespace HolderLinux {

internal class ConflictComparisonDialog : Adw.Dialog {
    private Gtk.Stack states;
    private Gtk.Label status;

    public ConflictComparisonDialog() {
        Object(title: "Compare card versions", content_width: 900, content_height: 600);

        var toolbar = new Adw.ToolbarView();
        var header = new Adw.HeaderBar();
        toolbar.add_top_bar(header);
        states = new Gtk.Stack();
        status = new Gtk.Label("Loading versions…") {
            wrap = true,
            margin_start = 18,
            margin_end = 18
        };
        states.add_named(status, "status");
        toolbar.set_content(states);
        set_child(toolbar);
    }

    public void show_versions(ConflictComparison comparison) {
        var detail = new Gtk.Box(Gtk.Orientation.VERTICAL, 6);
        var versions = new Gtk.Box(Gtk.Orientation.HORIZONTAL, 12) {
            homogeneous = true,
            margin_start = 12,
            margin_end = 12,
            margin_top = 12,
            margin_bottom = 12
        };
        versions.append(version_view("Original", comparison.original,
                                     comparison.original_changed_lines, "rgba(224,66,55,0.16)"));
        versions.append(version_view("Preserved copy", comparison.copy_card,
                                     comparison.copy_changed_lines, "rgba(46,160,93,0.20)"));
        if (comparison.truncated) {
            var note = new Gtk.Label("Some changes are too large to highlight. Both versions are shown in full.") {
                wrap = true,
                xalign = 0.0f,
                margin_start = 12,
                margin_end = 12
            };
            detail.append(note);
        }
        detail.append(versions);
        states.add_named(detail, "versions");
        states.set_visible_child_name("versions");
    }

    public void show_load_error(string message) {
        status.set_text("Could not load both card versions.\n" + message);
    }

    private static Gtk.Widget version_view(string role, CardDetail card, int[] changed_lines,
                                           string highlight_color) {
        var column = new Gtk.Box(Gtk.Orientation.VERTICAL, 6);
        var label = new Gtk.Label(role) { xalign = 0.0f };
        label.add_css_class("heading");
        column.append(label);
        var title = new Gtk.Label(card.title) { xalign = 0.0f, ellipsize = Pango.EllipsizeMode.END };
        title.set_tooltip_text(card.title);
        column.append(title);

        var content = new Gtk.TextView() {
            editable = false,
            cursor_visible = false,
            monospace = true,
            wrap_mode = Gtk.WrapMode.WORD_CHAR,
            left_margin = 8,
            right_margin = 8,
            top_margin = 8,
            bottom_margin = 8
        };
        content.update_property(Gtk.AccessibleProperty.LABEL, role + " card content", -1);
        var buffer = content.get_buffer();
        buffer.set_text(card.content);
        var color = Gdk.RGBA();
        color.parse(highlight_color);
        var changed = buffer.create_tag("changed-line", "paragraph-background-rgba", color,
                                        "underline", Pango.Underline.SINGLE);
        foreach (var line in changed_lines) {
            if (line < 1 || line > buffer.get_line_count()) continue;
            Gtk.TextIter start;
            buffer.get_iter_at_line(out start, line - 1);
            var end = start;
            end.forward_line();
            buffer.apply_tag(changed, start, end);
        }
        var scroll = new Gtk.ScrolledWindow() { vexpand = true, has_frame = true };
        scroll.set_child(content);
        column.append(scroll);
        return column;
    }
}

}
