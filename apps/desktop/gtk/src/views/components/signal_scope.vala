namespace HolderLinux {

// The signal handlers a builder connects for one dialog or panel, disconnected when it closes.
//
// The closures one Vala method makes share a single block, which holds every local any of them
// captures: typically the dialog and its entries. Each handler on one of those widgets keeps the
// block, and the block keeps the widgets, so a dialog closed, or a form taken off screen, would
// never be freed. Disconnecting the handlers drops the closures, and with them the block.
//
// The dialog or widget owns the scope, and the scope holds the objects it disconnects weakly: one
// already freed took its handlers with it.
public class SignalScope : Object {
    private class Handler {
        public WeakRef instance;
        public ulong id;

        public Handler(Object instance, ulong id) {
            this.instance = WeakRef(instance);
            this.id = id;
        }
    }

    private const string DATA_KEY = "holder-signal-scope";

    private Gee.ArrayList<Handler> handlers = new Gee.ArrayList<Handler>();

    // Released once the dialog has closed. An alert dialog closes before it emits the response
    // that closed it, so the release waits for the main loop: the response handler still runs.
    public static SignalScope until_closed(Adw.Dialog dialog) {
        var scope = new SignalScope();
        dialog.closed.connect(scope.release_soon);
        dialog.set_data<SignalScope>(DATA_KEY, scope);
        return scope;
    }

    // Released when the widget, once shown in a parent, is taken out of it.
    public static SignalScope until_removed(Gtk.Widget widget) {
        var scope = new SignalScope();
        widget.notify["parent"].connect(scope.release_when_orphaned);
        widget.set_data<SignalScope>(DATA_KEY, scope);
        return scope;
    }

    // Records a handler, for example: scope.add(entry, entry.changed.connect(...)).
    public void add(Object instance, ulong handler_id) {
        handlers.add(new Handler(instance, handler_id));
    }

    private void release_soon() {
        Idle.add(() => {
            release();
            return Source.REMOVE;
        });
    }

    private void release_when_orphaned(Object widget, ParamSpec pspec) {
        if (((Gtk.Widget) widget).get_parent() == null) {
            release();
        }
    }

    private void release() {
        var connected = handlers;
        handlers = new Gee.ArrayList<Handler>();
        foreach (var handler in connected) {
            var instance = handler.instance.get();
            if (instance != null && SignalHandler.is_connected(instance, handler.id)) {
                SignalHandler.disconnect(instance, handler.id);
            }
        }
    }
}

}
