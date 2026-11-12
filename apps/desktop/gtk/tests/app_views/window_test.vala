using GLib;

namespace HolderLinuxTests {

private class RecordingPresence : Object, HolderLinux.IPresence {
    public int starts = 0;
    public int stops = 0;
    public int leaves = 0;

    public void start() {
        starts++;
    }

    public void stop() {
        stops++;
    }

    public void leave(uint timeout_ms) {
        leaves++;
    }
}

// The update check's view of holder.team: it answers with `body` and counts what it was asked.
private class OfflineUpdateTransport : Object, HolderLinux.IUpdateMetadataTransport {
    public int fetches = 0;
    public string body = "{}";

    public async string fetch_text(string url) throws Error {
        fetches++;
        return body;
    }
}

// The main window, built with doubles for everything it reaches outside the app through: the fake
// API, a fixed daemon address, no daemon to start, a recorded presence, an offline update check and
// its own in-memory settings.
private class WindowFixture : Object {
    private static Adw.Application? shared_app = null;

    public MainControllerFakeApi api = new MainControllerFakeApi();
    public FakeServerDiscovery discovery = new FakeServerDiscovery();
    public RecordingPresence presence = new RecordingPresence();
    public OfflineUpdateTransport updates = new OfflineUpdateTransport();
    public Settings settings;
    public HolderLinux.MainWindow window;

    public WindowFixture() {
        settings = new Settings.with_backend(
            HolderLinux.AppSettings.SCHEMA_ID,
            SettingsBackend.memory_settings_backend_new()
        );
    }

    public static Adw.Application app() {
        if (shared_app == null) {
            shared_app = new Adw.Application("team.holder.Holder.AppViewTests", ApplicationFlags.NON_UNIQUE);
            try {
                ((!) shared_app).register(null);
            } catch (Error e) {
                assert_not_reached();
            }
        }
        return (!) shared_app;
    }

    // Builds and shows the window; it starts talking to the fake API straight away.
    public void open() {
        var services = new HolderLinux.MainWindowServices();
        services.api_factory = new MainControllerFakeApiFactory(api, api);
        services.server_discovery = discovery;
        services.find_backend_starter = false;
        services.presence = presence;
        services.update_check = new HolderLinux.UpdateCheckService(updates);
        services.settings = settings;
        window = new HolderLinux.MainWindow(app(), 1000, 700, services);
        window.present();
    }

    public bool shows_label(string text) {
        av_settle();
        return av_find_label(window, text) != null;
    }

    public void destroy() {
        window.destroy();
        av_settle();
    }
}

private void test_the_window_starts_and_loads_the_first_project() {
    var f = new WindowFixture();
    f.open();

    assert(wait_for_condition(() => f.shows_label("Project 1")));
    assert(f.api.list_projects_calls > 0);
    assert(f.presence.starts == 1);
    // The update check runs once it is due; nothing is newer here, so no dialog follows.
    assert(wait_for_condition(() => f.updates.fetches == 1));
    assert(f.window.get_visible_dialog() == null);
    f.destroy();
}

internal void register_window_tests() {
    Test.add_func("/app_views/window/starts_and_loads", test_the_window_starts_and_loads_the_first_project);
}

}
