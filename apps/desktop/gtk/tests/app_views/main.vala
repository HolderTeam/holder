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

    register_editor_renderer_tests();
    register_internal_link_navigator_tests();
    return Test.run();
}

}
