"""Assemble Holder's public operation catalogue and HTTP routes.

Domain modules declare operations beside the types they use. This module provides the
single Contract loaded by the compiler and sets shared HTTP error statuses.
"""

from authoring import Contract

from .ai import operations as ai_operations
from .cards import operations as card_operations
from .cards.errors import HierarchyInvalid, InvalidMove
from .daemon import operations as daemon_operations
from .documents import operations as document_operations
from .errors import Conflict, DataCorrupted, KeyMissing, KeyringUnavailable, NotFound
from .events import operations as event_operations
from .git import operations as git_operations
from .history import operations as history_operations
from .history.errors import HistoryKeyUnavailable, HistoryResponseTooLarge, HistoryUnavailable, RevisionAmbiguous
from .links import operations as link_operations
from .locations import operations as location_operations
from .maintenance import operations as maintenance_operations
from .maintenance.errors import RebuildFailed
from .milestones import operations as milestone_operations
from .projects import operations as project_operations
from .recovery import operations as recovery_operations
from .recovery.errors import RecoveryTokenInvalid
from .resources import operations as resource_operations
from .search import operations as search_operations
from .trash import operations as trash_operations
from .trash.errors import NotTrashed

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
link_operations.register(contract)
milestone_operations.register(contract)
resource_operations.register(contract)
project_operations.register(contract)
maintenance_operations.register(contract)
recovery_operations.register(contract)
git_operations.register(contract)
history_operations.register(contract)
trash_operations.register(contract)
search_operations.register(contract)
daemon_operations.register(contract)
event_operations.register(contract)
document_operations.register(contract)
ai_operations.register(contract)

app = contract.app
