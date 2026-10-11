namespace HolderLinux {

public delegate void CardActionConfirmed();
// trash_subtree: the user chose to send the card's descendants to Trash too.
public delegate void CardTrashConfirmed(bool trash_subtree);

internal class CardActionDialogAdapter : Object {
    private Gtk.Window parent; // LCOV_EXCL_LINE GCOVR_EXCL_LINE: field released only by the generated finalizer

    public CardActionDialogAdapter(Gtk.Window parent) {
        this.parent = parent;
    }

    public void confirm_move_to_trash(string card_title, owned CardActionConfirmed on_confirmed) {
        var dialog = new Adw.AlertDialog(
            "Move to Trash",
            DialogTextPresenter.move_to_trash_body(card_title)
        );
        dialog.add_response("cancel", "Cancel");
        dialog.add_response("trash", "Move to Trash");
        dialog.set_response_appearance("trash", Adw.ResponseAppearance.DESTRUCTIVE);
        dialog.set_default_response("trash");
        dialog.set_close_response("cancel");
        dialog.response.connect((response) => {
            if (response == "trash") {
                on_confirmed();
            }
        });
        dialog.present(parent);
    }

    // For a card with children: keep them (moving them up a level, the default), or trash the
    // whole subtree.
    public void confirm_move_to_trash_with_children(string card_title,
                                                    int child_count,
                                                    owned CardTrashConfirmed on_confirmed) {
        var dialog = new Adw.AlertDialog(
            "Move to Trash",
            DialogTextPresenter.move_to_trash_with_children_body(card_title, child_count)
        );
        dialog.add_response("cancel", "Cancel");
        dialog.add_response("subtree", "Move Entire Subtree to Trash");
        dialog.add_response("keep", "Move to Trash and Keep Children");
        dialog.set_response_appearance("subtree", Adw.ResponseAppearance.DESTRUCTIVE);
        dialog.set_response_appearance("keep", Adw.ResponseAppearance.SUGGESTED);
        dialog.set_default_response("keep");
        dialog.set_close_response("cancel");
        dialog.response.connect((response) => {
            if (response == "keep") {
                on_confirmed(false);
            } else if (response == "subtree") {
                on_confirmed(true);
            }
        });
        dialog.present(parent);
    }

    public void confirm_create_linked_card(string target, owned CardActionConfirmed on_confirmed) {
        var dialog = new Adw.AlertDialog(
            "Create Linked Card?",
            DialogTextPresenter.create_linked_card_body(target)
        );
        dialog.add_response("cancel", "Cancel");
        dialog.add_response("create", "Create Card");
        dialog.set_response_appearance("create", Adw.ResponseAppearance.SUGGESTED);
        dialog.set_default_response("create");
        dialog.set_close_response("cancel");
        dialog.response.connect((response) => {
            if (response == "create") {
                on_confirmed();
            }
        });
        dialog.present(parent);
    }
}

}
