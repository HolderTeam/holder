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


NATIVE_ERRORS = {
    4040: (404, "not_found"),
    4090: (409, "conflict"),
    5030: (503, "keyring_unavailable"),
    4091: (409, "key_missing"),
    5002: (500, "data_corrupted"),
    4220: (422, "invalid_move"),
}
