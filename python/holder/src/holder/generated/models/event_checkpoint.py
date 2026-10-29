from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING, Any, TypeVar

from attrs import define as _attrs_define
from typing_extensions import Self

if TYPE_CHECKING:
    from ..models.event_checkpoint_git_revisions import EventCheckpointGitRevisions
    from ..models.event_checkpoint_history_urls import EventCheckpointHistoryUrls


T = TypeVar("T", bound="EventCheckpoint")


@_attrs_define
class EventCheckpoint:
    """A replay cursor and the observed Git state.

    Attributes:
        cursor (str): Opaque process-local SSE cursor; preserve it for Last-Event-ID.
        git_revisions (EventCheckpointGitRevisions): Observed project Git HEADs, null for missing or unborn
            repositories.
        history_urls (EventCheckpointHistoryUrls): Project history endpoints for durable recovery.
    """

    cursor: str
    git_revisions: EventCheckpointGitRevisions
    history_urls: EventCheckpointHistoryUrls

    def to_dict(self) -> dict[str, Any]:
        cursor = self.cursor

        git_revisions = self.git_revisions.to_dict()

        history_urls = self.history_urls.to_dict()

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "cursor": cursor,
                "git_revisions": git_revisions,
                "history_urls": history_urls,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        from ..models.event_checkpoint_git_revisions import (
            EventCheckpointGitRevisions,
        )
        from ..models.event_checkpoint_history_urls import (
            EventCheckpointHistoryUrls,
        )

        d = dict(src_dict)
        cursor = d.pop("cursor")

        git_revisions = EventCheckpointGitRevisions.from_dict(d.pop("git_revisions"))

        history_urls = EventCheckpointHistoryUrls.from_dict(d.pop("history_urls"))

        event_checkpoint = cls(
            cursor=cursor,
            git_revisions=git_revisions,
            history_urls=history_urls,
        )

        return event_checkpoint
