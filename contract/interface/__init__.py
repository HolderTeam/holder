"""Holder's contract. Nothing here runs in Holder.

Each domain package declares its records and error kinds in model modules and its operations in
operations.py; app.py assembles the operations. DECLARATION_ORDER fixes the order of the model
modules in the IR, independently of file paths.
"""

DECLARATION_ORDER = (
    "ai.models",
    "cards.references",
    "pagination.models",
    "projects.ids",
    "cards.models",
    "daemon.models",
    "errors",
    "events.models",
    "git.models",
    "history.models",
    "links.models",
    "locations.models",
    "maintenance.models",
    "milestones.models",
    "projects.models",
    "maintenance.rebuild",
    "recovery.models",
    "resources.models",
    "search.models",
    "trash.models",
)
