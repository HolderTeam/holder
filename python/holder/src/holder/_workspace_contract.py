"""Generated from the Holder contract. Run make -C contract update."""

from typing import Generic, Literal, NotRequired, Protocol, TypedDict, TypeVar

ProjectId = str
ProjectName = str
GitRemoteUrl = str
PrivacyMode = Literal["encrypted_git", "plain"]
CardId = str
Tag = str
TrashItemType = Literal["card", "ai_message"]
LinkKind = str
LinkTargetType = Literal["card", "ai_message", "ai_thread", "resource"]
LinkTargetId = str
MilestoneId = str
MilestoneRole = str
Day = str
DateTime = str
MilestoneText = str

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


class CardBreadcrumb(TypedDict):
    """One project or live card in a card's path from its project root."""

    kind: Literal["project", "card"]
    id: str
    title: str


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


class TrashItem(TypedDict):
    """A trashed card or AI message."""

    type: TrashItemType
    project_id: str
    card_id: str | None
    message_id: str | None
    thread_id: str | None
    title: str | None
    role: str | None
    deleted_at: int
    created_at: int | None
    rel_path: str | None


class TagCount(TypedDict):
    """A tag and how many of a project's live cards carry it."""

    tag: Tag
    card_count: int


class CardReferenceResolveRequest(TypedDict):
    """A reference to resolve among one project's cards."""

    project_id: ProjectId
    reference: str
    scope: Literal["live", "trashed", "either"]


class CardReferenceCandidate(TypedDict):
    """A card that matches a reference."""

    card_id: CardId
    title: str
    deleted_at: int | None


class CardReferenceResolveData(TypedDict):
    """A resolved, ambiguous, or absent card reference."""

    status: Literal["resolved", "ambiguous", "not_found"]
    match_kind: NotRequired[str]
    card: NotRequired[CardReferenceCandidate]
    candidates: NotRequired[list[CardReferenceCandidate]]


class SearchCard(TypedDict):
    """A card matching a full-text query."""

    card_id: str
    title: str
    updated_at: int
    created_at: int
    snippet: str
    rank: float


class Link(TypedDict):
    """
    A link from a card, or from an AI message, to something in a project. A link names
    both of its ends in full: each end's project, type and ID.
    """

    source_project_id: ProjectId
    source_type: Literal["card", "ai_message"]
    source_id: str
    target_project_id: ProjectId
    target_type: LinkTargetType
    target_id: LinkTargetId
    kind: LinkKind
    label: str | None
    created_at: int


class NewLink(TypedDict):
    """A link to make from a card. Its source is the card it is made from."""

    target_type: LinkTargetType
    target_id: LinkTargetId
    target_project_id: NotRequired[ProjectId]
    kind: NotRequired[LinkKind]
    label: NotRequired[str | None]
    created_at: NotRequired[int]


class LinkKindLabels(TypedDict):
    """
    One of the link kinds Holder knows, with how it reads from each end of a link. A
    link's kind is free text; a kind Holder does not know has no labels of its own.
    """

    kind: LinkKind
    forward_label: str
    reverse_label: str


class MilestoneDate(TypedDict):
    """One of a milestone's dates: a role and exactly one of a day or an instant."""

    role: MilestoneRole
    date: NotRequired[Day]
    datetime: NotRequired[DateTime]


class Milestone(TypedDict):
    """
    A milestone on a card: its dates, which may repeat a role or be none, and what it
    is.
    """

    milestone_id: MilestoneId
    project_id: ProjectId
    card_id: CardId
    dates: list[MilestoneDate]
    kind: MilestoneText | None
    description: MilestoneText | None
    created_at: int
    updated_at: int


class NewMilestone(TypedDict):
    """A milestone to put on a card."""

    dates: list[MilestoneDate]
    kind: NotRequired[MilestoneText | None]
    description: NotRequired[MilestoneText | None]


class MilestoneUpdate(TypedDict):
    """A change to a milestone. Each field left out keeps its value."""

    dates: NotRequired[list[MilestoneDate]]
    kind: NotRequired[MilestoneText | None]
    description: NotRequired[MilestoneText | None]


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

    def search_cards(
        self,
        project_id: ProjectId,
        q: str,
        *,
        limit: int = 20,
        offset: int = 0,
    ) -> list[SearchCard]:
        """Search a project's cards, ordered by relevance, best first."""
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

    def get_card_metadata(
        self,
        project_id: ProjectId,
        card_id: CardId,
    ) -> CardMetadata:
        """
        Fetch a live card's metadata without reading its content.

        A card in the trash or in another project is not found.
        """
        ...

    def get_card_breadcrumbs(
        self,
        project_id: ProjectId,
        card_id: CardId,
    ) -> list[CardBreadcrumb]:
        """
        Read a live card's path from its project root.

        The project comes first, then live ancestor cards and the requested card. A
        missing or trashed card is not found; a broken or cyclic ancestor chain is
        invalid.
        """
        ...

    def resolve_card_reference(
        self,
        request: CardReferenceResolveRequest,
    ) -> CardReferenceResolveData:
        """Resolve a card reference within a project and deletion scope."""
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

    def add_card_tag(
        self,
        project_id: ProjectId,
        card_id: CardId,
        tag: Tag,
    ) -> Card:
        """
        Tag a card.

        The tag is added to the card's trailing tag line, which is made when the card
        has none. A card that already carries the tag, anywhere in its content, is left
        as it is. A card in the trash or in another project is not found.
        """
        ...

    def remove_card_tag(
        self,
        project_id: ProjectId,
        card_id: CardId,
        tag: Tag,
    ) -> Card:
        """
        Take a tag off a card.

        The tag is removed from the card's trailing tag line, and the line with it when
        it is left empty, so that the card no longer carries it. A card that does not
        carry the tag is left as it is. Holder does not rewrite prose, so a card that
        would still carry the tag in its prose is a Conflict, and is left as it is. A
        card in the trash or in another project is not found.
        """
        ...

    def list_card_links(
        self,
        project_id: ProjectId,
        card_id: CardId,
        *,
        include_deleted: bool = False,
    ) -> list[Link]:
        """
        List a card's links.

        The links the card makes, newest first. A card in the trash or in another
        project is not found.
        """
        ...

    def list_card_backlinks(
        self,
        project_id: ProjectId,
        card_id: CardId,
        *,
        scope: Literal["local", "external", "all_available"] = "local",
        include_deleted: bool = False,
    ) -> list[Link]:
        """
        List the links to a card.

        Links from cards and AI messages, newest first. External links come only from
        projects on this device, so they are never all there may be. A card in the trash
        or in another project is not found.
        """
        ...

    def add_card_link(
        self,
        project_id: ProjectId,
        card_id: CardId,
        link: NewLink,
    ) -> Link:
        """
        Link a card to something.

        A link is one target, of one type in one project, with one kind. Making a link
        that already exists sets its label and keeps when it was made. A target in the
        card's project must exist and not be in the trash; a card in another project may
        be one this device does not have. A card in the trash or in another project is
        not found, and neither is a target that does not exist or is in the trash.
        """
        ...

    def remove_card_link(
        self,
        project_id: ProjectId,
        card_id: CardId,
        target_type: LinkTargetType,
        target_id: LinkTargetId,
        *,
        kind: LinkKind = "ref",
        target_project_id: ProjectId | None = None,
    ) -> list[Link]:
        """
        Remove a link from a card.

        Removes the one link to that target, of that type in that project, with that
        kind, and returns the card's remaining links as list_card_links lists them by
        default. A card without that link is left as it is. A card in the trash or in
        another project is not found.
        """
        ...

    def list_card_milestones(
        self,
        project_id: ProjectId,
        card_id: CardId,
    ) -> list[Milestone]:
        """
        List a card's milestones.

        By when they were made, then by ID. A card in the trash or in another project is
        not found.
        """
        ...

    def add_card_milestone(
        self,
        project_id: ProjectId,
        card_id: CardId,
        milestone: NewMilestone,
    ) -> Milestone:
        """
        Put a milestone on a card.

        The milestone gets a new ID. Each date must be a real day or instant. A card in
        the trash or in another project is not found.
        """
        ...

    def update_card_milestone(
        self,
        project_id: ProjectId,
        card_id: CardId,
        milestone_id: MilestoneId,
        update: MilestoneUpdate,
    ) -> Milestone:
        """
        Change a milestone.

        Each field left out keeps its value, and an update that changes nothing writes
        nothing. Each date must be a real day or instant. A card in the trash or in
        another project is not found, and neither is a milestone that is not on the
        card.
        """
        ...

    def remove_card_milestone(
        self,
        project_id: ProjectId,
        card_id: CardId,
        milestone_id: MilestoneId,
    ) -> list[Milestone]:
        """
        Take a milestone off a card.

        Returns the card's remaining milestones, as list_card_milestones lists them. A
        card without the milestone is left as it is. A card in the trash or in another
        project is not found.
        """
        ...


NATIVE_ERRORS = {
    4040: (404, "not_found"),
    4090: (409, "conflict"),
    5030: (503, "keyring_unavailable"),
    4091: (409, "key_missing"),
    5002: (500, "data_corrupted"),
    4220: (422, "invalid_move"),
    4092: (409, "hierarchy_invalid"),
}
