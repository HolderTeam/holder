from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING, Any, TypeVar, cast

from attrs import define as _attrs_define
from typing_extensions import Self

if TYPE_CHECKING:
    from ..models.card_history_entry import CardHistoryEntry


T = TypeVar("T", bound="CardHistoryPage")


@_attrs_define
class CardHistoryPage:
    """One bounded page of saved card changes, newest first.

    Attributes:
        head_oid (None | str): The project HEAD captured for this page; null for an empty repository.
        entries (list[CardHistoryEntry]): The card changes on this page.
        next_cursor (None | str): The cursor for another page; null at the end.
        scan_limited (bool): Whether History reached its bounded scan before the end.
    """

    head_oid: None | str
    entries: list[CardHistoryEntry]
    next_cursor: None | str
    scan_limited: bool

    def to_dict(self) -> dict[str, Any]:
        head_oid: None | str
        head_oid = self.head_oid

        entries = []
        for entries_item_data in self.entries:
            entries_item = entries_item_data.to_dict()
            entries.append(entries_item)

        next_cursor: None | str
        next_cursor = self.next_cursor

        scan_limited = self.scan_limited

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "head_oid": head_oid,
                "entries": entries,
                "next_cursor": next_cursor,
                "scan_limited": scan_limited,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        from ..models.card_history_entry import CardHistoryEntry

        d = dict(src_dict)

        def _parse_head_oid(data: object) -> None | str:
            if data is None:
                return data
            return cast(None | str, data)

        head_oid = _parse_head_oid(d.pop("head_oid"))

        entries = []
        _entries = d.pop("entries")
        for entries_item_data in _entries:
            entries_item = CardHistoryEntry.from_dict(entries_item_data)

            entries.append(entries_item)

        def _parse_next_cursor(data: object) -> None | str:
            if data is None:
                return data
            return cast(None | str, data)

        next_cursor = _parse_next_cursor(d.pop("next_cursor"))

        scan_limited = d.pop("scan_limited")

        card_history_page = cls(
            head_oid=head_oid,
            entries=entries,
            next_cursor=next_cursor,
            scan_limited=scan_limited,
        )

        return card_history_page
