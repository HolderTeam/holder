from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING, Any, TypeVar, cast

from attrs import define as _attrs_define
from typing_extensions import Self

from ..models.event_resync_data_reason import EventResyncDataReason

if TYPE_CHECKING:
    from ..models.event_resync_data_git_revisions import EventResyncDataGitRevisions
    from ..models.event_resync_data_history_urls import EventResyncDataHistoryUrls


T = TypeVar("T", bound="EventResyncData")


@_attrs_define
class EventResyncData:
    """A feed checkpoint that requires a fresh snapshot.

    Attributes:
        cursor (str): Opaque process-local SSE cursor; preserve it for Last-Event-ID.
        git_revisions (EventResyncDataGitRevisions): Observed project Git HEADs, null for missing or unborn
            repositories.
        history_urls (EventResyncDataHistoryUrls): Project history endpoints for durable recovery.
        reason (EventResyncDataReason): Why replay is unavailable.
        last_revision (None | str): The last revision supplied by the caller, when any.
    """

    cursor: str
    git_revisions: EventResyncDataGitRevisions
    history_urls: EventResyncDataHistoryUrls
    reason: EventResyncDataReason
    last_revision: None | str

    def to_dict(self) -> dict[str, Any]:
        cursor = self.cursor

        git_revisions = self.git_revisions.to_dict()

        history_urls = self.history_urls.to_dict()

        reason = self.reason.value

        last_revision: None | str
        last_revision = self.last_revision

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "cursor": cursor,
                "git_revisions": git_revisions,
                "history_urls": history_urls,
                "reason": reason,
                "last_revision": last_revision,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        from ..models.event_resync_data_git_revisions import (
            EventResyncDataGitRevisions,
        )
        from ..models.event_resync_data_history_urls import (
            EventResyncDataHistoryUrls,
        )

        d = dict(src_dict)
        cursor = d.pop("cursor")

        git_revisions = EventResyncDataGitRevisions.from_dict(d.pop("git_revisions"))

        history_urls = EventResyncDataHistoryUrls.from_dict(d.pop("history_urls"))

        reason = EventResyncDataReason(d.pop("reason"))

        def _parse_last_revision(data: object) -> None | str:
            if data is None:
                return data
            return cast(None | str, data)

        last_revision = _parse_last_revision(d.pop("last_revision"))

        event_resync_data = cls(
            cursor=cursor,
            git_revisions=git_revisions,
            history_urls=history_urls,
            reason=reason,
            last_revision=last_revision,
        )

        return event_resync_data
