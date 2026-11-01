"""Home: the project Holder makes on a device that has none, with one welcome card.

Every platform makes the same Home from these constants, through ensure_home_project.
"""

from typing import Final

from authoring import Record

from .models import Project

HOME_PROJECT_NAME: Final = "Home"
"""The name of the project Holder makes on a device that has none. Project lists put it first."""

WELCOME_CARD: Final = """\
# Welcome to Holder

Click here to edit your first card (tap if you are using a touchscreen).

The first line is the card’s title. By Markdown convention, we declare the title with a hash (#), but you don't have to.

Try changing the title above.

Two square brackets create an internal link: [[MySecondCard]]

Follow the link to it. Since it doesn’t exist yet, you’ll be prompted to create it.

Create more cards whenever you like, or simply by making internal links.

Explore the other tools when you’re ready, this is enough to get started.

Happy card writing!
"""
"""The content of Home's welcome card. Its first line, without the heading's hashes, is the card's title."""


class HomeProject(Record):
    """What ensure_home_project did."""

    project: Project | None
    """The Home project it made; null when the device already had a project, which it left alone."""
