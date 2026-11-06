from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar, cast

from attrs import define as _attrs_define
from typing_extensions import Self

from ..models.change_event_data_entity import ChangeEventDataEntity

T = TypeVar("T", bound="ChangeEventData")


@_attrs_define
class ChangeEventData:
    """One entity invalidation from the live change feed.

    Attributes:
        entity (ChangeEventDataEntity): The changed entity type.
        entity_id (str): The changed entity ID.
        project_id (None | str): The owning project, when known.
        deleted (bool): Whether the entity disappeared from the observed database snapshot.
        git_revision (None | str): The observed project Git HEAD, when available.
    """

    entity: ChangeEventDataEntity
    entity_id: str
    project_id: None | str
    deleted: bool
    git_revision: None | str

    def to_dict(self) -> dict[str, Any]:
        entity = self.entity.value

        entity_id = self.entity_id

        project_id: None | str
        project_id = self.project_id

        deleted = self.deleted

        git_revision: None | str
        git_revision = self.git_revision

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "entity": entity,
                "entity_id": entity_id,
                "project_id": project_id,
                "deleted": deleted,
                "git_revision": git_revision,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        entity = ChangeEventDataEntity(d.pop("entity"))

        entity_id = d.pop("entity_id")

        def _parse_project_id(data: object) -> None | str:
            if data is None:
                return data
            return cast(None | str, data)

        project_id = _parse_project_id(d.pop("project_id"))

        deleted = d.pop("deleted")

        def _parse_git_revision(data: object) -> None | str:
            if data is None:
                return data
            return cast(None | str, data)

        git_revision = _parse_git_revision(d.pop("git_revision"))

        change_event_data = cls(
            entity=entity,
            entity_id=entity_id,
            project_id=project_id,
            deleted=deleted,
            git_revision=git_revision,
        )

        return change_event_data
