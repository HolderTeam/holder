from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar, cast

from attrs import define as _attrs_define
from typing_extensions import Self

T = TypeVar("T", bound="PullChanges")


@_attrs_define
class PullChanges:
    """What a pull changed on this device to bring two devices' versions of the same cards together. Each card
    change is its own commit, so card history can undo it.

        Attributes:
            merged_card_ids (list[str]): Cards changed on both devices in different parts (title, place, text, links or
                milestones), now holding
                both changes.
            kept_out_of_trash_card_ids (list[str]): Cards moved to Trash on this device while the other device edited them.
                They stay out of Trash with
                that edit and can be moved to Trash again.
            restored_from_trash_card_ids (list[str]): Cards edited on this device while the other device moved them to
                Trash. They are back out of Trash
                with this device's edit.
            conflict_copy_ids (list[str]): New cards holding this device's version of a card whose changes clashed with the
                other device's. The
                original holds the other device's version.
            hidden_cards_repaired (int): Live cards the pull left under a missing or trashed parent, made reachable again.
    """

    merged_card_ids: list[str]
    kept_out_of_trash_card_ids: list[str]
    restored_from_trash_card_ids: list[str]
    conflict_copy_ids: list[str]
    hidden_cards_repaired: int

    def to_dict(self) -> dict[str, Any]:
        merged_card_ids = self.merged_card_ids

        kept_out_of_trash_card_ids = self.kept_out_of_trash_card_ids

        restored_from_trash_card_ids = self.restored_from_trash_card_ids

        conflict_copy_ids = self.conflict_copy_ids

        hidden_cards_repaired = self.hidden_cards_repaired

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "merged_card_ids": merged_card_ids,
                "kept_out_of_trash_card_ids": kept_out_of_trash_card_ids,
                "restored_from_trash_card_ids": restored_from_trash_card_ids,
                "conflict_copy_ids": conflict_copy_ids,
                "hidden_cards_repaired": hidden_cards_repaired,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        merged_card_ids = cast(list[str], d.pop("merged_card_ids"))

        kept_out_of_trash_card_ids = cast(
            list[str], d.pop("kept_out_of_trash_card_ids")
        )

        restored_from_trash_card_ids = cast(
            list[str], d.pop("restored_from_trash_card_ids")
        )

        conflict_copy_ids = cast(list[str], d.pop("conflict_copy_ids"))

        hidden_cards_repaired = d.pop("hidden_cards_repaired")

        pull_changes = cls(
            merged_card_ids=merged_card_ids,
            kept_out_of_trash_card_ids=kept_out_of_trash_card_ids,
            restored_from_trash_card_ids=restored_from_trash_card_ids,
            conflict_copy_ids=conflict_copy_ids,
            hidden_cards_repaired=hidden_cards_repaired,
        )

        return pull_changes
