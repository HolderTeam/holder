from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar

from attrs import define as _attrs_define
from typing_extensions import Self

T = TypeVar("T", bound="LinkKindLabels")


@_attrs_define
class LinkKindLabels:
    """One of the link kinds Holder knows, with how it reads from each end of a link. A link's kind is free text;
    a kind Holder does not know has no labels of its own.

        Attributes:
            kind (str):
            forward_label (str): How the link reads from the card it is from, such as Depends on.
            reverse_label (str): How the link reads from what it points at, such as Required by.
    """

    kind: str
    forward_label: str
    reverse_label: str

    def to_dict(self) -> dict[str, Any]:
        kind = self.kind

        forward_label = self.forward_label

        reverse_label = self.reverse_label

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "kind": kind,
                "forward_label": forward_label,
                "reverse_label": reverse_label,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        kind = d.pop("kind")

        forward_label = d.pop("forward_label")

        reverse_label = d.pop("reverse_label")

        link_kind_labels = cls(
            kind=kind,
            forward_label=forward_label,
            reverse_label=reverse_label,
        )

        return link_kind_labels
