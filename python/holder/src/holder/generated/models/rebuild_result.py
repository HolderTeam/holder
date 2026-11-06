from __future__ import annotations

from collections.abc import Mapping
from typing import Any, TypeVar

from attrs import define as _attrs_define
from typing_extensions import Self

T = TypeVar("T", bound="RebuildResult")


@_attrs_define
class RebuildResult:
    """The numbers of entities restored from a project's files.

    Attributes:
        project_id (str):
        cards (int): Cards restored.
        ai_messages (int): AI messages restored.
        ai_threads (int): AI threads restored.
        links (int): Links restored.
    """

    project_id: str
    cards: int
    ai_messages: int
    ai_threads: int
    links: int

    def to_dict(self) -> dict[str, Any]:
        project_id = self.project_id

        cards = self.cards

        ai_messages = self.ai_messages

        ai_threads = self.ai_threads

        links = self.links

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "project_id": project_id,
                "cards": cards,
                "ai_messages": ai_messages,
                "ai_threads": ai_threads,
                "links": links,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        d = dict(src_dict)
        project_id = d.pop("project_id")

        cards = d.pop("cards")

        ai_messages = d.pop("ai_messages")

        ai_threads = d.pop("ai_threads")

        links = d.pop("links")

        rebuild_result = cls(
            project_id=project_id,
            cards=cards,
            ai_messages=ai_messages,
            ai_threads=ai_threads,
            links=links,
        )

        return rebuild_result
