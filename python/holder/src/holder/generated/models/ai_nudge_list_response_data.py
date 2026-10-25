from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING, Any, TypeVar

from attrs import define as _attrs_define
from attrs import field as _attrs_field
from typing_extensions import Self

if TYPE_CHECKING:
    from ..models.ai_nudge import AiNudge


T = TypeVar("T", bound="AiNudgeListResponseData")


@_attrs_define
class AiNudgeListResponseData:
    """
    Attributes:
        nudges (list[AiNudge]):
    """

    nudges: list[AiNudge]
    additional_properties: dict[str, Any] = _attrs_field(init=False, factory=dict)

    def to_dict(self) -> dict[str, Any]:
        nudges = []
        for nudges_item_data in self.nudges:
            nudges_item = nudges_item_data.to_dict()
            nudges.append(nudges_item)

        field_dict: dict[str, Any] = {}
        field_dict.update(self.additional_properties)
        field_dict.update(
            {
                "nudges": nudges,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        from ..models.ai_nudge import AiNudge

        d = dict(src_dict)
        nudges = []
        _nudges = d.pop("nudges")
        for nudges_item_data in _nudges:
            nudges_item = AiNudge.from_dict(nudges_item_data)

            nudges.append(nudges_item)

        ai_nudge_list_response_data = cls(
            nudges=nudges,
        )

        ai_nudge_list_response_data.additional_properties = d
        return ai_nudge_list_response_data

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
