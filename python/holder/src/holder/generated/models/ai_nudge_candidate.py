from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING, Any, TypeVar

from attrs import define as _attrs_define
from attrs import field as _attrs_field
from typing_extensions import Self

from ..types import UNSET, Unset

if TYPE_CHECKING:
    from ..models.ai_nudge_candidate_facts import AiNudgeCandidateFacts


T = TypeVar("T", bound="AiNudgeCandidate")


@_attrs_define
class AiNudgeCandidate:
    """
    Attributes:
        kind (str): The candidate's kind, such as card.title_only; an unknown kind is not accepted.
        project_id (str):
        created_at (int):
        facts (AiNudgeCandidateFacts): The evidence for the candidate, which depends on its kind.
        card_id (str | Unset):
        basis_fingerprint (str | Unset):
        basis_commit (str | Unset):
    """

    kind: str
    project_id: str
    created_at: int
    facts: AiNudgeCandidateFacts
    card_id: str | Unset = UNSET
    basis_fingerprint: str | Unset = UNSET
    basis_commit: str | Unset = UNSET
    additional_properties: dict[str, Any] = _attrs_field(init=False, factory=dict)

    def to_dict(self) -> dict[str, Any]:
        kind = self.kind

        project_id = self.project_id

        created_at = self.created_at

        facts = self.facts.to_dict()

        card_id = self.card_id

        basis_fingerprint = self.basis_fingerprint

        basis_commit = self.basis_commit

        field_dict: dict[str, Any] = {}
        field_dict.update(self.additional_properties)
        field_dict.update(
            {
                "kind": kind,
                "project_id": project_id,
                "created_at": created_at,
                "facts": facts,
            }
        )
        if card_id is not UNSET:
            field_dict["card_id"] = card_id
        if basis_fingerprint is not UNSET:
            field_dict["basis_fingerprint"] = basis_fingerprint
        if basis_commit is not UNSET:
            field_dict["basis_commit"] = basis_commit

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        from ..models.ai_nudge_candidate_facts import (
            AiNudgeCandidateFacts,
        )

        d = dict(src_dict)
        kind = d.pop("kind")

        project_id = d.pop("project_id")

        created_at = d.pop("created_at")

        facts = AiNudgeCandidateFacts.from_dict(d.pop("facts"))

        card_id = d.pop("card_id", UNSET)

        basis_fingerprint = d.pop("basis_fingerprint", UNSET)

        basis_commit = d.pop("basis_commit", UNSET)

        ai_nudge_candidate = cls(
            kind=kind,
            project_id=project_id,
            created_at=created_at,
            facts=facts,
            card_id=card_id,
            basis_fingerprint=basis_fingerprint,
            basis_commit=basis_commit,
        )

        ai_nudge_candidate.additional_properties = d
        return ai_nudge_candidate

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
