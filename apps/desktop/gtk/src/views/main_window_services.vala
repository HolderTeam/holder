namespace HolderLinux {

// What the main window reaches outside the app through: the daemon's API and where to find it,
// starting the daemon, telling it the app is there, the update check, and saved settings. The app
// uses these defaults; the window tests pass doubles, so a test never starts a daemon or goes online.
public class MainWindowServices : Object {
    public IApiFactory api_factory;
    public IServerDiscovery server_discovery;
    // How to start the daemon. Null with find_backend_starter set (the default) looks for holderctl
    // beside the app or on PATH; null without it uses a daemon that is already running.
    public IBackendStarter? backend_starter = null;
    public bool find_backend_starter = true;
    // Null holds the daemon's event stream open, as the app does.
    public IPresence? presence = null;
    public UpdateCheckService update_check;
    public Settings? settings;

    public MainWindowServices() {
        api_factory = new DefaultApiFactory();
        server_discovery = new FileServerDiscovery();
        update_check = new UpdateCheckService();
        settings = AppSettings.open_or_null();
    }
}

}
