from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar

from attrs import define as _attrs_define
from attrs import field as _attrs_field
from typing_extensions import Self

T = TypeVar("T", bound="CardTagOccurrence")


@_attrs_define
class CardTagOccurrence:
    """
    Attributes:
        tag (str):
        byte_start (int): Inclusive byte offset of the leading # in content.
        byte_end (int): Exclusive byte offset after the tag name.
    """

    tag: str
    byte_start: int
    byte_end: int
    additional_properties: dict[str, Any] = _attrs_field(init=False, factory=dict)

    def to_dict(self) -> dict[str, Any]:
        tag = self.tag

        byte_start = self.byte_start

        byte_end = self.byte_end

        field_dict: dict[str, Any] = {}
        field_dict.update(self.additional_properties)
        field_dict.update(
            {
                "tag": tag,
                "byte_start": byte_start,
                "byte_end": byte_end,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        tag = d.pop("tag")

        byte_start = d.pop("byte_start")

        byte_end = d.pop("byte_end")

        card_tag_occurrence = cls(
            tag=tag,
            byte_start=byte_start,
            byte_end=byte_end,
        )

        card_tag_occurrence.additional_properties = d
        return card_tag_occurrence

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
