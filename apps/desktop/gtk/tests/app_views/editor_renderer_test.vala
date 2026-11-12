using GLib;

namespace HolderLinuxTests {

private class RendererFixture : WorkspaceFixture {
    public HolderLinux.WindowEditorRenderer renderer;
    public int rendered = 0;

    public RendererFixture() {
        base();
        renderer = new HolderLinux.WindowEditorRenderer(
            window,
            workspace,
            workspace.search_summary_label,
            workspace.ai_panel
        );
        renderer.content_rendered.connect(() => {
            rendered++;
        });
    }

    public string mode() {
        return (!) workspace.content_stack.get_visible_child_name();
    }
}

private void test_applying_shows_an_empty_read_only_editor_titled_holder() {
    var f = new RendererFixture();
    f.workspace.editor_buffer.set_text("left over", -1);

    f.renderer.apply();

    assert(f.editor_text() == "");
    assert(!f.workspace.editor_view.get_editable());
    assert(f.window.get_title() == "Holder");
    assert(av_find_label(f.workspace.widget, "Holder") != null);
    assert(av_find_label(f.workspace.widget, "Thread: none selected") != null);
    assert(f.workspace.search_summary_label.get_text() == "");
    assert(f.mode() == "editor");
    assert(f.rendered == 1);
    f.destroy();
}

private void test_editor_state_is_applied_and_marked_while_it_is() {
    var f = new RendererFixture();
    var marked_while_changing = false;
    f.workspace.editor_buffer.changed.connect(() => {
        marked_while_changing = f.renderer.is_applying_content;
    });

    f.renderer.set_editor_state("# Card\n\nBody", true);

    assert(f.editor_text() == "# Card\n\nBody");
    assert(f.workspace.editor_view.get_editable());
    // Edits made while the renderer applies content are its own, not the user's.
    assert(marked_while_changing);
    assert(!f.renderer.is_applying_content);
    assert(f.rendered == 1);
    f.destroy();
}

private void test_the_chrome_follows_each_setter() {
    var f = new RendererFixture();

    f.renderer.update_window_title("Boiler service");
    assert(f.window.get_title() == "Boiler service");
    assert(av_find_label(f.workspace.widget, "Boiler service") != null);

    f.renderer.set_save_state_text("Saved");
    var save_state = av_find_label(f.workspace.widget, "Saved");
    assert(save_state != null && ((!) save_state).get_visible());

    f.renderer.set_search_summary_text("3 results");
    assert(f.workspace.search_summary_label.get_text() == "3 results");

    f.renderer.set_ai_thread_title("Plan the week");
    assert(av_find_label(f.workspace.widget, "Thread: Plan the week") != null);

    f.renderer.show_search_mode();
    assert(f.mode() == "search");
    f.renderer.show_editor_mode();
    assert(f.mode() == "editor");

    // None of that re-renders the editor's content.
    assert(f.rendered == 0);
    f.destroy();
}

private void test_new_content_keeps_the_chrome_already_set() {
    var f = new RendererFixture();
    f.renderer.update_window_title("Boiler service");
    f.renderer.set_save_state_text("Saved");
    f.renderer.set_ai_thread_title("Plan the week");
    f.renderer.show_search_mode();

    f.renderer.set_editor_state("Other text", false);

    assert(f.window.get_title() == "Boiler service");
    assert(av_find_label(f.workspace.widget, "Saved") != null);
    assert(av_find_label(f.workspace.widget, "Thread: Plan the week") != null);
    assert(f.mode() == "search");
    assert(f.editor_text() == "Other text");
    f.destroy();
}

internal void register_editor_renderer_tests() {
    Test.add_func("/app_views/editor_renderer/apply_defaults", test_applying_shows_an_empty_read_only_editor_titled_holder);
    Test.add_func("/app_views/editor_renderer/editor_state", test_editor_state_is_applied_and_marked_while_it_is);
    Test.add_func("/app_views/editor_renderer/chrome_setters", test_the_chrome_follows_each_setter);
    Test.add_func("/app_views/editor_renderer/content_keeps_chrome", test_new_content_keeps_the_chrome_already_set);
}

}
