using GLib;

namespace HolderLinuxTests {

// The Vala binding puts the destroy callback before the test callback.
[CCode (cname = "g_test_add_data_func_full", cheader_filename = "glib.h")]
private extern void add_test_with_data(string path,
                                      [CCode (delegate_target_pos = 1.9, destroy_notify_pos = 2.9)]
                                      owned TestDataFunc test_case);

internal void av_add_test(string path, TestFunc test_case) {
    add_test_with_data(path, () => {
        Test.message("Running %s", path);
        var started = get_monotonic_time();
        test_case();
        var elapsed_ms = (get_monotonic_time() - started) / 1000.0;
        Test.message("%s took %.1f ms", path, elapsed_ms);
    });
}

public static int main(string[] args) {
    av_report_warnings_as_failures();
    // Keep going past a failed test, so one run reports them all. (A failed assert still stops it.)
    string[] all_args = args;
    all_args += "--keep-going";
    // Test.init takes out the options it handles without freeing them, so it gets its own array of
    // pointers: all_args keeps owning, and freeing, every string.
    (unowned string)[] pointers = {};
    foreach (unowned string arg in all_args) pointers += arg;
    unowned string[] test_args = (string[]) pointers;
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
    register_lifetime_tests();
#if HOLDER_TERMINAL_VTE
    register_terminal_tool_view_tests();
#endif
    return Test.run();
}

}
