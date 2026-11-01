"""Assemble Holder's public operation catalogue and HTTP routes.

Domain modules declare operations beside the types they use. This module provides the
single Contract loaded by the compiler and sets shared HTTP error statuses.
"""

from authoring import Contract

from .cards import operations as card_operations
from .daemon import operations as daemon_operations
from .documents import operations as document_operations
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
from .events import operations as event_operations
from .locations import operations as location_operations
from .operations import (
    ai,
    git,
    history,
    links,
    maintenance,
    milestones,
    resources,
    search,
    trash,
)
from .projects import operations as project_operations

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

location_operations.register(contract)
card_operations.register(contract)
links.register(contract)
milestones.register(contract)
resources.register(contract)
project_operations.register(contract)
maintenance.register(contract)
git.register(contract)
history.register(contract)
trash.register(contract)
search.register(contract)
daemon_operations.register(contract)
event_operations.register(contract)
document_operations.register(contract)
ai.register(contract)

app = contract.app
