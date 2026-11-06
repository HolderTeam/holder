"""Pages of a listing, shared by every domain that pages its results."""

from authoring import Record


class Page[T](Record):
    """One page of a listing."""

    items: list[T]
    """The page's items."""
    next_cursor: str | None
    """Pass back to get the next page; null on the last page."""
