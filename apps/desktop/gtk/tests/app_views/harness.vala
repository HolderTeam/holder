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

// A workspace, as the window builds it, with nothing loaded.
internal class WorkspaceFixture : Object {
    public GLib.ListStore search_store = new GLib.ListStore(typeof(HolderLinux.SearchCardResult));
    public HolderLinux.WorkspacePane workspace;
    public Gtk.Window window = new Gtk.Window();

    public WorkspaceFixture() {
        workspace = new HolderLinux.WorkspacePane(search_store);
        window.set_child(workspace.widget);
    }

    public string editor_text() {
        Gtk.TextIter start;
        Gtk.TextIter end;
        workspace.editor_buffer.get_bounds(out start, out end);
        return workspace.editor_buffer.get_text(start, end, false);
    }

    public void destroy() {
        window.destroy();
        av_settle();
    }
}

}
