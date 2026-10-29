from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar

from attrs import define as _attrs_define
from typing_extensions import Self

from ..models.card_breadcrumb_kind import CardBreadcrumbKind

T = TypeVar("T", bound="CardBreadcrumb")


@_attrs_define
class CardBreadcrumb:
    """One project or live card in a card's path from its project root.

    Attributes:
        kind (CardBreadcrumbKind): Whether this step names the project or a card.
        id (str): The project or card ID.
        title (str): The project or card title.
    """

    kind: CardBreadcrumbKind
    id: str
    title: str

    def to_dict(self) -> dict[str, Any]:
        kind = self.kind.value

        id = self.id

        title = self.title

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "kind": kind,
                "id": id,
                "title": title,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        kind = CardBreadcrumbKind(d.pop("kind"))

        id = d.pop("id")

        title = d.pop("title")

        card_breadcrumb = cls(
            kind=kind,
            id=id,
            title=title,
        )

        return card_breadcrumb
