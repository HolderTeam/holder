namespace HolderLinux {

internal class ConflictComparisonDialog : Adw.Dialog {
    private Gtk.Stack states;
    private Gtk.Label status;
    private Gtk.Paned panes;
    private Gtk.Box choices;
    private Gtk.Box result_editor;
    private Gtk.Entry result_title;
    private Gtk.TextView result_content;
    private Gtk.Label feedback;
    private Gtk.Button save_button;
    private Gtk.Button refresh_button;
    private ConflictComparison? comparison;
    private bool busy;
    private bool saved;

    public signal void resolution_requested(string review_token, string title, string content);
    public signal void refresh_requested();

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
        states.add_named(build_comparison_view(), "versions");
        toolbar.set_content(states);
        set_child(toolbar);
        close_attempt.connect(confirm_discard);
    }

    public void show_versions(ConflictComparison comparison) {
        bool refreshed = this.comparison != null;
        this.comparison = comparison;
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
        panes.set_start_child(detail);
        states.set_visible_child_name("versions");
        refresh_button.set_visible(false);
        set_feedback(refreshed ? "Versions refreshed. Review both versions before saving." : "");
        set_busy(false);
    }

    public void show_load_error(string message) {
        if (comparison == null) {
            status.set_text("Could not load both card versions.\n" + message);
        } else {
            show_resolution_error("Could not refresh both card versions.\n" + message);
        }
    }

    public void show_resolution_error(string message, bool allow_refresh = true) {
        set_feedback(message, true);
        refresh_button.set_visible(allow_refresh);
        set_busy(false);
    }

    public void show_resolution_saved() {
        saved = true;
        choices.set_visible(false);
        save_button.set_visible(false);
        refresh_button.set_visible(false);
        set_feedback("Saved to the original card. The preserved copy is still available.");
        set_busy(false);
    }

    private Gtk.Widget build_comparison_view() {
        var layout = new Gtk.Box(Gtk.Orientation.VERTICAL, 6);
        panes = new Gtk.Paned(Gtk.Orientation.VERTICAL) { vexpand = true, wide_handle = true };
        panes.set_shrink_start_child(false);
        panes.set_shrink_end_child(false);
        layout.append(panes);

        result_editor = new Gtk.Box(Gtk.Orientation.VERTICAL, 6) {
            visible = false, margin_start = 12, margin_end = 12, margin_top = 8
        };
        var heading = new Gtk.Label("Result for the original card") { xalign = 0.0f };
        heading.add_css_class("heading");
        result_editor.append(heading);
        result_title = new Gtk.Entry() { placeholder_text = "Card title" };
        result_title.update_property(Gtk.AccessibleProperty.LABEL, "Result title", -1);
        result_editor.append(result_title);
        result_content = new Gtk.TextView() {
            monospace = true, wrap_mode = Gtk.WrapMode.WORD_CHAR,
            left_margin = 8, right_margin = 8, top_margin = 8, bottom_margin = 8
        };
        result_content.update_property(Gtk.AccessibleProperty.LABEL, "Result content", -1);
        var scroll = new Gtk.ScrolledWindow() { vexpand = true, has_frame = true, min_content_height = 100 };
        scroll.set_child(result_content);
        result_editor.append(scroll);
        panes.set_end_child(result_editor);

        feedback = new Gtk.Label("") { visible = false, wrap = true, xalign = 0.0f,
                                      margin_start = 12, margin_end = 12 };
        layout.append(feedback);
        var actions = new Gtk.Box(Gtk.Orientation.HORIZONTAL, 6) {
            halign = Gtk.Align.END, margin_start = 12, margin_end = 12, margin_bottom = 12
        };
        choices = new Gtk.Box(Gtk.Orientation.HORIZONTAL, 6);
        var keep = new Gtk.Button.with_label("Keep original");
        keep.clicked.connect(() => {
            if (comparison != null) {
                submit(comparison.original.title, comparison.original.content);
            }
        });
        choices.append(keep);
        var use_copy = new Gtk.Button.with_label("Start from copy");
        use_copy.clicked.connect(() => {
            if (comparison != null) start_editing(comparison.copy_card);
        });
        choices.append(use_copy);
        var combine = new Gtk.Button.with_label("Combine versions");
        combine.clicked.connect(() => {
            if (comparison != null) start_editing(comparison.original);
        });
        choices.append(combine);
        actions.append(choices);

        refresh_button = new Gtk.Button.with_label("Refresh versions") { visible = false };
        refresh_button.clicked.connect(() => {
            set_feedback("Refreshing both versions…");
            set_busy(true);
            refresh_requested();
        });
        actions.append(refresh_button);
        save_button = new Gtk.Button.with_label("Save to original") { visible = false };
        save_button.add_css_class("suggested-action");
        save_button.clicked.connect(() => submit(result_title.text, result_content.buffer.text));
        result_title.changed.connect(() => save_button.set_sensitive(!busy && result_title.text.strip() != ""));
        actions.append(save_button);
        layout.append(actions);
        return layout;
    }

    private void start_editing(CardDetail source) {
        result_title.text = source.title;
        result_content.buffer.text = source.content;
        result_editor.set_visible(true);
        panes.set_position((int) (panes.get_height() * 0.56));
        choices.set_visible(false);
        save_button.set_visible(true);
        set_can_close(false);
        result_content.grab_focus();
    }

    private void submit(string title, string content) {
        if (comparison == null || busy || saved) return;
        set_busy(true);
        set_feedback("Saving to the original card…");
        resolution_requested(comparison.review_token, title, content);
    }

    private void set_busy(bool value) {
        busy = value;
        set_can_close(!busy && (!result_editor.visible || saved));
        choices.set_sensitive(!busy);
        refresh_button.set_sensitive(!busy);
        save_button.set_sensitive(!busy && result_title.text.strip() != "");
        result_title.set_editable(!busy && !saved);
        result_content.set_editable(!busy && !saved);
    }

    private void set_feedback(string message, bool error = false) {
        feedback.set_text(message);
        feedback.set_visible(message != "");
        if (error) {
            feedback.add_css_class("error");
        } else {
            feedback.remove_css_class("error");
        }
    }

    private void confirm_discard() {
        if (busy) return;
        var dialog = new Adw.AlertDialog("Discard the result draft?", "Both reviewed cards will stay as they are.");
        dialog.add_response("keep", "Keep Editing");
        dialog.add_response("discard", "Discard Draft");
        dialog.set_close_response("keep");
        dialog.set_default_response("keep");
        dialog.set_response_appearance("discard", Adw.ResponseAppearance.DESTRUCTIVE);
        dialog.response.connect((response) => {
            if (response == "discard") force_close();
        });
        dialog.present(this);
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
