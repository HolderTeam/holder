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
    private Gtk.Button review_copy_button;
    private Gtk.Button? trash_copy_button;
    private Gtk.Button? keep_copy_button;
    private Gtk.Label? cleanup_feedback;
    private ConflictComparison? comparison;
    private bool busy;
    private bool saved;

    public signal void resolution_requested(string review_token, string title, string content);
    public signal void refresh_requested();
    public signal void copy_trash_requested(string cleanup_token);

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
        versions.append(version_view("Original", comparison.original, comparison.original_changed_lines));
        versions.append(version_view("Preserved copy", comparison.copy_card, comparison.copy_changed_lines));
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

    public void show_resolution_saved(ConflictCopyCleanupReview cleanup, GLib.ListModel cards) {
        saved = true;
        choices.set_visible(false);
        save_button.set_visible(false);
        refresh_button.set_visible(false);
        states.add_named(build_cleanup_view(cleanup, cards), "cleanup");
        review_copy_button.set_visible(true);
        set_feedback("Saved to the original card. The preserved copy is still available.");
        set_busy(false);
    }

    public void show_copy_trashed() {
        states.set_visible_child_name("versions");
        review_copy_button.set_visible(false);
        set_feedback("The preserved copy was moved to Trash. You can restore it from there.");
        set_busy(false);
    }

    public void show_copy_trash_error(string message) {
        if (cleanup_feedback != null) {
            cleanup_feedback.set_text("Could not move the copy to Trash.\n" + message);
            cleanup_feedback.add_css_class("error");
            cleanup_feedback.set_visible(true);
        }
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

        var note = new Gtk.Label("The original keeps its place in the project, its links, and its milestones.") {
            wrap = true, xalign = 0.0f, margin_start = 12, margin_end = 12
        };
        note.add_css_class("dim-label");
        layout.append(note);

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
        review_copy_button = new Gtk.Button.with_label("Review preserved copy…") { visible = false };
        review_copy_button.clicked.connect(() => states.set_visible_child_name("cleanup"));
        actions.append(review_copy_button);
        layout.append(actions);
        return layout;
    }

    private Gtk.Widget build_cleanup_view(ConflictCopyCleanupReview cleanup, GLib.ListModel cards) {
        var layout = new Gtk.Box(Gtk.Orientation.VERTICAL, 0);
        var scroll = new Gtk.ScrolledWindow() { vexpand = true, hscrollbar_policy = Gtk.PolicyType.NEVER };
        var page = new Gtk.Box(Gtk.Orientation.VERTICAL, 16) {
            margin_start = 24, margin_end = 24, margin_top = 18, margin_bottom = 16
        };
        scroll.set_child(page);
        layout.append(scroll);

        var heading = cleanup_label("Review preserved copy");
        heading.add_css_class("title-2");
        page.append(heading);
        page.append(cleanup_label("The result is saved. Moving the copy to Trash leaves these details with the copy. " +
                                  "You can restore the copy from Trash."));

        if (cleanup.placement_differs) {
            page.append(cleanup_label("The copy occupies a different position in the project."));
        }
        var groups = new Gtk.FlowBox() {
            selection_mode = Gtk.SelectionMode.NONE, homogeneous = true, valign = Gtk.Align.START,
            min_children_per_line = 1, max_children_per_line = 2, column_spacing = 16, row_spacing = 12
        };
        page.append(groups);
        string[] tags = {};
        foreach (var tag in cleanup.copy_tags) tags += "#" + tag;
        append_cleanup_group(groups, "Tags only on the copy", tags);
        append_cleanup_group(groups, "Links and attachments only on the copy", describe_links(cleanup.copy_links, cards, false));
        append_cleanup_group(groups, "Milestones only on the copy", cleanup.copy_milestones);
        append_cleanup_group(groups, "Links pointing to the copy", describe_links(cleanup.backlinks, cards, true));
        if (cleanup.children.length > 0) {
            var destination = cleanup.promotion_parent == null
                ? "These cards would move to the project’s top level."
                : "These cards would move under “%s”.".printf(cleanup.promotion_parent);
            append_cleanup_group(groups, "Child cards", cleanup.children,
                                 destination + " Restoring the copy keeps them there.");
        }
        if (tags.length == 0 && cleanup.copy_links.length == 0 && cleanup.copy_milestones.length == 0 &&
            cleanup.backlinks.length == 0 && cleanup.children.length == 0) {
            page.append(cleanup_label("There are no copy-only tags, links, milestones, or child cards to review."));
        }
        if (!cleanup.preserves_original) {
            var blocked = cleanup_label("Moving the copy to Trash would also move the original. Move the original first.");
            blocked.add_css_class("warning");
            page.append(blocked);
        }
        cleanup_feedback = cleanup_label("");
        cleanup_feedback.set_margin_start(24);
        cleanup_feedback.set_margin_end(24);
        cleanup_feedback.set_visible(false);
        layout.append(cleanup_feedback);

        var actions = new Gtk.Box(Gtk.Orientation.HORIZONTAL, 6) {
            halign = Gtk.Align.END, margin_start = 12, margin_end = 12,
            margin_top = 8, margin_bottom = 12
        };
        keep_copy_button = new Gtk.Button.with_label("Keep copy");
        keep_copy_button.clicked.connect(() => states.set_visible_child_name("versions"));
        actions.append(keep_copy_button);
        if (cleanup.preserves_original) {
            trash_copy_button = new Gtk.Button.with_label("Move copy to Trash");
            trash_copy_button.add_css_class("destructive-action");
            trash_copy_button.clicked.connect(() => {
                set_busy(true);
                cleanup_feedback.set_text("Moving the copy to Trash…");
                cleanup_feedback.remove_css_class("error");
                cleanup_feedback.set_visible(true);
                copy_trash_requested(cleanup.cleanup_token);
            });
            actions.append(trash_copy_button);
        }
        layout.append(actions);
        return layout;
    }

    private static Gtk.Label cleanup_label(string message) {
        return new Gtk.Label(message) { wrap = true, wrap_mode = Pango.WrapMode.WORD_CHAR, xalign = 0.0f };
    }

    private static void append_cleanup_group(Gtk.FlowBox groups, string title, string[] items,
                                             string? description = null) {
        if (items.length == 0) return;
        var group = new Gtk.Box(Gtk.Orientation.VERTICAL, 8) { valign = Gtk.Align.START };
        var heading = cleanup_label(title);
        heading.set_max_width_chars(34);
        heading.add_css_class("heading");
        group.append(heading);
        if (description != null) {
            var note = cleanup_label(description);
            note.set_max_width_chars(34);
            note.add_css_class("dim-label");
            group.append(note);
        }
        var list = new Gtk.ListBox() { selection_mode = Gtk.SelectionMode.NONE };
        list.add_css_class("boxed-list");
        foreach (var item in items) {
            var label = cleanup_label(item);
            label.set_max_width_chars(34);
            label.set_margin_start(12);
            label.set_margin_end(12);
            label.set_margin_top(10);
            label.set_margin_bottom(10);
            list.append(new Gtk.ListBoxRow() { child = label, activatable = false });
        }
        group.append(list);
        groups.append(new Gtk.FlowBoxChild() { child = group, focusable = false });
    }

    private string[] describe_links(CardLink[] links, GLib.ListModel cards, bool incoming) {
        var titles = new Gee.HashMap<string, string>();
        for (uint i = 0; i < cards.get_n_items(); i++) {
            var card = (CardSummary) cards.get_item(i);
            titles[card.card_id] = card.title;
        }
        var project_id = comparison?.original.project_id;
        string[] rows = {};
        foreach (var link in links) {
            var id = incoming ? link.from_card_id : link.to_card_id;
            var link_project = incoming ? link.source_project_id : link.target_project_id;
            var target = (link_project == project_id ? titles[id] : null) ?? id;
            var label = link.label ?? (link.kind == "conflict_copy_of" ? "Preserved version" :
                                       link.to_type == "resource" ? "Attachment" : link.kind.replace("_", " "));
            if (link_project != project_id) target += " · " + link_project;
            rows += "%s · %s".printf(label, target);
        }
        return rows;
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
        review_copy_button.set_sensitive(!busy);
        if (trash_copy_button != null) trash_copy_button.set_sensitive(!busy);
        if (keep_copy_button != null) keep_copy_button.set_sensitive(!busy);
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
            if (response == "discard") {
                Idle.add(() => {
                    force_close();
                    return Source.REMOVE;
                });
            }
        });
        dialog.present(this);
    }

    private static Gtk.Widget version_view(string role, CardDetail card, int[] changed_lines) {
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
        var changed = buffer.create_tag("changed-line", "underline", Pango.Underline.SINGLE);
        follow_accent(column, content, changed);
        // Core's line numbers, not the buffer's: they differ after a lone "\r" or U+2029.
        var ranges = ConflictLineRanges.of(card.content, changed_lines);
        for (var i = 0; i + 1 < ranges.length; i += 2) {
            Gtk.TextIter start;
            Gtk.TextIter end;
            buffer.get_iter_at_offset(out start, ranges[i]);
            buffer.get_iter_at_offset(out end, ranges[i + 1]);
            buffer.apply_tag(changed, start, end);
        }
        var scroll = new Gtk.ScrolledWindow() { vexpand = true, has_frame = true };
        scroll.set_child(content);
        column.append(scroll);
        return column;
    }

    // Both versions are equals, so both mark their changed lines the same way: the theme's accent
    // colour, faint enough to read through, plus the underline so colour is not the only cue.
    // An invisible label styled with libadwaita's "accent" class reports that colour for the
    // current light or dark theme; the tag follows theme changes while the view is shown.
    private static void follow_accent(Gtk.Box column, Gtk.TextView content, Gtk.TextTag changed) {
        var accent = new Gtk.Label("") { visible = false };
        accent.add_css_class("accent");
        column.append(accent);
        var style = Adw.StyleManager.get_default();
        ulong watch = 0;
        content.map.connect(() => {
            highlight_with(changed, accent.get_color(), style.high_contrast);
            if (watch == 0) {
                watch = style.notify.connect(() => {
                    highlight_with(changed, accent.get_color(), style.high_contrast);
                });
            }
        });
        content.unmap.connect(() => {
            if (watch != 0) style.disconnect(watch);
            watch = 0;
        });
    }

    internal static void highlight_with(Gtk.TextTag changed, Gdk.RGBA accent, bool high_contrast) {
        accent.alpha = high_contrast ? 0.35f : 0.18f;
        changed.paragraph_background_rgba = accent;
    }

}

}
