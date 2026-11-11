#include <gio/gio.h>

gchar *holder_macos_program_directory(void);
gchar *holder_windows_program_directory(void);
gchar *holder_windows_capture_command(gchar **argv, gint *exit_code, GError **error);
void holder_windows_capture_async(gchar **argv, GAsyncReadyCallback callback, gpointer data);
gchar *holder_windows_capture_finish(GAsyncResult *result, gint *exit_code, GError **error);

static void test_program_directories(void) {
    gchar *macos = holder_macos_program_directory();
    gchar *windows = holder_windows_program_directory();
#ifdef __APPLE__
    g_assert_nonnull(macos);
    g_assert_true(g_path_is_absolute(macos));
#else
    g_assert_null(macos);
#endif
#ifdef G_OS_WIN32
    g_assert_nonnull(windows);
    g_assert_true(g_path_is_absolute(windows));
#else
    g_assert_null(windows);
#endif
    g_free(macos);
    g_free(windows);
}

#ifndef G_OS_WIN32
static void test_capture_unavailable(void) {
    gchar *argv[] = { "unused-command", NULL };
    gint status = -1;
    GError *error = NULL;
    gchar *output = holder_windows_capture_command(argv, &status, &error);
    g_assert_null(output);
    g_assert_error(error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED);
    g_assert_cmpstr(error->message, ==, "Windows command capture is unavailable");
    g_assert_cmpint(status, ==, -1);
    g_clear_error(&error);
}

static void captured(GObject *source, GAsyncResult *result, gpointer data) {
    (void)source;
    gint status = -1;
    GError *error = NULL;
    gchar *output = holder_windows_capture_finish(result, &status, &error);
    g_assert_null(output);
    g_assert_error(error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED);
    g_assert_cmpstr(error->message, ==, "Windows command capture is unavailable");
    g_assert_cmpint(status, ==, -1);
    g_clear_error(&error);
    g_main_loop_quit(data);
}

static gboolean timed_out(gpointer data) {
    (void)data;
    g_error("command capture did not complete");
    return G_SOURCE_REMOVE;
}

static void test_async_capture_unavailable(void) {
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    guint timeout = g_timeout_add_seconds(5, timed_out, NULL);
    gchar *argv[] = { "unused-command", NULL };
    holder_windows_capture_async(argv, captured, loop);
    g_main_loop_run(loop);
    g_source_remove(timeout);
    g_main_loop_unref(loop);
}
#endif

int main(int argc, char **argv) {
    g_test_init(&argc, &argv, NULL);
    g_test_add_func("/command-capture/program-directories", test_program_directories);
#ifndef G_OS_WIN32
    g_test_add_func("/command-capture/unavailable", test_capture_unavailable);
    g_test_add_func("/command-capture/async-unavailable", test_async_capture_unavailable);
#endif
    return g_test_run();
}
