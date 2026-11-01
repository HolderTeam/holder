"""Assemble Holder's public operation catalogue and HTTP routes.

Domain modules declare operations beside the types they use. This module provides the
single Contract loaded by the compiler, and gives each error kind its HTTP status and C error code.
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

# Each error kind's HTTP status and its C error code. A code is its status and one more digit,
# so it reads as the HTTP status it echoes. Codes are the error kinds' identity in the C API: once
# released, a code keeps its meaning and is never reused. holder.h's base codes take 4000 (an invalid
# argument), 5000 (an unexpected failure) and 5001 (an allocation failure).
contract = Contract(errors={
    RecoveryTokenInvalid: (400, 4001),
    RebuildFailed: (400, 4002),
    NotFound: (404, 4040),
    Conflict: (409, 4090),
    # The project's state rules the request out until its key is restored.
    KeyMissing: (409, 4091),
    HierarchyInvalid: (409, 4092),
    HistoryKeyUnavailable: (409, 4093),
    RevisionAmbiguous: (409, 4094),
    NotTrashed: (409, 4095),
    HistoryResponseTooLarge: (413, 4130),
    InvalidMove: (422, 4220),
    # Not something the caller can fix; the code names the condition.
    DataCorrupted: (500, 5002),
    KeyringUnavailable: (503, 5030),
    HistoryUnavailable: (503, 5031),
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
