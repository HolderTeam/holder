from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING, Any, TypeVar

from attrs import define as _attrs_define
from typing_extensions import Self

if TYPE_CHECKING:
    from ..models.event_checkpoint import EventCheckpoint


T = TypeVar("T", bound="EventCheckpointResponse")


@_attrs_define
class EventCheckpointResponse:
    """A change-feed checkpoint response.

    Attributes:
        ok (bool): Whether the request succeeded.
        data (EventCheckpoint): A replay cursor and the observed Git state.
    """

    ok: bool
    data: EventCheckpoint

    def to_dict(self) -> dict[str, Any]:
        ok = self.ok

        data = self.data.to_dict()

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "ok": ok,
                "data": data,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        from ..models.event_checkpoint import EventCheckpoint

        d = dict(src_dict)
        ok = d.pop("ok")

        data = EventCheckpoint.from_dict(d.pop("data"))

        event_checkpoint_response = cls(
            ok=ok,
            data=data,
        )

        return event_checkpoint_response
