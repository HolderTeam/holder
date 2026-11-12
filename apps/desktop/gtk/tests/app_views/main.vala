using GLib;

namespace HolderLinuxTests {

public static int main(string[] args) {
    Test.init(ref args);
    if (!Gtk.init_check()) {
        stdout.printf("Skipping app view tests: GTK display is unavailable.\n");
        return 0;
    }
    Adw.init();
    av_use_source_markdown_language();
    av_shadow_github_cli();

    register_editor_renderer_tests();
    register_internal_link_navigator_tests();
    register_toolbox_tests();
    register_workspace_pane_tests();
    register_window_tests();
#if HOLDER_TERMINAL_VTE
    register_terminal_tool_view_tests();
#endif
    return Test.run();
}

}
