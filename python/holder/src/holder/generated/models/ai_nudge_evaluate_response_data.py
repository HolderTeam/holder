from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING, Any, TypeVar, cast

from attrs import define as _attrs_define
from attrs import field as _attrs_field
from typing_extensions import Self

if TYPE_CHECKING:
    from ..models.ai_nudge import AiNudge


T = TypeVar("T", bound="AiNudgeEvaluateResponseData")


@_attrs_define
class AiNudgeEvaluateResponseData:
    """
    Attributes:
        kind (str):
        accepted (bool):
        should_nudge (bool):
        reason (str):
        nudge (AiNudge | None): The recorded nudge, or null when there is nothing to show.
    """

    kind: str
    accepted: bool
    should_nudge: bool
    reason: str
    nudge: AiNudge | None
    additional_properties: dict[str, Any] = _attrs_field(init=False, factory=dict)

    def to_dict(self) -> dict[str, Any]:
        from ..models.ai_nudge import AiNudge

        kind = self.kind

        accepted = self.accepted

        should_nudge = self.should_nudge

        reason = self.reason

        nudge: dict[str, Any] | None
        if isinstance(self.nudge, AiNudge):
            nudge = self.nudge.to_dict()
        else:
            nudge = self.nudge

        field_dict: dict[str, Any] = {}
        field_dict.update(self.additional_properties)
        field_dict.update(
            {
                "kind": kind,
                "accepted": accepted,
                "should_nudge": should_nudge,
                "reason": reason,
                "nudge": nudge,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        from ..models.ai_nudge import AiNudge

        d = dict(src_dict)
        kind = d.pop("kind")

        accepted = d.pop("accepted")

        should_nudge = d.pop("should_nudge")

        reason = d.pop("reason")

        def _parse_nudge(data: object) -> AiNudge | None:
            if data is None:
                return data
            try:
                if not isinstance(data, dict):
                    raise TypeError()
                nudge_type_0 = AiNudge.from_dict(data)

                return nudge_type_0
            except (TypeError, ValueError, AttributeError, KeyError):
                pass
            return cast(AiNudge | None, data)

        nudge = _parse_nudge(d.pop("nudge"))

        ai_nudge_evaluate_response_data = cls(
            kind=kind,
            accepted=accepted,
            should_nudge=should_nudge,
            reason=reason,
            nudge=nudge,
        )

        ai_nudge_evaluate_response_data.additional_properties = d
        return ai_nudge_evaluate_response_data

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
