"""Public links operations."""

from typing import Annotated, Literal

from fastapi import Body, Path, Query

from authoring import Contract

from ..cards.models import CardId, ProjectId
from ..errors import NotFound
from ..links import Link, LinkKind, LinkTargetId, LinkTargetType, NewLink
from .common import KEYS


def register(contract: Contract) -> None:
    @contract.route("GET", "/projects/{project_id}/cards/{card_id}/links", errors=[NotFound])
    def list_card_links(
        project_id: Annotated[ProjectId, Path()],
        card_id: Annotated[CardId, Path()],
        include_deleted: Annotated[bool, Query(
            description="Include links to targets in the trash. A link to a card in another project is listed whether "
                        "or not that card can be found here.")] = False,
    ) -> list[Link]:
        """List a card's links.

        The links the card makes, newest first. A card in the trash or in another project is not found.
        """


    @contract.route("GET", "/projects/{project_id}/cards/{card_id}/backlinks", errors=[NotFound])
    def list_card_backlinks(
        project_id: Annotated[ProjectId, Path()],
        card_id: Annotated[CardId, Path()],
        scope: Annotated[Literal["local", "external", "all_available"], Query(
            description="local lists links from the card's own project; external, links from the other projects on "
                        "this device; all_available, both.")] = "local",
        include_deleted: Annotated[bool, Query(description="Include links from cards and AI messages in the trash.")]
        = False,
    ) -> list[Link]:
        """List the links to a card.

        Links from cards and AI messages, newest first. External links come only from projects on this device, so
        they are never all there may be. A card in the trash or in another project is not found.
        """


    @contract.route("POST", "/projects/{project_id}/cards/{card_id}/links", errors=[NotFound, *KEYS])
    def add_card_link(
        project_id: Annotated[ProjectId, Path()],
        card_id: Annotated[CardId, Path()],
        link: Annotated[NewLink, Body()],
    ) -> Link:
        """Link a card to something.

        A link is one target, of one type in one project, with one kind. Making a link that already exists sets its
        label and keeps when it was made. A target in the card's project must exist and not be in the trash; a card in
        another project may be one this device does not have. A card in the trash or in another project is not found,
        and neither is a target that does not exist or is in the trash.
        """


    @contract.route("DELETE", "/projects/{project_id}/cards/{card_id}/links", errors=[NotFound, *KEYS])
    def remove_card_link(
        project_id: Annotated[ProjectId, Path()],
        card_id: Annotated[CardId, Path()],
        target_type: Annotated[LinkTargetType, Query()],
        target_id: Annotated[LinkTargetId, Query()],
        kind: Annotated[LinkKind, Query()] = "ref",
        target_project_id: Annotated[ProjectId | None, Query(description="Omit for the card's own project.")] = None,
    ) -> list[Link]:
        """Remove a link from a card.

        Removes the one link to that target, of that type in that project, with that kind, and returns the card's
        remaining links as list_card_links lists them by default. A card without that link is left as it is. A card in
        the trash or in another project is not found.
        """
