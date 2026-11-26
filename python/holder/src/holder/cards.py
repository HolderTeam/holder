"""Shared, project-scoped Card operations for live and embedded sessions."""

from __future__ import annotations

from importlib import import_module
from importlib.resources import files
from os import PathLike, fspath
from typing import TYPE_CHECKING, Literal, Self, TypedDict, cast

from ._workspace_contract import (
    NATIVE_ERRORS,
    Card,
    CardBreadcrumb,
    CardListItem,
    CardMetadata,
    CardMove,
    CardOperations,
    CardReferenceCandidate,
    CardReferenceResolveData,
    CardReferenceResolveRequest,
    CardUpdate,
    Link,
    LinkKindLabels,
    LinkTargetType,
    Milestone,
    MilestoneDate,
    MilestoneUpdate,
    NewCard,
    NewLink,
    NewMilestone,
    Page,
    SearchCard,
    TagCount,
)
from ._workspace_contract import Project as ProjectRecord
from ._workspace_contract import TrashItem as TrashItemRecord
from .exceptions import APIError, ClosedError, HolderError, NotFoundError
from .generated.api.default import (
    delete_projects_project_id_cards_card_id,
    delete_projects_project_id_cards_card_id_links,
    delete_projects_project_id_cards_card_id_milestones_milestone_id,
    delete_projects_project_id_cards_card_id_tags_tag,
    get_projects_project_id_cards,
    get_projects_project_id_cards_card_id,
    get_projects_project_id_cards_card_id_backlinks,
    get_projects_project_id_cards_card_id_breadcrumbs,
    get_projects_project_id_cards_card_id_links,
    get_projects_project_id_cards_card_id_metadata,
    get_projects_project_id_cards_card_id_milestones,
    get_search_cards,
    patch_projects_project_id_cards_card_id,
    patch_projects_project_id_cards_card_id_milestones_milestone_id,
    post_card_references_resolve,
    post_projects_project_id_cards,
    post_projects_project_id_cards_card_id_links,
    post_projects_project_id_cards_card_id_milestones,
    post_projects_project_id_cards_card_id_move,
    post_projects_project_id_cards_card_id_restore,
    put_projects_project_id_cards_card_id_tags_tag,
)
from .generated.models import CardMove as HTTPCardMove
from .generated.models import (
    CardMoveIntent,
    CardReferenceResolveRequestScope,
    DeleteProjectsProjectIdCardsCardIdChildren,
    GetProjectsProjectIdCardsCardIdBacklinksScope,
    GetProjectsProjectIdCardsOrder,
)
from .generated.models import (
    CardReferenceResolveRequest as HTTPCardReferenceResolveRequest,
)
from .generated.models import CardUpdate as HTTPCardUpdate
from .generated.models import LinkTargetType as HTTPLinkTargetType
from .generated.models import MilestoneUpdate as HTTPMilestoneUpdate
from .generated.models import NewCard as HTTPNewCard
from .generated.models import NewLink as HTTPNewLink
from .generated.models import NewMilestone as HTTPNewMilestone

if TYPE_CHECKING:
    from holderkit._native import Context

    from .client import Holder

__all__ = [
    "Card",
    "CardBreadcrumb",
    "CardListItem",
    "CardMetadata",
    "CardMove",
    "CardOperations",
    "CardReferenceCandidate",
    "CardReferenceResolveData",
    "CardReferenceResolveRequest",
    "CardUpdate",
    "EmbeddedUnavailableError",
    "Link",
    "LinkTargetType",
    "Milestone",
    "MilestoneDate",
    "MilestoneUpdate",
    "NewCard",
    "NewLink",
    "NewMilestone",
    "Page",
    "SearchCard",
    "open_workspace",
]


class EmbeddedUnavailableError(HolderError):
    """This installation cannot open an embedded Holder workspace."""


class _DaemonCards:
    def __init__(self, holder: Holder) -> None:
        self._holder = holder

    def list_cards(
        self,
        project_id: str,
        *,
        parent: str | None = None,
        tag: str | None = None,
        order: Literal["card_id_asc", "updated_desc", "sort_key_asc"] = "card_id_asc",
        limit: int = 200,
        cursor: str | None = None,
        include_content: bool = False,
        include_child_count: bool = False,
    ) -> Page[CardListItem]:
        result = self._holder._call(
            get_projects_project_id_cards,
            project_id,
            parent=parent,
            tag=tag,
            order=GetProjectsProjectIdCardsOrder(order),
            limit=limit,
            cursor=cursor,
            include_content=include_content,
            include_child_count=include_child_count,
        )
        return cast("Page[CardListItem]", result.to_dict())

    def search_cards(
        self, project_id: str, q: str, *, limit: int = 20, offset: int = 0
    ) -> list[SearchCard]:
        results = self._holder._call(
            get_search_cards, project_id=project_id, q=q, limit=limit, offset=offset
        )
        return [cast("SearchCard", result.to_dict()) for result in results]

    def get_card(self, project_id: str, card_id: str) -> Card:
        result = self._holder._call(
            get_projects_project_id_cards_card_id, project_id, card_id
        )
        return cast("Card", result.to_dict())

    def get_card_metadata(self, project_id: str, card_id: str) -> CardMetadata:
        result = self._holder._call(
            get_projects_project_id_cards_card_id_metadata, project_id, card_id
        )
        return cast("CardMetadata", result.to_dict())

    def get_card_breadcrumbs(
        self, project_id: str, card_id: str
    ) -> list[CardBreadcrumb]:
        results = self._holder._call(
            get_projects_project_id_cards_card_id_breadcrumbs, project_id, card_id
        )
        return [cast("CardBreadcrumb", result.to_dict()) for result in results]

    def resolve_card_reference(
        self, request: CardReferenceResolveRequest
    ) -> CardReferenceResolveData:
        result = self._holder._call(
            post_card_references_resolve,
            body=HTTPCardReferenceResolveRequest(
                project_id=request["project_id"],
                reference=request["reference"],
                scope=CardReferenceResolveRequestScope(request["scope"]),
            ),
        )
        return cast("CardReferenceResolveData", result.to_dict())

    def create_card(self, project_id: str, card: NewCard) -> Card:
        result = self._holder._call(
            post_projects_project_id_cards,
            project_id,
            body=HTTPNewCard(**card),
            write=True,
        )
        return cast("Card", result.to_dict())

    def update_card(self, project_id: str, card_id: str, update: CardUpdate) -> Card:
        result = self._holder._call(
            patch_projects_project_id_cards_card_id,
            project_id,
            card_id,
            body=HTTPCardUpdate(**update),
            write=True,
        )
        return cast("Card", result.to_dict())

    def move_card(self, project_id: str, card_id: str, move: CardMove) -> Card:
        result = self._holder._call(
            post_projects_project_id_cards_card_id_move,
            project_id,
            card_id,
            body=HTTPCardMove(**{**move, "intent": CardMoveIntent(move["intent"])}),
            write=True,
        )
        return cast("Card", result.to_dict())

    def trash_card(
        self,
        project_id: str,
        card_id: str,
        *,
        children: Literal["promote", "trash_subtree"] = "promote",
    ) -> Card:
        result = self._holder._call(
            delete_projects_project_id_cards_card_id,
            project_id,
            card_id,
            children=DeleteProjectsProjectIdCardsCardIdChildren(children),
            write=True,
        )
        return cast("Card", result.to_dict())

    def restore_card(self, project_id: str, card_id: str) -> Card:
        result = self._holder._call(
            post_projects_project_id_cards_card_id_restore,
            project_id,
            card_id,
            write=True,
        )
        return cast("Card", result.to_dict())

    def add_card_tag(self, project_id: str, card_id: str, tag: str) -> Card:
        result = self._holder._call(
            put_projects_project_id_cards_card_id_tags_tag,
            project_id,
            card_id,
            tag,
            write=True,
        )
        return cast("Card", result.to_dict())

    def remove_card_tag(self, project_id: str, card_id: str, tag: str) -> Card:
        result = self._holder._call(
            delete_projects_project_id_cards_card_id_tags_tag,
            project_id,
            card_id,
            tag,
            write=True,
        )
        return cast("Card", result.to_dict())

    def list_card_links(
        self, project_id: str, card_id: str, *, include_deleted: bool = False
    ) -> list[Link]:
        results = self._holder._call(
            get_projects_project_id_cards_card_id_links,
            project_id,
            card_id,
            include_deleted=include_deleted,
        )
        return [cast("Link", result.to_dict()) for result in results]

    def list_card_backlinks(
        self,
        project_id: str,
        card_id: str,
        *,
        scope: Literal["local", "external", "all_available"] = "local",
        include_deleted: bool = False,
    ) -> list[Link]:
        results = self._holder._call(
            get_projects_project_id_cards_card_id_backlinks,
            project_id,
            card_id,
            scope=GetProjectsProjectIdCardsCardIdBacklinksScope(scope),
            include_deleted=include_deleted,
        )
        return [cast("Link", result.to_dict()) for result in results]

    def add_card_link(self, project_id: str, card_id: str, link: NewLink) -> Link:
        result = self._holder._call(
            post_projects_project_id_cards_card_id_links,
            project_id,
            card_id,
            body=HTTPNewLink(
                **{**link, "target_type": HTTPLinkTargetType(link["target_type"])}
            ),
            write=True,
        )
        return cast("Link", result.to_dict())

    def remove_card_link(
        self,
        project_id: str,
        card_id: str,
        target_type: LinkTargetType,
        target_id: str,
        *,
        kind: str = "ref",
        target_project_id: str | None = None,
    ) -> list[Link]:
        results = self._holder._call(
            delete_projects_project_id_cards_card_id_links,
            project_id,
            card_id,
            target_type=HTTPLinkTargetType(target_type),
            target_id=target_id,
            kind=kind,
            target_project_id=target_project_id,
            write=True,
        )
        return [cast("Link", result.to_dict()) for result in results]

    def list_card_milestones(self, project_id: str, card_id: str) -> list[Milestone]:
        results = self._holder._call(
            get_projects_project_id_cards_card_id_milestones, project_id, card_id
        )
        return [cast("Milestone", result.to_dict()) for result in results]

    def add_card_milestone(
        self, project_id: str, card_id: str, milestone: NewMilestone
    ) -> Milestone:
        result = self._holder._call(
            post_projects_project_id_cards_card_id_milestones,
            project_id,
            card_id,
            body=HTTPNewMilestone.from_dict(milestone),
            write=True,
        )
        return cast("Milestone", result.to_dict())

    def update_card_milestone(
        self, project_id: str, card_id: str, milestone_id: str, update: MilestoneUpdate
    ) -> Milestone:
        result = self._holder._call(
            patch_projects_project_id_cards_card_id_milestones_milestone_id,
            project_id,
            card_id,
            milestone_id,
            body=HTTPMilestoneUpdate.from_dict(update),
            write=True,
        )
        return cast("Milestone", result.to_dict())

    def remove_card_milestone(
        self, project_id: str, card_id: str, milestone_id: str
    ) -> list[Milestone]:
        results = self._holder._call(
            delete_projects_project_id_cards_card_id_milestones_milestone_id,
            project_id,
            card_id,
            milestone_id,
            write=True,
        )
        return [cast("Milestone", result.to_dict()) for result in results]


class EmbeddedWorkspace:
    """A session in an explicitly selected, private Holder data directory."""

    def __init__(self, data_dir: str | PathLike[str]) -> None:
        self._data_dir = data_dir
        self._context: Context | None = None
        self._closed = False
        self._native_error: type[RuntimeError] = RuntimeError
        self.card_operations: CardOperations = _EmbeddedCards(self)

    def __enter__(self) -> Self:
        if self._closed or self._context is not None:
            raise ClosedError("Embedded workspace contexts cannot be reused")

        package = f"{__package__}._embedded"
        try:
            native = import_module(f"{package}._native")
        except ModuleNotFoundError as error:
            if error.name != f"{package}._native":
                raise
            # A same-checkout Kit install also supports source development.
            package = "holderkit"
            try:
                native = import_module("holderkit._native")
            except ImportError as kit_error:
                raise EmbeddedUnavailableError(
                    "Embedded Holder support is unavailable. Install a compatible "
                    "native wheel or connect to a Holder daemon."
                ) from kit_error
        except ImportError as error:
            raise EmbeddedUnavailableError(
                "The bundled Holder extension failed to load."
            ) from error

        if not hasattr(native.Context, "list_link_kinds"):
            raise EmbeddedUnavailableError(
                "This Holder Kit build is too old for the shared Card API."
            )

        schema = files(package).joinpath("_schema.sql").read_text(encoding="utf-8")
        self._context = native.Context(fspath(self._data_dir), schema)
        self._native_error = native.HolderError
        return self

    def __exit__(self, *args: object) -> None:
        self._closed = True
        if self._context is not None:
            self._context.close()

    def list_projects(self) -> list[ProjectRecord]:
        """List the projects visible in this session as detached records."""
        return cast("list[ProjectRecord]", self._call("list_projects"))

    def get_project(self, project_id: str) -> ProjectRecord:
        """Fetch a detached project record by its ID."""
        return cast("ProjectRecord", self._call("get_project", project_id))

    def create_plain_project(self, *, name: str) -> ProjectRecord:
        """Create a project whose files are stored without encryption."""
        return cast("ProjectRecord", self._call("create_project", name))

    def list_trash(
        self, project_id: str, *, type: str | None = None
    ) -> list[TrashItemRecord]:
        """List detached Trash records, optionally filtered by item type."""
        return cast(
            "list[TrashItemRecord]", self._call("list_trash", project_id, type=type)
        )

    def list_project_tags(self, project_id: str) -> list[TagCount]:
        """List project tags and their live card counts."""
        return cast("list[TagCount]", self._call("list_project_tag_counts", project_id))

    def list_link_kinds(self) -> list[LinkKindLabels]:
        """List Holder's known link kinds and their forward and reverse labels."""
        return cast("list[LinkKindLabels]", self._call("list_link_kinds"))

    def _call(self, method: str, *args: object, **kwargs: object) -> object:
        if self._closed or self._context is None:
            raise ClosedError("Use the workspace inside with open_workspace(...)")
        try:
            return getattr(self._context, method)(*args, **kwargs)
        except (self._native_error, ValueError) as error:
            native_status = getattr(error, "native_status", None)
            if native_status is None:
                raise
            status, code = NATIVE_ERRORS.get(
                native_status,
                (400, "invalid_argument")
                if native_status == 4000
                else (500, "internal_error"),
            )
            error_type = NotFoundError if status == 404 else APIError
            raise error_type(status, code, str(error)) from error


class _NativeCardPage(TypedDict):
    cards: list[CardListItem]
    next_cursor: str | None


class _EmbeddedCards:
    def __init__(self, workspace: EmbeddedWorkspace) -> None:
        self._workspace = workspace

    def list_cards(
        self,
        project_id: str,
        *,
        parent: str | None = None,
        tag: str | None = None,
        order: Literal["card_id_asc", "updated_desc", "sort_key_asc"] = "card_id_asc",
        limit: int = 200,
        cursor: str | None = None,
        include_content: bool = False,
        include_child_count: bool = False,
    ) -> Page[CardListItem]:
        native_order = {
            "card_id_asc": "card_id",
            "updated_desc": "updated",
            "sort_key_asc": "sort_key",
        }
        result = cast(
            _NativeCardPage,
            self._workspace._call(
                "list_cards_page",
                project_id,
                parent_card_id=None if parent == "roots" else parent,
                roots=parent == "roots",
                tag=tag,
                order=native_order[order],
                limit=limit,
                cursor=cursor,
                include_content=include_content,
                include_child_count=include_child_count,
            ),
        )
        return {"items": result["cards"], "next_cursor": result["next_cursor"]}

    def search_cards(
        self, project_id: str, q: str, *, limit: int = 20, offset: int = 0
    ) -> list[SearchCard]:
        return cast(
            "list[SearchCard]",
            self._workspace._call(
                "search_cards", project_id, q, limit=limit, offset=offset
            ),
        )

    def get_card(self, project_id: str, card_id: str) -> Card:
        return cast("Card", self._workspace._call("get_live_card", project_id, card_id))

    def get_card_metadata(self, project_id: str, card_id: str) -> CardMetadata:
        return cast(
            "CardMetadata",
            self._workspace._call("get_card_metadata", project_id, card_id),
        )

    def get_card_breadcrumbs(
        self, project_id: str, card_id: str
    ) -> list[CardBreadcrumb]:
        return cast(
            "list[CardBreadcrumb]",
            self._workspace._call("get_card_breadcrumbs", project_id, card_id),
        )

    def resolve_card_reference(
        self, request: CardReferenceResolveRequest
    ) -> CardReferenceResolveData:
        return cast(
            "CardReferenceResolveData",
            self._workspace._call("resolve_card_reference", **request),
        )

    def create_card(self, project_id: str, card: NewCard) -> Card:
        return cast(
            "Card", self._workspace._call("create_card_record", project_id, **card)
        )

    def update_card(self, project_id: str, card_id: str, update: CardUpdate) -> Card:
        return cast(
            "Card", self._workspace._call("patch_card", project_id, card_id, **update)
        )

    def move_card(self, project_id: str, card_id: str, move: CardMove) -> Card:
        return cast(
            "Card",
            self._workspace._call("move_card_record", project_id, card_id, **move),
        )

    def trash_card(
        self,
        project_id: str,
        card_id: str,
        *,
        children: Literal["promote", "trash_subtree"] = "promote",
    ) -> Card:
        return cast(
            "Card",
            self._workspace._call(
                "trash_card_record", project_id, card_id, children=children
            ),
        )

    def restore_card(self, project_id: str, card_id: str) -> Card:
        return cast(
            "Card", self._workspace._call("restore_card_record", project_id, card_id)
        )

    def add_card_tag(self, project_id: str, card_id: str, tag: str) -> Card:
        return cast(
            "Card", self._workspace._call("add_card_tag", project_id, card_id, tag)
        )

    def remove_card_tag(self, project_id: str, card_id: str, tag: str) -> Card:
        return cast(
            "Card", self._workspace._call("remove_card_tag", project_id, card_id, tag)
        )

    def list_card_links(
        self, project_id: str, card_id: str, *, include_deleted: bool = False
    ) -> list[Link]:
        return cast(
            "list[Link]",
            self._workspace._call(
                "list_links", project_id, card_id, include_deleted=include_deleted
            ),
        )

    def list_card_backlinks(
        self,
        project_id: str,
        card_id: str,
        *,
        scope: Literal["local", "external", "all_available"] = "local",
        include_deleted: bool = False,
    ) -> list[Link]:
        return cast(
            "list[Link]",
            self._workspace._call(
                "list_card_backlinks",
                project_id,
                card_id,
                scope=scope,
                include_deleted=include_deleted,
            ),
        )

    def add_card_link(self, project_id: str, card_id: str, link: NewLink) -> Link:
        return cast(
            "Link", self._workspace._call("add_card_link", project_id, card_id, **link)
        )

    def remove_card_link(
        self,
        project_id: str,
        card_id: str,
        target_type: LinkTargetType,
        target_id: str,
        *,
        kind: str = "ref",
        target_project_id: str | None = None,
    ) -> list[Link]:
        return cast(
            "list[Link]",
            self._workspace._call(
                "remove_card_link",
                project_id,
                card_id,
                target_type,
                target_id,
                kind=kind,
                target_project_id=target_project_id,
            ),
        )

    def list_card_milestones(self, project_id: str, card_id: str) -> list[Milestone]:
        return cast(
            "list[Milestone]",
            self._workspace._call("list_milestones", project_id, card_id),
        )

    def add_card_milestone(
        self, project_id: str, card_id: str, milestone: NewMilestone
    ) -> Milestone:
        return cast(
            "Milestone",
            self._workspace._call("add_milestone", project_id, card_id, **milestone),
        )

    def update_card_milestone(
        self, project_id: str, card_id: str, milestone_id: str, update: MilestoneUpdate
    ) -> Milestone:
        return cast(
            "Milestone",
            self._workspace._call(
                "update_milestone", project_id, card_id, milestone_id, update
            ),
        )

    def remove_card_milestone(
        self, project_id: str, card_id: str, milestone_id: str
    ) -> list[Milestone]:
        return cast(
            "list[Milestone]",
            self._workspace._call(
                "remove_milestone", project_id, card_id, milestone_id
            ),
        )


def open_workspace(data_dir: str | PathLike[str]) -> EmbeddedWorkspace:
    """Open a private data directory in-process. Use as a context manager."""

    return EmbeddedWorkspace(data_dir)
