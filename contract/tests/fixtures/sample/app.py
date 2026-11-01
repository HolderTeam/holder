"""The sample contract's operations, as routes."""

from typing import Annotated, Literal

from fastapi import Body, Path, Query

from authoring import Bytes, Contract, Field, Int32, Stream

from .cards import Card, CardId, CardListItem, Conflict, MessagePatch, NotFound, Page, RunEvent

contract = Contract(statuses={NotFound: 404, Conflict: 409})


@contract.route("GET", "/cards/{card_id}", errors=[NotFound])
def get_card(card_id: Annotated[CardId, Path()]) -> Card:
    """Fetch a card.

    A card in the trash is not found.
    """


@contract.route("GET", "/projects/{project_id}/cards")
def list_cards(
    project_id: Annotated[Annotated[str, Field(min_length=1)], Path(description="The project to list.")],
    parent: Annotated[Literal["roots"] | CardId | None, Query()] = None,
    limit: Annotated[Int32, Query(ge=1, le=5000)] = 200,
    cursor: Annotated[str | None, Query()] = None,
) -> Page[CardListItem]:
    """List a project's cards, one page at a time."""


@contract.route("PATCH", "/ai/messages/{message_id}", errors=[NotFound, Conflict])
def update_message(message_id: Annotated[str, Path()], patch: Annotated[MessagePatch, Body()]) -> None:
    """Change an AI message."""


@contract.route("GET", "/assets/{asset_id}/content", errors=[NotFound])
def get_asset_content(asset_id: Annotated[str, Path()]) -> Bytes:
    """Read an asset's bytes."""


@contract.route("POST", "/ai/runs", status=201, core=False)
def run_ai(prompt: Annotated[Annotated[str, Field(min_length=1)], Body(embed=True)]) -> Stream[RunEvent]:
    """Start an AI run and stream its events."""
