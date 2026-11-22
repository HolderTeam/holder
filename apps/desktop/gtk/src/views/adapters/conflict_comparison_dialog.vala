namespace HolderLinux {

// Shows the two versions Holder kept when a pull could not combine edits to one card, and lets
// the user keep one, combine them, keep both as separate cards, or decide later. Whatever is kept
// lands on the original card, so the card keeps its identity, place, links and milestones. The
// dialog never says "original" or "copy": each version is described by where and when it was last
// edited.
internal class ConflictComparisonDialog : Adw.Dialog {
    public const string HEADLINE = "Holder couldn't combine the changes to this card automatically.";
    public const string REASSURANCE = "Both versions have been saved, so nothing is lost.";
    private const string COPY_SUFFIX = " (conflicted copy)";

    private bool use_markers;
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
    private Gtk.Button? keep_original_button;
    private Gtk.Button? keep_copy_button;
    private Gtk.Button? finish_button;
    private Gtk.Label? cleanup_feedback;
    private ConflictComparison? comparison;
    private bool busy;
    private bool saved;

    public signal void resolution_requested(string review_token, string title, string content);
    public signal void refresh_requested();
    // carry_over is null when the copy has nothing of its own to review.
    public signal void copy_trash_requested(string cleanup_token, ConflictCarryOver? carry_over);
    public signal void separation_requested(string review_token, string title);

    public ConflictComparisonDialog(bool use_markers = false) {
        Object(title: "Compare card versions", content_width: 940, content_height: 640);
        this.use_markers = use_markers;

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
        var detail = new Gtk.Box(Gtk.Orientation.VERTICAL, 6) { margin_top = 12 };
        var headline = new Gtk.Label(HEADLINE) { wrap = true, xalign = 0.0f, margin_start = 12, margin_end = 12 };
        headline.add_css_class("title-4");
        detail.append(headline);
        var reassurance = new Gtk.Label(REASSURANCE) { wrap = true, xalign = 0.0f, margin_start = 12, margin_end = 12 };
        reassurance.add_css_class("dim-label");
        detail.append(reassurance);
        if (comparison.truncated) {
            var note = new Gtk.Label("Some changes are too large to highlight. Both versions are shown in full.") {
                wrap = true,
                xalign = 0.0f,
                margin_start = 12,
                margin_end = 12
            };
            detail.append(note);
        }
        var versions = new Gtk.Box(Gtk.Orientation.HORIZONTAL, 12) {
            homogeneous = true,
            margin_start = 12,
            margin_end = 12,
            margin_top = 6,
            margin_bottom = 12
        };
        keep_original_button = new Gtk.Button.with_label("Keep this version") { halign = Gtk.Align.END };
        keep_original_button.clicked.connect(() => {
            if (this.comparison != null) {
                submit(this.comparison.original.title, this.comparison.original.content);
            }
        });
        keep_copy_button = new Gtk.Button.with_label("Keep this version") { halign = Gtk.Align.END };
        keep_copy_button.clicked.connect(() => {
            if (this.comparison != null) {
                submit(title_without_suffix(this.comparison.copy_card.title), this.comparison.copy_card.content);
            }
        });
        versions.append(version_view(comparison.original, comparison.original_edited_on,
                                     comparison.original_changed_lines, keep_original_button));
        versions.append(version_view(comparison.copy_card, comparison.copy_edited_on,
                                     comparison.copy_changed_lines, keep_copy_button));
        detail.append(versions);
        panes.set_start_child(detail);
        set_keep_buttons_visible(!result_editor.visible);
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

    // The chosen version is on the original card. When the copy has nothing the original lacks,
    // it goes straight to Trash, where the window offers Undo. Otherwise the user picks which of
    // its details to keep first.
    public void show_resolution_saved(ConflictCopyCleanupReview cleanup, GLib.ListModel cards) {
        saved = true;
        choices.set_visible(false);
        save_button.set_visible(false);
        refresh_button.set_visible(false);
        if (cleanup.preserves_original && !has_details(cleanup)) {
            set_feedback("Saved. Moving the other version to Trash…");
            set_busy(true);
            copy_trash_requested(cleanup.cleanup_token, null);
            return;
        }
        states.add_named(build_cleanup_view(cleanup, cards), "cleanup");
        states.set_visible_child_name("cleanup");
        set_busy(false);
    }

    public void show_copy_trash_error(string message) {
        if (cleanup_feedback != null) {
            cleanup_feedback.set_text("Could not move the other version to Trash.\n" + message);
            cleanup_feedback.add_css_class("error");
            cleanup_feedback.set_visible(true);
        } else {
            set_feedback("Saved, but could not move the other version to Trash.\n" + message, true);
        }
        set_busy(false);
    }

    public void show_separation_error(string message) {
        show_resolution_error("Could not keep both cards.\n" + message);
    }

    // "Edited on this device · 10:42". Without the device, the time alone.
    internal static string edited_label(ConflictEditedOn edited_on, int64 updated_at, int64 now) {
        var when = edited_time(updated_at, now);
        switch (edited_on) {
        case ConflictEditedOn.THIS_DEVICE:
            return when == "" ? "Edited on this device" : "Edited on this device · " + when;
        case ConflictEditedOn.ANOTHER_DEVICE:
            return when == "" ? "Edited on another device" : "Edited on another device · " + when;
        default:
            return when == "" ? "" : "Edited " + when;
        }
    }

    // A time today, otherwise the day and time, in local time.
    internal static string edited_time(int64 updated_at, int64 now) {
        if (updated_at <= 0) return "";
        var at = new DateTime.from_unix_local(updated_at);
        var today = new DateTime.from_unix_local(now);
        if (at.get_year() == today.get_year() && at.get_day_of_year() == today.get_day_of_year()) {
            return at.format("%H:%M");
        }
        return at.format(at.get_year() == today.get_year() ? "%-d %b %H:%M" : "%-d %b %Y %H:%M");
    }

    // What a Git conflict marker calls a version.
    internal static string marker_label(ConflictEditedOn edited_on, int64 updated_at, int64 now) {
        switch (edited_on) {
        case ConflictEditedOn.THIS_DEVICE:
            return "this device";
        case ConflictEditedOn.ANOTHER_DEVICE:
            return "another device";
        default:
            var when = edited_time(updated_at, now);
            return when == "" ? "other version" : "version edited " + when;
        }
    }

    // A preserved copy is titled "<title> (conflicted copy)"; kept on its own, it is just "<title>".
    internal static string title_without_suffix(string title) {
        return title.has_suffix(COPY_SUFFIX) ? title.substring(0, title.length - COPY_SUFFIX.length) : title;
    }

    private static bool has_details(ConflictCopyCleanupReview cleanup) {
        return cleanup.copy_tags.length > 0 || cleanup.copy_links.length > 0 || cleanup.copy_milestones.length > 0 ||
               cleanup.backlinks.length > 0 || cleanup.children.length > 0;
    }

    private Gtk.Widget build_comparison_view() {
        var layout = new Gtk.Box(Gtk.Orientation.VERTICAL, 6);
        panes = new Gtk.Paned(Gtk.Orientation.VERTICAL) { vexpand = true, wide_handle = true };
        // The versions scroll, so they can give way to the combined editor on a short screen.
        panes.set_shrink_start_child(true);
        panes.set_shrink_end_child(false);
        layout.append(panes);

        result_editor = new Gtk.Box(Gtk.Orientation.VERTICAL, 6) {
            visible = false, margin_start = 12, margin_end = 12, margin_top = 8
        };
        var heading = new Gtk.Label("Combined version") { xalign = 0.0f };
        heading.add_css_class("heading");
        result_editor.append(heading);
        var hint = new Gtk.Label("Both versions are below. Remove what you don't want, then save.") {
            wrap = true, xalign = 0.0f
        };
        hint.add_css_class("dim-label");
        result_editor.append(hint);
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

        var note = new Gtk.Label("Whichever version you keep, the card keeps its place in the project, its links and its milestones.") {
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
        var combine = new Gtk.Button.with_label("Combine changes");
        combine.clicked.connect(start_combining);
        choices.append(combine);
        var keep_both = new Gtk.Button.with_label("Keep both as separate cards");
        keep_both.clicked.connect(confirm_keep_both);
        choices.append(keep_both);
        var later = new Gtk.Button.with_label("Decide later");
        later.clicked.connect(() => close());
        choices.append(later);
        actions.append(choices);

        refresh_button = new Gtk.Button.with_label("Refresh versions") { visible = false };
        refresh_button.clicked.connect(() => {
            set_feedback("Refreshing both versions…");
            set_busy(true);
            refresh_requested();
        });
        actions.append(refresh_button);
        save_button = new Gtk.Button.with_label("Save") { visible = false };
        save_button.add_css_class("suggested-action");
        save_button.clicked.connect(() => submit(result_title.text, result_content.buffer.text));
        result_title.changed.connect(() => save_button.set_sensitive(!busy && result_title.text.strip() != ""));
        actions.append(save_button);
        layout.append(actions);
        return layout;
    }

    private void start_combining() {
        if (comparison == null) return;
        var now = new DateTime.now_local().to_unix();
        result_title.text = comparison.original.title;
        if (comparison.truncated) {
            result_content.buffer.text = comparison.original.content;
            set_feedback("These versions are too large to combine automatically, so the editor starts from the first one.");
        } else {
            result_content.buffer.text = ConflictCombiner.combine(
                comparison.original.content, comparison.original_changed_lines,
                comparison.copy_card.content, comparison.copy_changed_lines, use_markers,
                marker_label(comparison.original_edited_on, comparison.original.updated_at, now),
                marker_label(comparison.copy_edited_on, comparison.copy_card.updated_at, now));
        }
        result_editor.set_visible(true);
        panes.set_position((int) (panes.get_height() * 0.5));
        choices.set_visible(false);
        set_keep_buttons_visible(false);
        save_button.set_visible(true);
        set_can_close(false);
        result_content.grab_focus();
    }

    private void confirm_keep_both() {
        if (comparison == null || busy) return;
        var dialog = new Adw.AlertDialog("Keep both as separate cards?",
                                         "The other version becomes a card of its own, next to this one.");
        var name = new Gtk.Entry() { text = title_without_suffix(comparison.copy_card.title), activates_default = true };
        name.update_property(Gtk.AccessibleProperty.LABEL, "Title for the other card", -1);
        dialog.set_extra_child(name);
        dialog.add_response("cancel", "Cancel");
        dialog.add_response("keep", "Keep Both");
        dialog.set_close_response("cancel");
        dialog.set_default_response("keep");
        dialog.set_response_appearance("keep", Adw.ResponseAppearance.SUGGESTED);
        name.changed.connect(() => dialog.set_response_enabled("keep", name.text.strip() != ""));
        dialog.response.connect((response) => {
            if (response != "keep" || comparison == null) return;
            set_busy(true);
            set_feedback("Keeping both cards…");
            separation_requested(comparison.review_token, name.text.strip());
        });
        dialog.present(this);
    }

    private Gtk.Widget build_cleanup_view(ConflictCopyCleanupReview cleanup, GLib.ListModel cards) {
        var layout = new Gtk.Box(Gtk.Orientation.VERTICAL, 0);
        var scroll = new Gtk.ScrolledWindow() { vexpand = true, hscrollbar_policy = Gtk.PolicyType.NEVER };
        var page = new Gtk.Box(Gtk.Orientation.VERTICAL, 16) {
            margin_start = 24, margin_end = 24, margin_top = 18, margin_bottom = 16
        };
        scroll.set_child(page);
        layout.append(scroll);

        var heading = cleanup_label("Keep these details?");
        heading.add_css_class("title-2");
        page.append(heading);
        page.append(cleanup_label("Saved. The other version has these details that the card you kept doesn't. " +
                                  "Ticked ones are added to the card you kept. The rest go to Trash with the " +
                                  "other version, where you can restore them."));
        if (cleanup.placement_differs) {
            page.append(cleanup_label("The other version was in a different place in the project. " +
                                      "The card you kept stays where it is."));
        }
        var groups = new Gtk.FlowBox() {
            selection_mode = Gtk.SelectionMode.NONE, homogeneous = true, valign = Gtk.Align.START,
            min_children_per_line = 1, max_children_per_line = 2, column_spacing = 16, row_spacing = 12
        };
        page.append(groups);

        var tag_checks = new Gee.ArrayList<Gtk.CheckButton>();
        string[] tags = {};
        foreach (var tag in cleanup.copy_tags) tags += "#" + tag;
        append_cleanup_group(groups, "Tags", tags, tag_checks);
        var link_checks = new Gee.ArrayList<Gtk.CheckButton>();
        append_cleanup_group(groups, "Links and attachments", describe_links(cleanup.copy_links, cards, false),
                             link_checks);
        var milestone_checks = new Gee.ArrayList<Gtk.CheckButton>();
        string[] milestones = {};
        foreach (var milestone in cleanup.copy_milestones) milestones += describe_milestone(milestone);
        append_cleanup_group(groups, "Milestones", milestones, milestone_checks);

        // Links from cards can point at the kept card instead; links from AI messages stay.
        CardLink[] card_backlinks = {};
        CardLink[] other_backlinks = {};
        foreach (var link in cleanup.backlinks) {
            if (link.source_type == "card") {
                card_backlinks += link;
            } else {
                other_backlinks += link;
            }
        }
        var backlink_checks = new Gee.ArrayList<Gtk.CheckButton>();
        append_cleanup_group(groups, "Links pointing to it", describe_links(card_backlinks, cards, true),
                             backlink_checks, "Ticked links will point at the card you kept.");
        append_cleanup_group(groups, "AI messages pointing to it", describe_links(other_backlinks, cards, true), null,
                             "These keep pointing at the other version.");
        var child_checks = new Gee.ArrayList<Gtk.CheckButton>();
        if (cleanup.children.length > 0) {
            var destination = cleanup.promotion_parent == null
                ? "Unticked cards move to the project’s top level."
                : "Unticked cards move under “%s”.".printf(((!) cleanup.promotion_parent).title);
            string[] children = {};
            foreach (var child in cleanup.children) children += child.title;
            append_cleanup_group(groups, "Cards inside it", children, child_checks,
                                 "Ticked cards move inside the card you kept. " + destination);
        }
        if (!cleanup.preserves_original) {
            var blocked = cleanup_label("Moving the other version to Trash would also move the card you kept. " +
                                        "Move the card you kept out of it first.");
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
        var later = new Gtk.Button.with_label("Decide later");
        later.clicked.connect(() => close());
        actions.append(later);
        if (cleanup.preserves_original) {
            finish_button = new Gtk.Button.with_label("Finish");
            finish_button.add_css_class("suggested-action");
            finish_button.clicked.connect(() => {
                string[] tags_chosen = {};
                CardLink[] links_chosen = {};
                string[] milestones_chosen = {};
                CardLink[] backlinks_chosen = {};
                string[] children_chosen = {};
                for (var i = 0; i < tag_checks.size; i++) {
                    if (tag_checks[i].active) tags_chosen += cleanup.copy_tags[i];
                }
                for (var i = 0; i < link_checks.size; i++) {
                    if (link_checks[i].active) links_chosen += cleanup.copy_links[i];
                }
                for (var i = 0; i < milestone_checks.size; i++) {
                    if (milestone_checks[i].active) milestones_chosen += cleanup.copy_milestones[i].milestone_id;
                }
                for (var i = 0; i < backlink_checks.size; i++) {
                    if (backlink_checks[i].active) backlinks_chosen += card_backlinks[i];
                }
                for (var i = 0; i < child_checks.size; i++) {
                    if (child_checks[i].active) children_chosen += cleanup.children[i].card_id;
                }
                var carry = new ConflictCarryOver();
                carry.tags = tags_chosen;
                carry.links = links_chosen;
                carry.milestone_ids = milestones_chosen;
                carry.backlinks = backlinks_chosen;
                carry.child_ids = children_chosen;
                set_busy(true);
                cleanup_feedback.set_text("Finishing…");
                cleanup_feedback.remove_css_class("error");
                cleanup_feedback.set_visible(true);
                copy_trash_requested(cleanup.cleanup_token, carry);
            });
            actions.append(finish_button);
        }
        layout.append(actions);
        return layout;
    }

    // "Launch · deadline", then one line per date: "due · 2026-12-01".
    internal static string describe_milestone(ConflictCleanupMilestone milestone) {
        var detail = new StringBuilder(milestone.description ?? milestone.kind ?? "Milestone");
        if (milestone.description != null && milestone.kind != null) detail.append(" · " + (!) milestone.kind);
        foreach (var date in milestone.dates) {
            var value = date.date ?? date.datetime;
            if (value != null) detail.append("\n%s · %s".printf(date.role, (!) value));
        }
        return detail.str;
    }

    private static Gtk.Label cleanup_label(string message) {
        return new Gtk.Label(message) { wrap = true, wrap_mode = Pango.WrapMode.WORD_CHAR, xalign = 0.0f };
    }

    // checks collects one ticked check box per item, in order; null lists the items without one.
    private static void append_cleanup_group(Gtk.FlowBox groups, string title, string[] items,
                                             Gee.ArrayList<Gtk.CheckButton>? checks,
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
            Gtk.Widget row_child = label;
            if (checks != null) {
                var check = new Gtk.CheckButton() { active = true, child = label };
                check.update_property(Gtk.AccessibleProperty.LABEL, title + ": " + item, -1);
                checks.add(check);
                row_child = check;
            }
            row_child.set_margin_start(12);
            row_child.set_margin_end(12);
            row_child.set_margin_top(10);
            row_child.set_margin_bottom(10);
            list.append(new Gtk.ListBoxRow() { child = row_child, activatable = checks != null });
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

    private void set_keep_buttons_visible(bool visible) {
        if (keep_original_button != null) keep_original_button.set_visible(visible);
        if (keep_copy_button != null) keep_copy_button.set_visible(visible);
    }

    private void submit(string title, string content) {
        if (comparison == null || busy || saved) return;
        set_busy(true);
        set_feedback("Saving…");
        resolution_requested(comparison.review_token, title, content);
    }

    private void set_busy(bool value) {
        busy = value;
        set_can_close(!busy && (!result_editor.visible || saved));
        choices.set_sensitive(!busy);
        refresh_button.set_sensitive(!busy);
        if (keep_original_button != null) keep_original_button.set_sensitive(!busy && !saved);
        if (keep_copy_button != null) keep_copy_button.set_sensitive(!busy && !saved);
        if (finish_button != null) finish_button.set_sensitive(!busy);
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
        var dialog = new Adw.AlertDialog("Discard the combined version?", "Both versions will stay as they are.");
        dialog.add_response("keep", "Keep Editing");
        dialog.add_response("discard", "Discard");
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

    private static Gtk.Widget version_view(CardDetail card, ConflictEditedOn edited_on, int[] changed_lines,
                                           Gtk.Button keep) {
        var column = new Gtk.Box(Gtk.Orientation.VERTICAL, 6);
        var title = new Gtk.Label(title_without_suffix(card.title)) {
            xalign = 0.0f, ellipsize = Pango.EllipsizeMode.END
        };
        title.add_css_class("heading");
        title.set_tooltip_text(card.title);
        column.append(title);
        var edited = edited_label(edited_on, card.updated_at, new DateTime.now_local().to_unix());
        if (edited != "") {
            var subheading = new Gtk.Label(edited) { xalign = 0.0f, ellipsize = Pango.EllipsizeMode.END };
            subheading.add_css_class("dim-label");
            subheading.add_css_class("caption");
            column.append(subheading);
        }

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
        content.update_property(Gtk.AccessibleProperty.LABEL,
                                (edited == "" ? "Version" : edited) + " card content", -1);
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
        column.append(keep);
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
