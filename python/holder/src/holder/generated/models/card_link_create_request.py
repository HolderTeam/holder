from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar, cast

from attrs import define as _attrs_define
from attrs import field as _attrs_field
from typing_extensions import Self

from ..types import UNSET, Unset

T = TypeVar("T", bound="CardLinkCreateRequest")


@_attrs_define
class CardLinkCreateRequest:
    """CardLinkCreateRequest.

    Attributes:
        to_card_id (str): The to card id.
        target_project_id (None | str | Unset): Optional for card targets. A named external target project need not be
            loaded locally.
        to_type (None | str | Unset): Optional; defaults to "card".
        kind (None | str | Unset): Optional; defaults to "ref".
        label (None | str | Unset): The label.
        created_at (int | None | Unset): Optional; server uses current time if omitted or 0.
    """

    to_card_id: str
    target_project_id: None | str | Unset = UNSET
    to_type: None | str | Unset = UNSET
    kind: None | str | Unset = UNSET
    label: None | str | Unset = UNSET
    created_at: int | None | Unset = UNSET
    additional_properties: dict[str, Any] = _attrs_field(init=False, factory=dict)

    def to_dict(self) -> dict[str, Any]:
        to_card_id = self.to_card_id

        target_project_id: None | str | Unset
        if isinstance(self.target_project_id, Unset):
            target_project_id = UNSET
        else:
            target_project_id = self.target_project_id

        to_type: None | str | Unset
        if isinstance(self.to_type, Unset):
            to_type = UNSET
        else:
            to_type = self.to_type

        kind: None | str | Unset
        if isinstance(self.kind, Unset):
            kind = UNSET
        else:
            kind = self.kind

        label: None | str | Unset
        if isinstance(self.label, Unset):
            label = UNSET
        else:
            label = self.label

        created_at: int | None | Unset
        if isinstance(self.created_at, Unset):
            created_at = UNSET
        else:
            created_at = self.created_at

        field_dict: dict[str, Any] = {}
        field_dict.update(self.additional_properties)
        field_dict.update(
            {
                "to_card_id": to_card_id,
            }
        )
        if target_project_id is not UNSET:
            field_dict["target_project_id"] = target_project_id
        if to_type is not UNSET:
            field_dict["to_type"] = to_type
        if kind is not UNSET:
            field_dict["kind"] = kind
        if label is not UNSET:
            field_dict["label"] = label
        if created_at is not UNSET:
            field_dict["created_at"] = created_at

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        to_card_id = d.pop("to_card_id")

        def _parse_target_project_id(data: object) -> None | str | Unset:
            if data is None:
                return data
            if isinstance(data, Unset):
                return data
            return cast(None | str | Unset, data)

        target_project_id = _parse_target_project_id(d.pop("target_project_id", UNSET))

        def _parse_to_type(data: object) -> None | str | Unset:
            if data is None:
                return data
            if isinstance(data, Unset):
                return data
            return cast(None | str | Unset, data)

        to_type = _parse_to_type(d.pop("to_type", UNSET))

        def _parse_kind(data: object) -> None | str | Unset:
            if data is None:
                return data
            if isinstance(data, Unset):
                return data
            return cast(None | str | Unset, data)

        kind = _parse_kind(d.pop("kind", UNSET))

        def _parse_label(data: object) -> None | str | Unset:
            if data is None:
                return data
            if isinstance(data, Unset):
                return data
            return cast(None | str | Unset, data)

        label = _parse_label(d.pop("label", UNSET))

        def _parse_created_at(data: object) -> int | None | Unset:
            if data is None:
                return data
            if isinstance(data, Unset):
                return data
            return cast(int | None | Unset, data)

        created_at = _parse_created_at(d.pop("created_at", UNSET))

        card_link_create_request = cls(
            to_card_id=to_card_id,
            target_project_id=target_project_id,
            to_type=to_type,
            kind=kind,
            label=label,
            created_at=created_at,
        )

        card_link_create_request.additional_properties = d
        return card_link_create_request

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
