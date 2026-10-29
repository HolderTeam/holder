from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING, Any, TypeVar, cast

from attrs import define as _attrs_define
from attrs import field as _attrs_field
from typing_extensions import Self

from ..types import UNSET, Unset

if TYPE_CHECKING:
    from ..models.ai_nudge_meta_json import AiNudgeMetaJson


T = TypeVar("T", bound="AiNudge")


@_attrs_define
class AiNudge:
    """AI Nudge.

    Attributes:
        nudge_id (str): The nudge id.
        kind (str): The kind.
        project_id (str): The project id.
        card_id (None | str): The card id.
        title (str): The title.
        body (str): The body.
        meta_json (AiNudgeMetaJson): The evidence and details the nudge was built from.
        basis_fingerprint (None | str): The basis fingerprint.
        basis_commit (None | str): The basis commit.
        created_at (int): The created at.
        suggestions (list[str] | None | Unset): Suggested titles, for a card.title_suggestion nudge.
    """

    nudge_id: str
    kind: str
    project_id: str
    card_id: None | str
    title: str
    body: str
    meta_json: AiNudgeMetaJson
    basis_fingerprint: None | str
    basis_commit: None | str
    created_at: int
    suggestions: list[str] | None | Unset = UNSET
    additional_properties: dict[str, Any] = _attrs_field(init=False, factory=dict)

    def to_dict(self) -> dict[str, Any]:
        nudge_id = self.nudge_id

        kind = self.kind

        project_id = self.project_id

        card_id: None | str
        card_id = self.card_id

        title = self.title

        body = self.body

        meta_json = self.meta_json.to_dict()

        basis_fingerprint: None | str
        basis_fingerprint = self.basis_fingerprint

        basis_commit: None | str
        basis_commit = self.basis_commit

        created_at = self.created_at

        suggestions: list[str] | None | Unset
        if isinstance(self.suggestions, Unset):
            suggestions = UNSET
        elif isinstance(self.suggestions, list):
            suggestions = self.suggestions

        else:
            suggestions = self.suggestions

        field_dict: dict[str, Any] = {}
        field_dict.update(self.additional_properties)
        field_dict.update(
            {
                "nudge_id": nudge_id,
                "kind": kind,
                "project_id": project_id,
                "card_id": card_id,
                "title": title,
                "body": body,
                "meta_json": meta_json,
                "basis_fingerprint": basis_fingerprint,
                "basis_commit": basis_commit,
                "created_at": created_at,
            }
        )
        if suggestions is not UNSET:
            field_dict["suggestions"] = suggestions

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        from ..models.ai_nudge_meta_json import AiNudgeMetaJson

        d = dict(src_dict)
        nudge_id = d.pop("nudge_id")

        kind = d.pop("kind")

        project_id = d.pop("project_id")

        def _parse_card_id(data: object) -> None | str:
            if data is None:
                return data
            return cast(None | str, data)

        card_id = _parse_card_id(d.pop("card_id"))

        title = d.pop("title")

        body = d.pop("body")

        meta_json = AiNudgeMetaJson.from_dict(d.pop("meta_json"))

        def _parse_basis_fingerprint(data: object) -> None | str:
            if data is None:
                return data
            return cast(None | str, data)

        basis_fingerprint = _parse_basis_fingerprint(d.pop("basis_fingerprint"))

        def _parse_basis_commit(data: object) -> None | str:
            if data is None:
                return data
            return cast(None | str, data)

        basis_commit = _parse_basis_commit(d.pop("basis_commit"))

        created_at = d.pop("created_at")

        def _parse_suggestions(data: object) -> list[str] | None | Unset:
            if data is None:
                return data
            if isinstance(data, Unset):
                return data
            try:
                if not isinstance(data, list):
                    raise TypeError()
                suggestions_type_0 = cast(list[str], data)

                return suggestions_type_0
            except (TypeError, ValueError, AttributeError, KeyError):
                pass
            return cast(list[str] | None | Unset, data)

        suggestions = _parse_suggestions(d.pop("suggestions", UNSET))

        ai_nudge = cls(
            nudge_id=nudge_id,
            kind=kind,
            project_id=project_id,
            card_id=card_id,
            title=title,
            body=body,
            meta_json=meta_json,
            basis_fingerprint=basis_fingerprint,
            basis_commit=basis_commit,
            created_at=created_at,
            suggestions=suggestions,
        )

        ai_nudge.additional_properties = d
        return ai_nudge

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
