"""Search operations."""

from typing import Annotated

from fastapi import Query

from authoring import Contract, Int32

from ..projects.ids import ProjectId
from .models import SearchCard, SearchMessage


def register(contract: Contract) -> None:
    @contract.route("GET", "/search/cards")
    def search_cards(
        project_id: Annotated[ProjectId, Query(description="The project to search.")],
        q: Annotated[str, Query(min_length=1, description="An SQLite FTS5 query for card titles and bodies.")],
        limit: Annotated[Int32, Query(ge=1)] = 20,
        offset: Annotated[Int32, Query(ge=0)] = 0,
    ) -> list[SearchCard]:
        """Search a project's cards, ordered by relevance, best first."""


    @contract.route("GET", "/search/ai")
    def search_ai_messages(
        project_id: Annotated[ProjectId, Query(description="The project to search.")],
        q: Annotated[str, Query(min_length=1, description="An SQLite FTS5 query for AI message content.")],
        limit: Annotated[Int32, Query(ge=1)] = 20,
        offset: Annotated[Int32, Query(ge=0)] = 0,
    ) -> list[SearchMessage]:
        """Search a project's AI messages, ordered by relevance, best first."""


    # Daemon-only routes use Python for their public contract and the daemon's HTTP host for execution.
