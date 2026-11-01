"""Core's runtime operations: they make, release or run without a context, so the daemon serves none."""

from typing import Annotated

from authoring import Contract, Field

from .models import CoreVersion, DatabaseRebuildReport


def register(contract: Contract) -> None:
    @contract.runtime_operation(context="none")
    def version() -> CoreVersion:
        """The version of the core library loaded."""

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
