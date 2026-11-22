from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING, Any, TypeVar

from attrs import define as _attrs_define
from typing_extensions import Self

from ..types import UNSET, Unset

if TYPE_CHECKING:
    from ..models.conflict_copy_backlink_source import ConflictCopyBacklinkSource
    from ..models.conflict_copy_child_choice import ConflictCopyChildChoice
    from ..models.conflict_copy_link_target import ConflictCopyLinkTarget
    from ..models.conflict_copy_milestone_choice import ConflictCopyMilestoneChoice
    from ..models.conflict_copy_tag_choice import ConflictCopyTagChoice


T = TypeVar("T", bound="TrashConflictCopyRequest")


@_attrs_define
class TrashConflictCopyRequest:
    """Confirmation of the cleanup effects returned when the resolution was saved, and which of the copy's own
    details to carry over to the original first. Everything named must be in the reviewed cleanup; anything not
    carried over stays with the copy in Trash.

        Attributes:
            cleanup_token (str): The token returned with the cleanup review.
            carry_links (list[ConflictCopyLinkTarget] | Unset): Links from copy_links to make on the original, with their
                labels.
            carry_milestones (list[ConflictCopyMilestoneChoice] | Unset): Milestones from copy_milestones to put on the
                original, as new milestones with the same dates.
            carry_tags (list[ConflictCopyTagChoice] | Unset): Tags from copy_tags to add to the original.
            adopt_children (list[ConflictCopyChildChoice] | Unset): Children of the copy to move under the original instead
                of up a level.
            retarget_backlinks (list[ConflictCopyBacklinkSource] | Unset): Card links from backlinks to point at the
                original instead of the copy.
    """

    cleanup_token: str
    carry_links: list[ConflictCopyLinkTarget] | Unset = UNSET
    carry_milestones: list[ConflictCopyMilestoneChoice] | Unset = UNSET
    carry_tags: list[ConflictCopyTagChoice] | Unset = UNSET
    adopt_children: list[ConflictCopyChildChoice] | Unset = UNSET
    retarget_backlinks: list[ConflictCopyBacklinkSource] | Unset = UNSET

    def to_dict(self) -> dict[str, Any]:
        cleanup_token = self.cleanup_token

        carry_links: list[dict[str, Any]] | Unset = UNSET
        if not isinstance(self.carry_links, Unset):
            carry_links = []
            for carry_links_item_data in self.carry_links:
                carry_links_item = carry_links_item_data.to_dict()
                carry_links.append(carry_links_item)

        carry_milestones: list[dict[str, Any]] | Unset = UNSET
        if not isinstance(self.carry_milestones, Unset):
            carry_milestones = []
            for carry_milestones_item_data in self.carry_milestones:
                carry_milestones_item = carry_milestones_item_data.to_dict()
                carry_milestones.append(carry_milestones_item)

        carry_tags: list[dict[str, Any]] | Unset = UNSET
        if not isinstance(self.carry_tags, Unset):
            carry_tags = []
            for carry_tags_item_data in self.carry_tags:
                carry_tags_item = carry_tags_item_data.to_dict()
                carry_tags.append(carry_tags_item)

        adopt_children: list[dict[str, Any]] | Unset = UNSET
        if not isinstance(self.adopt_children, Unset):
            adopt_children = []
            for adopt_children_item_data in self.adopt_children:
                adopt_children_item = adopt_children_item_data.to_dict()
                adopt_children.append(adopt_children_item)

        retarget_backlinks: list[dict[str, Any]] | Unset = UNSET
        if not isinstance(self.retarget_backlinks, Unset):
            retarget_backlinks = []
            for retarget_backlinks_item_data in self.retarget_backlinks:
                retarget_backlinks_item = retarget_backlinks_item_data.to_dict()
                retarget_backlinks.append(retarget_backlinks_item)

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "cleanup_token": cleanup_token,
            }
        )
        if carry_links is not UNSET:
            field_dict["carry_links"] = carry_links
        if carry_milestones is not UNSET:
            field_dict["carry_milestones"] = carry_milestones
        if carry_tags is not UNSET:
            field_dict["carry_tags"] = carry_tags
        if adopt_children is not UNSET:
            field_dict["adopt_children"] = adopt_children
        if retarget_backlinks is not UNSET:
            field_dict["retarget_backlinks"] = retarget_backlinks

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        from ..models.conflict_copy_backlink_source import (
            ConflictCopyBacklinkSource,
        )
        from ..models.conflict_copy_child_choice import (
            ConflictCopyChildChoice,
        )
        from ..models.conflict_copy_link_target import (
            ConflictCopyLinkTarget,
        )
        from ..models.conflict_copy_milestone_choice import (
            ConflictCopyMilestoneChoice,
        )
        from ..models.conflict_copy_tag_choice import (
            ConflictCopyTagChoice,
        )

        d = dict(src_dict)
        cleanup_token = d.pop("cleanup_token")

        _carry_links = d.pop("carry_links", UNSET)
        carry_links: list[ConflictCopyLinkTarget] | Unset = UNSET
        if _carry_links is not UNSET:
            carry_links = []
            for carry_links_item_data in _carry_links:
                carry_links_item = ConflictCopyLinkTarget.from_dict(
                    carry_links_item_data
                )

                carry_links.append(carry_links_item)

        _carry_milestones = d.pop("carry_milestones", UNSET)
        carry_milestones: list[ConflictCopyMilestoneChoice] | Unset = UNSET
        if _carry_milestones is not UNSET:
            carry_milestones = []
            for carry_milestones_item_data in _carry_milestones:
                carry_milestones_item = ConflictCopyMilestoneChoice.from_dict(
                    carry_milestones_item_data
                )

                carry_milestones.append(carry_milestones_item)

        _carry_tags = d.pop("carry_tags", UNSET)
        carry_tags: list[ConflictCopyTagChoice] | Unset = UNSET
        if _carry_tags is not UNSET:
            carry_tags = []
            for carry_tags_item_data in _carry_tags:
                carry_tags_item = ConflictCopyTagChoice.from_dict(carry_tags_item_data)

                carry_tags.append(carry_tags_item)

        _adopt_children = d.pop("adopt_children", UNSET)
        adopt_children: list[ConflictCopyChildChoice] | Unset = UNSET
        if _adopt_children is not UNSET:
            adopt_children = []
            for adopt_children_item_data in _adopt_children:
                adopt_children_item = ConflictCopyChildChoice.from_dict(
                    adopt_children_item_data
                )

                adopt_children.append(adopt_children_item)

        _retarget_backlinks = d.pop("retarget_backlinks", UNSET)
        retarget_backlinks: list[ConflictCopyBacklinkSource] | Unset = UNSET
        if _retarget_backlinks is not UNSET:
            retarget_backlinks = []
            for retarget_backlinks_item_data in _retarget_backlinks:
                retarget_backlinks_item = ConflictCopyBacklinkSource.from_dict(
                    retarget_backlinks_item_data
                )

                retarget_backlinks.append(retarget_backlinks_item)

        trash_conflict_copy_request = cls(
            cleanup_token=cleanup_token,
            carry_links=carry_links,
            carry_milestones=carry_milestones,
            carry_tags=carry_tags,
            adopt_children=adopt_children,
            retarget_backlinks=retarget_backlinks,
        )

        return trash_conflict_copy_request
