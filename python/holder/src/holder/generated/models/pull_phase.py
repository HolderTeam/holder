from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar, cast

from attrs import define as _attrs_define
from typing_extensions import Self

from ..models.pull_phase_status import PullPhaseStatus

T = TypeVar("T", bound="PullPhase")


@_attrs_define
class PullPhase:
    """What a pull did.

    Attributes:
        attempted (bool): Whether a pull was tried.
        status (PullPhaseStatus): How the pull ended.
        conflicts_resolved (int): Cards changed both here and on the remote, merged by Holder's card rules.
        hidden_cards_repaired (int): Live cards the pull left under a missing or trashed parent, made reachable again.
        error_message (None | str): Why the pull failed; null when it did not.
    """

    attempted: bool
    status: PullPhaseStatus
    conflicts_resolved: int
    hidden_cards_repaired: int
    error_message: None | str

    def to_dict(self) -> dict[str, Any]:
        attempted = self.attempted

        status = self.status.value

        conflicts_resolved = self.conflicts_resolved

        hidden_cards_repaired = self.hidden_cards_repaired

        error_message: None | str
        error_message = self.error_message

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "attempted": attempted,
                "status": status,
                "conflicts_resolved": conflicts_resolved,
                "hidden_cards_repaired": hidden_cards_repaired,
                "error_message": error_message,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        attempted = d.pop("attempted")

        status = PullPhaseStatus(d.pop("status"))

        conflicts_resolved = d.pop("conflicts_resolved")

        hidden_cards_repaired = d.pop("hidden_cards_repaired")

        def _parse_error_message(data: object) -> None | str:
            if data is None:
                return data
            return cast(None | str, data)

        error_message = _parse_error_message(d.pop("error_message"))

        pull_phase = cls(
            attempted=attempted,
            status=status,
            conflicts_resolved=conflicts_resolved,
            hidden_cards_repaired=hidden_cards_repaired,
            error_message=error_message,
        )

        return pull_phase
