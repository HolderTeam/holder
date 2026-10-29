from __future__ import annotations

from collections.abc import Mapping
from typing import (
    Any,
    Literal,
    TypeVar,
    cast,
)

from attrs import define as _attrs_define
from attrs import field as _attrs_field
from typing_extensions import Self

T = TypeVar("T", bound="AiStreamResyncData")


@_attrs_define
class AiStreamResyncData:
    """AI StreamResyncData.

    Attributes:
        reason (Literal['history_unavailable']): The stream's retained history no longer covers Last-Event-ID, for
            example after a daemon restart: fetch the run, then reconnect without Last-Event-ID.
    """

    reason: Literal["history_unavailable"]
    additional_properties: dict[str, Any] = _attrs_field(init=False, factory=dict)

    def to_dict(self) -> dict[str, Any]:
        reason = self.reason

        field_dict: dict[str, Any] = {}
        field_dict.update(self.additional_properties)
        field_dict.update(
            {
                "reason": reason,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        reason = cast(Literal["history_unavailable"], d.pop("reason"))
        if reason != "history_unavailable":
            raise ValueError(
                f"reason must match const 'history_unavailable', got '{reason}'"
            )

        ai_stream_resync_data = cls(
            reason=reason,
        )

        ai_stream_resync_data.additional_properties = d
        return ai_stream_resync_data

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
