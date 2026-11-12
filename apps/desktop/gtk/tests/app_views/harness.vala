using GLib;

namespace HolderLinuxTests {

// Installed, the app's Markdown language is found among the data directories. The tests use the
// one in the source tree, named by HOLDER_MARKDOWN_LANGUAGE_DIR.
internal void av_use_source_markdown_language() {
    var language_dir = Environment.get_variable("HOLDER_MARKDOWN_LANGUAGE_DIR");
    if (language_dir != null) {
        GtkSource.LanguageManager.get_default().prepend_search_path((!) language_dir);
    }
}

// The Git Sync tool asks the GitHub CLI whether it is logged in, and then asks GitHub who the user
// is. The tests put a stand-in `gh` first on PATH that always says it is not logged in, so no test
// reads the user's login or goes online. Windows runs commands its own way and the tests do not
// install gh there.
internal void av_shadow_github_cli() {
    if (Path.DIR_SEPARATOR_S != "/") {
        return;
    }
    var dir = av_temp_dir();
    var gh = Path.build_filename(dir, "gh");
    try {
        FileUtils.set_contents(gh, "#!/bin/sh\necho 'You are not logged into any GitHub hosts.' >&2\nexit 1\n");
    } catch (FileError e) {
        assert_not_reached();
    }
    FileUtils.chmod(gh, 0755);
    var path = Environment.get_variable("PATH") ?? "/usr/bin:/bin";
    Environment.set_variable("PATH", dir + ":" + path, true);
}

internal void av_settle() {
    var context = MainContext.default();
    for (int i = 0; i < 20; i++) {
        while (context.iteration(false)) {}
    }
}

private void av_collect(Gtk.Widget root, Gee.ArrayList<Gtk.Widget> widgets) {
    widgets.add(root);
    for (var child = root.get_first_child(); child != null; child = ((!) child).get_next_sibling()) {
        av_collect((!) child, widgets);
    }
}

internal Gee.ArrayList<Gtk.Widget> av_descendants(Gtk.Widget root) {
    var widgets = new Gee.ArrayList<Gtk.Widget>();
    av_collect(root, widgets);
    return widgets;
}

// The first label under root showing exactly this text, or null.
internal Gtk.Label? av_find_label(Gtk.Widget root, string text) {
    foreach (var widget in av_descendants(root)) {
        var label = widget as Gtk.Label;
        if (label != null && ((!) label).get_text() == text) {
            return label;
        }
    }
    return null;
}

internal string av_temp_dir() {
    try {
        return DirUtils.make_tmp("holder-app-views-XXXXXX");
    } catch (FileError e) {
        assert_not_reached();
    }
}

// A GtkSourceView colour scheme with only the given styles.
internal void av_write_scheme(string dir, string id, string styles) {
    var xml = "<?xml version=\"1.0\"?>\n<style-scheme id=\"%s\" name=\"%s\" version=\"1.0\">\n%s\n</style-scheme>\n"
        .printf(id, id, styles);
    try {
        FileUtils.set_contents(Path.build_filename(dir, id + ".xml"), xml);
    } catch (FileError e) {
        assert_not_reached();
    }
}

// Answers an alert dialog by clicking the button for this response, as the user would: the dialog
// reports the response and then closes. (Emitting the response from code leaves the dialog
// unanswered, so closing it afterwards would report its close response as well.)
internal void av_answer(Adw.AlertDialog dialog, string response) {
    av_button_with_label(dialog, (!) dialog.get_response_label(response)).clicked();
}

internal Gtk.Button av_button_with_tooltip(Gtk.Widget root, string tooltip) {
    foreach (var widget in av_descendants(root)) {
        var button = widget as Gtk.Button;
        if (button != null && ((!) button).get_tooltip_text() == tooltip) {
            return (!) button;
        }
    }
    assert_not_reached();
}

internal Gtk.Button av_button_with_label(Gtk.Widget root, string label) {
    foreach (var widget in av_descendants(root)) {
        var button = widget as Gtk.Button;
        if (button != null && ((!) button).get_label() == label) {
            return (!) button;
        }
    }
    assert_not_reached();
}

// The most recently added controller of this type on a widget: GTK lists a widget's controllers
// newest first, so this is the app's own rather than one the widget made for itself.
internal Gtk.EventController av_newest_controller(Gtk.Widget widget, Type type) {
    var controllers = widget.observe_controllers();
    for (uint i = 0; i < controllers.get_n_items(); i++) {
        var controller = (Gtk.EventController) controllers.get_item(i);
        if (controller.get_type().is_a(type)) {
            return controller;
        }
    }
    assert_not_reached();
}

// Raises key-pressed on the widget's newest key controller, as a key press would.
internal bool av_press(Gtk.Widget widget, uint keyval, Gdk.ModifierType state = 0) {
    var keys = av_newest_controller(widget, typeof(Gtk.EventControllerKey));
    bool handled = false;
    Signal.emit_by_name(keys, "key-pressed", keyval, 0u, state, out handled);
    return handled;
}

// A workspace inside a toast overlay, as the window builds it, with nothing loaded.
internal class WorkspaceFixture : Object {
    public GLib.ListStore search_store = new GLib.ListStore(typeof(HolderLinux.SearchCardResult));
    public HolderLinux.WorkspacePane workspace;
    public Adw.ToastOverlay toast_overlay = new Adw.ToastOverlay();
    public Adw.Window window = new Adw.Window();

    public WorkspaceFixture() {
        workspace = new HolderLinux.WorkspacePane(search_store);
        toast_overlay.set_child(workspace.widget);
        window.set_content(toast_overlay);
    }

    // Shows the window and waits until the editor has a size, for tests that need its layout.
    public void present() {
        window.set_default_size(1000, 700);
        window.present();
        assert(wait_for_condition(() => workspace.widget.get_width() > 0 && workspace.content_stack.get_width() > 0));
        av_settle();
    }

    // Whether a toast with this title is showing.
    public bool has_toast(string title) {
        av_settle();
        return av_find_label(toast_overlay, title) != null;
    }

    public string editor_text() {
        Gtk.TextIter start;
        Gtk.TextIter end;
        workspace.editor_buffer.get_bounds(out start, out end);
        return workspace.editor_buffer.get_text(start, end, false);
    }

    public virtual void destroy() {
        window.destroy();
        av_settle();
    }
}

}
