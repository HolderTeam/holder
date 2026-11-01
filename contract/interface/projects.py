"""Projects: the collections of cards, each a Git repository, that this device holds."""

from typing import Annotated, Literal

from authoring import MISSING, Field, Int32, Record, Timestamp

from .cards.models import ProjectId

type ProjectName = Annotated[str, Field(min_length=1)]
"""A project's name."""

type GitRemoteUrl = Annotated[str, Field(min_length=1)]
"""A Git remote's URL, such as https://example.com/notes.git or git@example.com:team/notes.git."""

type PrivacyMode = Literal["encrypted_git", "plain"]
"""How a project's files are stored: encrypted for the project, so that its Git remote holds only ciphertext, or
as they are."""


class Project(Record):
    """A project on this device."""

    project_id: ProjectId
    """The project's ID."""
    name: ProjectName
    """The project's name."""
    root_path: str
    """The directory that holds the project's repository on this device."""
    git_remote_url: GitRemoteUrl | None
    """The Git remote the project syncs with; null when it has none."""
    git_provider: str | None
    """Who hosts the Git remote, such as github; null when unknown."""
    privacy_mode: PrivacyMode
    """How the project's files are stored."""
    project_key_id: str | None
    """The ID of an encrypted project's key; null for a plain project."""
    created_at: Timestamp
    """When the project was made."""
    updated_at: Timestamp
    """When the project's settings last changed."""


class ProjectListItem(Project):
    """A project in a listing."""

    card_count: Int32 | MISSING = MISSING
    """The project's live cards; present when asked for."""
    root_card_count: Int32 | MISSING = MISSING
    """The project's live root cards; present when asked for."""


class NewProject(Record):
    """A project to make."""

    project_id: ProjectId | MISSING = MISSING
    """The project's ID, for imports; omit for a new one."""
    name: ProjectName
    """The project's name."""
    root_path: str | MISSING = MISSING
    """The directory for the project's repository, an absolute path; omit for one under this device's projects
    directory, named for the project."""
    git_remote_url: GitRemoteUrl | None = None
    """The Git remote to sync with; null or omitted for none. One Git does not accept is an invalid argument."""
    git_provider: str | None = None
    """Who hosts the Git remote; null or omitted when unknown."""
    privacy_mode: PrivacyMode = "encrypted_git"
    """How the project's files are stored."""
    created_at: Annotated[Timestamp, Field(ge=1)] | MISSING = MISSING
    """When the project was made, for imports, after the epoch; omit for now."""
    updated_at: Annotated[Timestamp, Field(ge=1)] | MISSING = MISSING
    """When the project's settings last changed, for imports, after the epoch; omit for when it was made."""


class ProjectUpdate(Record):
    """A change to a project's settings. Each field left out keeps its value."""

    name: ProjectName | MISSING = MISSING
    """The project's name."""
    git_remote_url: GitRemoteUrl | None | MISSING = MISSING
    """The Git remote to sync with; null for none."""
    git_provider: str | None | MISSING = MISSING
    """Who hosts the Git remote; null when unknown."""
