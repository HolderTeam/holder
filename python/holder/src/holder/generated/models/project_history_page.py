from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING, Any, TypeVar, cast

from attrs import define as _attrs_define
from typing_extensions import Self

if TYPE_CHECKING:
    from ..models.project_activity import ProjectActivity


T = TypeVar("T", bound="ProjectHistoryPage")


@_attrs_define
class ProjectHistoryPage:
    """One bounded page of project activity, newest first.

    Attributes:
        head_oid (None | str): The project HEAD captured for this page; null for an empty repository.
        activities (list[ProjectActivity]): The activities on this page, newest first.
        next_cursor (None | str): The cursor for another page; null at the end.
        scan_limited (bool): Whether History reached its bounded scan before the end.
    """

    head_oid: None | str
    activities: list[ProjectActivity]
    next_cursor: None | str
    scan_limited: bool

    def to_dict(self) -> dict[str, Any]:
        head_oid: None | str
        head_oid = self.head_oid

        activities = []
        for activities_item_data in self.activities:
            activities_item = activities_item_data.to_dict()
            activities.append(activities_item)

        next_cursor: None | str
        next_cursor = self.next_cursor

        scan_limited = self.scan_limited

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "head_oid": head_oid,
                "activities": activities,
                "next_cursor": next_cursor,
                "scan_limited": scan_limited,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        from ..models.project_activity import ProjectActivity

        d = dict(src_dict)

        def _parse_head_oid(data: object) -> None | str:
            if data is None:
                return data
            return cast(None | str, data)

        head_oid = _parse_head_oid(d.pop("head_oid"))

        activities = []
        _activities = d.pop("activities")
        for activities_item_data in _activities:
            activities_item = ProjectActivity.from_dict(activities_item_data)

            activities.append(activities_item)

        def _parse_next_cursor(data: object) -> None | str:
            if data is None:
                return data
            return cast(None | str, data)

        next_cursor = _parse_next_cursor(d.pop("next_cursor"))

        scan_limited = d.pop("scan_limited")

        project_history_page = cls(
            head_oid=head_oid,
            activities=activities,
            next_cursor=next_cursor,
            scan_limited=scan_limited,
        )

        return project_history_page
