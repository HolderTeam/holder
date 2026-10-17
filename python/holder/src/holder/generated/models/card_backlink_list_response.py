from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING, Any, TypeVar

from attrs import define as _attrs_define
from attrs import field as _attrs_field
from typing_extensions import Self

from ..models.card_backlink_list_response_coverage import (
    CardBacklinkListResponseCoverage,
)
from ..models.card_backlink_list_response_scope import CardBacklinkListResponseScope

if TYPE_CHECKING:
    from ..models.card_link import CardLink


T = TypeVar("T", bound="CardBacklinkListResponse")


@_attrs_define
class CardBacklinkListResponse:
    """
    Attributes:
        ok (bool):
        data (list[CardLink]):
        scope (CardBacklinkListResponseScope):
        coverage (CardBacklinkListResponseCoverage): External backlinks cover only source projects available to this
            daemon request, never all possible projects.
    """

    ok: bool
    data: list[CardLink]
    scope: CardBacklinkListResponseScope
    coverage: CardBacklinkListResponseCoverage
    additional_properties: dict[str, Any] = _attrs_field(init=False, factory=dict)

    def to_dict(self) -> dict[str, Any]:
        ok = self.ok

        data = []
        for data_item_data in self.data:
            data_item = data_item_data.to_dict()
            data.append(data_item)

        scope = self.scope.value

        coverage = self.coverage.value

        field_dict: dict[str, Any] = {}
        field_dict.update(self.additional_properties)
        field_dict.update(
            {
                "ok": ok,
                "data": data,
                "scope": scope,
                "coverage": coverage,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        from ..models.card_link import CardLink

        d = dict(src_dict)
        ok = d.pop("ok")

        data = []
        _data = d.pop("data")
        for data_item_data in _data:
            data_item = CardLink.from_dict(data_item_data)

            data.append(data_item)

        scope = CardBacklinkListResponseScope(d.pop("scope"))

        coverage = CardBacklinkListResponseCoverage(d.pop("coverage"))

        card_backlink_list_response = cls(
            ok=ok,
            data=data,
            scope=scope,
            coverage=coverage,
        )

        card_backlink_list_response.additional_properties = d
        return card_backlink_list_response

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
