from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING, Any, TypeVar, cast

from attrs import define as _attrs_define
from typing_extensions import Self

from ..models.pull_phase_status import PullPhaseStatus

if TYPE_CHECKING:
    from ..models.pull_changes import PullChanges


T = TypeVar("T", bound="PullPhase")


@_attrs_define
class PullPhase:
    """What a pull did.

    Attributes:
        attempted (bool): Whether a pull was tried.
        status (PullPhaseStatus): How the pull ended.
        conflicts_resolved (int): Local card versions actually preserved as new cards by this pull.
        hidden_cards_repaired (int): Live cards the pull left under a missing or trashed parent, made reachable again.
        changes (PullChanges): What a pull changed on this device to bring two devices' versions of the same cards
            together. Each card
            change is its own commit, so card history can undo it.
        error_message (None | str): Why the pull failed; null when it did not.
    """

    attempted: bool
    status: PullPhaseStatus
    conflicts_resolved: int
    hidden_cards_repaired: int
    changes: PullChanges
    error_message: None | str

    def to_dict(self) -> dict[str, Any]:
        attempted = self.attempted

        status = self.status.value

        conflicts_resolved = self.conflicts_resolved

        hidden_cards_repaired = self.hidden_cards_repaired

        changes = self.changes.to_dict()

        error_message: None | str
        error_message = self.error_message

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "attempted": attempted,
                "status": status,
                "conflicts_resolved": conflicts_resolved,
                "hidden_cards_repaired": hidden_cards_repaired,
                "changes": changes,
                "error_message": error_message,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        from ..models.pull_changes import PullChanges

        d = dict(src_dict)
        attempted = d.pop("attempted")

        status = PullPhaseStatus(d.pop("status"))

        conflicts_resolved = d.pop("conflicts_resolved")

        hidden_cards_repaired = d.pop("hidden_cards_repaired")

        changes = PullChanges.from_dict(d.pop("changes"))

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
            changes=changes,
            error_message=error_message,
        )

        return pull_phase
