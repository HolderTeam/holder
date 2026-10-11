namespace HolderLinux {

internal class WindowPresenter : Object { // LCOV_EXCL_LINE: declaration-only coverage artifact
    public const string RECOVERY_DIALOG_TITLE = "Recover unsaved changes?";
    public const string UNKNOWN_CARD_TITLE = "this card";

    public static bool is_blank(string? text) {
        return text == null || ((!) text).strip().length == 0;
    }

    public static string card_title_for_id(ListModel cards, string card_id) {
        for (uint i = 0; i < cards.get_n_items(); i++) {
            var card = cards.get_item(i) as CardSummary;
            if (card != null && card.card_id == card_id) {
                return card.title;
            }
        }
        return UNKNOWN_CARD_TITLE;
    }

    // Live children in the loaded project cards. The server decides what is trashed; this only
    // chooses which confirmation to show.
    public static int child_count_for_id(ListModel cards, string card_id) {
        int count = 0;
        for (uint i = 0; i < cards.get_n_items(); i++) {
            var card = cards.get_item(i) as CardSummary;
            if (card != null && card.parent_card_id == card_id) {
                count++;
            }
        }
        return count;
    }

    public static string recovery_draft_body(string card_title, int64 saved_at, TimeZone timezone) {
        var when = new DateTime.from_unix_utc(saved_at).to_timezone(timezone);
        return "Holder found a local recovery copy of “%s” from %s.".printf(
            card_title, when.format("%c")
        );
    }
}

}
