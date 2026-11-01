"""Public history operations."""

from typing import Annotated, Literal

from fastapi import Path, Query

from authoring import Contract, Int32

from ..cards import CardId, ProjectId
from ..errors import HistoryKeyUnavailable, HistoryResponseTooLarge, HistoryUnavailable, NotFound, RevisionAmbiguous
from ..history import (
    CardHistoryComparison,
    CardHistoryPage,
    CardHistoryRestoreResult,
    CardHistorySnapshot,
    HistoryObjectKind,
    ProjectHistoryPage,
    RevisionRef,
)


def register(contract: Contract) -> None:
    @contract.route("GET", "/projects/{project_id}/history",
                    errors=[NotFound, HistoryKeyUnavailable, HistoryResponseTooLarge, HistoryUnavailable])
    def list_project_history(
        project_id: Annotated[ProjectId, Path()],
        limit: Annotated[Int32, Query(ge=1, le=200)] = 50,
        cursor: Annotated[str | None, Query(description="A full commit OID from next_cursor.")] = None,
        kind: Annotated[HistoryObjectKind | None, Query(description="Only activities affecting this object kind.")] = None,
    ) -> ProjectHistoryPage:
        """Return bounded, cursor-paginated Git activities for a project.

        Each commit groups its changed paths by Holder object kind. Unrecognised paths remain visible as unknown.
        A missing decryption key is HistoryKeyUnavailable; other history read failures are HistoryUnavailable.
        """


    @contract.route("GET", "/projects/{project_id}/history/cards/{card_id}",
                    errors=[NotFound, HistoryKeyUnavailable, HistoryResponseTooLarge, HistoryUnavailable])
    def list_card_history(
        project_id: Annotated[ProjectId, Path()],
        card_id: Annotated[CardId, Path()],
        limit: Annotated[Int32, Query(ge=1, le=200)] = 50,
        cursor: Annotated[str | None, Query(description="A full commit ID from next_cursor.")] = None,
    ) -> CardHistoryPage:
        """List saved history for a card.

        Returns commits reachable from the captured project HEAD that changed the card. Adjacent autosaves may be
        grouped into one displayed entry.
        """


    @contract.route("GET", "/projects/{project_id}/history/cards/{card_id}/compare",
                    errors=[NotFound, RevisionAmbiguous, HistoryKeyUnavailable, HistoryResponseTooLarge,
                            HistoryUnavailable])
    def compare_card_history(
        project_id: Annotated[ProjectId, Path()],
        card_id: Annotated[CardId, Path()],
        to_oid: Annotated[RevisionRef, Query(description="The ending revision, or the change to inspect.")],
        from_oid: Annotated[RevisionRef | None, Query(description="Required in since mode; ignored in change mode.")]
        = None,
        mode: Annotated[Literal["since", "change"], Query(description="Compare two snapshots or one introduced change.")]
        = "since",
    ) -> CardHistoryComparison:
        """Compare saved card versions or one revision's change.

        In since mode, compares the selected before and after snapshots. In change mode, compares the first parent
        of to_oid with to_oid. A root commit has a previously nonexistent card as its before state; a merge uses only
        its first parent.
        """


    @contract.route("GET", "/projects/{project_id}/history/cards/{card_id}/snapshot",
                    errors=[NotFound, RevisionAmbiguous, HistoryKeyUnavailable, HistoryResponseTooLarge,
                            HistoryUnavailable])
    def get_card_history_snapshot(
        project_id: Annotated[ProjectId, Path()],
        card_id: Annotated[CardId, Path()],
        oid: Annotated[RevisionRef, Query(description="The revision containing the card snapshot.")],
    ) -> CardHistorySnapshot:
        """Read a card snapshot at a selected revision, including an explicit absent state."""


    @contract.route("POST", "/projects/{project_id}/history/cards/{card_id}/restore",
                    errors=[NotFound, RevisionAmbiguous, HistoryKeyUnavailable, HistoryUnavailable])
    def restore_card_history_snapshot(
        project_id: Annotated[ProjectId, Path()],
        card_id: Annotated[CardId, Path()],
        oid: Annotated[RevisionRef, Query(description="The revision containing the snapshot to restore.")],
    ) -> CardHistoryRestoreResult:
        """Restore a card to a saved snapshot.

        Restores the complete snapshot as a new commit, preserving the old history. Lifecycle rules still apply:
        restoring a trashed snapshot promotes live children, and restoring a live snapshot places the card under a
        reachable parent.
        """
