from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar

from attrs import define as _attrs_define
from typing_extensions import Self

from ..models.card_reference_resolve_request_scope import (
    CardReferenceResolveRequestScope,
)

T = TypeVar("T", bound="CardReferenceResolveRequest")


@_attrs_define
class CardReferenceResolveRequest:
    """A reference to resolve among one project's cards.

    Attributes:
        project_id (str):
        reference (str): A full canonical UUID, eligible UUID prefix, or exact card title.
        scope (CardReferenceResolveRequestScope): Which cards may match.
    """

    project_id: str
    reference: str
    scope: CardReferenceResolveRequestScope

    def to_dict(self) -> dict[str, Any]:
        project_id = self.project_id

        reference = self.reference

        scope = self.scope.value

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "project_id": project_id,
                "reference": reference,
                "scope": scope,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        project_id = d.pop("project_id")

        reference = d.pop("reference")

        scope = CardReferenceResolveRequestScope(d.pop("scope"))

        card_reference_resolve_request = cls(
            project_id=project_id,
            reference=reference,
            scope=scope,
        )

        return card_reference_resolve_request
