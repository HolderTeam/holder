// Native regression for GTK/AccessKit menu labels. No external AX permissions
// or user data: inspect and activate this test process's own AppKit objects.
#import <Cocoa/Cocoa.h>
#include <gtk/gtk.h>

static GtkApplication *application;
static GtkMenuButton *menu_button;
static GtkWidget *first_model;
static GMenu *section;
static unsigned int activations;
static unsigned int stage;
static gint64 deadline;
static gboolean failed;
static gboolean complete;

static void fail(const char *message)
{
    g_printerr("FAIL: %s (stage %u)\n", message, stage);
    failed = TRUE;
    g_application_quit(G_APPLICATION(application));
}

static id find_title(id node, NSString *title, unsigned int depth)
{
    if (depth > 40)
        return nil;
    if ([node respondsToSelector:@selector(accessibilityTitle)] &&
        [[node accessibilityTitle] isEqualToString:title] &&
        [node respondsToSelector:@selector(accessibilityRole)] &&
        [[node accessibilityRole] isEqualToString:NSAccessibilityMenuItemRole])
        return node;
    if ([node respondsToSelector:@selector(accessibilityChildren)]) {
        for (id child in [node accessibilityChildren]) {
            id match = find_title(child, title, depth + 1);
            if (match)
                return match;
        }
    }
    return nil;
}

static id item(NSString *title)
{
    for (NSWindow *window in [NSApp windows]) {
        if ([[window title] isEqualToString:@"Holder menu accessibility test"])
            return find_title(window, title, 0);
    }
    return nil;
}

static void next_stage(void)
{
    stage++;
    deadline = g_get_monotonic_time() + 5 * G_TIME_SPAN_SECOND;
}

static GtkWidget *find_first_model(GtkWidget *widget)
{
    if (g_str_equal(G_OBJECT_TYPE_NAME(widget), "GtkModelButton")) {
        char *text = NULL;
        g_object_get(widget, "text", &text, NULL);
        gboolean first = g_strcmp0(text, "First action") == 0;
        g_free(text);
        if (first)
            return widget;
    }
    for (GtkWidget *child = gtk_widget_get_first_child(widget); child;
         child = gtk_widget_get_next_sibling(child)) {
        GtkWidget *found = find_first_model(child);
        if (found)
            return found;
    }
    return NULL;
}

static gboolean has_normal_label(GtkWidget *model)
{
    for (GtkWidget *child = gtk_widget_get_first_child(model); child;
         child = gtk_widget_get_next_sibling(child)) {
        if (GTK_IS_LABEL(child) &&
            g_strcmp0(gtk_label_get_text(GTK_LABEL(child)), "First action") == 0)
            return gtk_accessible_get_accessible_role(GTK_ACCESSIBLE(child)) == GTK_ACCESSIBLE_ROLE_LABEL;
    }
    return FALSE;
}

static gboolean reopen_menu(gpointer unused)
{
    gtk_menu_button_popup(menu_button);
    return G_SOURCE_REMOVE;
}

static gboolean check(gpointer unused)
{
    @autoreleasepool {
        if (g_get_monotonic_time() > deadline) {
            fail("Timed out waiting for named menu items, state or activation");
            return G_SOURCE_REMOVE;
        }
        id first = item(stage == 0 ? @"First action" :
                        stage >= 7 ? @"Renamed action" : @"Changed visible text");
        if (stage == 0 && first) {
            id disabled = item(@"Disabled action");
            id toggle = item(@"Enabled option");
            if (!disabled || !toggle || !item(@"More") || !item(@"Formatted action"))
                return G_SOURCE_CONTINUE;
            if (![first isAccessibilityEnabled] || [disabled isAccessibilityEnabled]) {
                fail("Menu enabled states were not preserved");
                return G_SOURCE_REMOVE;
            }
            if ([[toggle accessibilityValue] boolValue]) {
                fail("Unchecked menu state was not preserved");
                return G_SOURCE_REMOVE;
            }
            if (![first isAccessibilityFocused])
                return G_SOURCE_CONTINUE;
            NSRect frame = [first accessibilityFrame];
            if (NSIsEmptyRect(frame)) {
                fail("Named menu item has no native bounds");
                return G_SOURCE_REMOVE;
            }
            g_print("PASS: names, mnemonics, disabled state, unchecked state, focus and bounds\n");
            first_model = find_first_model(GTK_WIDGET(menu_button));
            if (!first_model || !has_normal_label(first_model)) {
                fail("Menu item's internal text must have the normal label role");
                return G_SOURCE_REMOVE;
            }
            gtk_accessible_update_property(GTK_ACCESSIBLE(first_model),
                                           GTK_ACCESSIBLE_PROPERTY_LABEL, "Explicit menu name", -1);
            next_stage();
        } else if (stage == 1 && item(@"Explicit menu name")) {
            g_object_set(first_model, "text", "Changed visible _text", NULL);
            next_stage();
        } else if (stage == 2 && item(@"Explicit menu name")) {
            gtk_accessible_reset_property(GTK_ACCESSIBLE(first_model), GTK_ACCESSIBLE_PROPERTY_LABEL);
            next_stage();
        } else if (stage == 3 && first) {
            g_print("PASS: normal label role, markup, explicit name preservation and reset\n");
            if (![first accessibilityPerformPress]) {
                fail("Native menu item did not support activation");
                return G_SOURCE_REMOVE;
            }
            next_stage();
        } else if (stage == 4 && activations == 1) {
            if (gtk_widget_get_mapped(GTK_WIDGET(gtk_menu_button_get_popover(menu_button))))
                return G_SOURCE_CONTINUE;
            // Let macOS finish disposing the old popup surface before reopening.
            g_timeout_add(250, reopen_menu, NULL);
            next_stage();
        } else if (stage == 5 && first && item(@"Enabled option")) {
            if (![item(@"Enabled option") accessibilityPerformPress]) {
                fail("Native check item did not support activation");
                return G_SOURCE_REMOVE;
            }
            next_stage();
        } else if (stage == 6) {
            GAction *toggle = g_action_map_lookup_action(G_ACTION_MAP(application), "toggle");
            GVariant *state = g_action_get_state(toggle);
            gboolean checked = g_variant_get_boolean(state);
            g_variant_unref(state);
            if (!checked)
                return G_SOURCE_CONTINUE;
            if (gtk_widget_get_mapped(GTK_WIDGET(gtk_menu_button_get_popover(menu_button))))
                return G_SOURCE_CONTINUE;
            // Reopening and replacing the menu item exercises label updates,
            // not only labels produced during initial tree construction.
            g_menu_remove(section, 0);
            g_menu_insert(section, 0, "_Renamed action", "app.test");
            g_timeout_add(250, reopen_menu, NULL);
            next_stage();
        } else if (stage == 7 && first && item(@"Enabled option")) {
            if (![[item(@"Enabled option") accessibilityValue] boolValue])
                return G_SOURCE_CONTINUE;
            g_print("PASS: activation, check state, reopening and changed label\n");
            if (![item(@"More") accessibilityPerformPress]) {
                fail("Native submenu did not support activation");
                return G_SOURCE_REMOVE;
            }
            next_stage();
        } else if (stage == 8 && item(@"Nested action")) {
            if (![item(@"Nested action") accessibilityPerformPress]) {
                fail("Nested item did not support activation");
                return G_SOURCE_REMOVE;
            }
            next_stage();
        } else if (stage == 9 && activations == 2) {
            g_print("PASS: submenu name and native activation\n");
            complete = TRUE;
            g_application_quit(G_APPLICATION(application));
            return G_SOURCE_REMOVE;
        }
        return G_SOURCE_CONTINUE;
    }
}

static void activate_action(GSimpleAction *action, GVariant *parameter, gpointer unused)
{
    activations++;
}

static void toggle_action(GSimpleAction *action, GVariant *parameter, gpointer unused)
{
    GVariant *state = g_action_get_state(G_ACTION(action));
    g_simple_action_set_state(action, g_variant_new_boolean(!g_variant_get_boolean(state)));
    g_variant_unref(state);
}

static gboolean open_menu(gpointer unused)
{
    gtk_menu_button_popup(menu_button);
    deadline = g_get_monotonic_time() + 5 * G_TIME_SPAN_SECOND;
    g_timeout_add(50, check, NULL);
    return G_SOURCE_REMOVE;
}

static void activate(GtkApplication *app, gpointer unused)
{
    g_object_set(gtk_settings_get_default(), "gtk-enable-animations", FALSE, NULL);
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Holder menu accessibility test");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 240);
    GSimpleAction *action = g_simple_action_new("test", NULL);
    g_signal_connect(action, "activate", G_CALLBACK(activate_action), NULL);
    g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(action));
    g_object_unref(action);
    action = g_simple_action_new("disabled", NULL);
    g_simple_action_set_enabled(action, FALSE);
    g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(action));
    g_object_unref(action);
    action = g_simple_action_new_stateful("toggle", NULL, g_variant_new_boolean(FALSE));
    g_signal_connect(action, "activate", G_CALLBACK(toggle_action), NULL);
    g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(action));
    g_object_unref(action);

    GMenu *menu = g_menu_new();
    section = g_menu_new();
    g_menu_append(section, "_First action", "app.test");
    g_menu_append(section, "Disabled action", "app.disabled");
    g_menu_append(section, "Enabled option", "app.toggle");
    GMenuItem *formatted = g_menu_item_new("<b>Formatted</b> _action", "app.test");
    // GTK's menu tracker enables markup when this string attribute is present.
    g_menu_item_set_attribute(formatted, "use-markup", "s", "true");
    g_menu_append_item(section, formatted);
    g_object_unref(formatted);
    g_menu_append_section(menu, NULL, G_MENU_MODEL(section));
    GMenu *submenu = g_menu_new();
    g_menu_append(submenu, "Nested action", "app.test");
    g_menu_append_submenu(menu, "More", G_MENU_MODEL(submenu));
    g_object_unref(submenu);
    menu_button = GTK_MENU_BUTTON(gtk_menu_button_new());
    gtk_menu_button_set_label(menu_button, "Open test menu");
    gtk_menu_button_set_menu_model(menu_button, G_MENU_MODEL(menu));
    g_object_unref(menu);
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_append(GTK_BOX(box), GTK_WIDGET(menu_button));
    gtk_window_set_child(GTK_WINDOW(window), box);
    gtk_window_present(GTK_WINDOW(window));
    g_timeout_add(200, open_menu, NULL);
}

int main(int argc, char **argv)
{
    application = gtk_application_new("team.holder.MenuAccessibilityTest", G_APPLICATION_NON_UNIQUE);
    g_signal_connect(application, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(application), argc, argv);
    g_clear_object(&section);
    g_object_unref(application);
    return failed || !complete ? 1 : status;
}
