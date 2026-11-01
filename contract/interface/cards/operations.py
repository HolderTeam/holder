"""Public cards operations."""

from typing import Annotated, Literal

from fastapi import Body, Path, Query

from authoring import Contract, Field, Int32

from ..errors import Conflict, DataCorrupted, KeyMissing, KeyringUnavailable, NotFound
from ..operations.common import KEYS
from ..pagination.models import Page
from ..projects.ids import ProjectId
from .errors import HierarchyInvalid, InvalidMove
from .models import (
    Card,
    CardBreadcrumb,
    CardId,
    CardListItem,
    CardMetadata,
    CardMove,
    CardUpdate,
    NewCard,
    Tag,
    TagCount,
)
from .references import CardReferenceResolveData, CardReferenceResolveRequest


def register(contract: Contract) -> None:
    @contract.route("GET", "/projects/{project_id}/cards/{card_id}",
                    errors=[NotFound, KeyringUnavailable, KeyMissing, DataCorrupted])
    def get_card(project_id: Annotated[ProjectId, Path()], card_id: Annotated[CardId, Path()]) -> Card:
        """Fetch a card.

        A card in the trash or in another project is not found.
        """


    @contract.route("GET", "/projects/{project_id}/cards/{card_id}/metadata", errors=[NotFound])
    def get_card_metadata(project_id: Annotated[ProjectId, Path()], card_id: Annotated[CardId, Path()]) -> CardMetadata:
        """Fetch a live card's metadata without reading its content.

        A card in the trash or in another project is not found.
        """


    @contract.route("GET", "/projects/{project_id}/cards/{card_id}/breadcrumbs",
                    errors=[NotFound, HierarchyInvalid])
    def get_card_breadcrumbs(
        project_id: Annotated[ProjectId, Path()], card_id: Annotated[CardId, Path()],
    ) -> list[CardBreadcrumb]:
        """Read a live card's path from its project root.

        The project comes first, then live ancestor cards and the requested card. A missing or trashed
        card is not found; a broken or cyclic ancestor chain is invalid.
        """


    @contract.route("POST", "/card-references/resolve", errors=[NotFound])
    def resolve_card_reference(
        request: Annotated[CardReferenceResolveRequest, Body()],
    ) -> CardReferenceResolveData:
        """Resolve a card reference within a project and deletion scope."""


    @contract.route("GET", "/projects/{project_id}/cards",
                    errors=[NotFound, KeyringUnavailable, KeyMissing, DataCorrupted])
    def list_cards(
        project_id: Annotated[ProjectId, Path()],
        parent: Annotated[Literal["roots"] | CardId | None, Query(
            description="Omit for every level; roots selects root cards; a card ID selects its children.")] = None,
        tag: Annotated[Tag | None, Query(description="Only cards with this tag.")] = None,
        order: Annotated[Literal["card_id_asc", "updated_desc", "sort_key_asc"], Query(
            description="updated_desc breaks ties by descending card ID; sort_key_asc follows sibling order.")]
        = "card_id_asc",
        limit: Annotated[Int32, Query(ge=1, le=5000, description="At most 1000 when include_content is true.")] = 200,
        cursor: Annotated[Annotated[str, Field(min_length=1, max_length=8192)] | None, Query(
            description="The previous page's next_cursor. Keep the project, filters and order unchanged.")] = None,
        include_content: Annotated[bool, Query(description="Include each card's body.")] = False,
        include_child_count: Annotated[bool, Query(description="Include each card's count of live children.")] = False,
    ) -> Page[CardListItem]:
        """Page through a project's live cards.

        Filters combine. Each page reads current data; iterating is not a snapshot.
        """


    @contract.route("POST", "/projects/{project_id}/cards", status=201,
                    errors=[NotFound, Conflict, KeyringUnavailable, KeyMissing, DataCorrupted])
    def create_card(project_id: Annotated[ProjectId, Path()], card: Annotated[NewCard, Body()]) -> Card:
        """Create a card.

        Fails with NotFound when the project or the parent does not exist.
        """


    @contract.route("PATCH", "/projects/{project_id}/cards/{card_id}", errors=[NotFound, InvalidMove, *KEYS])
    def update_card(
        project_id: Annotated[ProjectId, Path()],
        card_id: Annotated[CardId, Path()],
        update: Annotated[CardUpdate, Body()],
    ) -> Card:
        """Change a card.

        Each field left out keeps its value, and an update that leaves every field out changes nothing. The title and
        content are written first, then the parent and place, each as its own change in the project's history. A card
        in the trash or in another project is not found, and neither is a new parent that does not exist or is in the
        trash; a parent under the card itself is an InvalidMove.
        """


    @contract.route("DELETE", "/projects/{project_id}/cards/{card_id}", errors=[NotFound, *KEYS])
    def trash_card(
        project_id: Annotated[ProjectId, Path()],
        card_id: Annotated[CardId, Path()],
        children: Annotated[Literal["promote", "trash_subtree"], Query(
            description="promote moves the card's live children into its place among its siblings, keeping their own "
                        "descendants; trash_subtree moves every live descendant to the trash with it.")] = "promote",
    ) -> Card:
        """Move a card to the trash.

        The card and every card that changes with it are written as one change in the project's history. A card already
        in the trash, or in another project, is not found. Returns the card as it is in the trash.
        """


    @contract.route("POST", "/projects/{project_id}/cards/{card_id}/restore", errors=[NotFound, Conflict, *KEYS])
    def restore_card(project_id: Annotated[ProjectId, Path()], card_id: Annotated[CardId, Path()]) -> Card:
        """Restore a card from the trash.

        Restores this card only. It returns to its parent when that parent is live and reachable, otherwise to its
        nearest live, reachable ancestor or the top level. Children promoted when it was trashed stay where they are;
        descendants trashed with it stay in the trash, and come back under it when restored. A card in another
        project is not found, and one that is not in the trash is a Conflict.
        """


    @contract.route("POST", "/projects/{project_id}/cards/{card_id}/move", errors=[NotFound, InvalidMove, *KEYS])
    def move_card(
        project_id: Annotated[ProjectId, Path()],
        card_id: Annotated[CardId, Path()],
        move: Annotated[CardMove, Body()],
    ) -> Card:
        """Move a card in its project's hierarchy.

        When the card's new neighbours leave no room between their places, nearby siblings are re-spaced in the same
        change, keeping their order; other cards' sort_key values a caller holds may then be out of date. A card in the
        trash or in another project is not found, and neither is a target or parent that does not exist or is in the
        trash. A move that changes nothing writes nothing.
        """


    @contract.route("PUT", "/projects/{project_id}/cards/{card_id}/tags/{tag}", errors=[NotFound, *KEYS])
    def add_card_tag(
        project_id: Annotated[ProjectId, Path()],
        card_id: Annotated[CardId, Path()],
        tag: Annotated[Tag, Path()],
    ) -> Card:
        """Tag a card.

        The tag is added to the card's trailing tag line, which is made when the card has none. A card that already
        carries the tag, anywhere in its content, is left as it is. A card in the trash or in another project is not
        found.
        """


    @contract.route("DELETE", "/projects/{project_id}/cards/{card_id}/tags/{tag}", errors=[NotFound, Conflict, *KEYS])
    def remove_card_tag(
        project_id: Annotated[ProjectId, Path()],
        card_id: Annotated[CardId, Path()],
        tag: Annotated[Tag, Path()],
    ) -> Card:
        """Take a tag off a card.

        The tag is removed from the card's trailing tag line, and the line with it when it is left empty, so that the
        card no longer carries it. A card that does not carry the tag is left as it is. Holder does not rewrite prose,
        so a card that would still carry the tag in its prose is a Conflict, and is left as it is. A card in the trash
        or in another project is not found.
        """


    @contract.route("GET", "/projects/{project_id}/tags", errors=[NotFound])
    def list_project_tags(project_id: Annotated[ProjectId, Path()]) -> list[TagCount]:
        """List a project's tags.

        Each tag carried by a live card in the project, with how many carry it, by descending count and then by tag.
        """
