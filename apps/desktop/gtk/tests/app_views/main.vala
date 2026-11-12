using GLib;

namespace HolderLinuxTests {

public static int main(string[] args) {
    av_report_warnings_as_failures();
    // Keep going past a failed test, so one run reports them all. (A failed assert still stops it.)
    string[] all_args = args;
    all_args += "--keep-going";
    unowned string[] test_args = all_args;
    Test.init(ref test_args);
    av_stop_aborting_on_warnings();
    av_isolate_cache();
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
