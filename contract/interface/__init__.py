"""Holder's contract. Nothing here runs in Holder.

cards.py and errors.py declare records and error kinds; app.py declares the operations, as the
daemon's routes.
"""
"""Holder's public contract declarations, in stable output order."""

DECLARATION_ORDER = (
    "ai",
    "card_references",
    "projects.ids",
    "cards.models",
    "daemon_routes",
    "errors",
    "git",
    "history",
    "links",
    "locations",
    "maintenance",
    "milestones",
    "projects.models",
    "rebuild",
    "recovery",
    "resources",
    "search",
    "trash",
)
