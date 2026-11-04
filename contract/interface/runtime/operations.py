"""Core's runtime operations: they make, release or run without a context, so the daemon serves none."""

from typing import Annotated

from authoring import Contract, Field

from .hosts import KeyringProvider, TimeZoneProvider
from .models import CoreVersion, DatabaseRebuildReport


def register(contract: Contract) -> None:
    @contract.runtime_operation(context="none")
    def version() -> CoreVersion:
        """The version of the core library loaded."""

    @contract.runtime_operation(context="none", host=TimeZoneProvider)
    def register_time_zone_provider() -> None:
        """Install this device's time-zone provider for the process.

        Registration takes ownership of the provider even when validation fails. A replacement
        releases the previous provider after active callbacks finish. Callbacks may run on any
        thread. Null offset means an unknown zone; a callback failure is an unexpected error.
        """

    @contract.runtime_operation(context="none", host=KeyringProvider)
    def register_keyring_provider() -> None:
        """Install a process-wide secure store for secrets.

        Registration takes ownership even on failure. Replacement releases the previous provider
        after active callbacks finish. Callbacks may run on any thread. A null lookup result means
        no secret; callback failure remains distinct from a miss.
        """

    @contract.runtime_operation(context="none")
    def configure_git_home(path: Annotated[str, Field(min_length=1)]) -> None:
        """Set libgit2's process-wide home directory before Git operations.

        On Android, pass a writable app-private directory so SSH can find its known_hosts file.
        Call once before Git work starts; desktop hosts with a normal home directory need no override.
        """

    @contract.runtime_operation(context="opens")
    def context_open(data_dir: Annotated[str, Field(min_length=1)], schema_sql: str | None = None) -> None:
        """Open a context on a data directory: the handle every other operation works through.

        Opening makes the directory's database, or rebuilds it from the project files when it is
        missing, damaged or from an older schema; that needs schema_sql. A context holds one database
        connection and must not run two calls at once: open one per thread or serial worker, the first
        before the others since it may rebuild. The platform keyring must be registered first when
        encrypted projects may need rebuilding. Release the context with context_destroy.
        """

    @contract.runtime_operation(context="closes")
    def context_destroy() -> None:
        """Release a context. No call on it may still be running, and it must not be used again."""

    @contract.runtime_operation(context="none")
    def database_rebuild(data_dir: Annotated[str, Field(min_length=1)],
                         schema_sql: Annotated[str, Field(min_length=1)], dry_run: bool = False) -> DatabaseRebuildReport:
        """Rebuild a data directory's database from its project files, keeping the old one aside.

        No context on the directory may be open while this runs: close them all first and open them
        again after. The platform keyring must be registered first when encrypted projects are present.
        A dry run only counts what a rebuild would read.
        """

    @contract.runtime_operation(context="none", error="reads")
    def error_message() -> str:
        """The message of an error core returned, as UTF-8 text.

        The text belongs to the error and lasts until error_destroy releases it. An error that is null has the
        empty message.
        """

    @contract.runtime_operation(context="none", error="closes")
    def error_destroy() -> None:
        """Release an error core returned. Releasing null does nothing; an error must not be used again."""
