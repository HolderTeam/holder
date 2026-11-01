"""Errors particular to cards and their hierarchy."""

from authoring import HolderError


class InvalidMove(HolderError):
    """A card cannot go where it was asked to: under itself or one of its descendants, beside a card that cannot be its sibling, or up a level from the top."""


class HierarchyInvalid(HolderError):
    """A card's ancestor chain is broken, cyclic or too deep to follow."""
