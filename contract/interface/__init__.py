"""Holder's contract. Nothing here runs in Holder.

cards.py and errors.py declare records and error kinds; app.py declares the operations, as the
daemon's routes.
"""
"""Holder's public contract declarations, in stable output order."""

DECLARATION_ORDER = (
    "ai",
    "card_references",
    "pagination.models",
    "projects.ids",
    "cards.models",
    "daemon.models",
    "errors",
    "events.models",
    "git",
    "history",
    "links",
    "locations.models",
    "maintenance",
    "milestones",
    "projects.models",
    "rebuild",
    "recovery",
    "resources",
    "search",
    "trash",
)
