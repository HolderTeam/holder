"""Assemble Holder's public operation catalogue and HTTP routes.

Domain modules declare operations beside the types they use. This module provides the
single Contract loaded by the compiler and sets shared HTTP error statuses.
"""

from authoring import Contract

from .errors import (
    Conflict,
    DataCorrupted,
    HierarchyInvalid,
    HistoryKeyUnavailable,
    HistoryResponseTooLarge,
    HistoryUnavailable,
    InvalidMove,
    KeyMissing,
    KeyringUnavailable,
    NotFound,
    NotTrashed,
    RebuildFailed,
    RecoveryTokenInvalid,
    RevisionAmbiguous,
)
from .operations import (
    ai,
    cards,
    git,
    history,
    links,
    locations,
    maintenance,
    milestones,
    projects,
    resources,
    search,
    system,
    trash,
)

contract = Contract(statuses={
    NotFound: 404,
    Conflict: 409,
    # The project's state rules the request out until its key is restored.
    KeyMissing: 409,
    KeyringUnavailable: 503,
    # Not something the caller can fix; the code names the condition.
    DataCorrupted: 500,
    InvalidMove: 422,
    HierarchyInvalid: 409,
    HistoryResponseTooLarge: 413,
    HistoryKeyUnavailable: 409,
    RevisionAmbiguous: 409,
    NotTrashed: 409,
    HistoryUnavailable: 503,
    RecoveryTokenInvalid: 400,
    RebuildFailed: 400,
})

locations.register(contract)
cards.register(contract)
links.register(contract)
milestones.register(contract)
resources.register(contract)
projects.register(contract)
maintenance.register(contract)
git.register(contract)
history.register(contract)
trash.register(contract)
search.register(contract)
system.register(contract)
ai.register(contract)

app = contract.app
