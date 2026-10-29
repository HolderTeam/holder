from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar, cast

from attrs import define as _attrs_define
from attrs import field as _attrs_field
from typing_extensions import Self

from ..types import UNSET, Unset

T = TypeVar("T", bound="CardLinkDeleteRequest")


@_attrs_define
class CardLinkDeleteRequest:
    """CardLinkDeleteRequest.

    Attributes:
        to_card_id (None | str | Unset): The to card id.
        target_project_id (None | str | Unset): Selects the target project. Requires to_card_id, to_type and kind; use
            it to distinguish equal card IDs in different projects.
        to_type (None | str | Unset): The to type.
        kind (None | str | Unset): The kind.
    """

    to_card_id: None | str | Unset = UNSET
    target_project_id: None | str | Unset = UNSET
    to_type: None | str | Unset = UNSET
    kind: None | str | Unset = UNSET
    additional_properties: dict[str, Any] = _attrs_field(init=False, factory=dict)

    def to_dict(self) -> dict[str, Any]:
        to_card_id: None | str | Unset
        if isinstance(self.to_card_id, Unset):
            to_card_id = UNSET
        else:
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

        field_dict: dict[str, Any] = {}
        field_dict.update(self.additional_properties)
        field_dict.update({})
        if to_card_id is not UNSET:
            field_dict["to_card_id"] = to_card_id
        if target_project_id is not UNSET:
            field_dict["target_project_id"] = target_project_id
        if to_type is not UNSET:
            field_dict["to_type"] = to_type
        if kind is not UNSET:
            field_dict["kind"] = kind

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)

        def _parse_to_card_id(data: object) -> None | str | Unset:
            if data is None:
                return data
            if isinstance(data, Unset):
                return data
            return cast(None | str | Unset, data)

        to_card_id = _parse_to_card_id(d.pop("to_card_id", UNSET))

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

        card_link_delete_request = cls(
            to_card_id=to_card_id,
            target_project_id=target_project_id,
            to_type=to_type,
            kind=kind,
        )

        card_link_delete_request.additional_properties = d
        return card_link_delete_request

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
