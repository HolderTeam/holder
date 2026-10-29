from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING, Any, TypeVar

from attrs import define as _attrs_define
from attrs import field as _attrs_field
from typing_extensions import Self

if TYPE_CHECKING:
    from ..models.ai_runner import AiRunner


T = TypeVar("T", bound="AiRunnerListResponseData")


@_attrs_define
class AiRunnerListResponseData:
    """AI RunnerList responseData.

    Attributes:
        runners (list[AiRunner]): The runners.
    """

    runners: list[AiRunner]
    additional_properties: dict[str, Any] = _attrs_field(init=False, factory=dict)

    def to_dict(self) -> dict[str, Any]:
        runners = []
        for runners_item_data in self.runners:
            runners_item = runners_item_data.to_dict()
            runners.append(runners_item)

        field_dict: dict[str, Any] = {}
        field_dict.update(self.additional_properties)
        field_dict.update(
            {
                "runners": runners,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        from ..models.ai_runner import AiRunner

        d = dict(src_dict)
        runners = []
        _runners = d.pop("runners")
        for runners_item_data in _runners:
            runners_item = AiRunner.from_dict(runners_item_data)

            runners.append(runners_item)

        ai_runner_list_response_data = cls(
            runners=runners,
        )

        ai_runner_list_response_data.additional_properties = d
        return ai_runner_list_response_data

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
