"""Generated from the Holder contract. Run make -C contract update."""

from typing import Generic, Literal, NotRequired, Protocol, TypedDict, TypeVar

ProjectId = str
ProjectName = str
GitRemoteUrl = str
PrivacyMode = Literal["encrypted_git", "plain"]
CardId = str
Tag = str

T = TypeVar("T")


class CardMetadata(TypedDict):
    """Everything about a card except its content."""

    card_id: CardId
    project_id: ProjectId
    title: str
    rel_path: str
    parent_card_id: CardId | None
    sort_key: float
    created_at: int
    updated_at: int
    deleted_at: int | None


class TagOccurrence(TypedDict):
    """A tag written in a card's content."""

    tag: str
    byte_start: int
    byte_end: int


class CardListItem(CardMetadata):
    """A card in a listing."""

    child_count: NotRequired[int]
    content: NotRequired[str]


class Card(CardMetadata):
    """A card, with its content and tags."""

    content: str
    tags: list[str]
    tag_occurrences: list[TagOccurrence]
    removable_tags: list[str]


class NewCard(TypedDict):
    """A card to create."""

    title: str
    content: NotRequired[str]
    parent_card_id: NotRequired[CardId | None]
    sort_key: NotRequired[float]
    created_at: NotRequired[int]
    updated_at: NotRequired[int]


class CardUpdate(TypedDict):
    """Changes to a card. Each field left out keeps its value."""

    title: NotRequired[str]
    content: NotRequired[str]
    parent_card_id: NotRequired[CardId | None]
    sort_key: NotRequired[float]
    updated_at: NotRequired[int]


class CardMove(TypedDict):
    """Where to move a card, relative to another card or among its siblings."""

    intent: Literal["into", "before", "after", "left", "right", "to_start", "to_end", "up_level"]
    target_card_id: NotRequired[CardId]
    parent_card_id: NotRequired[CardId | None]


class Page(TypedDict, Generic[T]):
    """One page of a listing."""

    items: list[T]
    next_cursor: str | None


class Project(TypedDict):
    """A project on this device."""

    project_id: ProjectId
    name: ProjectName
    root_path: str
    git_remote_url: GitRemoteUrl | None
    git_provider: str | None
    privacy_mode: PrivacyMode
    project_key_id: str | None
    created_at: int
    updated_at: int


class CardOperations(Protocol):
    """The Card operations shared by daemon and embedded sessions."""

    def list_cards(
        self,
        project_id: ProjectId,
        *,
        parent: Literal["roots"] | CardId | None = None,
        tag: Tag | None = None,
        order: Literal["card_id_asc", "updated_desc", "sort_key_asc"] = "card_id_asc",
        limit: int = 200,
        cursor: str | None = None,
        include_content: bool = False,
        include_child_count: bool = False,
    ) -> Page[CardListItem]:
        """
        Page through a project's live cards.

        Tag result views use updated_desc, which puts the latest card update first and
        breaks ties by descending card ID. Rebuilding the projection from the same card
        files preserves this order.

        Filters combine. Each page reads current data; iterating is not a snapshot.
        """
        ...

    def get_card(
        self,
        project_id: ProjectId,
        card_id: CardId,
    ) -> Card:
        """
        Fetch a card.

        A card in the trash or in another project is not found.
        """
        ...

    def create_card(
        self,
        project_id: ProjectId,
        card: NewCard,
    ) -> Card:
        """
        Create a card.

        Fails with NotFound when the project or the parent does not exist.
        """
        ...

    def update_card(
        self,
        project_id: ProjectId,
        card_id: CardId,
        update: CardUpdate,
    ) -> Card:
        """
        Change a card.

        Each field left out keeps its value, and an update that leaves every field out
        changes nothing. The title and content are written first, then the parent and
        place, each as its own change in the project's history. A card in the trash or
        in another project is not found, and neither is a new parent that does not exist
        or is in the trash; a parent under the card itself is an InvalidMove.
        """
        ...

    def move_card(
        self,
        project_id: ProjectId,
        card_id: CardId,
        move: CardMove,
    ) -> Card:
        """
        Move a card in its project's hierarchy.

        When the card's new neighbours leave no room between their places, nearby
        siblings are re-spaced in the same change, keeping their order; other cards'
        sort_key values a caller holds may then be out of date. A card in the trash or
        in another project is not found, and neither is a target or parent that does not
        exist or is in the trash. A move that changes nothing writes nothing.
        """
        ...

    def trash_card(
        self,
        project_id: ProjectId,
        card_id: CardId,
        *,
        children: Literal["promote", "trash_subtree"] = "promote",
    ) -> Card:
        """
        Move a card to the trash.

        The card and every card that changes with it are written as one change in the
        project's history. A card already in the trash, or in another project, is not
        found. Returns the card as it is in the trash.
        """
        ...

    def restore_card(
        self,
        project_id: ProjectId,
        card_id: CardId,
    ) -> Card:
        """
        Restore a card from the trash.

        Restores this card only. It returns to its parent when that parent is live and
        reachable, otherwise to its nearest live, reachable ancestor or the top level.
        Children promoted when it was trashed stay where they are; descendants trashed
        with it stay in the trash, and come back under it when restored. A card in
        another project is not found, and one that is not in the trash is a Conflict.
        """
        ...


NATIVE_ERRORS = {
    4040: (404, "not_found"),
    4090: (409, "conflict"),
    5030: (503, "keyring_unavailable"),
    4091: (409, "key_missing"),
    5002: (500, "data_corrupted"),
    4220: (422, "invalid_move"),
}
