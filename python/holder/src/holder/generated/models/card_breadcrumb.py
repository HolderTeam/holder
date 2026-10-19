from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar

from attrs import define as _attrs_define
from attrs import field as _attrs_field
from typing_extensions import Self

from ..models.card_breadcrumb_kind import CardBreadcrumbKind

T = TypeVar("T", bound="CardBreadcrumb")


@_attrs_define
class CardBreadcrumb:
    """
    Attributes:
        kind (CardBreadcrumbKind):
        id (str):
        title (str):
    """

    kind: CardBreadcrumbKind
    id: str
    title: str
    additional_properties: dict[str, Any] = _attrs_field(init=False, factory=dict)

    def to_dict(self) -> dict[str, Any]:
        kind = self.kind.value

        id = self.id

        title = self.title

        field_dict: dict[str, Any] = {}
        field_dict.update(self.additional_properties)
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

        card_breadcrumb.additional_properties = d
        return card_breadcrumb

    @property
    def additional_keys(self) -> list[str]:
        return list(self.additional_properties.keys())

    def __getitem__(self, key: str) -> Any:
        return self.additional_properties[key]

    def __setitem__(self, key: str, value: Any) -> None:
        self.additional_properties[key] = value

    def __delitem__(self, key: str) -> None:
        del self.additional_properties[key]

    def __contains__(self, key: str) -> bool:
        return key in self.additional_properties
