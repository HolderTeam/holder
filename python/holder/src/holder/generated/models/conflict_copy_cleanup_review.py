from __future__ import annotations

from collections.abc import Mapping
from typing import TYPE_CHECKING, Any, TypeVar, cast

from attrs import define as _attrs_define
from typing_extensions import Self

if TYPE_CHECKING:
    from ..models.card_summary import CardSummary
    from ..models.link import Link
    from ..models.milestone import Milestone


T = TypeVar("T", bound="ConflictCopyCleanupReview")


@_attrs_define
class ConflictCopyCleanupReview:
    """What stays on the preserved copy or is affected if the copy moves to Trash.

    Attributes:
        cleanup_token (str): An opaque token for the saved result, preserved copy, and the reviewed cleanup effects.
        copy_links (list[Link]): Outgoing links and attachments without an equivalent on the original.
        copy_milestones (list[Milestone]): Milestones without an equivalent kind, description and dates on the original.
        backlinks (list[Link]): Live links on this device that point to the copy itself.
        children (list[CardSummary]): Live children that would move up a level using the normal Trash behavior.
        promotion_parent (CardSummary | None): Where children of the copy would move, or null for the project root.
        placement_differs (bool): Whether the parent or position of the copy differs from that of the original.
        preserves_original (bool): Whether moving the copy to Trash can leave the original's placement untouched.
        copy_tags (list[str]): Tags present on the copy and absent from the saved original.
    """

    cleanup_token: str
    copy_links: list[Link]
    copy_milestones: list[Milestone]
    backlinks: list[Link]
    children: list[CardSummary]
    promotion_parent: CardSummary | None
    placement_differs: bool
    preserves_original: bool
    copy_tags: list[str]

    def to_dict(self) -> dict[str, Any]:
        from ..models.card_summary import CardSummary

        cleanup_token = self.cleanup_token

        copy_links = []
        for copy_links_item_data in self.copy_links:
            copy_links_item = copy_links_item_data.to_dict()
            copy_links.append(copy_links_item)

        copy_milestones = []
        for copy_milestones_item_data in self.copy_milestones:
            copy_milestones_item = copy_milestones_item_data.to_dict()
            copy_milestones.append(copy_milestones_item)

        backlinks = []
        for backlinks_item_data in self.backlinks:
            backlinks_item = backlinks_item_data.to_dict()
            backlinks.append(backlinks_item)

        children = []
        for children_item_data in self.children:
            children_item = children_item_data.to_dict()
            children.append(children_item)

        promotion_parent: dict[str, Any] | None
        if isinstance(self.promotion_parent, CardSummary):
            promotion_parent = self.promotion_parent.to_dict()
        else:
            promotion_parent = self.promotion_parent

        placement_differs = self.placement_differs

        preserves_original = self.preserves_original

        copy_tags = self.copy_tags

        field_dict: dict[str, Any] = {}

        field_dict.update(
            {
                "cleanup_token": cleanup_token,
                "copy_links": copy_links,
                "copy_milestones": copy_milestones,
                "backlinks": backlinks,
                "children": children,
                "promotion_parent": promotion_parent,
                "placement_differs": placement_differs,
                "preserves_original": preserves_original,
                "copy_tags": copy_tags,
            }
        )

        return field_dict

    @classmethod
    def from_dict(cls, src_dict: Mapping[str, Any]) -> Self:
        from ..models.card_summary import CardSummary
        from ..models.link import Link
        from ..models.milestone import Milestone

        d = dict(src_dict)
        cleanup_token = d.pop("cleanup_token")

        copy_links = []
        _copy_links = d.pop("copy_links")
        for copy_links_item_data in _copy_links:
            copy_links_item = Link.from_dict(copy_links_item_data)

            copy_links.append(copy_links_item)

        copy_milestones = []
        _copy_milestones = d.pop("copy_milestones")
        for copy_milestones_item_data in _copy_milestones:
            copy_milestones_item = Milestone.from_dict(copy_milestones_item_data)

            copy_milestones.append(copy_milestones_item)

        backlinks = []
        _backlinks = d.pop("backlinks")
        for backlinks_item_data in _backlinks:
            backlinks_item = Link.from_dict(backlinks_item_data)

            backlinks.append(backlinks_item)

        children = []
        _children = d.pop("children")
        for children_item_data in _children:
            children_item = CardSummary.from_dict(children_item_data)

            children.append(children_item)

        def _parse_promotion_parent(data: object) -> CardSummary | None:
            if data is None:
                return data
            try:
                if not isinstance(data, dict):
                    raise TypeError()
                promotion_parent_type_0 = CardSummary.from_dict(data)

                return promotion_parent_type_0
            except (TypeError, ValueError, AttributeError, KeyError):
                pass
            return cast(CardSummary | None, data)

        promotion_parent = _parse_promotion_parent(d.pop("promotion_parent"))

        placement_differs = d.pop("placement_differs")

        preserves_original = d.pop("preserves_original")

        copy_tags = cast(list[str], d.pop("copy_tags"))

        conflict_copy_cleanup_review = cls(
            cleanup_token=cleanup_token,
            copy_links=copy_links,
            copy_milestones=copy_milestones,
            backlinks=backlinks,
            children=children,
            promotion_parent=promotion_parent,
            placement_differs=placement_differs,
            preserves_original=preserves_original,
            copy_tags=copy_tags,
        )

        return conflict_copy_cleanup_review
