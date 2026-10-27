from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING, Any, TypeVar, cast

from attrs import define as _attrs_define
from typing_extensions import Self

if TYPE_CHECKING:
    from ..models.card_list_item import CardListItem


T = TypeVar("T", bound="CardListItemPage")


@_attrs_define
class CardListItemPage:
    """
    Attributes:
        items (list[CardListItem]): The page's items.
        next_cursor (None | str): Pass back to get the next page; null on the last page.
    """

    items: list[CardListItem]
    next_cursor: None | str

    def to_dict(self) -> dict[str, Any]:
        items = []
        for items_item_data in self.items:
            items_item = items_item_data.to_dict()
            items.append(items_item)

        next_cursor: None | str
        next_cursor = self.next_cursor

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "items": items,
                "next_cursor": next_cursor,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        from ..models.card_list_item import CardListItem

        d = dict(src_dict)
        items = []
        _items = d.pop("items")
        for items_item_data in _items:
            items_item = CardListItem.from_dict(items_item_data)

            items.append(items_item)

        def _parse_next_cursor(data: object) -> None | str:
            if data is None:
                return data
            return cast(None | str, data)

        next_cursor = _parse_next_cursor(d.pop("next_cursor"))

        card_list_item_page = cls(
            items=items,
            next_cursor=next_cursor,
        )

        return card_list_item_page
