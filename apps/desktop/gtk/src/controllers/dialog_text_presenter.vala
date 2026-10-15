namespace HolderLinux {

internal class DialogTextPresenter : Object { // LCOV_EXCL_LINE: declaration-only coverage artifact
    public static string update_prompt_body(string message, string version, string current_version) {
        return "%s\n\nHolder %s is available. You are running %s.".printf(
            message,
            version,
            current_version
        );
    }

    public static string move_to_trash_body(string card_title) {
        return "Move \"%s\" to Trash?\n\nYou can restore it from the Trash tool.".printf(card_title);
    }

    public static string move_to_trash_with_children_body(string card_title, int child_count) {
        var children = child_count == 1 ? "Its child card" : "Its %d child cards".printf(child_count);
        return ("Move \"%s\" to Trash?\n\n%s can move up a level to take its place, or go to Trash "
                + "with it, together with any cards inside them.\n\nYou can restore cards from the "
                + "Trash tool, one at a time.").printf(card_title, children);
    }

    public static string create_linked_card_body(string target) {
        return "No card matches [[%s]] in this project.".printf(target);
    }
}

}
