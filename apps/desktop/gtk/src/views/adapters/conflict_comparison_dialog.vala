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

    public void show_versions(CardDetail original, CardDetail copy) {
        var versions = new Gtk.Box(Gtk.Orientation.HORIZONTAL, 12) {
            homogeneous = true,
            margin_start = 12,
            margin_end = 12,
            margin_top = 12,
            margin_bottom = 12
        };
        versions.append(version_view("Original", original));
        versions.append(version_view("Preserved copy", copy));
        states.add_named(versions, "versions");
        states.set_visible_child_name("versions");
    }

    public void show_load_error(string message) {
        status.set_text("Could not load both card versions.\n" + message);
    }

    private static Gtk.Widget version_view(string role, CardDetail card) {
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
        content.get_buffer().set_text(card.content);
        var scroll = new Gtk.ScrolledWindow() { vexpand = true, has_frame = true };
        scroll.set_child(content);
        column.append(scroll);
        return column;
    }
}

}
