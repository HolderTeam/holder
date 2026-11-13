using GLib;

namespace HolderLinuxTests {

// Drops the caller's last reference to obj and reports whether it is then freed. A reference
// cycle keeps it alive: typically a closure that captures it, stored by something it owns.
private bool lt_freed_when_released(owned Object obj) {
    var weak = WeakRef(obj);
    obj = null;
    // GTK finishes some teardown, such as unrealizing, from the main loop.
    for (int i = 0; i < 20; i++) {
        while (MainContext.default().iteration(false)) {}
        var alive = weak.get();
        if (alive == null) return true;
        alive = null;
        Thread.usleep(5000);
    }
    return weak.get() == null;
}

private void lt_check(string what, owned Object obj) {
    if (!lt_freed_when_released((owned) obj)) {
        Test.message("%s is still alive after its last reference was dropped", what);
        Test.fail();
    }
}

private void test_tool_views_are_freed() {
    lt_check("FlowboardToolView", new HolderLinux.FlowboardToolView());
    lt_check("ConnectionsToolView", new HolderLinux.ConnectionsToolView());
    lt_check("TagsToolView", new HolderLinux.TagsToolView());
    lt_check("ResourcesToolView", new HolderLinux.ResourcesToolView());
    lt_check("MilestonesToolView", new HolderLinux.MilestonesToolView());
    lt_check("HistoryToolView", new HolderLinux.HistoryToolView());
    lt_check("GitSyncToolView", new HolderLinux.GitSyncToolView());
    lt_check("RecoveryKeyToolView", new HolderLinux.RecoveryKeyToolView());
    lt_check("TrashToolView", new HolderLinux.TrashToolView());
    lt_check("DebugToolView", new HolderLinux.DebugToolView());
    lt_check("SharingToolView", new HolderLinux.SharingToolView());
    lt_check("AiPanel", new HolderLinux.AiPanel());
}

private void test_preferences_dialog_is_freed() {
    var view = new GtkSource.View();
    var buffer = (GtkSource.Buffer) view.get_buffer();
    var dialog = new HolderLinux.PreferencesDialog(buffer, view, null, null, new HolderLinux.EditorFontStyle(view));
    lt_check("PreferencesDialog", (owned) dialog);
}

// Builders like the app's: closures in one method share a block holding the dialog and its entry,
// and the entry's handler keeps the block. Built in a function of their own, as the app's are, so
// the test does not hold that block itself.
private Adw.AlertDialog lt_scoped_dialog() {
    var dialog = new Adw.AlertDialog("Name", null);
    dialog.add_response("cancel", "Cancel");
    var entry = new Gtk.Entry();
    dialog.set_extra_child(entry);
    var handlers = HolderLinux.SignalScope.until_closed(dialog);
    handlers.add(entry, entry.changed.connect(() => {
        dialog.set_response_enabled("cancel", entry.get_text() != "");
    }));
    return dialog;
}

private Gtk.Box lt_scoped_form() {
    var form = new Gtk.Box(Gtk.Orientation.VERTICAL, 0);
    var button = new Gtk.Button.with_label("Save");
    form.append(button);
    var handlers = HolderLinux.SignalScope.until_removed(form);
    handlers.add(button, button.clicked.connect(() => {
        form.set_sensitive(false);
        button.set_label("Saved");
    }));
    return form;
}

private void test_a_scoped_dialog_is_freed_when_it_closes() {
    var window = new Adw.Window();
    window.present();
    var dialog = lt_scoped_dialog();
    dialog.present(window);
    av_settle();
    dialog.force_close();
    av_settle();
    lt_check("A closed dialog", (owned) dialog);
    window.destroy();
}

private void test_a_scoped_form_is_freed_when_it_is_removed() {
    var parent = new Gtk.Box(Gtk.Orientation.VERTICAL, 0);
    var form = lt_scoped_form();
    parent.append(form);
    parent.remove(form);
    lt_check("A removed form", (owned) form);
}

public void register_lifetime_tests() {
    Test.add_func("/app-views/lifetime/scoped-dialog", test_a_scoped_dialog_is_freed_when_it_closes);
    Test.add_func("/app-views/lifetime/scoped-form", test_a_scoped_form_is_freed_when_it_is_removed);
    Test.add_func("/app-views/lifetime/tool-views", test_tool_views_are_freed);
    Test.add_func("/app-views/lifetime/preferences-dialog", test_preferences_dialog_is_freed);
}

}
